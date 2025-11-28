#include "../include/resourceManager.hpp"
#include <raylib.h>

// Define SHADER_LOC_VERTEX_INSTANCE_TX for raylib 5.5 compatibility
// In raylib 5.6+, this is already defined in the enum
#ifndef SHADER_LOC_VERTEX_INSTANCE_TX
#define SHADER_LOC_VERTEX_INSTANCE_TX (SHADER_LOC_BONE_MATRICES + 1)
#endif

bool resourceManager::initialized = false;
std::unordered_map<machineType, Model> resourceManager::machineModels;
std::unordered_map<machineType, Texture2D> resourceManager::machineTextures;
Texture2D resourceManager::terrainTexture = {0};
Texture2D resourceManager::waterTexture = {0};
Texture2D resourceManager::waterDisplacementTexture = {0};
Shader resourceManager::terrainShader;
Shader resourceManager::waterShader;
Shader resourceManager::grassShader;
Material resourceManager::grassMaterial;
TerrainShaderLocs resourceManager::terrainLocs = {0};
WaterShaderLocs resourceManager::waterLocs = {0};
GrassShaderLocs resourceManager::grassLocs = {0};
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

void resourceManager::cacheShaderLocations() {
    // Cache terrain shader uniform locations
    terrainLocs.sunDirection = GetShaderLocation(terrainShader, "sunDirection");
    terrainLocs.sunColor = GetShaderLocation(terrainShader, "sunColor");
    terrainLocs.ambientStrength = GetShaderLocation(terrainShader, "ambientStrength");
    terrainLocs.ambientColor = GetShaderLocation(terrainShader, "ambientColor");
    terrainLocs.shiftIntensity = GetShaderLocation(terrainShader, "shiftIntensity");
    terrainLocs.shiftDisplacement = GetShaderLocation(terrainShader, "shiftDisplacement");
    
    // Cache water shader uniform locations
    waterLocs.waterHue = GetShaderLocation(waterShader, "waterHue");
    waterLocs.waterSaturation = GetShaderLocation(waterShader, "waterSaturation");
    waterLocs.waterValue = GetShaderLocation(waterShader, "waterValue");
    waterLocs.minDepth = GetShaderLocation(waterShader, "minDepth");
    waterLocs.maxDepth = GetShaderLocation(waterShader, "maxDepth");
    waterLocs.minAlpha = GetShaderLocation(waterShader, "minAlpha");
    waterLocs.maxAlpha = GetShaderLocation(waterShader, "maxAlpha");
    waterLocs.time = GetShaderLocation(waterShader, "time");
    
    // Cache grass shader uniform locations
    // CRITICAL: Set MVP location for instancing to work!
    grassShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(grassShader, "mvp");
    grassShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(grassShader, "viewPos");
    
    // CRITICAL: Set instance transform attribute location for DrawMeshInstanced to work!
    // This tells raylib where to bind the per-instance transform matrices
    grassShader.locs[SHADER_LOC_VERTEX_INSTANCE_TX] = GetShaderLocationAttrib(grassShader, "instanceTransform");
    
    grassLocs.mvp = grassShader.locs[SHADER_LOC_MATRIX_MVP];
    grassLocs.viewPos = grassShader.locs[SHADER_LOC_VECTOR_VIEW];
    grassLocs.time = GetShaderLocation(grassShader, "time");
    grassLocs.windStrength = GetShaderLocation(grassShader, "windStrength");
    grassLocs.windDirection = GetShaderLocation(grassShader, "windDirection");
    grassLocs.windSpeed = GetShaderLocation(grassShader, "windSpeed");
    grassLocs.sunDirection = GetShaderLocation(grassShader, "sunDirection");
    grassLocs.sunColor = GetShaderLocation(grassShader, "sunColor");
    grassLocs.ambientStrength = GetShaderLocation(grassShader, "ambientStrength");
    grassLocs.ambientColor = GetShaderLocation(grassShader, "ambientColor");
    grassLocs.shiftIntensity = GetShaderLocation(grassShader, "shiftIntensity");
    grassLocs.shiftDisplacement = GetShaderLocation(grassShader, "shiftDisplacement");
}

