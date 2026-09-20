# Animator API Documentation

The `animator` object handles automated transformations, procedural motion loops, and skeletal/mesh animations frame-by-frame.

---

## Methods

### 1. `animator:procedural(node, axis, direction, type, speed)`
Sets up a continuous procedural animation loop (such as constant rotation or translation) on a node.

* **Parameters:**
  * `node` (`Node`): The target node to animate.
  * `axis` (`string`): Transformation axis (`"x"`, `"y"`, or `"z"`).
  * `direction` (`string`): Movement or rotation direction (`"positive"` or `"negative"`).
  * `type` (`string`): Property to modify (`"rotation"` or `"position"`).
  * `speed` (`number`): Speed multiplier value.
* **Returns:**
  * `number`: A unique procedural animation ID handle used to control playback later, or `-1` on failure.
* **Example in `game.lua`:**
  ```lua
  frogRotationId = animator:procedural(frog, "y", "positive", "rotation", 1.0)
  ```

---

### 2. `animator:stopProcedural(id)`
Halts an active procedural animation loop using its assigned handle ID.

* **Parameters:**
  * `id` (`number`): The handle ID returned by `animator:procedural()`.
* **Example in `game.lua`:**
  ```lua
  if timeElapsed > 5.0 then
      animator:stopProcedural(frogRotationId)
  end
  ```

---

### 3. `animator:playProcedural(id)` / `animator:playAnimation(node, animName)`
* **`playProcedural(id)`**: Resumes a previously stopped procedural animation loop.
* **`playAnimation(node, animName)`**: Triggers a pre-loaded clip from the GLB animation library by name.

---

### 4. `animator:stopAnimation(node, animName)`
Stops a currently playing GLB node animation clip.

* **Parameters:**
  * `node` (`Node`): The target node.
  * `animName` (`string`): The name of the animation clip to halt.