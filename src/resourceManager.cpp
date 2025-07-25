#include "../include/resourceManager.hpp"
#include <raylib.h>

bool resourceManager::initialized = false;
std::unordered_map<machineType, Model> resourceManager::machineModels;
std::unordered_map<machineType, Texture2D> resourceManager::machineTextures;
std::unordered_map<machineType, std::string> resourceManager::modelPaths = {
    { CONVEYORMK1, "assets/models/conveyor_mk1.glb" },
    { DRILLMK1,    "assets/models/drill_mk1.glb" }
};
std::unordered_map<machineType, std::string> resourceManager::texturePaths = {
    { CONVEYORMK1, "assets/textures/conveyor_mk1.png" },
    { DRILLMK1,    "assets/textures/drill_mk1.png" }
};

void resourceManager::initialize() {
    if (initialized) return;

    for (auto& entry : modelPaths) {
        machineType type = entry.first;
        const std::string& path = entry.second;
        Model model = LoadModel(path.c_str());
        Texture2D texture = LoadTexture(texturePaths[type].c_str());
        // Apply texture to model's first material
        if (model.materialCount > 0) {
            model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
        }
        machineModels[type] = model;
        machineTextures[type] = texture;
    }

    initialized = true;
}

void resourceManager::cleanup() {
    if (!initialized) return;

    for (auto& entry : machineModels) {
        UnloadModel(entry.second);
    }
    for (auto& entry : machineTextures) {
        UnloadTexture(entry.second);
    }

    machineModels.clear();
    machineTextures.clear();

    initialized = false;
}

Model& resourceManager::getMachineModel(machineType type) {
    return machineModels[type];
}

Texture2D& resourceManager::getMachineTexture(machineType type) {
    return machineTextures[type];
}
