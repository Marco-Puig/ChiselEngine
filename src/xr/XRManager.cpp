#include "XRManager.h"
#include "platform/Window.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef CHISEL_ENABLE_OPENXR
#ifdef _WIN32
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <unknwn.h>
#endif
#include <openxr/openxr_platform.h>
#include <glad/glad.h>
#include <cstring>
#endif

XRManager& XRManager::getInstance() {
    static XRManager instance;
    return instance;
}

XRManager::~XRManager() {
    shutdown();
}

bool XRManager::setSimulationMode(bool enabled, Window& window) {
    if (enabled == m_simulationRequested && (enabled || m_running))
        return enabled || m_running;
    shutdown();
    m_simulationRequested = enabled;
    if (enabled)
        return true;
    return init(window);
}

bool XRManager::init(Window& window) {
    if (m_simulationRequested)
        return false;
#ifdef CHISEL_ENABLE_OPENXR
    if (createInstance() && createSession(window) && createActions() && createSwapchains()) {
        m_running = true;
        return true;
    }
    destroySessionResources();
#endif
    std::cerr << "OpenXR unavailable; using desktop rendering.\n";
    return false;
}

void XRManager::shutdown() {
#ifdef CHISEL_ENABLE_OPENXR
    destroySessionResources();
#endif
    m_running = false;
}

bool XRManager::beginFrame() {
#ifdef CHISEL_ENABLE_OPENXR
    pollEvents();
    if (!m_running || !m_sessionReady)
        return false;
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    if (!check(xrWaitFrame(m_session, &waitInfo, &m_frameState), "xrWaitFrame"))
        return false;
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    if (!check(xrBeginFrame(m_session, &beginInfo), "xrBeginFrame"))
        return false;

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = m_viewConfiguration;
    locateInfo.displayTime = m_frameState.predictedDisplayTime;
    locateInfo.space = m_stageSpace;
    uint32_t count = 0;
    if (!check(xrLocateViews(m_session, &locateInfo, &viewState, 2,
                             &count, m_views.data()), "xrLocateViews"))
        return false;
    m_frameBegun = true;
    return count == 2;
#else
    return false;
#endif
}

void XRManager::pollEvents() {
#ifdef CHISEL_ENABLE_OPENXR
    if (m_instance == XR_NULL_HANDLE)
        return;
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(m_instance, &event) == XR_SUCCESS) {
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            const auto& changed =
                *reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
            m_sessionState = changed.state;
            if (m_sessionState == XR_SESSION_STATE_READY && !m_sessionReady) {
                XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                beginInfo.primaryViewConfigurationType = m_viewConfiguration;
                m_sessionReady = check(xrBeginSession(m_session, &beginInfo),
                                       "xrBeginSession");
            } else if (m_sessionState == XR_SESSION_STATE_STOPPING && m_sessionReady) {
                check(xrEndSession(m_session), "xrEndSession");
                m_sessionReady = false;
            } else if (m_sessionState == XR_SESSION_STATE_EXITING ||
                       m_sessionState == XR_SESSION_STATE_LOSS_PENDING) {
                m_running = false;
            }
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }
#endif
}

bool XRManager::acquireView(uint32_t eye, glm::mat4& view, glm::mat4& projection) {
#ifdef CHISEL_ENABLE_OPENXR
    if (!m_frameBegun || eye >= 2)
        return false;
    const XrSwapchain swapchain = m_swapchains[eye];
    uint32_t imageIndex = 0;
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (!check(xrAcquireSwapchainImage(swapchain, &acquireInfo, &imageIndex),
               "xrAcquireSwapchainImage"))
        return false;
    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    if (!check(xrWaitSwapchainImage(swapchain, &waitInfo), "xrWaitSwapchainImage"))
        return false;
    m_acquired[eye] = true;
    m_acquiredImage[eye] = imageIndex;

    const XrPosef& pose = m_views[eye].pose;
    const glm::quat orientation(pose.orientation.w, pose.orientation.x,
                                pose.orientation.y, pose.orientation.z);
    const glm::vec3 position(pose.position.x, pose.position.y, pose.position.z);
    const glm::mat4 rotation = glm::mat4_cast(glm::conjugate(orientation));
    view = rotation * glm::translate(glm::mat4(1.0f), -position);
    const XrFovf& fov = m_views[eye].fov;
    const float nearClip = 0.05f;
    const float farClip = 100.0f;
    const float left = nearClip * std::tan(fov.angleLeft);
    const float right = nearClip * std::tan(fov.angleRight);
    const float down = nearClip * std::tan(fov.angleDown);
    const float up = nearClip * std::tan(fov.angleUp);
    projection = glm::frustum(left, right, down, up, nearClip, farClip);
    return true;
#else
    (void)eye; (void)view; (void)projection;
    return false;
#endif
}

