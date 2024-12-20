# ChiselEngine
Chisel Engine is an OpenXR/OpenGL Game Engine that is being developed for my Steam game - Rock Life: The Rock Simulator 

## Instructions 
Since this is project is working of a Solution (.sln) file, I recommend to use Visual Studio 2022

Ensure you have the C++ Development Package Installed (You will be shown these as option in the Visual Studio Installer)

Additionally, you will need to use your own **OBJ** file(s) in the Resources folder.\
For example: `Resources/rock.obj`

Before running, make sure you have an instance of OpenXR running along with a connected VR Headset or MR Device.

## Features
- Desktop Window to Display what is shown on the VR Headset via OpenXR
- Controller detection and input support
- Game Logic Component (No need to work with the Engine to start creating your Game)

## Getting Started
```C++
Model rockModel, sceneModel; // Models
Texture rockTexture, sceneTexture; // Their respective textures
glm::mat4 modelMatrix; // transform, rotate, scale - model matrix

// Logic that runs once at the start of the game and used for initialization/declarations
void Game::init() {
	rockModel = loadModel("Resources/rock.obj", "Resources/rock_texture.jpeg");
	sceneModel = loadModel("Resources/SANDnSTONE_simplified.obj", "Resources/SANDnSTONE_simplified.jpeg");
}

// Logic that runs once per frame - used for rendering
void Game::render(const glm::mat4& viewProj) {
	modelMatrix = 
		glm::translate(glm::mat4(1.0f), 
		glm::vec3(0.0f, 0.0f, -5.0f))* glm::scale(glm::mat4(1.0f),
		glm::vec3(1.0f));

	// Render the models - they can share the same matrix
	drawModel(rockModel, viewProj, modelMatrix);
	drawModel(sceneModel, viewProj, modelMatrix);
}

// Logic that updates per frame - used for game logic
void Game::update() {

}

// Logic that runs once at the end of the game - can also destroy objects at runtime
void Game::destroy() {
	cleanupModel(rockModel);
	cleanupModel(sceneModel);
}
```
<img width="573" alt="Screenshot 2024-12-16 233921" src="https://github.com/user-attachments/assets/de5c6c8d-4527-48e8-b493-6c9620fb4ea3" />






