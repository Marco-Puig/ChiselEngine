# Step 4: Rendering Pipeline & Assets

## Objective
Create the OpenGL rendering backend and asset loaders.

## Instructions for AI
1. **RenderSystem (`src/rendering/RenderSystem.h/.cpp`)**:
   - Singleton.
   - Manages OpenGL state (depth test, culling).
   - `render(Node* rootNode)`: Traverses the scene graph, calculates MVP matrices, and issues draw calls.
2. **Shader (`src/rendering/Shader.h/.cpp`)**:
   - Loads vertex/fragment shaders from files.
   - Provides `setMat4`, `setVec3`, `setInt` uniform setters.
3. **Lighting (`src/rendering/Light.h`)**:
   - Base `Light` class. Derived `DirectionalLight` and `PointLight`.
   - `RenderSystem` must maintain a `std::vector<Light*>` to pass to shaders.
4. **GLBLoader (`src/rendering/GLBLoader.h/.cpp`)**:
   - Use `tinygltf` to parse `.glb` files.
   - Extract meshes, textures, and animation data.
   - Return a fully constructed `MeshNode` hierarchy.