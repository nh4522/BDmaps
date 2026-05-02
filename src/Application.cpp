#include "Application.h"
#include "Camera.h"
#include "Renderer.h"
#include "MapData.h"
#include "InputHandler.h"
#include "UIOverlay.h"
#include "RiverData.h"
#include "RiverRenderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

static Application* g_app = nullptr;

void Application::framebufferSizeCallback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
    if (g_app) {
        g_app->m_width = w;
        g_app->m_height = h;
        if (g_app->m_camera) {
            g_app->m_camera->setAspect(static_cast<float>(w) / static_cast<float>(h));
        }
        if (g_app->m_input) {
            g_app->m_input->updateDimensions(w, h);
        }
    }
}

void Application::mouseButtonCallback(GLFWwindow*, int btn, int action, int mods) {
    if (g_app) g_app->m_input->onMouseButton(btn, action, mods);
}

void Application::cursorPosCallback(GLFWwindow*, double x, double y) {
    if (g_app) g_app->m_input->onCursorPos(x, y);
}

void Application::scrollCallback(GLFWwindow*, double dx, double dy) {
    if (g_app) g_app->m_input->onScroll(dx, dy);
}

void Application::keyCallback(GLFWwindow*, int key, int sc, int action, int mods) {
    if (g_app) g_app->m_input->onKey(key, sc, action, mods);
}

Application::Application(int width, int height, const std::string& title)
    : m_width(width), m_height(height), m_title(title)
{
    g_app = this;
    initGLFW();
    initGLAD();
    initSystems();
}

Application::~Application() {
    m_ui.reset();
    m_renderer.reset();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::initGLFW() {
    if (!glfwInit()) throw std::runtime_error("Failed to initialise GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);  // Allow resizing

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) { glfwTerminate(); throw std::runtime_error("Failed to create GLFW window"); }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    // Set up callbacks
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetKeyCallback(m_window, keyCallback);

    // Set up window resize callback for ImGui
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int w, int h) {
        if (g_app) {
            g_app->m_width = w;
            g_app->m_height = h;
            if (g_app->m_camera) {
                g_app->m_camera->setAspect(static_cast<float>(w) / static_cast<float>(h));
            }
        }
        });
}

void Application::initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialise GLAD");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    std::cout << "[GL] Vendor  : " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "[GL] Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "[GL] Version : " << glGetString(GL_VERSION) << std::endl;
}

void Application::initSystems() {
    m_mapData = std::make_unique<MapData>("data/bangladesh_districts.geojson");
    m_camera = std::make_unique<Camera>(m_width, m_height);
    m_riverData = std::make_unique<RiverData>("data/riverl_bgd (1).json");
    m_riverRenderer = std::make_unique<RiverRenderer>(*m_riverData);

    // Set better initial camera position
    m_camera->setPosition(glm::vec3(0.0f, 12.0f, 22.0f));
    m_camera->setTarget(glm::vec3(0.0f, 2.0f, 0.0f));

    m_renderer = std::make_unique<Renderer>(*m_mapData);
    m_input = std::make_unique<InputHandler>(*m_camera, *m_renderer, *m_mapData, m_width, m_height);
    m_ui = std::make_unique<UIOverlay>(m_window, m_width, m_height);
    m_lastTime = glfwGetTime();

    std::cout << "[Application] Systems initialized successfully" << std::endl;
}

void Application::run() {
    std::cout << "[Application] Starting main loop" << std::endl;

    while (!glfwWindowShouldClose(m_window)) {
        double now = glfwGetTime();
        float  dt = static_cast<float>(now - m_lastTime);
        m_lastTime = now;
        m_totalTime += dt;

        processInput();
        update(dt);
        render();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Application::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);
    m_input->update(m_totalTime);
}

void Application::update(float dt) {
    m_camera->update(dt);
    m_renderer->update(dt, m_totalTime);

    // Handle search result selection from UI
    int searchResult = m_ui->getSelectedSearchResult();
    if (searchResult >= 0) {
        m_input->selectDistrict(searchResult);
        m_ui->clearSelectedSearchResult();
    }
}

void Application::render() {
    glClearColor(0.04f, 0.08f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_renderer->draw(*m_camera, m_totalTime);
    m_ui->draw(*m_mapData, *m_camera, m_width, m_height);
    m_riverRenderer->draw(*m_camera, m_totalTime);
}