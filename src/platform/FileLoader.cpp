#include "platform/FileLoader.h"

#include <cstdio>
#include <stdexcept>

#ifdef CHISEL_TARGET_ANDROID
#include <android/asset_manager.h>
#endif

namespace FileLoader {

#ifdef CHISEL_TARGET_ANDROID
static AAssetManager* g_assetManager = nullptr;

void setAssetManager(AAssetManager* manager) {
    g_assetManager = manager;
}

AAssetManager* getAssetManager() {
    return g_assetManager;
}
#endif

std::vector<unsigned char> loadBinaryFile(const std::string& path) {
#ifdef CHISEL_TARGET_ANDROID
    if (g_assetManager == nullptr) {
        throw std::runtime_error("AAssetManager is not initialized. Cannot load: " + path);
    }

    AAsset* asset = AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_BUFFER);
    
    if (asset == nullptr) {
        throw std::runtime_error("Failed to open Android asset: " + path);
    }

    const size_t size = static_cast<size_t>(AAsset_getLength64(asset));
    std::vector<unsigned char> data(size);

    if (size > 0) {
        const int bytesRead = AAsset_read(asset, data.data(), size);
        if (bytesRead < 0 || static_cast<size_t>(bytesRead) != size) {
            AAsset_close(asset);
            throw std::runtime_error("Failed to read entire Android asset: " + path);
        }
    }

    AAsset_close(asset);
    return data;

#else
    // Desktop implementation
    FILE* file = std::fopen(path.c_str(), "rb");

    if (file == nullptr) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::fseek(file, 0, SEEK_END);
    const long fileSize = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if (fileSize < 0) {
        std::fclose(file);
        throw std::runtime_error("Failed to determine file size: " + path);
    }

    std::vector<unsigned char> data(static_cast<size_t>(fileSize));

    if (fileSize > 0) {
        const size_t bytesRead = std::fread(data.data(), 1, static_cast<size_t>(fileSize), file);

        if (bytesRead != static_cast<size_t>(fileSize)) {
            std::fclose(file);
            throw std::runtime_error("Failed to read entire file: " + path);
        }
    }

    std::fclose(file);
    return data;
#endif
}

std::string loadTextFile(const std::string& path) {
    const std::vector<unsigned char> data = loadBinaryFile(path);
    return std::string(data.begin(), data.end());
}

bool exists(const std::string& path) {
#ifdef CHISEL_TARGET_ANDROID
    if (g_assetManager == nullptr) {
        return false;
    }
    
    AAsset* asset = AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_UNKNOWN);
    if (asset != nullptr) {
        AAsset_close(asset);
        return true;
    }
    return false;

#else
    FILE* file = std::fopen(path.c_str(), "rb");

    if (file != nullptr) {
        std::fclose(file);
        return true;
    }

    return false;
#endif
}

}