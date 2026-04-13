#include <glad/glad.h>
#include "Renderer.h"
#include "Camera.h"
#include "MapData.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

extern const std::array<DivisionInfo, 8> DIVISIONS;

Renderer::Renderer(MapData& mapData) : m_mapData(mapData) {
    m_districtShader = std::make_unique<Shader>("shaders/district.vert", "shaders/district.frag");
    m_wireShader = std::make_unique<Shader>("shaders/wire.vert", "shaders/wire.frag");
    m_waterShader = std::make_unique<Shader>("shaders/water.vert", "shaders/water.frag");

    std::cout << "[Renderer] Shaders loaded successfully" << std::endl;
    buildWaterMesh();
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &m_waterVAO);
    glDeleteBuffers(1, &m_waterVBO);
    glDeleteBuffers(1, &m_waterEBO);
}

void Renderer::update(float dt, float totalTime) {
    m_mapData.update(dt, totalTime);
}

void Renderer::buildWaterMesh() {
    const int   GRID = 80;
    const float SIZE = 70.0f;
    const float HALF = SIZE * 0.5f;

    std::vector<float>    verts;
    std::vector<uint32_t> indices;

    for (int z = 0; z <= GRID; ++z) {
        for (int x = 0; x <= GRID; ++x) {
            float px = -HALF + (float)x / GRID * SIZE;
            float pz = -HALF + (float)z / GRID * SIZE;
            verts.insert(verts.end(), { px, -0.3f, pz,
                                       (float)x / GRID, (float)z / GRID });
        }
    }

    for (int z = 0; z < GRID; ++z) {
        for (int x = 0; x < GRID; ++x) {
            uint32_t tl = z * (GRID + 1) + x;
            uint32_t tr = tl + 1;
            uint32_t bl = tl + (GRID + 1);
            uint32_t br = bl + 1;
            indices.insert(indices.end(), { tl, bl, tr, tr, bl, br });
        }
    }

    m_waterIndexCount = (int)indices.size();

    glGenVertexArrays(1, &m_waterVAO);
    glGenBuffers(1, &m_waterVBO);
    glGenBuffers(1, &m_waterEBO);

    glBindVertexArray(m_waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_waterVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    std::cout << "[Renderer] Water mesh created with " << m_waterIndexCount << " indices" << std::endl;
}

void Renderer::draw(const Camera& cam, float time) {
    drawWater(cam, time);
    drawDistricts(cam, time);
}

void Renderer::drawDistricts(const Camera& cam, float time) {
    auto view = cam.getView();
    auto proj = cam.getProjection();
    glm::mat4 model = glm::mat4(1.0f);

    float dayCycle = sin(time * 0.05f) * 0.5f + 0.5f;
    glm::vec3 lightColor = m_lightColor * (0.6f + dayCycle * 0.5f);
    glm::vec3 lightDir = m_lightDir;

    float angle = time * 0.02f;
    lightDir = glm::normalize(glm::vec3(
        cos(angle) * 0.5f + 0.5f,
        1.0f,
        sin(angle) * 0.5f + 0.3f
    ));

    m_districtShader->use();
    m_districtShader->setMat4("uModel", model);
    m_districtShader->setMat4("uView", view);
    m_districtShader->setMat4("uProjection", proj);
    m_districtShader->setVec3("uLightDir", lightDir);
    m_districtShader->setVec3("uLightColor", lightColor);
    m_districtShader->setVec3("uCameraPos", cam.getPosition());
    m_districtShader->setFloat("uTime", time);

    for (auto& d : m_mapData.districts()) {
        glm::vec3 col = DIVISIONS[d.divisionIdx].color;
        if (d.hovered) col = DIVISIONS[d.divisionIdx].colorLight;
        if (d.selected) col = glm::vec3(0.95f, 0.85f, 0.55f);

        m_districtShader->setVec3("uBaseColor", col);
        m_districtShader->setFloat("uCurrentY", d.currentY);
        m_districtShader->setFloat("uHovered", d.hovered ? 1.f : 0.f);
        m_districtShader->setFloat("uSelected", d.selected ? 1.f : 0.f);

        m_districtShader->setInt("uIsSide", 0);
        glBindVertexArray(d.vao);
        glDrawArrays(GL_TRIANGLES, 0, d.vertexCount);

        m_districtShader->setInt("uIsSide", 1);
        glBindVertexArray(d.sideVao);
        glDrawArrays(GL_TRIANGLES, 0, d.sideVertCount);
    }

    m_wireShader->use();
    m_wireShader->setMat4("uModel", model);
    m_wireShader->setMat4("uView", view);
    m_wireShader->setMat4("uProjection", proj);

    glLineWidth(1.5f);
    for (auto& d : m_mapData.districts()) {
        m_wireShader->setFloat("uCurrentY", d.currentY);

        glm::vec4 wireColor = d.selected ? glm::vec4(1.0f, 0.85f, 0.3f, 1.0f)
            : d.hovered ? glm::vec4(0.95f, 0.95f, 1.0f, 0.9f)
            : glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);
        m_wireShader->setVec4("uColor", wireColor);
        glBindVertexArray(d.wireVao);
        glDrawArrays(GL_LINES, 0, d.wireVertCount);
    }
    glBindVertexArray(0);
}

void Renderer::drawWater(const Camera& cam, float time) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 model = glm::mat4(1.0f);
    auto view = cam.getView();
    auto proj = cam.getProjection();

    m_waterShader->use();
    m_waterShader->setMat4("uModel", model);
    m_waterShader->setMat4("uView", view);
    m_waterShader->setMat4("uProjection", proj);
    m_waterShader->setVec3("uCameraPos", cam.getPosition());
    m_waterShader->setVec3("uLightDir", m_lightDir);
    m_waterShader->setFloat("uTime", time);

    glBindVertexArray(m_waterVAO);
    glDrawElements(GL_TRIANGLES, m_waterIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}