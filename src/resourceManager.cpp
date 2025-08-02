#include "../include/resourceManager.hpp"
#include <raylib.h>

bool resourceManager::initialized = false;
std::unordered_map<machineType, Model> resourceManager::machineModels;
std::unordered_map<machineType, Texture2D> resourceManager::machineTextures;
Shader resourceManager::terrainShader;
Shader resourceManager::waterShader;
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
    sun sunData;
    if (initialized) return;

    // Load terrain and water shaders
    terrainShader = LoadShader("assets/shaders/terrainShader.vs", "assets/shaders/terrainShader.fs");
    waterShader   = LoadShader("assets/shaders/waterShader.vs",   "assets/shaders/waterShader.fs");
    // Set default HSV shift for water shader

    float defaultShift = -0.1f;
    float defaultSat = 4.0f;
    float defaultVal = 0.5f;
    int locHue = GetShaderLocation(waterShader, "waterHue");
    int locSat = GetShaderLocation(waterShader, "waterSaturation");
    int locVal = GetShaderLocation(waterShader, "waterValue");
    SetShaderValue(waterShader, locHue, &defaultShift, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locSat, &defaultSat, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locVal, &defaultVal, SHADER_UNIFORM_FLOAT);
    
    // Set default depth-based alpha parameters
    float minDepth = 0.0f;      // Start alpha calculation at 0 depth
    float maxDepth = 4.0f;      // Full alpha at 2 units deep
    float minAlpha = 0.4f;      // 20% alpha for very shallow water
    float maxAlpha = 1.0f;      // 80% alpha for deep water
    int locMinDepth = GetShaderLocation(waterShader, "minDepth");
    int locMaxDepth = GetShaderLocation(waterShader, "maxDepth");
    int locMinAlpha = GetShaderLocation(waterShader, "minAlpha");
    int locMaxAlpha = GetShaderLocation(waterShader, "maxAlpha");
    SetShaderValue(waterShader, locMinDepth, &minDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locMaxDepth, &maxDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locMinAlpha, &minAlpha, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locMaxAlpha, &maxAlpha, SHADER_UNIFORM_FLOAT);

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
            
            // Set initial shader values for this model
            SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "sunDirection"), &sunData.sunDirection, SHADER_UNIFORM_VEC3);
            SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "sunColor"), &sunData.sunColor, SHADER_UNIFORM_VEC3);
            SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "ambientStrength"), &sunData.ambientStrength, SHADER_UNIFORM_FLOAT);
            SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "ambientColor"), &sunData.ambientColor, SHADER_UNIFORM_VEC3);
            SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "shiftIntensity"), &sunData.shiftIntensity, SHADER_UNIFORM_FLOAT);
            SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "shiftDisplacement"), &sunData.shiftDisplacement, SHADER_UNIFORM_FLOAT);
        }
        
        machineModels[type] = model;
        machineTextures[type] = texture;
    }

    initialized = true;
}

// Update shader uniforms for machine models (if dynamic lighting changes needed)
void resourceManager::updateMachineLighting(Vector3 sunDirection, Color sunColor,
                                           float ambientStrength, Color ambientColor,
                                           float shiftIntensity, float shiftDisplacement) {
    // Convert Color to Vector3 for sunColor and ambientColor
    Vector3 sunCol = { sunColor.r/255.0f, sunColor.g/255.0f, sunColor.b/255.0f };
    Vector3 ambCol = { ambientColor.r/255.0f, ambientColor.g/255.0f, ambientColor.b/255.0f };
    //I know, this sucks ass
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "sunDirection"), &sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "sunColor"), &sunCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "ambientStrength"), &ambientStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "ambientColor"), &ambCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "shiftIntensity"), &shiftIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "shiftDisplacement"), &shiftDisplacement, SHADER_UNIFORM_FLOAT);
}

// Update shader uniforms for terrain chunks
void resourceManager::updateTerrainLighting(Vector3 sunDirection, Vector3 sunColor,
                                            float ambientStrength, Vector3 ambientColor,
                                            float shiftIntensity, float shiftDisplacement) {
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "sunDirection"), &sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "sunColor"), &sunColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "ambientStrength"), &ambientStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "ambientColor"), &ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "shiftIntensity"), &shiftIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "shiftDisplacement"), &shiftDisplacement, SHADER_UNIFORM_FLOAT);
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
    UnloadShader(waterShader);

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

Shader& resourceManager::getShader(int n) {
    switch(n) {
        case 0:
            return terrainShader;
        case 1:
            return waterShader;
        default:
            return terrainShader;
    }
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

// Update water depth-based alpha parameters
void resourceManager::updateWaterDepthParams(float minDepth, float maxDepth, float minAlpha, float maxAlpha) {
    int locMinDepth = GetShaderLocation(waterShader, "minDepth");
    int locMaxDepth = GetShaderLocation(waterShader, "maxDepth");
    int locMinAlpha = GetShaderLocation(waterShader, "minAlpha");
    int locMaxAlpha = GetShaderLocation(waterShader, "maxAlpha");
    SetShaderValue(waterShader, locMinDepth, &minDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locMaxDepth, &maxDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locMinAlpha, &minAlpha, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, locMaxAlpha, &maxAlpha, SHADER_UNIFORM_FLOAT);
}
