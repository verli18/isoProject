#include "../include/resourceManager.hpp"
#include "../include/visualSettings.hpp"
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
    // Erosion dithering uniform locations
    terrainLocs.erosionThreshold = GetShaderLocation(terrainShader, "erosionThreshold");
    terrainLocs.erosionFullExpose = GetShaderLocation(terrainShader, "erosionFullExpose");
    terrainLocs.ditherIntensity = GetShaderLocation(terrainShader, "ditherIntensity");
    // Per-biome exposed texture U offsets
    terrainLocs.grassExposedU = GetShaderLocation(terrainShader, "grassExposedU");
    terrainLocs.snowExposedU = GetShaderLocation(terrainShader, "snowExposedU");
    terrainLocs.sandExposedU = GetShaderLocation(terrainShader, "sandExposedU");
    terrainLocs.stoneExposedU = GetShaderLocation(terrainShader, "stoneExposedU");
    // Visualization mode
    terrainLocs.visualizationMode = GetShaderLocation(terrainShader, "visualizationMode");
    
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
    
    // Cache grass color uniforms - warm biome
    grassLocs.grassTipColor = GetShaderLocation(grassShader, "grassTipColor");
    grassLocs.grassBaseColor = GetShaderLocation(grassShader, "grassBaseColor");
    
    // Cache tundra color uniforms
    grassLocs.tundraTipColor = GetShaderLocation(grassShader, "tundraTipColor");
    grassLocs.tundraBaseColor = GetShaderLocation(grassShader, "tundraBaseColor");
    
    // Cache snow color uniforms
    grassLocs.snowTipColor = GetShaderLocation(grassShader, "snowTipColor");
    grassLocs.snowBaseColor = GetShaderLocation(grassShader, "snowBaseColor");
    
    // Cache desert color uniforms
    grassLocs.desertTipColor = GetShaderLocation(grassShader, "desertTipColor");
    grassLocs.desertBaseColor = GetShaderLocation(grassShader, "desertBaseColor");
    
    // Cache temperature threshold uniforms - cold
    grassLocs.tundraStartTemp = GetShaderLocation(grassShader, "tundraStartTemp");
    grassLocs.tundraFullTemp = GetShaderLocation(grassShader, "tundraFullTemp");
    grassLocs.snowStartTemp = GetShaderLocation(grassShader, "snowStartTemp");
    grassLocs.snowFullTemp = GetShaderLocation(grassShader, "snowFullTemp");
    
    // Cache temperature threshold uniforms - hot
    grassLocs.desertStartTemp = GetShaderLocation(grassShader, "desertStartTemp");
    grassLocs.desertFullTemp = GetShaderLocation(grassShader, "desertFullTemp");
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
    
    // Get settings from VisualSettings
    GrassSettings& grassSettings = VisualSettings::getInstance().getGrassSettings();
    WaterSettings& waterSettings = VisualSettings::getInstance().getWaterSettings();
    
    // Set grass shader wind values from settings
    float windDir[2] = {grassSettings.windDirection.x, grassSettings.windDirection.y};
    SetShaderValue(grassShader, grassLocs.windStrength, &grassSettings.windStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windSpeed, &grassSettings.windSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windDirection, windDir, SHADER_UNIFORM_VEC2);
    
    // Set grass color uniforms - warm biome
    SetShaderValue(grassShader, grassLocs.grassTipColor, &grassSettings.tipColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.grassBaseColor, &grassSettings.baseColor, SHADER_UNIFORM_VEC3);
    
    // Set tundra color uniforms
    SetShaderValue(grassShader, grassLocs.tundraTipColor, &grassSettings.tundraTipColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.tundraBaseColor, &grassSettings.tundraBaseColor, SHADER_UNIFORM_VEC3);
    
    // Set snow color uniforms
    SetShaderValue(grassShader, grassLocs.snowTipColor, &grassSettings.snowTipColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.snowBaseColor, &grassSettings.snowBaseColor, SHADER_UNIFORM_VEC3);
    
    // Set desert color uniforms
    SetShaderValue(grassShader, grassLocs.desertTipColor, &grassSettings.desertTipColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.desertBaseColor, &grassSettings.desertBaseColor, SHADER_UNIFORM_VEC3);
    
    // Set temperature thresholds - cold
    SetShaderValue(grassShader, grassLocs.tundraStartTemp, &grassSettings.tundraStartTemp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.tundraFullTemp, &grassSettings.tundraFullTemp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.snowStartTemp, &grassSettings.snowStartTemp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.snowFullTemp, &grassSettings.snowFullTemp, SHADER_UNIFORM_FLOAT);
    
    // Set temperature thresholds - hot
    SetShaderValue(grassShader, grassLocs.desertStartTemp, &grassSettings.desertStartTemp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.desertFullTemp, &grassSettings.desertFullTemp, SHADER_UNIFORM_FLOAT);
    
    // Set water shader HSV parameters from settings
    SetShaderValue(waterShader, waterLocs.waterHue, &waterSettings.hueShift, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.waterSaturation, &waterSettings.saturationMult, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.waterValue, &waterSettings.valueMult, SHADER_UNIFORM_FLOAT);
    
    // Set water depth-based alpha parameters from settings
    SetShaderValue(waterShader, waterLocs.minDepth, &waterSettings.minDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxDepth, &waterSettings.maxDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.minAlpha, &waterSettings.minAlpha, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxAlpha, &waterSettings.maxAlpha, SHADER_UNIFORM_FLOAT);

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

