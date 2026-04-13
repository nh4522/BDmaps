#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error("Cannot open shader: " + path);
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

Shader::Shader(const std::string& vp, const std::string& fp) {
    auto vs = readFile(vp); auto fs = readFile(fp);
    GLuint v = compile(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = compile(GL_FRAGMENT_SHADER, fs.c_str());
    link(v, f);
}
Shader::Shader(const char* vs, const char* fs, bool) {
    GLuint v = compile(GL_VERTEX_SHADER,   vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);
    link(v, f);
}
Shader::~Shader() { if (m_id) glDeleteProgram(m_id); }

GLuint Shader::compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
        throw std::runtime_error(std::string("Shader compile error:\n") + log);
    }
    return s;
}
void Shader::link(GLuint v, GLuint f) {
    m_id = glCreateProgram();
    glAttachShader(m_id, v); glAttachShader(m_id, f);
    glLinkProgram(m_id);
    GLint ok; glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(m_id, 1024, nullptr, log);
        throw std::runtime_error(std::string("Shader link error:\n") + log);
    }
    glDeleteShader(v); glDeleteShader(f);
}
void Shader::use() const { glUseProgram(m_id); }
void Shader::setInt  (const char* n, int v)         const { glUniform1i(glGetUniformLocation(m_id,n), v); }
void Shader::setFloat(const char* n, float v)       const { glUniform1f(glGetUniformLocation(m_id,n), v); }
void Shader::setVec2 (const char* n, glm::vec2 v)  const { glUniform2fv(glGetUniformLocation(m_id,n),1,glm::value_ptr(v)); }
void Shader::setVec3 (const char* n, glm::vec3 v)  const { glUniform3fv(glGetUniformLocation(m_id,n),1,glm::value_ptr(v)); }
void Shader::setVec4 (const char* n, glm::vec4 v)  const { glUniform4fv(glGetUniformLocation(m_id,n),1,glm::value_ptr(v)); }
void Shader::setMat4 (const char* n, glm::mat4& m) const { glUniformMatrix4fv(glGetUniformLocation(m_id,n),1,GL_FALSE,glm::value_ptr(m)); }
