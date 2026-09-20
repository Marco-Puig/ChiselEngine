# Physics Engine API Documentation

The `Engine` namespace provides rigid body creation, collision settings, and environment properties (powered by Jolt Physics under the hood).

---

## Namespace Functions

### 1. `Engine.addRigidBody(node, type, collider, friction, restitution)`
Converts a standard 3D mesh node into a physics-simulated rigid body.

* **Parameters:**
  * `node` (`Node`): The target node to attach physics to.
  * `type` (`string`): Motion type. Options include:
    * `"static"`: Immovable environmental geometry (does not move under forces).
    * `"dynamic"`: Fully simulated physics body responding to gravity and forces.
    * `"kinematic"`: Moved via script logic rather than forces, but can push dynamic bodies.
  * `collider` (`string`): Collision shape generation mode (e.g., `"convex"` or `"box"`).
  * `friction` (`number`): Surface friction coefficient (e.g., `0.6` or `0.8`).
  * `restitution` (`number`): Bounciness coefficient (e.g., `0.1`).
* **Returns:**
  * `boolean`: `true` if the rigid body was successfully created and added.
* **Example in `game.lua`:**
  ```lua
  Engine.addRigidBody(floor, "static", "convex", 0.8, 0.0)
  Engine.addRigidBody(frog, "dynamic", "convex", 0.6, 0.1)
  ```

---

### 2. `Engine.setSkybox(path)`
Loads and sets the background skybox environment map for the active scene.

* **Parameters:**
  * `path` (`string`): File path to the skybox texture (e.g., `"resources/skybox.jpg"`).
* **Example in `game.lua`:**
  ```lua
  Engine.setSkybox("resources/skybox.jpg")
  ```

---

## Node Physics Methods
Once a rigid body is added, you can manipulate its physics state via the `Node` reference:

* **`node:setPosition(x, y, z)`**: Instantly teleports the node and syncs its physics body coordinates.
* **`node:addForce(x, y, z)`**: Applies an instantaneous physical force vector to dynamic bodies.
* **`node:setLinearVelocity(x, y, z)`**: Sets the linear movement speed vector directly.
* **`node:getPositionY()`**: Returns the vertical coordinate, commonly used for out-of-bounds checks:
  ```lua
  if frog:getPositionY() < -100.0 then
      frog:setPosition(0.0, 3.0, 0.0)
  end
  ```