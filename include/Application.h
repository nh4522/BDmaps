#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>

class Camera;
class Renderer;
class MapData;
class InputHandler;
class UIOverlay;

class Application {
public:
    Application(int width, int height, const std::string& title);
    ~Application();
    void run();

    // Callbacks (static, forwarded to instance)
    static void framebufferSizeCallback(GLFWwindow* w, int width, int height);
    static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* w, double x, double y);
    static void scrollCallback(GLFWwindow* w, double dx, double dy);
    static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);

private:
    void initGLFW();
    void initGLAD();
    void initSystems();
    void update(float dt);
    void render();
    void processInput();

    GLFWwindow* m_window = nullptr;
    int                          m_width, m_height;
    std::string                  m_title;
    double                       m_lastTime = 0.0;
    float                        m_totalTime = 0.0f;

    std::unique_ptr<Camera>      m_camera;
    std::unique_ptr<Renderer>    m_renderer;
    std::unique_ptr<MapData>     m_mapData;
    std::unique_ptr<InputHandler> m_input;
    std::unique_ptr<UIOverlay>   m_ui;        // Make sure this line exists
};