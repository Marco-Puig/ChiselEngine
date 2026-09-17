#pragma once

class DevUI {
public:
    static DevUI& getInstance() {
        static DevUI instance;
        return instance;
    }

    void init();
    void render();

private:
    DevUI() = default;
    bool m_simulatedVR = true;
};
