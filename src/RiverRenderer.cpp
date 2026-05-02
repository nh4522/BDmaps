#include "RiverRenderer.h"
#include "Camera.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

RiverRenderer::RiverRenderer(RiverData& riverData) : m_riverData(riverData) {
    m_shader = std::make_unique<Shader>("shaders/river.vert", "shaders/river.frag");
    buildLineStrips();
    buildGPUBuffers();
    std::cout << "[RiverRenderer] Initialized with " << m_riverData.segmentCount() << " segments" << std::endl;
}

RiverRenderer::~RiverRenderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
}

void RiverRenderer::buildLineStrips() {
    m_firstIndices.clear();
    m_vertexCounts.clear();

    GLint offset = 0;
    for (const auto& seg : m_riverData.segments()) {
        m_firstIndices.push_back(offset);
        m_vertexCounts.push_back(static_cast<GLsizei>(seg.vertices.size()));
        offset += seg.vertices.size();
    }
    m_totalVertices = offset;
}

void RiverRenderer::buildGPUBuffers() {
    // Flatten all vertices into one buffer
    std::vector<float> flatVerts;
    flatVerts.reserve(m_totalVertices * 2);

    for (const auto& seg : m_riverData.segments()) {
        for (const auto& v : seg.vertices) {
            flatVerts.push_back(v.x);
            flatVerts.push_back(v.y);
        }
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, flatVerts.size() * sizeof(float),
        flatVerts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void RiverRenderer::draw(const Camera& camera, float time) {
    m_shader->use();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getView();
    glm::mat4 proj = camera.getProjection();

    m_shader->setMat4("uModel", model);
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", proj);
    m_shader->setFloat("uTime", time);
    m_shader->setFloat("uWaterLevel", -0.2f);  // Slightly below district base

    // River colors - cyan/blue with slight variation
    m_shader->setVec3("uColorDeep", glm::vec3(0.1f, 0.35f, 0.6f));
    m_shader->setVec3("uColorShallow", glm::vec3(0.2f, 0.55f, 0.8f));

    glLineWidth(2.5f);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vao);

    // Multi-draw all line strips
    glMultiDrawArrays(GL_LINE_STRIP, m_firstIndices.data(),
        m_vertexCounts.data(), m_vertexCounts.size());

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}