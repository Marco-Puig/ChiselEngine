# ChiselEngine

ChiselEngine is a modular VR game engine built on C++20, OpenGL, and OpenXR. It is designed to decouple complex VR boilerplate from gameplay logic.

## Philosophy
The engine follows a "Zero-Friction" philosophy. The goal is to allow developers to focus on gameplay mechanics rather than VR hardware abstraction. This is achieved through:
- Modular Singletons: Core systems (Rendering, XR, Physics) are accessible globally.
- Scene Graph: A Node-based hierarchy for intuitive object transformation and parenting.
- VR-First with Desktop Fallback: The engine prioritizes OpenXR but gracefully falls back to a simulated desktop mode for easier development and testing.

## Making a Game
Gameplay is implemented by inheriting from the `IGame` interface and implementing the `Game` class in `src/game/Game.cpp`.

### Game Life Cycle
- `start()`: Called once when the engine launches. Use this to initialize your scene, load models, and set up lighting.
- `update(float deltaTime)`: Called every frame. This is where you handle input and update game logic.

### Example Implementation
```cpp
void Game::start() {
    // Add a sun light to the world
    DirectionalLight* sun = new DirectionalLight("Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
    RenderSystem::getInstance().addLight(sun);

    // Load a model into the scene
    MeshNode* playerSword = GLBLoader::loadGLB("Resources/sword.glb");
    sceneRoot->addChild(std::unique_ptr<Node>(playerSword));
}

void Game::update(float deltaTime) {
    if (Input::isButtonPressed("BUTTON_A")) {
        swordAnimator->playAnimation("Attack_Swing");
    }
}
```

## Build and Run

### Prerequisites
- CMake 3.14+
- Visual Studio 2022 (MSVC)
- Git

### Build Instructions
1. Open a terminal in the root directory.
2. Create and enter the build folder:
   ```powershell
   mkdir build; cd build
   ```
3. Configure the project:
   ```powershell
   cmake ..
   ```
4. Compile the engine:
   ```powershell
   cmake --build .
   ```

### Running the Engine
Run the compiled executable from the build folder:
```powershell
.\Debug\ChiselEngine.exe
```
