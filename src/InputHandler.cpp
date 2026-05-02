#include "InputHandler.h"
#include "Camera.h"
#include "Renderer.h"
#include "MapData.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

InputHandler::InputHandler(Camera& cam, Renderer& renderer, MapData& mapData, int w, int h)
    : m_camera(cam), m_renderer(renderer), m_mapData(mapData), m_width(w), m_height(h) {
}

void InputHandler::onMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        m_leftDown = (action == GLFW_PRESS);
        if (action == GLFW_PRESS) {
            int idx = m_mapData.hoveredIdx();
            if (idx >= 0) {
                if (m_mapData.selectedIdx() == idx) {
                    m_mapData.setSelected(-1);
                    std::cout << "🔘 Deselected district" << std::endl;
                }
                else {
                    m_mapData.setSelected(idx);
                    auto& d = m_mapData.districts()[idx];
                    std::cout << "\n✅ SELECTED: " << d.name << " (" << d.division << ")" << std::endl;
                    m_camera.focusOn(glm::vec3(d.centroid.x, 0.f, d.centroid.y), 8.0f);
                }
            }
            else {
                m_mapData.setSelected(-1);
            }
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        m_rightDown = (action == GLFW_PRESS);
    if (action == GLFW_RELEASE)
        m_firstMove = true;
}

void InputHandler::onCursorPos(double x, double y) {
    if (m_firstMove) { m_lastX = x; m_lastY = y; m_firstMove = false; return; }
    float dx = (float)(x - m_lastX);
    float dy = (float)(y - m_lastY);
    m_lastX = x; m_lastY = y;

    if (m_leftDown && !m_rightDown) {
        m_camera.orbit(dx * 0.35f, dy * 0.35f);
    }
    else if (m_rightDown) {
        m_camera.pan(dx, -dy);
    }
    else {
        doRaycasting(x, y);
    }
}

void InputHandler::onScroll(double, double dy) {
    m_camera.zoom((float)dy * 1.2f);
}

void InputHandler::onKey(int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    switch (key) {
    case GLFW_KEY_R:
        m_camera.focusOn(glm::vec3(0.f), 22.f);
        m_mapData.setSelected(-1);
        std::cout << "🔄 Camera reset" << std::endl;
        break;
    case GLFW_KEY_TAB: {
        int cur = m_mapData.selectedIdx();
        int next = (cur + 1) % (int)m_mapData.districts().size();
        m_mapData.setSelected(next);
        auto& d = m_mapData.districts()[next];
        std::cout << "\n⭕ TAB SELECTED: " << d.name << " (" << d.division << ")" << std::endl;
        m_camera.focusOn(glm::vec3(d.centroid.x, 0.f, d.centroid.y), 7.f);
        break;
    }
    case GLFW_KEY_ESCAPE:
        m_mapData.setSelected(-1);
        std::cout << "❌ Selection cleared" << std::endl;
        break;
    }
}

void InputHandler::update(float) {}

void InputHandler::doRaycasting(double mx, double my) {
    // Get current window size from camera aspect ratio
    // Use m_width and m_height that are updated on resize

    float ndcX = (2.0f * (float)mx / m_width) - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)my / m_height);

    glm::mat4 proj = m_camera.getProjection();
    glm::mat4 view = m_camera.getView();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 farPt = invVP * glm::vec4(ndcX, ndcY, 1.f, 1.f);
    nearPt /= nearPt.w;
    farPt /= farPt.w;

    glm::vec3 rayOrigin = glm::vec3(nearPt);
    glm::vec3 rayDir = glm::normalize(glm::vec3(farPt) - rayOrigin);

    if (std::abs(rayDir.y) < 1e-5f) { m_mapData.setHovered(-1); return; }
    float t = -rayOrigin.y / rayDir.y;
    if (t < 0.f) { m_mapData.setHovered(-1); return; }

    glm::vec3 hit = rayOrigin + t * rayDir;
    glm::vec2 pos2 = { hit.x, hit.z };

    int   bestIdx = -1;
    float bestDist = 1.5f;
    for (int i = 0; i < (int)m_mapData.districts().size(); ++i) {
        auto& d = m_mapData.districts()[i];
        float dist = glm::length(pos2 - d.centroid);
        if (dist < bestDist) { bestDist = dist; bestIdx = i; }
    }

    int oldHover = m_mapData.hoveredIdx();
    m_mapData.setHovered(bestIdx);

    if (bestIdx >= 0 && bestIdx != oldHover) {
        auto& d = m_mapData.districts()[bestIdx];
        // Optional: Uncomment for debugging
        // std::cout << "📍 Hover: " << d.name << std::endl;
    }
}

void InputHandler::selectDistrict(int idx) {
    if (idx >= 0 && idx < (int)m_mapData.districts().size()) {
        m_mapData.setSelected(idx);
        auto& d = m_mapData.districts()[idx];
        std::cout << "\n🔍 Selected: " << d.name << " (" << d.division << ")" << std::endl;
        m_camera.focusOn(glm::vec3(d.centroid.x, 0.f, d.centroid.y), 8.0f);
    }
}