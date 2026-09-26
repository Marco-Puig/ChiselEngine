#pragma once

#include <string>
#include <vector>

#ifdef CHISEL_TARGET_ANDROID
struct AAssetManager;
#endif

namespace FileLoader {

#ifdef CHISEL_TARGET_ANDROID
void setAssetManager(AAssetManager* manager);
AAssetManager* getAssetManager();
#endif
std::vector<unsigned char> loadBinaryFile(const std::string& path);
std::string loadTextFile(const std::string& path);
bool exists(const std::string& path);

}