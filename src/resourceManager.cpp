#include "../include/resourceManager.hpp"
#include <raylib.h>

bool resourceManager::initialized = false;
std::unordered_map<machineType, Model> resourceManager::machineModels;
std::unordered_map<machineType, Texture2D> resourceManager::machineTextures;
Shader resourceManager::terrainShader;
std::unordered_map<machineType, std::string> resourceManager::modelPaths = {
    { CONVEYORMK1, "assets/models/conveyor_mk1.glb" },
    { DRILLMK1,    "assets/models/drill_mk1.glb" }
};
std::unordered_map<machineType, std::string> resourceManager::texturePaths = {
    { CONVEYORMK1, "assets/textures/conveyor_mk1.png" },
    { DRILLMK1,    "assets/textures/drill_mk1.png" }
};

std::unordered_map<itemType, itemTextureUVs> resourceManager::itemTextureUVsMap = {
    {IRON_ORE, {0, 0, 16, 16}},
    {COPPER_ORE, {16, 0, 16, 16}}
};

Texture2D resourceManager::itemTexture;

void resourceManager::initialize() {
    if (initialized) return;

    // Load shader first
    terrainShader = LoadShader("assets/shaders/terrainShader.vs", 
                              "assets/shaders/terrainShader.fs");

    // Load item texture atlas
    itemTexture = LoadTexture("assets/textures/items.png");

    for (auto& entry : modelPaths) {
        machineType type = entry.first;
        const std::string& path = entry.second;
        Model model = LoadModel(path.c_str());
        Texture2D texture = LoadTexture(texturePaths[type].c_str());
        
        // Apply texture and shader to model's materials
        if (model.materialCount > 0) {
            model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
            model.materials[0].shader = terrainShader;
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
    
    UnloadTexture(itemTexture);
    UnloadShader(terrainShader);

    machineModels.clear();
    machineTextures.clear();

    initialized = false;
}

Model& resourceManager::getMachineModel(machineType type) {
    return machineModels.at(type);
}

Texture2D& resourceManager::getMachineTexture(machineType type) {
    return machineTextures[type];
}

Shader& resourceManager::getShader() {
    return terrainShader;
}

Texture2D& resourceManager::getItemTexture(itemType type) {
    return itemTexture;
}

Rectangle resourceManager::getItemTextureUV(itemType type) {
    auto it = itemTextureUVsMap.find(type);
    if (it != itemTextureUVsMap.end()) {
        const itemTextureUVs& uvs = it->second;
        return Rectangle{(float)uvs.Uoffset, (float)uvs.Voffset, (float)uvs.Usize, (float)uvs.Vsize};
    }
    // Return default UV if not found
    return Rectangle{0, 0, 16, 16};
}
