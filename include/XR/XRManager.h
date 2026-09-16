#pragma once

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <glad/glad.h>
#include <vector>
#include <string>

struct swapchain_surfdata_t {
    GLuint fbo;
    GLuint depthbuffer;
};

struct swapchain_t {
    XrSwapchain handle;
    int32_t width;
    int32_t height;
    std::vector<XrSwapchainImageOpenGLKHR> surface_images;
    std::vector<swapchain_surfdata_t>      surface_data;
};

struct input_state_t {
    XrActionSet actionSet;
    XrAction    poseAction;
    XrAction    selectAction;
    XrPath   handSubactionPath[2];
    XrSpace  handSpace[2];
    XrPosef  handPose[2];
    XrBool32 renderHand[2];
    XrBool32 handSelect[2];
};

class XRManager {
public:
    static XRManager& getInstance() {
        static XRManager instance;
        return instance;
    }

    bool init(const char* app_name, int64_t swapchain_format);
    void shutdown();
    void pollEvents(bool& exit);
    void pollActions();
    void pollPredicted(XrTime predicted_time);
    void renderFrame();
    bool renderLayer(XrTime predictedTime, std::vector<XrCompositionLayerProjectionView>& projectionViews, XrCompositionLayerProjection& layer);

    XrInstance getInstanceHandle() const { return m_instance; }
    XrSession getSessionHandle() const { return m_session; }
    std::vector<swapchain_t>& getSwapchains() { return m_swapchains; }

private:
    XRManager() = default;

    PFN_xrGetOpenGLGraphicsRequirementsKHR ext_xrGetOpenGLGraphicsRequirementsKHR = nullptr;
    PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
    PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;

    XrInstance     m_instance = {};

    XrSession      m_session = {};
    XrSessionState  m_session_state = XR_SESSION_STATE_UNKNOWN;
    bool           m_running = false;
    XrSpace        m_app_space = {};
    XrSystemId     m_system_id = XR_NULL_SYSTEM_ID;
    input_state_t  m_input = { };
    XrEnvironmentBlendMode m_blend = {};
    XrDebugUtilsMessengerEXT m_debug = {};
    std::vector<XrView> m_views;
    std::vector<XrViewConfigurationView> m_config_views;
    std::vector<swapchain_t> m_swapchains;
};
