#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader {
public:
    Shader(const std::string& vertPath, const std::string& fragPath);
    Shader(const char* vertSrc, const char* fragSrc, bool fromSource);
    ~Shader();

    void use() const;
    GLuint id() const { return m_id; }

    void setInt  (const char* name, int v)          const;
    void setFloat(const char* name, float v)        const;
    void setVec2 (const char* name, glm::vec2 v)   const;
    void setVec3 (const char* name, glm::vec3 v)   const;
    void setVec4 (const char* name, glm::vec4 v)   const;
    void setMat4 (const char* name, glm::mat4& m)  const;

private:
    GLuint compile(GLenum type, const char* src);
    void   link(GLuint vert, GLuint frag);
    GLuint m_id = 0;
};
