# Project Setup & Finalization Guide

This guide contains the necessary steps to finalize the build configuration for ChiselEngine after the architectural refactor.

## 🛠 Visual Studio Solution Setup

Since the project structure was moved to a modular `include/` and `src/` layout, you must update the Visual Studio project settings:

### 1. Add New Files to Project
The following files were created/moved and need to be added to the `.vcxproj` via the **Solution Explorer**:
- **Include Folder**: Add all `.h` files from `include/Core`, `include/Platform`, `include/Rendering`, `include/XR`, and `include/Scene`.
- **Source Folder**: Add all `.cpp` files from `src/Platform`, `src/Rendering`, `src/XR`, `src/Scene`, and the updated `Core/engine.cpp`.

### 2. Configure Include Directories
To ensure the compiler finds the modular headers:
1. Right-click the **Project** $\rightarrow$ **Properties**.
2. Navigate to **Configuration Properties** $\rightarrow$ **C/C++** $\rightarrow$ **General**.
3. In **Additional Include Directories**, add:
   - `$(ProjectDir)include`
   - `$(ProjectDir)Libraries/include`

---

## 📦 Library Linking (Dependencies)

### 1. Jolt Physics Setup
To enable the new Physics Wrapper:
1. **Download Jolt**: Obtain the Jolt Physics binaries/source.
2. **Include Path**: Add the Jolt `include` directory to the Additional Include Directories.
3. **Linker**: Navigate to **Linker** $\rightarrow$ **Input** $\rightarrow$ **Additional Dependencies** and add `Jolt.lib`.
4. **DLLs**: Place `Jolt.dll` in the output folder (where the `.exe` is generated).

### 2. Assimp (for GLB Loading)
To ensure `.glb` and `.gltf` files load correctly:
1. Ensure `assimp-vc14x-mt.lib` is linked in the project dependencies.
2. Ensure the Assimp DLL is present in the executable directory.

---

## ✅ Final Checklist (TODO)

- [ ] **Files Added**: All new `.h` and `.cpp` files added to the Solution Explorer.
- [ ] **Include Paths**: `include/` folder added to C++ General settings.
- [ ] **Jolt Linked**: Jolt `.lib` added to linker and `.dll` placed in bin.
- [ ] **Assimp Linked**: Assimp libraries correctly linked for GLB support.
- [ ] **Resources**: Sample `.glb` models placed in the `Resources/` folder.
- [ ] **Build**: Project compiles without errors in `Release` or `Debug` mode.
- [ ] **Launch**: Run `start.bat` to verify the engine and `DevUI` window launch.
