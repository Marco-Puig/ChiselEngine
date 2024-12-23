# ChiselEngine
Chisel Engine is a game engine built on OpenXR and OpenGL, designed for developing immersive VR applications.

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
void Game::start() {
	rockModel = loadModel("Resources/rock.obj", "Resources/rock_texture.jpeg");
	sceneModel = loadModel("Resources/SANDnSTONE_simplified.obj", "Resources/SANDnSTONE_simplified.jpeg");
}

// Logic that runs once per frame - used for game logic
void Game::update() {
	
}

// Logic that runs once per frame - used for rendering
void Game::render() {
	modelMatrix = 
		glm::translate(glm::mat4(1.0f), 
		glm::vec3(0.0f, 0.0f, -5.0f))* glm::scale(glm::mat4(1.0f),
		glm::vec3(1.0f));

	// Render the models - they can share the same matrix
	drawModel(rockModel, modelMatrix);
	drawModel(sceneModel, modelMatrix);
}
```
<img width="574" alt="Screenshot 2024-12-20 212619" src="https://github.com/user-attachments/assets/1571482e-8adf-43cb-a148-b198c25e78cd" />