void XRManager::releaseView(uint32_t eye) {
#ifdef CHISEL_ENABLE_OPENXR
    if (eye < 2 && m_acquired[eye]) {
        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        check(xrReleaseSwapchainImage(m_swapchains[eye], &releaseInfo),
              "xrReleaseSwapchainImage");
        m_acquired[eye] = false;
    }
#else
    (void)eye;
#endif
}

uint32_t XRManager::getViewTexture(uint32_t eye) const {
#ifdef CHISEL_ENABLE_OPENXR
    return eye < 2 && m_acquired[eye] ? m_swapchainImages[eye][m_acquiredImage[eye]] : 0;
#else
    (void)eye;
    return 0;
#endif
}

uint32_t XRManager::getViewWidth(uint32_t eye) const {
    return eye < 2 ? m_viewWidths[eye] : 0;
}

uint32_t XRManager::getViewHeight(uint32_t eye) const {
    return eye < 2 ? m_viewHeights[eye] : 0;
}

void XRManager::endFrame() {
#ifdef CHISEL_ENABLE_OPENXR
    if (!m_frameBegun)
        return;
    for (uint32_t eye = 0; eye < 2; ++eye)
        releaseView(eye);
    std::array<XrCompositionLayerProjectionView, 2> projectionViews{};
    for (auto& projectionView : projectionViews)
        projectionView.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
    for (uint32_t eye = 0; eye < 2; ++eye) {
        projectionViews[eye].pose = m_views[eye].pose;
        projectionViews[eye].fov = m_views[eye].fov;
        projectionViews[eye].subImage.swapchain = m_swapchains[eye];
        projectionViews[eye].subImage.imageRect.extent = {static_cast<int32_t>(m_viewWidths[eye]),
                                                          static_cast<int32_t>(m_viewHeights[eye])};
    }
    XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.space = m_stageSpace;
    layer.viewCount = 2;
    layer.views = projectionViews.data();
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = m_frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer)};
    endInfo.layers = layers;
    check(xrEndFrame(m_session, &endInfo), "xrEndFrame");
    m_frameBegun = false;
#endif
}

