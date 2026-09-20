# Scene API Documentation

The `scene` object is passed into your `game.onStart(scene, animator)` and `game.onUpdate(deltaTime, scene, animator)` functions. It allows you to load assets, create environmental lighting, and find nodes in your game world.

---

## Methods

### 1. `scene:loadMesh(path, name)`
Loads a 3D model (GLB format) from disk, adds it to the active scene graph, and assigns it a unique name string.

* **Parameters:**
  * `path` (`string`): File path to the asset (e.g., `"resources/frog.glb"`).
  * `name` (`string`): The identifier name to assign to the node (e.g., `"Frog"`).
* **Returns:**
  * `Node`: A reference to the newly created mesh node, or `nil` if loading failed.
* **Example in `game.lua`:**
  ```lua
  local frog = scene:loadMesh("resources/frog.glb", "Frog")
  ```

---

### 2. `scene:findNode(name)`
Searches the current scene hierarchy for a node matching the specified identifier string.

* **Parameters:**
  * `name` (`string`): The name of the node you want to locate.
* **Returns:**
  * `Node`: A reference to the node if found, or `nil` if no matching node exists.
* **Example in `game.lua`:**
  ```lua
  local frog = scene:findNode("Frog")
  if frog ~= nil then
      -- Do something with the frog node
  end
  ```

---

### 3. `scene:createDirectionalLight(name)`
Creates a global directional light source (ideal for sunlight or moon simulation) and registers it with the render system.

* **Parameters:**
  * `name` (`string`): The unique identifier for the light.
* **Returns:**
  * `DirectionalLight`: A reference to the directional light instance.
* **Example in `game.lua`:**
  ```lua
  local light = scene:createDirectionalLight("Sun")
  light:setIntensity(0.4)
  light:setColor(1.0, 0.9, 0.8)
  ```

---

### 4. `scene:createPointLight(name, x, y, z)`
Creates a positional point light source at specific coordinates.

* **Parameters:**
  * `name` (`string`): The unique identifier for the light.
  * `x`, `y`, `z` (`number`): World coordinates for the light's position.
* **Returns:**
  * `PointLight`: A reference to the point light instance.
* **Example in `game.lua`:**
  ```lua
  local lamp = scene:createPointLight("StreetLamp", 0.0, 5.0, 2.0)
  ```