#include "RiverData.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

RiverData::RiverData(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[RiverData] Could not open: " << path << std::endl;
        return;
    }

    try {
        json root = json::parse(f);

        if (root.contains("geometries")) {
            for (const auto& geom : root["geometries"]) {
                if (geom.contains("coordinates")) {
                    RiverSegment seg;
                    for (const auto& pt : geom["coordinates"]) {
                        seg.vertices.push_back({
                            pt[0].get<float>(),
                            pt[1].get<float>()
                            });
                    }
                    if (seg.vertices.size() >= 2) {
                        m_segments.push_back(std::move(seg));
                        m_totalVertices += m_segments.back().vertices.size();
                    }
                }
            }
        }

        std::cout << "[RiverData] Loaded " << m_segments.size()
            << " river segments (" << m_totalVertices << " vertices)" << std::endl;

        projectCoordinates();
    }
    catch (const std::exception& e) {
        std::cerr << "[RiverData] Error: " << e.what() << std::endl;
    }
}

RiverData::~RiverData() = default;

void RiverData::projectCoordinates() {
    // Find bounds
    float lonMin = 1e9f, lonMax = -1e9f, latMin = 1e9f, latMax = -1e9f;
    for (const auto& seg : m_segments) {
        for (const auto& v : seg.vertices) {
            lonMin = std::min(lonMin, v.x);
            lonMax = std::max(lonMax, v.x);
            latMin = std::min(latMin, v.y);
            latMax = std::max(latMax, v.y);
        }
    }

    // Same projection as districts (20 units wide)
    const float WORLD_SIZE = 20.0f;
    float lonRange = lonMax - lonMin;
    float latRange = latMax - latMin;
    float scale = WORLD_SIZE / std::max(lonRange, latRange);

    m_worldMin = glm::vec2(1e9f);
    m_worldMax = glm::vec2(-1e9f);

    for (auto& seg : m_segments) {
        for (auto& v : seg.vertices) {
            v = glm::vec2(
                (v.x - lonMin) * scale - WORLD_SIZE * lonRange / std::max(lonRange, latRange) * 0.5f,
                (latMax - v.y) * scale - WORLD_SIZE * latRange / std::max(lonRange, latRange) * 0.5f
            );
            m_worldMin.x = std::min(m_worldMin.x, v.x);
            m_worldMin.y = std::min(m_worldMin.y, v.y);
            m_worldMax.x = std::max(m_worldMax.x, v.x);
            m_worldMax.y = std::max(m_worldMax.y, v.y);
        }
    }

    std::cout << "[RiverData] Projected to world coords. Bounds: ["
        << m_worldMin.x << "," << m_worldMin.y << "] to ["
        << m_worldMax.x << "," << m_worldMax.y << "]" << std::endl;
}