void resourceManager::initialize() {
    sun sunData;
    if (initialized) return;

    terrainTexture = LoadTexture("textures.png");
    // Load terrain and water shaders
    terrainShader = LoadShader("assets/shaders/terrainShader.vs", "assets/shaders/terrainShader.fs");
    waterShader   = LoadShader("assets/shaders/waterShader.vs",   "assets/shaders/waterShader.fs");
    grassShader   = LoadShader("assets/shaders/grassShader.vs",   "assets/shaders/grassShader.fs");
    waterTexture = LoadTexture("assets/textures/water.png");
    waterDisplacementTexture = LoadTexture("assets/textures/waterDisplacement.png");
    
    // Cache all shader uniform locations once
    cacheShaderLocations();
    
    // Initialize grass material with the grass shader
    grassMaterial = LoadMaterialDefault();
    grassMaterial.shader = grassShader;
    grassMaterial.maps[MATERIAL_MAP_DIFFUSE].color = (Color){80, 160, 60, 255};  // Base green
    
    // Set default grass shader values
    float defaultWindStrength = 0.15f;
    float defaultWindSpeed = 2.0f;
    float windDir[2] = {0.7f, 0.7f};  // Diagonal wind
    SetShaderValue(grassShader, grassLocs.windStrength, &defaultWindStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windSpeed, &defaultWindSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windDirection, windDir, SHADER_UNIFORM_VEC2);
    
    // Set default HSV shift for water shader
    float defaultShift = -0.1f;
    float defaultSat = 4.0f;
    float defaultVal = 0.5f;
    SetShaderValue(waterShader, waterLocs.waterHue, &defaultShift, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.waterSaturation, &defaultSat, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.waterValue, &defaultVal, SHADER_UNIFORM_FLOAT);
    
    // Set default depth-based alpha parameters
    float minDepth = 0.0f;      // Start alpha calculation at 0 depth
    float maxDepth = 4.0f;      // Full alpha at 2 units deep
    float minAlpha = 0.4f;      // 20% alpha for very shallow water
    float maxAlpha = 1.0f;      // 80% alpha for deep water
    SetShaderValue(waterShader, waterLocs.minDepth, &minDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxDepth, &maxDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.minAlpha, &minAlpha, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxAlpha, &maxAlpha, SHADER_UNIFORM_FLOAT);

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
            
            // Set initial shader values for this model (using cached locations)
            SetShaderValue(terrainShader, terrainLocs.sunDirection, &sunData.sunDirection, SHADER_UNIFORM_VEC3);
            SetShaderValue(terrainShader, terrainLocs.sunColor, &sunData.sunColor, SHADER_UNIFORM_VEC3);
            SetShaderValue(terrainShader, terrainLocs.ambientStrength, &sunData.ambientStrength, SHADER_UNIFORM_FLOAT);
            SetShaderValue(terrainShader, terrainLocs.ambientColor, &sunData.ambientColor, SHADER_UNIFORM_VEC3);
            SetShaderValue(terrainShader, terrainLocs.shiftIntensity, &sunData.shiftIntensity, SHADER_UNIFORM_FLOAT);
            SetShaderValue(terrainShader, terrainLocs.shiftDisplacement, &sunData.shiftDisplacement, SHADER_UNIFORM_FLOAT);
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
    
    // Use cached locations instead of querying every call
    SetShaderValue(terrainShader, terrainLocs.sunDirection, &sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.sunColor, &sunCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.ambientStrength, &ambientStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.ambientColor, &ambCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.shiftIntensity, &shiftIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.shiftDisplacement, &shiftDisplacement, SHADER_UNIFORM_FLOAT);
}

// Update shader uniforms for terrain chunks
void resourceManager::updateTerrainLighting(Vector3 sunDirection, Vector3 sunColor,
                                            float ambientStrength, Vector3 ambientColor,
                                            float shiftIntensity, float shiftDisplacement) {
    // Use cached locations instead of querying every frame
    SetShaderValue(terrainShader, terrainLocs.sunDirection, &sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.sunColor, &sunColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.ambientStrength, &ambientStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.ambientColor, &ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.shiftIntensity, &shiftIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.shiftDisplacement, &shiftDisplacement, SHADER_UNIFORM_FLOAT);
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
    UnloadTexture(waterDisplacementTexture);
    UnloadShader(terrainShader);
    UnloadShader(waterShader);
    UnloadShader(grassShader);
    // Note: grassMaterial uses grassShader which is already unloaded above
    // Material doesn't own resources, so no separate unload needed

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
    SetShaderValue(waterShader, waterLocs.minDepth, &minDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxDepth, &maxDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.minAlpha, &minAlpha, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxAlpha, &maxAlpha, SHADER_UNIFORM_FLOAT);
}

// Update water time for displacement animation
void resourceManager::updateWaterTime(float time) {
    SetShaderValue(waterShader, waterLocs.time, &time, SHADER_UNIFORM_FLOAT);
}

// Update grass shader uniforms
void resourceManager::updateGrassUniforms(float time, Vector3 cameraPos,
                                          Vector3 sunDirection, Vector3 sunColor,
                                          float ambientStrength, Vector3 ambientColor,
                                          float shiftIntensity, float shiftDisplacement) {
    SetShaderValue(grassShader, grassLocs.time, &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassShader.locs[SHADER_LOC_VECTOR_VIEW], &cameraPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.sunDirection, &sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.sunColor, &sunColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.ambientStrength, &ambientStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.ambientColor, &ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.shiftIntensity, &shiftIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.shiftDisplacement, &shiftDisplacement, SHADER_UNIFORM_FLOAT);
}

Shader& resourceManager::getGrassShader() {
    return grassShader;
}

Material& resourceManager::getGrassMaterial() {
    return grassMaterial;
}

GrassShaderLocs& resourceManager::getGrassShaderLocs() {
    return grassLocs;
}
