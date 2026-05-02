#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "RiverData.h"

class Shader;
class Camera;

class RiverRenderer {
public:
    RiverRenderer(RiverData& riverData);
    ~RiverRenderer();

    void draw(const Camera& camera, float time);

private:
    void buildGPUBuffers();
    void buildLineStrips();

    RiverData& m_riverData;
    std::unique_ptr<Shader> m_shader;

    // Single VAO for all rivers (line strips)
    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    // For MultiDraw or separate draw calls
    std::vector<GLint> m_firstIndices;
    std::vector<GLsizei> m_vertexCounts;
    GLsizei m_totalVertices = 0;
};