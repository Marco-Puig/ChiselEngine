#pragma once
#include <string>

class Window;
class XRManager;

class DevUI {
public:
    static DevUI& getInstance();

    void init(Window& window);
    void shutdown();
    void beginFrame();
    void render(Window& window, XRManager& xr, float deltaTime);
    bool isVisible() const { return m_visible; }

private:
    DevUI() = default;
    ~DevUI();
    DevUI(const DevUI&) = delete;
    DevUI& operator=(const DevUI&) = delete;

    void querySystemInfo();

    bool m_initialized = false;
    bool m_visible = true;
    bool m_toggleKeyWasDown = false;
    bool m_simulatedVR = true;
    float m_fps = 0.0f;
    float m_frameTimeMs = 0.0f;
    std::string m_gpuName;
    std::string m_gpuMemory;
    std::string m_cpuName;
    std::string m_osName;
};
