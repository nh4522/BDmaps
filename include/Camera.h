#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(int width, int height);

    // Orbital controls
    void orbit(float dYaw, float dPitch);   // mouse drag
    void pan(float dx, float dy);           // right-click drag
    void zoom(float delta);                 // scroll wheel
    void focusOn(glm::vec3 target, float distance); // click to focus

    void update(float dt);

    glm::mat4 getView()       const;
    glm::mat4 getProjection() const;
    glm::vec3 getPosition()   const { return m_position; }
    glm::vec3 getTarget()     const { return m_target; }

    void setAspect(float aspect) { m_aspect = aspect; }

    // Added missing setter methods
    void setPosition(const glm::vec3& pos) { m_position = pos; recalcPosition(); }
    void setTarget(const glm::vec3& target) { m_target = target; m_targetGoal = target; recalcPosition(); }

    // Limits
    static constexpr float MIN_DIST = 2.0f;
    static constexpr float MAX_DIST = 40.0f;
    static constexpr float MIN_PITCH = 10.0f;
    static constexpr float MAX_PITCH = 88.0f;

private:
    void recalcPosition();

    glm::vec3 m_target = { 0.f, 0.f, 0.f };
    glm::vec3 m_position = { 0.f, 10.f, 20.f };
    glm::vec3 m_targetGoal = { 0.f, 0.f, 0.f };

    float m_yaw = 0.f;
    float m_pitch = 45.f;
    float m_dist = 22.f;
    float m_distGoal = 22.f;
    float m_aspect = 16.f / 9.f;
    float m_fov = 45.f;
    float m_near = 0.1f;
    float m_far = 200.f;
};