#pragma once
#include <glm/glm.hpp>
#include <string>
#include <iostream>

class XRManager {
public:
    static XRManager& getInstance() {
        static XRManager instance;
        return instance;
    }

    bool init() {
        std::cout << "Initializing OpenXR..." << std::endl;
        // In a real implementation, this would call xrCreateInstance, xrCreateSession, etc.
        bool xrInitSuccess = false; // Simulate failure for demo purposes

        if (!xrInitSuccess) {
            std::cout << "OpenXR failed to initialize. Switching to Simulation Mode." << std::endl;
            m_isSimulated = true;
        }
        return true;
    }

    void submitFrame() {
        if (m_isSimulated) {
            // Update simulated camera view/projection based on mouse/keyboard
        } else {
            // OpenXR session submit
        }
    }

    void setSimulationMode(bool enabled) { m_isSimulated = enabled; }
    bool isSimulated() const { return m_isSimulated; }

    glm::mat4 getViewMatrix() {
        return glm::mat4(1.0f); // Placeholder
    }

    glm::mat4 getProjectionMatrix() {
        return glm::mat4(1.0f); // Placeholder
    }

private:
    XRManager() = default;
    bool m_isSimulated = false;
};