void XRManager::syncActions() {
#ifdef CHISEL_ENABLE_OPENXR
    if (m_session == XR_NULL_HANDLE)
        return;
    XrActiveActionSet activeSet{m_actionSet, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeSet;
    check(xrSyncActions(m_session, &syncInfo), "xrSyncActions");
#endif
}

bool XRManager::controllerButtonPressed(uint32_t controller, uint32_t button) const {
#ifdef CHISEL_ENABLE_OPENXR
    if (button != 0 || controller > 1)
        return false;
    XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
    getInfo.action = controller == 0 ? m_leftSelectAction : m_rightSelectAction;
    getInfo.subactionPath = controller == 0 ? m_leftHandPath : m_rightHandPath;
    XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
    return xrGetActionStateBoolean(m_session, &getInfo, &state) == XR_SUCCESS &&
           state.isActive && state.currentState;
#else
    (void)controller; (void)button;
    return false;
#endif
}

#ifdef CHISEL_ENABLE_OPENXR
bool XRManager::check(XrResult result, const char* operation) const {
    if (XR_FAILED(result))
        std::cerr << operation << " failed: " << result << '\n';
    return XR_SUCCEEDED(result);
}

bool XRManager::createInstance() {
    uint32_t extensionCount = 0;
    if (!check(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
               "xrEnumerateInstanceExtensionProperties"))
        return false;
    std::vector<XrExtensionProperties> extensions(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions.data());
    bool opengl = false;
    for (const auto& extension : extensions)
        opengl |= std::strcmp(extension.extensionName, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0;
    if (!opengl)
        return false;
    const char* names[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    std::strncpy(createInfo.applicationInfo.applicationName, "ChiselEngine", XR_MAX_APPLICATION_NAME_SIZE - 1);
    std::strncpy(createInfo.applicationInfo.engineName, "ChiselEngine", XR_MAX_ENGINE_NAME_SIZE - 1);
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = names;
    if (!check(xrCreateInstance(&createInfo, &m_instance), "xrCreateInstance"))
        return false;
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    return check(xrGetSystem(m_instance, &systemInfo, &m_system), "xrGetSystem");
}

bool XRManager::createSession(Window& window) {
    PFN_xrGetOpenGLGraphicsRequirementsKHR getRequirements = nullptr;
    if (!check(xrGetInstanceProcAddr(m_instance, "xrGetOpenGLGraphicsRequirementsKHR",
                                     reinterpret_cast<PFN_xrVoidFunction*>(&getRequirements)),
               "xrGetOpenGLGraphicsRequirementsKHR"))
        return false;
    XrGraphicsRequirementsOpenGLKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
    if (!check(getRequirements(m_instance, m_system, &requirements),
               "xrGetOpenGLGraphicsRequirementsKHR"))
        return false;
#ifdef _WIN32
    XrGraphicsBindingOpenGLWin32KHR binding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
    binding.hDC = GetDC(glfwGetWin32Window(window.getHandle()));
    binding.hGLRC = wglGetCurrentContext();
    XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
    createInfo.next = &binding;
    if (!check(xrCreateSession(m_instance, &createInfo, &m_session), "xrCreateSession"))
        return false;
#else
    (void)window;
    return false;
#endif
    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    if (!check(xrCreateReferenceSpace(m_session, &spaceInfo, &m_stageSpace), "xrCreateReferenceSpace"))
        return false;
    XrReferenceSpaceCreateInfo viewSpaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    viewSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    viewSpaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    if (!check(xrCreateReferenceSpace(m_session, &viewSpaceInfo, &m_viewSpace),
               "xrCreateReferenceSpace"))
        return false;
    return true;
}

bool XRManager::createActions() {
    xrStringToPath(m_instance, "/user/hand/left", &m_leftHandPath);
    xrStringToPath(m_instance, "/user/hand/right", &m_rightHandPath);
    XrActionSetCreateInfo setInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
    std::strcpy(setInfo.actionSetName, "gameplay");
    std::strcpy(setInfo.localizedActionSetName, "Gameplay");
    setInfo.priority = 0;
    if (!check(xrCreateActionSet(m_instance, &setInfo, &m_actionSet), "xrCreateActionSet"))
        return false;
    const XrPath subactionPaths[] = {m_leftHandPath, m_rightHandPath};
    XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
    actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    actionInfo.countSubactionPaths = 2;
    actionInfo.subactionPaths = subactionPaths;
    std::strcpy(actionInfo.actionName, "select");
    std::strcpy(actionInfo.localizedActionName, "Select");
    if (!check(xrCreateAction(m_actionSet, &actionInfo, &m_leftSelectAction), "xrCreateAction"))
        return false;
    std::strcpy(actionInfo.actionName, "right_select");
    std::strcpy(actionInfo.localizedActionName, "Right Select");
    if (!check(xrCreateAction(m_actionSet, &actionInfo, &m_rightSelectAction), "xrCreateAction"))
        return false;
    actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    std::strcpy(actionInfo.actionName, "hand_pose");
    std::strcpy(actionInfo.localizedActionName, "Hand Pose");
    if (!check(xrCreateAction(m_actionSet, &actionInfo, &m_handPoseAction), "xrCreateAction"))
        return false;
    XrPath simpleProfile = XR_NULL_PATH;
    XrPath leftSelectPath = XR_NULL_PATH;
    XrPath rightSelectPath = XR_NULL_PATH;
    XrPath leftGripPath = XR_NULL_PATH;
    XrPath rightGripPath = XR_NULL_PATH;
    xrStringToPath(m_instance, "/interaction_profiles/khr/simple_controller", &simpleProfile);
    xrStringToPath(m_instance, "/user/hand/left/input/select/click", &leftSelectPath);
    xrStringToPath(m_instance, "/user/hand/right/input/select/click", &rightSelectPath);
    xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &leftGripPath);
    xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &rightGripPath);
    const XrActionSuggestedBinding bindings[] = {
        {m_leftSelectAction, leftSelectPath}, {m_rightSelectAction, rightSelectPath},
        {m_handPoseAction, leftGripPath}, {m_handPoseAction, rightGripPath}
    };
    XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggested.interactionProfile = simpleProfile;
    suggested.countSuggestedBindings = 4;
    suggested.suggestedBindings = bindings;
    if (!check(xrSuggestInteractionProfileBindings(m_instance, &suggested),
               "xrSuggestInteractionProfileBindings"))
        return false;
    XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    return check(xrAttachSessionActionSets(m_session, &attachInfo), "xrAttachSessionActionSets");
}

bool XRManager::createSwapchains() {
    uint32_t count = 0;
    if (!check(xrEnumerateViewConfigurationViews(m_instance, m_system, m_viewConfiguration,
                                                  0, &count, nullptr),
               "xrEnumerateViewConfigurationViews") || count != 2)
        return false;
    std::vector<XrViewConfigurationView> views(count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    xrEnumerateViewConfigurationViews(m_instance, m_system, m_viewConfiguration,
                                       count, &count, views.data());
    for (uint32_t eye = 0; eye < 2; ++eye) {
        XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        uint32_t formatCount = 0;
        if (!check(xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr),
                   "xrEnumerateSwapchainFormats"))
            return false;
        std::vector<int64_t> formats(formatCount);
        if (!check(xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount,
                                               formats.data()), "xrEnumerateSwapchainFormats"))
            return false;
        if (formats.empty())
            return false;
        const auto formatIt = std::find(formats.begin(), formats.end(),
                                         static_cast<int64_t>(GL_SRGB8_ALPHA8));
        info.format = formatIt != formats.end() ? GL_SRGB8_ALPHA8 :
                      (std::find(formats.begin(), formats.end(),
                                 static_cast<int64_t>(GL_RGBA8)) != formats.end()
                           ? GL_RGBA8
                           : static_cast<GLint>(formats.front()));
        info.sampleCount = 1;
        info.width = views[eye].recommendedImageRectWidth;
        info.height = views[eye].recommendedImageRectHeight;
        m_viewWidths[eye] = info.width;
        m_viewHeights[eye] = info.height;
        if (!check(xrCreateSwapchain(m_session, &info, &m_swapchains[eye]), "xrCreateSwapchain"))
            return false;
        uint32_t imageCount = 0;
        xrEnumerateSwapchainImages(m_swapchains[eye], 0, &imageCount, nullptr);
        std::vector<XrSwapchainImageOpenGLKHR> images(
            imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
        if (!check(xrEnumerateSwapchainImages(m_swapchains[eye], imageCount, &imageCount,
                                              reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),
                   "xrEnumerateSwapchainImages"))
            return false;
        m_swapchainImages[eye].resize(imageCount);
        for (uint32_t i = 0; i < imageCount; ++i)
            m_swapchainImages[eye][i] = images[i].image;
    }
    return true;
}

void XRManager::destroySessionResources() {
    if (m_sessionReady)
        xrEndSession(m_session);
    for (auto& swapchain : m_swapchains)
        if (swapchain != XR_NULL_HANDLE) xrDestroySwapchain(swapchain);
    if (m_stageSpace != XR_NULL_HANDLE) xrDestroySpace(m_stageSpace);
    if (m_viewSpace != XR_NULL_HANDLE) xrDestroySpace(m_viewSpace);
    if (m_session != XR_NULL_HANDLE) xrDestroySession(m_session);
    if (m_instance != XR_NULL_HANDLE) xrDestroyInstance(m_instance);
    m_swapchains = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    m_stageSpace = XR_NULL_HANDLE;
    m_viewSpace = XR_NULL_HANDLE;
    m_session = XR_NULL_HANDLE;
    m_instance = XR_NULL_HANDLE;
    m_sessionReady = false;
}
#endif
