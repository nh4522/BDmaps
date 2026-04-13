#pragma once
#include <glm/glm.hpp>

class Camera;
class Renderer;
class MapData;

class InputHandler {
public:
    InputHandler(Camera& cam, Renderer& renderer, MapData& mapData, int w, int h);

    void onMouseButton(int button, int action, int mods);
    void onCursorPos(double x, double y);
    void onScroll(double dx, double dy);
    void onKey(int key, int scancode, int action, int mods);
    void update(float totalTime);

    // Add this method declaration
    void selectDistrict(int idx);

private:
    glm::vec3 raycastToGroundPlane(double mx, double my);
    void      doRaycasting(double mx, double my);

    Camera& m_camera;
    Renderer& m_renderer;
    MapData& m_mapData;
    int       m_width, m_height;

    // Mouse state
    double m_lastX = 0, m_lastY = 0;
    bool   m_leftDown = false;
    bool   m_rightDown = false;
    bool   m_firstMove = true;
};