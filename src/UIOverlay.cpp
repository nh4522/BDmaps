#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "UIOverlay.h"
#include "MapData.h"
#include "Camera.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <iostream>

static const char* DIVISION_NAMES[] = {
    "Dhaka","Chittagong","Rajshahi","Khulna",
    "Barisal","Sylhet","Rangpur","Mymensingh"
};
static const ImVec4 DIV_COLORS[] = {
    {0.16f,0.67f,0.51f,1.f}, {0.15f,0.50f,0.83f,1.f},
    {0.69f,0.35f,0.15f,1.f}, {0.14f,0.61f,0.68f,1.f},
    {0.42f,0.24f,0.67f,1.f}, {0.23f,0.60f,0.15f,1.f},
    {0.69f,0.52f,0.15f,1.f}, {0.14f,0.36f,0.60f,1.f},
};

static std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

UIOverlay::UIOverlay(GLFWwindow* window, int, int) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.f;
    style.FrameRounding = 5.f;
    style.WindowBorderSize = 0.f;
    style.WindowPadding = { 14.f, 12.f };
    style.ItemSpacing = { 10.f, 8.f };
    style.Alpha = 0.95f;

    style.Colors[ImGuiCol_WindowBg] = { 0.06f, 0.10f, 0.17f, 0.95f };
    style.Colors[ImGuiCol_TitleBg] = { 0.04f, 0.07f, 0.13f, 0.95f };
    style.Colors[ImGuiCol_TitleBgActive] = { 0.06f, 0.12f, 0.22f, 0.95f };
    style.Colors[ImGuiCol_Separator] = { 0.25f, 0.35f, 0.50f, 1.0f };
    style.Colors[ImGuiCol_Text] = { 1.0f, 1.0f, 1.0f, 1.0f };
    style.Colors[ImGuiCol_TextDisabled] = { 0.7f, 0.7f, 0.7f, 1.0f };

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    m_searchQuery = "";
    m_selectedSearchResult = -1;
    m_selectedDivision = -1;
}

