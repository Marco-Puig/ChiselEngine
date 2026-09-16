#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <map>

class DevUI {
public:
    static DevUI& getInstance() {
        static DevUI instance;
        return instance;
    }

    bool init(const std::string& title);
    void update(); // Polls Win32 messages
    void shutdown();

    // UI Elements
    void addCheckbox(const std::string& label, bool defaultValue, std::function<void(bool)> callback);

private:
    DevUI() = default;
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    std::map<int, std::function<void(bool)>> m_callbacks;
    int m_nextId = 100;
};
