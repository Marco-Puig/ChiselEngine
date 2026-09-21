#pragma once

#include "VRPlayerRig.h"
#include "xr/XRManager.h"

#include <GLFW/glfw3.h>

inline VRInputFrame gatherVRInput(const XRManager& xr) {
    VRInputFrame input;

    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
        GLFWgamepadstate state{};

        if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
            input.move.x = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
            input.move.y = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];

            input.turn.x = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
            input.turn.y = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

            input.snapTurnLeft =
                state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;

            input.snapTurnRight =
                state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS;
        }
    }

    const glm::vec2 leftThumbstick = xr.getThumbstick(0);
    const glm::vec2 rightThumbstick = xr.getThumbstick(1);

    if (glm::length(leftThumbstick) > glm::length(input.move)) {
        input.move = leftThumbstick;
    }

    if (glm::length(rightThumbstick) > glm::length(input.turn)) {
        input.turn = rightThumbstick;
    }

    return input;
}