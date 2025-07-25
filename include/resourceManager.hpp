#pragma once

#include <raylib.h>
#include <unordered_map>
#include <string>
#include "machines.hpp"

class resourceManager {
public:
    // Initialize and load all machine assets
    static void initialize();
    // Unload all assets
    static void cleanup();

    // Accessors
    static Model& getMachineModel(machineType type);
    static Texture2D& getMachineTexture(machineType type);

private:
    // Prevent instantiation
    resourceManager() = delete;

    static bool initialized;
    static std::unordered_map<machineType, Model> machineModels;
    static std::unordered_map<machineType, Texture2D> machineTextures;
    static std::unordered_map<machineType, std::string> modelPaths;
    static std::unordered_map<machineType, std::string> texturePaths;
};
