# Step 5: OpenXR & VR Simulation Mode

## Objective
Abstract VR hardware and provide the desktop fallback.

## Instructions for AI
1. **XRManager (`src/xr/XRManager.h/.cpp`)**:
   - Singleton.
   - Initialize OpenXR Instance, System, and Session.
   - Create Swapchains for left/right eyes.
2. **Simulation Mode**:
   - Add a boolean `m_isSimulated`.
   - If OpenXR fails to initialize, automatically set `m_isSimulated = true`.
   - When simulated, `XRManager` should output a standard View/Projection matrix based on the desktop Window's aspect ratio and a virtual "VR Camera" controlled by Mouse/WASD.
3. **DevUI (`src/platform/DevUI.h/.cpp`)**:
   - Use Dear ImGui.
   - Create an overlay window with a toggle switch: "Simulated VR".
   - Toggling this switch dynamically switches the camera source between OpenXR HMD tracking and Desktop Fallback.