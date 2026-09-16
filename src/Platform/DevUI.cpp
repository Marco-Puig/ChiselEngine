#include "Platform/DevUI.h"
#include <windows.h>
#include <string>

#pragma comment(lib, "user32.lib")

bool DevUI::init(const std::string& title) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DevUI::windowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "DevUIClass";

    if (!RegisterClassEx(&wc)) return false;

    m_hwnd = CreateWindowEx(
        0, "DevUIClass", title.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!m_hwnd) return false;
    return true;
}

void DevUI::addCheckbox(const std::string& label, bool defaultValue, std::function<void(bool)> callback) {
    HWND hCheckbox = CreateWindow(
        "BUTTON", label.c_str(),
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        10, 10 + ((m_nextId - 100) * 30), 200, 25,
        m_hwnd, (HMENU)(UINT_PTR)m_nextId, GetModuleHandle(NULL), nullptr
    );

    if (defaultValue) {
        SendMessage(hCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    }

    m_callbacks[m_nextId] = callback;
    m_nextId++;
}

void DevUI::update() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK DevUI::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        auto it = DevUI::getInstance().m_callbacks.find(id);
        if (it != DevUI::getInstance().m_callbacks.end()) {
            HWND hBtn = (HWND)lParam;
            bool checked = SendMessage(hBtn, BM_GETCHECK, 0, 0) == BST_CHECKED;
            it->second(checked);
        }
    }
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DevUI::shutdown() {
    if (m_hwnd) DestroyWindow(m_hwnd);
}
