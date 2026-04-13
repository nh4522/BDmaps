#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Camera;
class MapData;
class Shader;

class Renderer {
public:
    explicit Renderer(MapData& mapData);
    ~Renderer();

    void update(float dt, float totalTime);
    void draw(const Camera& camera, float totalTime);

private:
    void buildWaterMesh();
    void drawDistricts(const Camera& camera, float totalTime);
    void drawWater(const Camera& camera, float totalTime);
    void drawSky(const Camera& camera);

    MapData& m_mapData;

    std::unique_ptr<Shader> m_districtShader;
    std::unique_ptr<Shader> m_wireShader;
    std::unique_ptr<Shader> m_waterShader;

    // Water
    GLuint m_waterVAO = 0, m_waterVBO = 0, m_waterEBO = 0;
    int    m_waterIndexCount = 0;

    // Lighting constants
    glm::vec3 m_lightDir   = glm::normalize(glm::vec3(0.6f, 1.2f, 0.8f));
    glm::vec3 m_lightColor = {1.0f, 0.97f, 0.88f};
};
