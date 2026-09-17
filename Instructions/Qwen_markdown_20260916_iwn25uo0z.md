# Step 2: Core Engine & Platform Layer

## Objective
Implement the main loop, window creation, and the base `IGame` interface.

## Instructions for AI
1. **Window Class (`src/platform/Window.h/.cpp`)**:
   - Wrap GLFW window creation.
   - Initialize GLAD.
   - Handle window resizing and input polling.
2. **Core Engine (`src/core/Engine.h/.cpp`)**:
   - Implement as a Singleton.
   - `Engine::init()`: Initializes Window, RenderSystem, and XRManager.
   - `Engine::run(IGame* game)`: Starts the main loop.
   - Main Loop logic:
     ```cpp
     while (!window.shouldClose()) {
         float dt = calculateDeltaTime();
         game->update(dt);
         renderSystem->render(sceneGraph);
         xrManager->submitFrame();
         devUI->render();
     }
     ```
3. **IGame Interface (`src/core/IGame.h`)**:
   - Define pure virtual methods: `virtual void start() = 0;` and `virtual void update(float deltaTime) = 0;`.
4. **Main Entry (`src/main.cpp`)**:
   - Instantiate Engine, instantiate the user's `Game` class, and call `Engine::run()`.