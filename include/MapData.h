#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <array>

struct DivisionInfo {
    std::string name;
    glm::vec3   color;
    glm::vec3   colorLight;
};

static const std::array<DivisionInfo, 8> DIVISIONS = { {
    {"Dhaka",      {0.12f, 0.48f, 0.37f}, {0.16f, 0.67f, 0.51f}},
    {"Chittagong", {0.10f, 0.37f, 0.63f}, {0.15f, 0.50f, 0.83f}},
    {"Rajshahi",   {0.48f, 0.24f, 0.10f}, {0.69f, 0.35f, 0.15f}},
    {"Khulna",     {0.10f, 0.42f, 0.48f}, {0.14f, 0.61f, 0.68f}},
    {"Barisal",    {0.29f, 0.16f, 0.48f}, {0.42f, 0.24f, 0.67f}},
    {"Sylhet",     {0.16f, 0.42f, 0.10f}, {0.23f, 0.60f, 0.15f}},
    {"Rangpur",    {0.48f, 0.35f, 0.10f}, {0.69f, 0.52f, 0.15f}},
    {"Mymensingh", {0.10f, 0.24f, 0.42f}, {0.14f, 0.36f, 0.60f}},
} };

struct District {
    std::string            name;
    std::string            division;
    int                    divisionIdx = 0;
    float                  area = 0.f;
    int                    population = 0;
    std::string            capital;
    std::vector<glm::vec2> polygon;
    glm::vec2              centroid = { 0.f, 0.f };
    float                  currentY = 0.f;
    float                  targetY = 0.f;
    float                  phaseOffset = 0.f;
    float                  extrudeHeight = 0.6f;
    bool                   hovered = false;
    bool                   selected = false;
    GLuint                 vao = 0, vbo = 0;
    GLuint                 sideVao = 0, sideVbo = 0;
    GLuint                 wireVao = 0, wireVbo = 0;
    int                    vertexCount = 0;
    int                    sideVertCount = 0;
    int                    wireVertCount = 0;
};

class MapData {
public:
    explicit MapData(const std::string& geojsonPath);
    ~MapData();
    void update(float dt, float totalTime);
    std::vector<District>& districts() { return m_districts; }
    const std::vector<District>& districts() const { return m_districts; }
    District* hoveredDistrict();
    District* selectedDistrict();
    const District* hoveredDistrict()  const;
    const District* selectedDistrict() const;
    void setHovered(int idx);
    void setSelected(int idx);
    int  hoveredIdx()  const { return m_hoveredIdx; }
    int  selectedIdx() const { return m_selectedIdx; }
    glm::vec2 worldMin()    const { return m_worldMin; }
    glm::vec2 worldMax()    const { return m_worldMax; }
    glm::vec2 worldCenter() const { return (m_worldMin + m_worldMax) * 0.5f; }

    // Add this method
    const std::vector<DivisionInfo>& getDivisions() const { return m_divisions; }

private:
    void loadGeoJSON(const std::string& path);
    void buildEmbeddedData();
    void projectCoordinates();
    void buildGPUBuffers();
    void computeCentroids();

    std::vector<District> m_districts;
    std::vector<DivisionInfo> m_divisions;  // ADD THIS LINE
    glm::vec2             m_worldMin = { 1e9f,  1e9f };
    glm::vec2             m_worldMax = { -1e9f, -1e9f };
    int                   m_hoveredIdx = -1;
    int                   m_selectedIdx = -1;
};