#include "VRPlayerRig.h"
#include <cmath>

VRPlayerRig& VRPlayerRig::getInstance() {
    static VRPlayerRig instance;
    return instance;
}

glm::vec2 VRPlayerRig::applyDeadzone(glm::vec2 value, float deadzone) {
    const float length = glm::length(value);
    if (length < deadzone) return glm::vec2(0.0f);
    const float normalized = (length - deadzone) / (1.0f - deadzone);
    return glm::normalize(value) * normalized;
}

void VRPlayerRig::update(float dt, const VRInputFrame& input, const glm::mat4& rawHeadView) {
    const glm::vec2 move = applyDeadzone(input.move);
    const glm::vec2 turn = applyDeadzone(input.turn);

    const float stickSnapThreshold = 0.75f;
    const bool wantSnapRight = input.snapTurnRight || turn.x > stickSnapThreshold;
    const bool wantSnapLeft = input.snapTurnLeft || turn.x < -stickSnapThreshold;

    if (useSnapTurn) {
        if (wantSnapRight && !m_snapTurnRightActive) {
            m_yaw -= snapTurnAngle;
            m_snapTurnRightActive = true;
        } else if (!wantSnapRight) {
            m_snapTurnRightActive = false;
        }

        if (wantSnapLeft && !m_snapTurnLeftActive) {
            m_yaw += snapTurnAngle;
            m_snapTurnLeftActive = true;
        } else if (!wantSnapLeft) {
            m_snapTurnLeftActive = false;
        }
    } else {
        m_yaw -= turn.x * smoothTurnSpeed * dt;
    }

    const glm::mat4 worldHeadView = applyToView(rawHeadView);

    glm::vec3 forward = -glm::vec3(worldHeadView[0][2], worldHeadView[1][2], worldHeadView[2][2]);
    glm::vec3 right = glm::vec3(worldHeadView[0][0], worldHeadView[1][0], worldHeadView[2][0]);

    auto flatten = [](glm::vec3 v) -> glm::vec3 {
        v.y = 0.0f;
        const float length = glm::length(v);
        if (length < 1e-4f) return glm::vec3(0.0f, 0.0f, -1.0f);
        return glm::normalize(v);
    };

    forward = flatten(forward);
    right = flatten(right);

    glm::vec3 moveDirection = forward * (-move.y) + right * move.x;

    const float directionLength = glm::length(moveDirection);
    if (directionLength > 1.0f) {
        moveDirection /= directionLength;
    }

    m_position += moveDirection * moveSpeed * dt;
}

glm::mat4 VRPlayerRig::getRigToWorld() const {
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_position);
    const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    return translation * rotation;
}

glm::mat4 VRPlayerRig::getWorldToRig() const {
    return glm::inverse(getRigToWorld());
}

glm::mat4 VRPlayerRig::applyToView(const glm::mat4& rawXRView) const {
    return rawXRView * getWorldToRig();
}

void VRPlayerRig::setPosition(float x, float y, float z) { m_position = glm::vec3(x, y, z); }
float VRPlayerRig::getPositionX() const { return m_position.x; }
float VRPlayerRig::getPositionY() const { return m_position.y; }
float VRPlayerRig::getPositionZ() const { return m_position.z; }

void VRPlayerRig::setYaw(float yaw) { m_yaw = yaw; }
float VRPlayerRig::getYaw() const { return m_yaw; }

void VRPlayerRig::setMoveSpeed(float speed) { moveSpeed = speed; }
float VRPlayerRig::getMoveSpeed() const { return moveSpeed; }

void VRPlayerRig::setSnapTurn(bool enabled) { useSnapTurn = enabled; }
bool VRPlayerRig::isSnapTurnEnabled() const { return useSnapTurn; }