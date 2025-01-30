#pragma once

#include <SFML/Audio.hpp> // Audio and sound lib from SFML Audio Module
#include <iostream> // std::cout debugging to console

class Audio {
private:
	sf::SoundBuffer buffer;
	sf::Sound sound;
public:
	void playAudio(const std::string& path);
	void stopAudio();
	void setVolume(float volume);
};
