# Step 6: Physics & The Developer's Game Class

## Objective
Implement the physics wrapper and the final `Game` class as described in the design document.

## Instructions for AI
1. **PhysicsSystem (`src/platform/PhysicsSystem.h/.cpp`)**:
   - Singleton. For now, implement a simplified custom rigid body solver or integrate a lightweight header-only physics lib (like ReactPhysics3D or Jolt via FetchContent).
   - Implement the exact API requested:
     ```cpp
     PhysicsBody* createRigidBody(Node* node, BodyType type);
     // PhysicsBody must automatically sync its transform back to the Node every frame.
     ```
2. **Game Class (`src/game/Game.h/.cpp`)**:
   - Inherits from `IGame`.
   - Implement `start()` and `update(float dt)` using the exact logic from the blueprint:
     ```cpp
     void Game::start() {
         DirectionalLight* sun = new DirectionalLight("Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
         sun->setColor(glm::vec3(1.0f, 0.9f, 0.8f));
         RenderSystem::getInstance().addLight(sun);
         MeshNode* playerSword = GLBLoader::loadGLB("Resources/sword.glb");
         sceneRoot->addChild(std::unique_ptr<Node>(playerSword));
     }
     
     void Game::update(float deltaTime) {
         if (Input::isButtonPressed("BUTTON_A")) {
             swordAnimator->playAnimation("Attack_Swing");
         }
     }
     ```
3. **Final Polish**:
   - Ensure all Singletons are destroyed in the correct reverse-order to prevent OpenGL context leaks during shutdown.