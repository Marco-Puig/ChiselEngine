# ChiselEngine
**A easy to use and portable VR Game Engine.**

ChiselEngine is a professional-grade VR game engine built on **OpenXR** and **OpenGL**. It is designed specifically to decouple the complex boilerplate of VR hardware and graphics rendering from the creative process of game development.

---

## Design Philosophy

ChiselEngine is built on the principle of **"Zero-Friction Development."** The architecture is designed so that a developer can focus on gameplay logic without needing to be an expert in Vulkan, OpenXR, or multi-threaded physics.

### 1. Modular Singleton Architecture
Instead of a monolithic "Engine" class, the engine is split into independent, specialized modules. This allows for easier debugging, faster compile times, and the ability to swap out systems (e.g., switching from OpenGL to Vulkan) without rewriting the game logic.

### 2. Node-Based Scene Graph (Babylon.js Inspired)
Everything in the world is a `Node`. Whether it's a camera, a light, or a 3D model, they all inherit from a common base. This allows for:
- **Hierarchical Transforms**: Parent-child relationships for complex objects.
- **Uniform Interaction**: An `Animator` can drive any `Node`, regardless of what it is.

### 3. VR-First Workflow
VR development is traditionally slow because of the "Headset Cycle" (Put on headset $\rightarrow$ Test $\rightarrow$ Take off headset $\rightarrow$ Fix code). ChiselEngine breaks this with **VR Simulation Mode**, allowing developers to test movements and logic in a desktop window before deploying to hardware.

---

## Engine Modules

| Module | Responsibility | Key Components |
| :--- | :--- | :--- |
| **Core** | The main part of the engine. Handles initialization, the main loop, and shutdown. | `engine.cpp`, `Game` class |
| **Platform** | Interfaces with the OS and Windowing system. | `Window`, `DevUI`, `PhysicsSystem` |
| **XR** | Manages the VR Headset, controllers, and OpenXR session. | `XRManager`, `Swapchains` |
| **Rendering**| The OpenGL pipeline. Handles shaders, buffers, and lighting. | `RenderSystem`, `Light`, `GLBLoader` |
| **Scene** | High-level object management and animation. | `Node`, `MeshNode`, `Animator` |

---

## Developer's Guide: How to make a game

As a developer, you don't touch the `Core`, `XR`, or `Rendering` modules. You spend 100% of your time in the **`Game`** class and the **`Scene`** module.

### 1. Define your World (`start()`)
In the `start()` method, you set up your environment. You load your models as **Nodes** and define your lighting.

```cpp
void Game::start() {
    // Setup a sun light
    DirectionalLight* sun = new DirectionalLight("Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
    sun->setColor(glm::vec3(1.0f, 0.9f, 0.8f));
    RenderSystem::getInstance().addLight(sun);

    // Load the bundled OpenGL test model as a Node
    MeshNode* cube = GLBLoader::loadGLB("resources/cube.glb");
}
```

### 2. Create Life (`update()`)
In the `update()` method, you define how the world changes over time. You can use the `Animator` to create movement.

```cpp
void Game::update(float deltaTime) {
    // Make the sword float up and down procedurally
    swordAnimator->animateAxis("y", 0.2f);
    
    // Or trigger a keyframe animation from the GLB file
    if (input.ButtonPressed(BUTTON_A)) {
        swordAnimator->playAnimation("Attack_Swing");
    }
}
```

### 3. Physics Interaction
To make an object physical, simply wrap the node in a `PhysicsBody`.

```cpp
PhysicsBody* body = PhysicsSystem::getInstance().createRigidBody(rockNode, BodyType::Dynamic);
body->setMass(5.0f);
```

---

## Quick Start
1. **Build**:

   **Option A - batch scripts (Windows, recommended):**
   ```
   build.bat
   run.bat
   ```
   `build.bat` configures and builds the engine. `run.bat` launches the built executable (defaults to Debug; pass `Release` for a release build, e.g. `run.bat Release`). The default game loads and renders `resources/cube.glb`.

   **Option B - run the commands yourself:**
   ```powershell
   Remove-Item -Recurse -Force build
   cmake -B build "-DCMAKE_POLICY_VERSION_MINIMUM=3.6"
   cmake --build build
   ```
2. **Configure**: Open the `DevUI` window and toggle **Simulated VR** to test without a headset.
3. **Create**: Open `Game/Game.cpp` and start building your world!
