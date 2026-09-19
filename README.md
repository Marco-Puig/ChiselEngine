# ChiselEngine
**A easy to use and portable VR Game Engine.**

ChiselEngine is a easy to use VR game engine built on **OpenXR** and **OpenGL**. The goal is to use this as a way to avoid using more bloated, general purpose engines to create specific games and simulations in the VR space.

<img width="1257" height="702" alt="image" src="https://github.com/user-attachments/assets/897ac9a4-fb01-43a5-bb36-a6e3c7721880" />

---

## Design Philosophy

ChiselEngine is built on the principle of **"Zero-Friction Development."** The architecture is designed so that a developer can focus on gameplay logic without needing to be an expert in OpenGL, OpenXR, or multi-threaded physics.

### Node-Based Scene Graph (Babylon.js Inspired)
Everything in the world is a `Node`. Whether it's a camera, a light, or a 3D model, they all inherit from a common base. This allows for:
- **Hierarchical Transforms**: Parent-child relationships for complex objects.
- **Uniform Interaction**: An `Animator` can drive any `Node`, regardless of what it is.

### VR-First Workflow
VR development is traditionally slow because of the "Headset Cycle" (Put on headset $\rightarrow$ Test $\rightarrow$ Take off headset $\rightarrow$ Fix code). ChiselEngine breaks this with **VR Simulation Mode**, allowing developers to test movements and logic in a desktop window before deploying to hardware.

---

## Engine Modules

| Module | Responsibility | Key Components |
| :--- | :--- | :--- |
| **Core** | The main part of the engine. Handles initialization, the main loop, and shutdown. | `engine.cpp`, `Game` class |
| **Platform** | Interfaces with the OS and Windowing system. | `Window`, `SceneEditor`, `PhysicsSystem` |
| **XR** | Manages the VR Headset, controllers, and OpenXR session. | `XRManager`, `Swapchains` |
| **Rendering**| The OpenGL pipeline. Handles shaders, buffers, and lighting. | `RenderSystem`, `Light`, `GLBLoader` |
| **Scene** | High-level object management and animation. | `Node`, `MeshNode`, `Animator` |

---

## Developer's Guide: How to make a game

As a developer, you don't touch the `Core`, `XR`, or `Rendering` modules. You spend 100% of your time in the **`Game`** class and the **`Scene`** module.

### 1. Setup the scene (`start()`)
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

### 2. Create an active node (`update()`)
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

### Procedural animation

`Animator::procedural` registers a continuous transform update in radians or world units per second:

```cpp
animator->procedural(cube, Axis::Y, Direction::Positive,
                     TransformType::Rotation, glm::radians(45.0f));
```

The sample game uses this API to rotate the bundled cube every frame.

### Physics Interaction
To make an object physical, simply wrap the node in a `PhysicsBody`.

```cpp
PhysicsBody* body = PhysicsSystem::getInstance().createRigidBody(rockNode, BodyType::Dynamic);
body->setMass(5.0f);
```

### Materials, gizmos, and desktop VR input

`GLBLoader` uploads embedded glTF images and stores the result in the loaded
`MeshNode`'s `Material`. Base color, normal, metallic-roughness, and emissive
maps are supported by the default OpenGL shader; models without a material
continue to use the neutral fallback color.

Select a node in the Scene Editor to use the Move or Rotate gizmo. ImGuizmo performs
the screen-ray/handle hit test and updates the node while dragging. While a
handle is hovered or active, `ArcRotateCamera` gives the mouse to the gizmo
instead of orbiting or panning.

In Simulated VR mode, the left and right controller action surfaces remain the
same as OpenXR. The left controller uses `WASD`/`Q`/`E` and the left mouse
button; the right controller uses the arrow keys/Page Up/Page Down and the
right mouse button. Shift provides grip, and `Tab`/`Enter` provide menu
buttons. Gameplay can query `XRManager::getControllerState()` without knowing
which input backend is active.

### Scene ownership and physics colliders

Use `Scene::create<T>()` for ordinary runtime nodes; it attaches the new node
to the scene root automatically. Use `Scene::create<T>(parent, ...)` or
`Scene::adopt()` when a node belongs under an explicit parent. This ownership
API is intentional: attaching from a C++ constructor would require taking
ownership of `this` before its `unique_ptr` exists and is unsafe.

`MeshNode` retains its local vertex positions for physics. Rigid bodies created
from a mesh use a deduplicated Jolt convex hull by default, with a small box
fallback only for degenerate point sets. Convex hulls are appropriate for
small convex dynamic props; large or concave static assets such as a detailed
floor should eventually use a Jolt triangle-mesh shape instead.

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
2. **Configure**: Open the `Scene Editor` window and toggle **Simulated VR** to test without a headset. OpenXR support is enabled by default; use `-DCHISEL_ENABLE_OPENXR=OFF` for a desktop-only build.

## Lua gameplay scripts

Gameplay scripts use Lua 5.4 with LuaBridge. The single entry script returns a
table with optional `onStart(scene, animator)` and
`onUpdate(deltaTime, scene, animator)`
functions. The first pass exposes node names, position/scale accessors, scene
name lookup, and the native `Animator` procedural API. Script code does not own
scene nodes; native C++ retains ownership and controls their lifetime.

The single `Game/game.lua` entry point registers a continuous rotation through
the native animator. Lua load, syntax, and callback errors are written to the
`[Lua]` log channel and do not terminate the process. Input, physics, rendering,
and arbitrary object ownership remain native until stable script-facing APIs are
defined.
3. **Create**: Edit `Game/game.lua` to implement gameplay behavior without
changing the native game loop.

When an OpenXR runtime and headset are available, the engine creates an OpenGL OpenXR session,
locates both eye views, renders each eye into its swapchain, and submits a projection layer.
Without a runtime, it automatically continues with the desktop GLFW framebuffer.
