#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

struct RiverSegment {
    std::vector<glm::vec2> vertices;
};

class RiverData {
public:
    RiverData(const std::string& path);
    ~RiverData();

    const std::vector<RiverSegment>& segments() const { return m_segments; }
    size_t segmentCount() const { return m_segments.size(); }
    size_t totalVertices() const { return m_totalVertices; }

private:
    void loadFromGeoJSON(const std::string& path);
    void projectCoordinates();

    std::vector<RiverSegment> m_segments;
    size_t m_totalVertices = 0;

    glm::vec2 m_worldMin, m_worldMax;
};