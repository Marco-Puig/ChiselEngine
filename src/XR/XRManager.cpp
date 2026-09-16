#include "XR/XRManager.h"
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <cstdio>

bool XRManager::init(const char* app_name, int64_t swapchain_format) {
    const char* ask_extensions[] = {
        XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
        XR_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    uint32_t ext_count = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
    std::vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
    xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

    std::vector<const char*> use_extensions;
    for (size_t i = 0; i < xr_exts.size(); i++) {
        for (int32_t ask = 0; ask < 2; ask++) {
            if (strcmp(ask_extensions[ask], xr_exts[i].extensionName) == 0) {
                use_extensions.push_back(ask_extensions[ask]);
                break;
            }
        }
    }

    if (!std::any_of(use_extensions.begin(), use_extensions.end(),
        [](const char* ext) { return strcmp(ext, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0; })) {
        MessageBox(nullptr, _T("XR_KHR_opengl_enable not available"), _T("Error"), MB_OK);
        return false;
    }

    XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    createInfo.enabledExtensionCount = (uint32_t)use_extensions.size();
    createInfo.enabledExtensionNames = use_extensions.data();
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strcpy_s(createInfo.applicationInfo.applicationName, app_name);

    XrResult res = xrCreateInstance(&createInfo, &m_instance);
    if (XR_FAILED(res) || m_instance == nullptr) return false;

    xrGetInstanceProcAddr(m_instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrCreateDebugUtilsMessengerEXT));
    xrGetInstanceProcAddr(m_instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrDestroyDebugUtilsMessengerEXT));
    xrGetInstanceProcAddr(m_instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetOpenGLGraphicsRequirementsKHR));

    XrDebugUtilsMessengerCreateInfoEXT debug_info = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debug_info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
    debug_info.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT* msg, void* user_data) {
        printf("%s: %s\n", msg->functionName, msg->message);
        return (XrBool32)XR_FALSE;
    };

    if (ext_xrCreateDebugUtilsMessengerEXT)
        ext_xrCreateDebugUtilsMessengerEXT(m_instance, &debug_info, &m_debug);

    XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };

    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    res = xrGetSystem(m_instance, &systemInfo, &m_system_id);
    if (XR_FAILED(res)) return false;

    xrEnumerateEnvironmentBlendModes(m_instance, m_system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 1, &blend_count, &m_blend);

    XrGraphicsBindingOpenGLWin32KHR graphicsBinding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    graphicsBinding.hDC = wglGetCurrentDC();
    graphicsBinding.hGLRC = wglGetCurrentContext();

    XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = m_system_id;

    res = xrCreateSession(m_instance, &sessionInfo, &m_session);
    if (XR_FAILED(res)) return false;

    XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    ref_space.poseInReferenceSpace = { {0,0,0,1}, {0,0,0} };
    ref_space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    xrCreateReferenceSpace(m_session, &ref_space, &m_app_space);

    return true;
}

void XRManager::shutdown() {
    for (auto& sc : m_swapchains) {
        xrDestroySwapchain(sc.handle);
    }
    m_swapchains.clear();
    if (m_session != XR_NULL_HANDLE) xrDestroySession(m_session);
    if (m_instance != XR_NULL_HANDLE) xrDestroyInstance(m_instance);
}

void XRManager::pollEvents(bool& exit) {
    XrEventDataBuffer event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
    while (xrPollEvent(m_instance, &event_buffer) == XR_SUCCESS) {
        if (event_buffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&event_buffer;
            m_session_state = changed->state;
            if (m_session_state == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo begin_info = { XR_TYPE_SESSION_BEGIN_INFO };
                begin_info.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                xrBeginSession(m_session, &begin_info);
                m_running = true;
            } else if (m_session_state == XR_SESSION_STATE_STOPPING) {
                m_running = false;
                xrEndSession(m_session);
            } else if (m_session_state == XR_SESSION_STATE_EXITING) {
                exit = true;
            }
        }
    }
}
