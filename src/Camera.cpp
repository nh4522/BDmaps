#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera(int width, int height)
    : m_aspect(static_cast<float>(width) / height)
{
    recalcPosition();
}

void Camera::orbit(float dYaw, float dPitch) {
    m_yaw += dYaw;
    m_pitch = std::clamp(m_pitch + dPitch, MIN_PITCH, MAX_PITCH);
    recalcPosition();
}

void Camera::pan(float dx, float dy) {
    // Get camera right and up vectors in world space
    glm::vec3 forward = glm::normalize(m_target - m_position);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // Calculate pan speed based on distance
    float speed = m_dist * 0.002f;

    // Move target and position
    // dy is negated so moving mouse UP pans UP (not down)
    m_target -= right * dx * speed;
    m_target += up * (-dy) * speed;
    m_targetGoal = m_target;

    recalcPosition();
}

void Camera::zoom(float delta) {
    m_distGoal = std::clamp(m_distGoal - delta * 1.5f, MIN_DIST, MAX_DIST);
}

void Camera::focusOn(glm::vec3 target, float distance) {
    m_targetGoal = target;
    m_distGoal = std::clamp(distance, MIN_DIST, MAX_DIST);
}

void Camera::update(float dt) {
    float t = 1.0f - std::exp(-8.0f * dt);
    m_dist = m_dist + (m_distGoal - m_dist) * t;
    m_target = m_target + (m_targetGoal - m_target) * t;
    recalcPosition();
}

void Camera::recalcPosition() {
    float yawR = glm::radians(m_yaw);
    float pitchR = glm::radians(m_pitch);
    m_position = m_target + glm::vec3(
        m_dist * std::cos(pitchR) * std::sin(yawR),
        m_dist * std::sin(pitchR),
        m_dist * std::cos(pitchR) * std::cos(yawR)
    );
}

glm::mat4 Camera::getView() const {
    return glm::lookAt(m_position, m_target, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::getProjection() const {
    return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
}