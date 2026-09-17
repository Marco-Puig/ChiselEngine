# Step 3: Scene Graph & Nodes

## Objective
Build a hierarchical scene graph where every object in the world is a `Node`.

## Instructions for AI
1. **Node Base Class (`src/scene/Node.h/.cpp`)**:
   - Properties: `std::string name`, `glm::vec3 position`, `glm::quat rotation`, `glm::vec3 scale`.
   - Relationships: `Node* parent`, `std::vector<std::unique_ptr<Node>> children`.
   - Methods: `addChild(std::unique_ptr<Node> child)`, `getWorldTransform()`.
   - `getWorldTransform()` must recursively multiply parent transforms.
2. **MeshNode (`src/scene/MeshNode.h/.cpp`)**:
   - Inherits from `Node`.
   - Holds OpenGL VAO/VBO/EBO IDs.
   - Holds a reference to a `Material` or `Shader`.
3. **Animator (`src/scene/Animator.h/.cpp`)**:
   - Attach to a Node to manipulate its transform over time.
   - Implement exactly this API:
     ```cpp
     void animateAxis(const std::string& axis, float amount); // e.g., "y" moves position.y
     void playAnimation(const std::string& animName); // Triggers pre-loaded GLB animations
     ```