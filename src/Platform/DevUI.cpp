#include "DevUI.h"
#include "Window.h"
#include "core/Version.h"
#include "xr/XRManager.h"
#include "PhysicsSystem.h"
#include "scene/Node.h"
#include "scene/ArcRotateCamera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#endif

DevUI& DevUI::getInstance() {
    static DevUI instance;
    return instance;
}

DevUI::~DevUI() {
    shutdown();
}

void DevUI::init(Window& window) {
    if (m_initialized)
        return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Prevent stale ImGui layout entries from reopening old developer windows.
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.getHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 450 core");
    querySystemInfo();
    m_initialized = true;
}

void DevUI::shutdown() {
    if (!m_initialized)
        return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void DevUI::beginFrame() {
    if (!m_initialized)
        return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

namespace {
void drawNodeList(Node* node, Node*& selected) {
    if (node == nullptr)
        return;
    const bool isSelected = selected == node;
    if (ImGui::Selectable(node->getName().c_str(), isSelected))
        selected = node;
    for (const auto& child : node->getChildren())
        drawNodeList(child.get(), selected);
}
}

void DevUI::render(Window& window, XRManager& xr, Node* sceneRoot,
                   ArcRotateCamera* camera, float deltaTime) {
    if (!m_initialized)
        return;

    if (!m_modeInitialized) {
        m_simulatedVR = xr.isSimulated();
        m_modeInitialized = true;
    }

    m_displayAccumulator += deltaTime;
    m_pendingFrameTimeMs = deltaTime * 1000.0f;
    m_intervalTime += deltaTime;
    ++m_intervalFrames;
    if (m_intervalTime > 0.0f)
        m_pendingFps = static_cast<float>(m_intervalFrames) / m_intervalTime;
    if (m_displayAccumulator >= 0.25f) {
        m_frameTimeMs = m_pendingFrameTimeMs;
        m_fps = m_pendingFps;
        m_displayAccumulator = 0.0f;
        m_intervalTime = 0.0f;
        m_intervalFrames = 0;
    }

    const bool f1Down = glfwGetKey(window.getHandle(), GLFW_KEY_F1) == GLFW_PRESS;
    if (f1Down && !m_toggleKeyWasDown)
        m_visible = !m_visible;
    m_toggleKeyWasDown = f1Down;

    if (m_visible) {
        ImGui::SetNextWindowSize(ImVec2(390.0f, 280.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("ChiselEngine DevUI", &m_visible);
        ImGui::Text("Engine version: %s", ChiselEngine::Version);
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", m_fps);
        ImGui::Text("Frame time: %.3f ms", m_frameTimeMs);
        ImGui::Separator();
        ImGui::Text("GPU: %s", m_gpuName.c_str());
        ImGui::Text("CPU: %s", m_cpuName.c_str());
        ImGui::Text("OS: %s", m_osName.c_str());
        ImGui::Separator();

        if (ImGui::Checkbox("Simulated VR", &m_simulatedVR))
            xr.setSimulationMode(m_simulatedVR, window);
        ImGui::SameLine();
        if (m_simulatedVR)
            ImGui::TextUnformatted("Desktop stereo simulation");
        else if (xr.isRunning())
            ImGui::TextUnformatted("OpenXR runtime");
        else
            ImGui::TextUnformatted("OpenXR unavailable; desktop fallback");
        if (ImGui::Checkbox("Show Collision Debug", &m_showCollisionDebug))
            PhysicsSystem::getInstance().setDebugDrawEnabled(m_showCollisionDebug);
        ImGui::Separator();
        ImGui::TextUnformatted("Scene nodes");
        ImGui::BeginChild("NodeList", ImVec2(0.0f, 90.0f), true);
        drawNodeList(sceneRoot, m_selectedNode);
        ImGui::EndChild();
        if (ImGui::RadioButton("Move", m_gizmoOperation == 0))
            m_gizmoOperation = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", m_gizmoOperation == 1))
            m_gizmoOperation = 1;
        ImGui::Text("Press F1 to toggle this window");
        ImGui::End();
    }

    if (m_visible && m_selectedNode != nullptr && camera != nullptr) {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImGuizmo::SetRect(0.0f, 0.0f, displaySize.x, displaySize.y);
        glm::mat4 transform = m_selectedNode->getWorldTransform();
        float matrix[16];
        std::memcpy(matrix, glm::value_ptr(transform), sizeof(matrix));
        const glm::mat4 view = camera->getViewMatrix();
        const glm::mat4 projection = camera->getProjectionMatrix();
        ImGuizmo::Manipulate(glm::value_ptr(view),
                             glm::value_ptr(projection),
                             m_gizmoOperation == 0 ? ImGuizmo::TRANSLATE : ImGuizmo::ROTATE,
                             ImGuizmo::WORLD, matrix);
        if (ImGuizmo::IsUsing()) {
            glm::mat4 edited;
            std::memcpy(glm::value_ptr(edited), matrix, sizeof(matrix));
            glm::vec3 translation;
            glm::vec3 rotation;
            glm::vec3 scale;
            ImGuizmo::DecomposeMatrixToComponents(matrix, &translation.x,
                                                   &rotation.x, &scale.x);
            m_selectedNode->setPosition(translation);
            m_selectedNode->setRotation(glm::quat(glm::radians(rotation)));
        }
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DevUI::querySystemInfo() {
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    m_gpuName = renderer != nullptr ? renderer : "Unknown GPU";
    m_gpuMemory = "Unavailable";

    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    const std::string extensionList = extensions != nullptr ? extensions : "";
    if (extensionList.find("GL_NVX_gpu_memory_info") != std::string::npos) {
        GLint dedicatedKb = 0;
        glGetIntegerv(0x9047, &dedicatedKb);
        if (dedicatedKb > 0)
            m_gpuMemory = std::to_string(dedicatedKb / 1024) + " MB";
    } else if (extensionList.find("GL_ATI_meminfo") != std::string::npos) {
        GLint dedicatedKb = 0;
        glGetIntegerv(0x87FC, &dedicatedKb);
        if (dedicatedKb > 0)
            m_gpuMemory = std::to_string(dedicatedKb) + " MB";
    }

#ifdef _WIN32
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    std::ostringstream cpu;
    cpu << systemInfo.dwNumberOfProcessors << " logical processors";
    m_cpuName = cpu.str();
    m_osName = "Windows";
#else
    m_cpuName = "Unknown CPU";
    m_osName = "Unknown OS";
#endif
}
