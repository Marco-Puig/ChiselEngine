#include "core/engine.cpp"

Model rockModel, sceneModel; // Models
Texture rockTexture, sceneTexture; // Their respective textures

// Logic that runs once at the start of the game and used for initialization/declarations
void Game::start() {
	rockModel = loadModel("Resources/rock.obj", "Resources/rock_texture.jpeg");
	sceneModel = loadModel("Resources/zen_garden.obj", "Resources/zen_garden_texture.jpeg");
}

// Logic that runs once per frame - used for game logic
void Game::update() {
}

// Logic that runs once per frame - used for rendering
void Game::render() {
	drawModel(rockModel);
	drawModel(sceneModel);
}
