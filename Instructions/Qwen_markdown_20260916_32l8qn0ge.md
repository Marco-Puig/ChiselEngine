# Project Blueprint: ChiselEngine

## Overview
ChiselEngine is a professional-grade, modular VR game engine built on OpenXR and OpenGL. It is designed to decouple complex VR boilerplate from gameplay logic using a "Zero-Friction" philosophy.

## Architecture Rules for AI Implementation
1. **Modular Singletons**: Core systems (Rendering, XR, Physics, Platform) must be Singletons to allow global access without deep dependency injection chains.
2. **Scene Graph**: Use a Node-based hierarchy inspired by Babylon.js. Nodes handle local/world transforms.
3. **C++ Standard**: Use C++20. Utilize `std::unique_ptr` for strict ownership of child nodes and engine resources.
4. **Math Library**: Use `glm` (OpenGL Mathematics) for all vectors, matrices, and quaternions.
5. **VR-First**: The engine must always attempt to initialize OpenXR. If no headset is found, or if "VR Simulation Mode" is active, it must gracefully fallback to a desktop window with mouselook.

## Module Responsibilities
- **Core**: Engine heartbeat, main loop, `Game` class interface.
- **Platform**: GLFW window creation, OS input, DevUI (ImGui).
- **XR**: OpenXR instance, session, and swapchain management.
- **Rendering**: OpenGL state, Shaders, GLB loading (TinyGLTF), Lighting.
- **Scene**: `Node` base class, `MeshNode`, `Animator`.