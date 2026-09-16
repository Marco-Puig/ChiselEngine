# ChiselEngine
Chisel Engine is a modular, high-performance game engine built on OpenXR and OpenGL, designed for developing immersive VR applications.

## Design Philosophy
ChiselEngine follows a **Modular Singleton Architecture**, inspired by modern engines like Babylon.js. The goal is to decouple the underlying platform and rendering complexity from the gameplay logic.

- **Separation of Concerns**: Platform, XR, Rendering, and Scene management are strictly isolated into independent modules.
- **Node-Based Scene Graph**: Instead of raw meshes, everything in the scene is a `Node`. This allows for hierarchical transformations and streamlined animation.
- **Data-Driven Animations**: Support for GLTF/GLB standards allows artists to create complex animations in tools like Blender and trigger them by name in code.
- **Developer-First Workflow**: Integrated tools like the `DevUI` and VR Simulation mode allow for rapid iteration without needing a headset for every change.

## Architecture
- **`Platform`**: Manages OS-level concerns, including GLFW window creation and the `DevUI` developer tool.
- **`XR`**: Handles the OpenXR session, headset tracking, swapchains, and VR input.
- **`Rendering`**: Manages the OpenGL pipeline, including a dynamic lighting system and GLB model loading.
- **`Scene`**: Contains the `Node` hierarchy, `Animator` logic, and the main `Game` class.
- **`Core`**: The entry point and coordination layer that initializes and shuts down the systems.

## Instructions 
Since this project is working off a Solution (.sln) file, I recommend using Visual Studio 2022.

Ensure you have the **C++ Development Package** installed via the Visual Studio Installer.

### Setup
1. Open `ChiselEngine.sln` in Visual Studio.
2. Set the include directories to point to the `/include` folder.
3. Ensure you have an instance of OpenXR running with a connected VR Headset or MR Device.
4. Use only **GLB** or **glTF** files in the `Resources/` folder.

## Getting Started - Game.cpp
To create your game, you only need to implement the methods in `src/Scene/Game.cpp`.

```C++
#include "Scene/Game.h"
#include "Scene/Node.h"
#include "Scene/Animator.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/DirectionalLight.h"
#include "Rendering/GLBLoader.h"

MeshNode* rockNode;
Animator* rockAnimator;

void Game::start() {
    // 1. Setup Lighting
    DirectionalLight* sun = new DirectionalLight("Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
    sun->setColor(glm::vec3(1.0f, 0.9f, 0.8f));
    sun->setIntensity(1.5f);
    RenderSystem::getInstance().addLight(sun);

    // 2. Load Model as a Node
    rockNode = GLBLoader::loadGLB("Resources/rock.glb");
    
    // 3. Setup Animator
    rockAnimator = new Animator(rockNode);
    
    // Procedural: Move along Z axis by 0.5 units/sec
    rockAnimator->animateAxis("z", 0.5f);
    
    // Keyframe: Play "Idle" clip loaded from GLB
    rockAnimator->playAnimation("Idle");
}

void Game::update(float deltaTime) {
    rockAnimator->update(deltaTime);
}

```

## Animation System
The engine supports two types of animation via the `Animator` class:

1. **Procedural Animation**: Direct control over axis velocity. Great for simple movement or floating effects.
   - `animator->animateAxis("x", 1.0f);`
2. **Keyframe Animation**: Fully compatible with GLTF/GLB. The engine interpolates between keyframes using SLERP for rotations, providing smooth, professional movement.
   - `animator->playAnimation("WalkCycle");`

## Physics System
ChiselEngine integrates **Jolt Physics** via a high-level wrapper for efficient, multi-threaded simulation.

### Using Physics
You can attach a `PhysicsBody` to any `Node` to make it react to gravity and collisions.

```C++
#include "Platform/PhysicsSystem.h"

// Create a dynamic physics body for a node
PhysicsBody* ballPhys = PhysicsSystem::getInstance().createRigidBody(ballNode, BodyType::Dynamic);
ballPhys->setMass(1.0f);
ballPhys->applyForce(glm::vec3(0, 10, 0)); // Apply upward force
```

### Body Types:
- **Static**: Unmovable objects (walls, floors).
- **Kinematic**: Moved via code, but can push dynamic objects.
- **Dynamic**: Fully simulated by the physics engine (affected by gravity).

## Features
- **VR-First Design**: Direct OpenXR integration for low-latency headset rendering.
- **Mirror Window**: Integrated desktop window to display the VR view for debugging.
- **DevUI**: A separate Win32 window to toggle simulation and debug engine state.
- **VR Simulation**: Test your game without a headset using the "Simulated VR" toggle.
- **Modern Lighting**: Real-time directional lighting with Phong shading.

## Special Thanks and Credits
- OpenGL: https://learnopengl.com/
- SFML: https://www.sfml-dev.org/
- OpenXR: https://github.com/khronosgroup/OpenXR-SDK-Source
