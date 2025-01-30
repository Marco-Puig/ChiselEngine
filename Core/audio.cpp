#include <audio.h>

void Audio::playAudio(const std::string& audioPath) {
	// Load audio file
	if (!buffer.loadFromFile(audioPath)) {
		std::cout << "Error loading audio file" << std::endl;
	}
	// Setup and Play audio
	sound.setBuffer(buffer);
	sound.play();
}

void Audio::stopAudio() {
	// Stop audio
	sound.stop();
}

void Audio::setVolume(float volume) {
	sound.setVolume(volume);
}