void resourceManager::applyVisualSettings() {
    VisualSettings& vs = VisualSettings::getInstance();
    
    // Apply lighting settings to terrain shader
    LightingSettings& light = vs.getLightingSettings();
    SetShaderValue(terrainShader, terrainLocs.sunDirection, &light.sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.sunColor, &light.sunColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.ambientStrength, &light.ambientStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.ambientColor, &light.ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, terrainLocs.shiftIntensity, &light.shiftIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.shiftDisplacement, &light.shiftDisplacement, SHADER_UNIFORM_FLOAT);
    
    // Apply terrain erosion/dithering settings
    TerrainSettings& terrain = vs.getTerrainSettings();
    SetShaderValue(terrainShader, terrainLocs.erosionThreshold, &terrain.erosionThreshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.erosionFullExpose, &terrain.erosionFullExpose, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.ditherIntensity, &terrain.ditherIntensity, SHADER_UNIFORM_FLOAT);
    // Per-biome exposed textures (U offset in atlas, normalized 0-1)
    float grassExpU = terrain.grassExposedU / 80.0f;
    float snowExpU = terrain.snowExposedU / 80.0f;
    float sandExpU = terrain.sandExposedU / 80.0f;
    float stoneExpU = terrain.stoneExposedU / 80.0f;
    SetShaderValue(terrainShader, terrainLocs.grassExposedU, &grassExpU, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.snowExposedU, &snowExpU, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.sandExposedU, &sandExpU, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, terrainLocs.stoneExposedU, &stoneExpU, SHADER_UNIFORM_FLOAT);
    TraceLog(LOG_INFO, "Terrain exposedU offsets: grass=%.3f snow=%.3f sand=%.3f stone=%.3f", grassExpU, snowExpU, sandExpU, stoneExpU);
    // Visualization mode
    SetShaderValue(terrainShader, terrainLocs.visualizationMode, &terrain.visualizationMode, SHADER_UNIFORM_INT);
    
    // Apply lighting to grass shader
    SetShaderValue(grassShader, grassLocs.sunDirection, &light.sunDirection, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.sunColor, &light.sunColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.ambientStrength, &light.ambientStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.ambientColor, &light.ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.shiftIntensity, &light.shiftIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.shiftDisplacement, &light.shiftDisplacement, SHADER_UNIFORM_FLOAT);
    
    // Apply grass settings - warm biome colors
    GrassSettings& grass = vs.getGrassSettings();
    SetShaderValue(grassShader, grassLocs.grassTipColor, &grass.tipColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.grassBaseColor, &grass.baseColor, SHADER_UNIFORM_VEC3);
    
    // Tundra colors
    SetShaderValue(grassShader, grassLocs.tundraTipColor, &grass.tundraTipColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.tundraBaseColor, &grass.tundraBaseColor, SHADER_UNIFORM_VEC3);
    
    // Snow colors
    SetShaderValue(grassShader, grassLocs.snowTipColor, &grass.snowTipColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(grassShader, grassLocs.snowBaseColor, &grass.snowBaseColor, SHADER_UNIFORM_VEC3);
    
    // Temperature thresholds
    SetShaderValue(grassShader, grassLocs.tundraStartTemp, &grass.tundraStartTemp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.tundraFullTemp, &grass.tundraFullTemp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.snowStartTemp, &grass.snowStartTemp, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.snowFullTemp, &grass.snowFullTemp, SHADER_UNIFORM_FLOAT);
    
    // Wind settings
    float windDir[2] = {grass.windDirection.x, grass.windDirection.y};
    SetShaderValue(grassShader, grassLocs.windStrength, &grass.windStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windSpeed, &grass.windSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windDirection, windDir, SHADER_UNIFORM_VEC2);
    
    // Apply water settings
    WaterSettings& water = vs.getWaterSettings();
    SetShaderValue(waterShader, waterLocs.waterHue, &water.hueShift, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.waterSaturation, &water.saturationMult, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.waterValue, &water.valueMult, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.minDepth, &water.minDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxDepth, &water.maxDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.minAlpha, &water.minAlpha, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterLocs.maxAlpha, &water.maxAlpha, SHADER_UNIFORM_FLOAT);
    
    vs.clearDirty();
}

void resourceManager::updateGrassWindSettings() {
    GrassSettings& grass = VisualSettings::getInstance().getGrassSettings();
    float windDir[2] = {grass.windDirection.x, grass.windDirection.y};
    SetShaderValue(grassShader, grassLocs.windStrength, &grass.windStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windSpeed, &grass.windSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(grassShader, grassLocs.windDirection, windDir, SHADER_UNIFORM_VEC2);
}
