# Step 1: CMake & Dependencies Setup

## Objective
Set up a modern CMake build system that automatically fetches required dependencies using `FetchContent`.

## Instructions for AI
1. Create a root `CMakeLists.txt`.
2. Set the C++ standard to C++20.
3. Use `FetchContent` to download and link the following libraries:
   - **GLFW**: For windowing and input.
   - **GLAD**: For OpenGL function loading (Core profile 4.5).
   - **GLM**: For math.
   - **Dear ImGui**: For the DevUI (dock the GLFW+OpenGL3 backend).
   - **TinyGLTF**: For loading `.glb` models.
   - **OpenXR-SDK**: For VR hardware abstraction.
4. Create a `src/` directory structure matching the modules: `core/`, `platform/`, `xr/`, `rendering/`, `scene/`, `game/`.
5. Create an executable target `ChiselEngine` that compiles all `.cpp` files in `src/`.
6. Ensure the `Resources/` folder is copied to the build output directory post-build.