UIOverlay::~UIOverlay() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIOverlay::draw(const MapData& mapData, const Camera& camera, int w, int h) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Left side - Top to Bottom with more spacing
    drawTitle();                    // Top-left (Y=10)
    drawLegend();                   // Below title (Y=80)
    drawDivisionDropdown(mapData);  // Moved LOWER to Y=320
    drawSearchBar(mapData);         // Bottom-left (Y=580)

    // Right side
    drawControls();                 // Top-right (Y=10)
    drawInfoPanel(mapData);         // Bottom-right (Y=520)

    drawDistrictLabelsInternal(mapData, camera, w, h);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIOverlay::drawDivisionDropdown(const MapData& mapData) {
    // Position LOWER at Y=320 to avoid overlapping with the map
    ImGui::SetNextWindowPos({ 10.f, 320.f }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ 250.f, 0.f });
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::Begin("Select Division", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Dropdown for divisions
    if (ImGui::BeginCombo("##Division", m_selectedDivision >= 0 ? DIVISION_NAMES[m_selectedDivision] : "Choose Division")) {
        for (int i = 0; i < 8; i++) {
            bool isSelected = (m_selectedDivision == i);
            if (ImGui::Selectable(DIVISION_NAMES[i], isSelected)) {
                m_selectedDivision = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Show districts in a dropdown below the division selection
    if (m_selectedDivision >= 0) {
        ImGui::Separator();
        ImGui::Text("Districts in %s:", DIVISION_NAMES[m_selectedDivision]);
        ImGui::Separator();

        // Collect districts for this division
        std::vector<std::pair<std::string, int>> divisionDistricts;
        for (size_t i = 0; i < mapData.districts().size(); i++) {
            if (mapData.districts()[i].divisionIdx == m_selectedDivision) {
                divisionDistricts.push_back({ mapData.districts()[i].name, (int)i });
            }
        }

        // Sort alphabetically
        std::sort(divisionDistricts.begin(), divisionDistricts.end());

        // Use a combo box for districts (dropdown)
        std::string preview = "Select District";
        for (const auto& district : divisionDistricts) {
            if (m_selectedSearchResult == district.second) {
                preview = district.first;
                break;
            }
        }

        if (ImGui::BeginCombo("##District", preview.c_str())) {
            for (const auto& district : divisionDistricts) {
                bool isSelected = (m_selectedSearchResult == district.second);
                if (ImGui::Selectable(district.first.c_str(), isSelected)) {
                    m_selectedSearchResult = district.second;
                    const_cast<MapData&>(mapData).setSelected(district.second);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::End();
}

void UIOverlay::drawSearchBar(const MapData& mapData) {
    // Position at bottom-left (Y=580) - moved lower
    ImGui::SetNextWindowPos({ 10.f, 580.f }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ 280.f, 0.f });
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::Begin("Search District", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    char buffer[256];
    strncpy(buffer, m_searchQuery.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    ImGui::PushItemWidth(200.0f);
    if (ImGui::InputText("##search", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        m_searchQuery = buffer;
        m_searchResults.clear();
        if (!m_searchQuery.empty()) {
            std::string queryLower = toLower(m_searchQuery);
            for (size_t i = 0; i < mapData.districts().size(); i++) {
                const auto& d = mapData.districts()[i];
                std::string nameLower = toLower(d.name);
                if (nameLower.find(queryLower) != std::string::npos) {
                    m_searchResults.push_back((int)i);
                }
            }
        }
        m_selectedSearchResult = -1;
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(50, 0))) {
        m_searchQuery = "";
        m_searchResults.clear();
        m_selectedSearchResult = -1;
        memset(buffer, 0, sizeof(buffer));
    }

    // Show search results in a dropdown below the search bar
    if (!m_searchResults.empty()) {
        ImGui::Separator();
        ImGui::Text("Found %d districts:", (int)m_searchResults.size());
        ImGui::Separator();

        // Use a combo box for search results
        std::string preview = "Select a district";
        for (int idx : m_searchResults) {
            if (m_selectedSearchResult == idx) {
                preview = mapData.districts()[idx].name;
                break;
            }
        }

        if (ImGui::BeginCombo("##SearchResult", preview.c_str())) {
            for (int idx : m_searchResults) {
                const auto& d = mapData.districts()[idx];
                std::string label = d.name + " (" + d.division + ")";
                bool isSelected = (m_selectedSearchResult == idx);
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    m_selectedSearchResult = idx;
                    const_cast<MapData&>(mapData).setSelected(idx);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    else if (!m_searchQuery.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "No districts found");
    }

    ImGui::End();
}

void UIOverlay::drawDistrictLabelsInternal(const MapData& mapData, const Camera& camera, int width, int height) {
    if (width <= 0 || height <= 0) return;

    glm::mat4 view, proj;
    try {
        view = camera.getView();
        proj = camera.getProjection();
    }
    catch (...) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);

    ImGui::Begin("DistrictLabels", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList) {
        int labelCount = 0;
        const District* hoveredDistrict = mapData.hoveredDistrict();
        const District* selectedDistrict = mapData.selectedDistrict();

        for (const auto& district : mapData.districts()) {
            if (labelCount++ > 80) break;
            if (district.name.empty()) continue;

            glm::vec4 worldPos(district.centroid.x, district.currentY + 0.5f, district.centroid.y, 1.0f);
            glm::vec4 clipPos = proj * view * worldPos;

            if (clipPos.w <= 0.001f) continue;

            glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

            if (ndc.x < -1.1f || ndc.x > 1.1f || ndc.y < -1.1f || ndc.y > 1.1f) continue;
            if (ndc.z < -1.0f || ndc.z > 1.0f) continue;

            float screenX = (ndc.x + 1.0f) * 0.5f * width;
            float screenY = (1.0f - ndc.y) * 0.5f * height;

            if (screenX < 15 || screenX > width - 15 || screenY < 15 || screenY > height - 15) continue;

            int divIdx = district.divisionIdx;
            if (divIdx < 0) divIdx = 0;
            if (divIdx >= 8) divIdx = 0;

            bool isHovered = (&district == hoveredDistrict);
            bool isSelected = (&district == selectedDistrict);

            ImVec4 divColor = DIV_COLORS[divIdx];

            if (isHovered) {
                divColor = ImVec4(divColor.x * 1.3f, divColor.y * 1.3f, divColor.z * 1.3f, 1.0f);
            }
            if (isSelected) {
                divColor = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
            }

            std::string label = district.name;
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());

            if (textSize.x <= 0 || textSize.y <= 0 || textSize.x > 300) continue;

            ImVec2 textPos(screenX - textSize.x * 0.5f, screenY - textSize.y * 0.5f);

            float padding = 6.0f;
            float rounding = 4.0f;
            ImVec2 bgMin(textPos.x - padding, textPos.y - padding);
            ImVec2 bgMax(textPos.x + textSize.x + padding, textPos.y + textSize.y + padding);

            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 200), rounding);

            float borderThickness = isHovered ? 2.0f : 1.2f;
            drawList->AddRect(bgMin, bgMax,
                IM_COL32((int)(divColor.x * 255), (int)(divColor.y * 255), (int)(divColor.z * 255), 200),
                rounding, 0, borderThickness);

            ImU32 textColor;
            if (isSelected) {
                textColor = IM_COL32(255, 220, 80, 255);
            }
            else if (isHovered) {
                textColor = IM_COL32(255, 255, 200, 255);
            }
            else {
                textColor = IM_COL32((int)(divColor.x * 255), (int)(divColor.y * 255), (int)(divColor.z * 255), 220);
            }

            drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 180), label.c_str());
            drawList->AddText(textPos, textColor, label.c_str());
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void UIOverlay::drawTitle() {
    ImGui::SetNextWindowPos({ 10.f, 10.f }, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::Begin("##title", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(1.35f);
    ImGui::Text("Bangladesh");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.85f, 1.0f, 1.0f));
    ImGui::Text("3D District Map");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.75f, 0.9f, 1.0f));
    ImGui::Text("64 districts  |  8 divisions");
    ImGui::PopStyleColor();

    ImGui::End();
}

void UIOverlay::drawLegend() {
    ImGui::SetNextWindowPos({ 10.f, 80.f }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ 175.f, 0.f });
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::Begin("Divisions", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    for (int i = 0; i < 8; ++i) {
        ImGui::ColorButton("##c", DIV_COLORS[i],
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
            { 14.f, 14.f });
        ImGui::SameLine(0, 8);
        ImGui::TextUnformatted(DIVISION_NAMES[i]);
    }
    ImGui::End();
}

void UIOverlay::drawInfoPanel(const MapData& mapData) {
    const District* d = mapData.selectedDistrict();
    if (!d) d = mapData.hoveredDistrict();
    if (!d) return;

    ImGuiIO& io = ImGui::GetIO();
    float sw = io.DisplaySize.x;
    float sh = io.DisplaySize.y;

    // Position at bottom-right
    ImGui::SetNextWindowPos({ sw - 280.f, sh - 180.f }, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::Begin("District Info", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize);

    int divIdx = d->divisionIdx;
    ImGui::ColorButton("##div", DIV_COLORS[divIdx],
        ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, { 12.f,12.f });
    ImGui::SameLine(0, 8);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(1.15f);
    ImGui::Text("%s", d->name.c_str());
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleColor();

    ImGui::Separator();

    auto row = [](const char* label, const std::string& val) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.85f, 1.0f, 1.0f));
        ImGui::Text("%-12s", label);
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 6);
        ImGui::Text("%s", val.c_str());
        };

    row("Division", d->division);
    if (d->population > 0) {
        std::ostringstream ss;
        int pop = d->population;
        if (pop >= 1000000) ss << (pop / 1000000) << "." << ((pop % 1000000) / 100000) << "M";
        else ss << (pop / 1000) << "K";
        row("Population", ss.str());
    }
    if (d->area > 0.f) {
        std::ostringstream ss; ss << (int)d->area << " km²";
        row("Area", ss.str());
    }
    if (!d->capital.empty()) row("Capital", d->capital);

    if (d == mapData.selectedDistrict()) {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
        ImGui::Text("  ★ Selected");
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

void UIOverlay::drawControls() {
    ImGuiIO& io = ImGui::GetIO();
    float sw = io.DisplaySize.x;

    ImGui::SetNextWindowPos({ sw - 210.f, 10.f }, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::Begin("Controls", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.85f, 1.0f, 1.0f));
    const char* controls[] = {
        "Left drag  orbit",
        "Right drag  pan",
        "Scroll  zoom",
        "Left click  select",
        "Tab  next district",
        "R  reset camera",
        "Esc  deselect",
    };
    for (auto c : controls) ImGui::Text("%s", c);
    ImGui::PopStyleColor();
    ImGui::End();
}