# ChiselEngine
Chisel Engine is a modular game engine built on OpenXR and OpenGL, designed for developing immersive VR applications.

## Architecture
The engine has been refactored into a modular singleton-based architecture to ensure scalability and ease of maintenance:

- **`Platform`**: Manages OS-level concerns, including GLFW window creation and input handling.
- **`XR`**: Handles the OpenXR session, headset tracking, swapchains, and VR input.
- **`Rendering`**: Manages the OpenGL pipeline, shaders, textures, and buffer management.
- **`Scene`**: Contains high-level game logic and the main `Game` class.
- **`Core`**: The entry point and coordination layer that initializes and shuts down the systems.

## Instructions 
Since this project is working off a Solution (.sln) file, I recommend using Visual Studio 2022.

Ensure you have the **C++ Development Package** installed via the Visual Studio Installer.

### Setup
1. Open `ChiselEngine.sln` in Visual Studio.
2. Set the include directories to point to the `/include` folder.
3. Ensure you have an instance of OpenXR running with a connected VR Headset or MR Device.
4. Use your own **OBJ** files in the `Resources/` folder.

## Getting Started - Game.cpp
To create your game, you only need to implement the methods in `src/Scene/Game.cpp`.

```C++
#include "Scene/Game.h"

Model rockModel;
Transform rockTransform;

// Logic that runs once at the start of the game
void Game::start() {
    rockModel.loadModel("Resources/rock.obj", "Resources/rock_texture.jpeg");
}

// Logic that runs once per frame - used for game logic
void Game::update() {
    // Move the rock forward in the z-axis
    rockTransform.position += glm::vec3(0.0f, 0.0f, 0.01f);
}

// Logic that runs once per frame - used for rendering
void Game::render() {
    rockModel.drawModel(rockTransform);
}
```

## Features
- **VR-First Design**: Direct OpenXR integration for low-latency headset rendering.
- **Mirror Window**: Integrated desktop window to display the VR view for debugging.
- **Modular Pipeline**: Clean separation between platform, rendering, and gameplay code.

## Special Thanks and Credits
- OpenGL: https://learnopengl.com/
- SFML: https://www.sfml-dev.org/
- OpenXR: https://github.com/khronosgroup/OpenXR-SDK-Source
