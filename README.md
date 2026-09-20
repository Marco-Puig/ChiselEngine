# ChiselEngine
**A easy to use and portable VR Game Engine.**

ChiselEngine is a easy to use VR game engine built on **OpenXR** and **OpenGL**. The goal is to use this as a way to avoid using more bloated, general purpose engines to create specific games and simulations in the VR space.

<img width="1579" height="889" alt="image" src="https://github.com/user-attachments/assets/d5ab940e-6e03-4f7b-87a9-bd81a50b363d" />

---

## Design Philosophy

ChiselEngine is designed so that a developer can focus on gameplay logic without needing to be an expert in OpenGL, OpenXR, or multi-threaded physics.

### Node-Based Scene Graph (Babylon.js Inspired)
Everything in the world is a `Node`. Whether it's a camera, a light, or a 3D model, they all inherit from a common base. This allows for:
- **Hierarchical Transforms**: Parent-child relationships for complex objects.
- **Uniform Interaction**: An `Animator` can drive any `Node`, regardless of what it is.

### Lua-First Game Development
The engine runtime is native C++, but game-specific scene setup and gameplay
live in the top-level `game/` project folder. Developers normally edit
`game/game.lua` rather than changing the engine's `Game` C++ plumbing. This
keeps the engine reusable while making iteration on a game script fast.

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

## Developer's Guide: Build a game in Lua

Game developers should normally work in `game/`, not in `src/game/`. The native
`Game` class only creates the scene/runtime plumbing, loads `game/game.lua`, and
drives its lifecycle. The script is the single game entry point and returns a
table with these optional callbacks:

```lua
local game = {}

function game.onStart(scene, animator)
    -- Create the initial scene and register gameplay behavior.
end

function game.onUpdate(deltaTime, scene, animator)
    -- Run per-frame gameplay logic.
end

return game

```

### Creating a scene

The first-pass Lua API lets the script create lights and load GLB meshes:

```lua
local sun = scene:createDirectionalLight("Sun")
sun:setColor(1.0, 0.9, 0.8)
sun:setPosition(0.0, 4.0, 0.0)
sun:setIntensity(1.0)
sun:setExposure(0.0)

local bulb = scene:createPointLight("Bulb", 2.0, 4.0, 2.0)
bulb:setColor(1.0, 0.8, 0.4)
bulb:setIntensity(2.0)
bulb:setExposure(0.0)
bulb:setRadius(15.0)

Engine.setSkybox("resources/skybox.jpg")

local floor = scene:loadMesh("resources/plane.glb", "Floor")
floor:setPosition(0.0, 0.0, 0.0)
Engine.addRigidBody(floor, "static", "box", 0.8, 0.0)

```

`scene:loadMesh(path, name)` loads the asset through the native glTF pipeline,
attaches the resulting node to the scene root, and returns the node to Lua.
The available body types are `"static"`, `"dynamic"`, and `"kinematic"`. The available collider types are `"box"` and `"convex"`.

### Transform and scene queries

Nodes currently expose:

```lua
node:getName()
node:setName("NewName")
node:setPosition(x, y, z)
node:getPositionX()
node:getPositionY()
node:getPositionZ()
node:setScale(x, y, z)

local node = scene:findNode("Floor")

```

Lua does not own scene nodes. The native scene retains ownership and controls
their lifetime.

### Animation Controls (Procedural & GLB Node Animations)

Register a continuous transform update through the native `Animator`. The `procedural` call returns an identifier so you can stop or play it dynamically:

```lua
local frogRotationId = animator:procedural(
    frog,
    "y",
    "positive",
    "rotation",
    1.0
)

-- Stop or resume the procedural animation later
animator:stopProcedural(frogRotationId)
animator:playProcedural(frogRotationId)

```

The axis may be `"x"`, `"y"`, or `"z"`. The direction may be `"positive"` or
`"negative"`, the type may be `"rotation"` or `"position"`, and the speed is
expressed in radians or world units per second.

You can also target animations embedded directly in GLB/gltf files attached to nodes:

```lua
-- Play or stop an embedded model animation by name
animator:playAnimation(frog, "Idle")
animator:stopAnimation(frog, "Idle")

```

### Demo project

The checked-in `game/game.lua`
creates the demo light, skybox, floor, frog mesh, physics bodies, and frog
rotation entirely from Lua:

```lua
function game.onStart(scene, animator)
    local floor = scene:loadMesh("resources/plane.glb", "Floor")
    Engine.addRigidBody(floor, "static", "box", 0.8, 0.0)

    local frog = scene:loadMesh("resources/frog.glb", "Frog")
    frog:setPosition(0.0, 3.0, 0.0)
    Engine.addRigidBody(frog, "dynamic", "convex", 0.6, 0.1)
    animator:procedural(frog, "y", "positive", "rotation", 1.0)
end

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


`build.bat` configures and builds the engine. `run.bat` launches the built executable (defaults to Debug; pass `Release` for a release build, e.g. `run.bat Release`). The default project loads `game/game.lua`, which defines the demo scene. \\

**Option B - run the commands yourself:**
```powershell
Remove-Item -Recurse -Force build
cmake -B build "-DCMAKE_POLICY_VERSION_MINIMUM=3.6"
cmake --build build

```


2. **Run and iterate**: Open the `Scene Editor` window and toggle **Simulated VR** to test without a headset. Edit `game/game.lua`, rebuild, and run again. OpenXR support is enabled by default; use `-DCHISEL_ENABLE_OPENXR=OFF` for a desktop-only build.

## Lua gameplay scripts

Gameplay scripts use Lua 5.4 with LuaBridge. The single entry script returns a
table with optional `onStart(scene, animator)` and
`onUpdate(deltaTime, scene, animator)`
functions. The first pass exposes node names, position/scale accessors, scene
name lookup, and the native `Animator` procedural and GLB clip playback APIs. Script code does not own
scene nodes; native C++ retains ownership and controls their lifetime.

Lua load, syntax, and callback errors are written to the `[Lua]` log channel.
The current API intentionally focuses on scene construction, transforms,
animation control, lights, skyboxes, and basic rigid-body creation. Input, OpenXR
actions, rendering internals, and arbitrary object ownership remain native
until stable script-facing APIs are defined.

When an OpenXR runtime and headset are available, the engine creates an OpenGL OpenXR session,
locates both eye views, renders each eye into its swapchain, and submits a projection layer.
Without a runtime, it automatically continues with the desktop GLFW framebuffer.
