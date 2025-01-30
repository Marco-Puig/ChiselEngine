#include "core/engine.cpp"

Model rockModel, sceneModel; // Models
Texture rockTexture, sceneTexture; // Their respective textures
Audio testSound;

// Logic that runs once at the start of the game and used for initialization/declarations
void Game::start() {
	rockModel.loadModel("Resources/rock.obj", "Resources/rock_texture.jpeg");
	sceneModel.loadModel("Resources/zen_garden.obj", "Resources/zen_garden_texture.jpeg");
	testSound.playAudio("Resources/test_sound.wav"); // assign audio file and play it (can use setVolume() to adjust volume)
}

// Logic that runs once per frame - used for game logic
void Game::update() {
	// input test!
	if (inputDetected())
		testSound.stopAudio(); // can also stop the same audio, for example, on input
}

// Logic that runs once per frame - used for rendering
void Game::render() {
	rockModel.drawModel();
	sceneModel.drawModel();
}
