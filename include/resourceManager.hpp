#pragma once

#include <raylib.h>
#include <sys/stat.h>
#include <unordered_map>
#include <string>
#include "machines.hpp"

class sun{
    public:
    
    Vector3 sunDirection;
    Vector3 sunColor;
    float ambientStrength;
    Vector3 ambientColor;
    float shiftIntensity;
    float shiftDisplacement;

    sun(){
        sunDirection = {0.59f, -1.0f, -0.8f};  // Direction from sun to ground
        sunColor = {1.0f, 0.9f, 0.7f};       // Warm sun color
        ambientStrength = 0.5f;                 // Ambient light strength
        ambientColor = {0.4f, 0.5f, 0.8f};   // Cool ambient color
        shiftIntensity = -0.05f;
        shiftDisplacement = 1.86f;
    }
};

// Cached shader uniform locations to avoid querying every frame
struct TerrainShaderLocs {
    int sunDirection;
    int sunColor;
    int ambientStrength;
    int ambientColor;
    int shiftIntensity;
    int shiftDisplacement;
};

struct WaterShaderLocs {
    int waterHue;
    int waterSaturation;
    int waterValue;
    int minDepth;
    int maxDepth;
    int minAlpha;
    int maxAlpha;
    int time;
};

struct GrassShaderLocs {
    int mvp;
    int viewPos;
    int time;
    int windStrength;
    int windDirection;
    int windSpeed;
    int sunDirection;
    int sunColor;
    int ambientStrength;
    int ambientColor;
    int shiftIntensity;
    int shiftDisplacement;
};

class resourceManager {
public:
    // Initialize and load all machine assets
    static void initialize();
    // Unload all assets
    static void cleanup();

    // Accessors
    static Model& getMachineModel(machineType type);
    static Texture2D& getMachineTexture(machineType type);
    static Shader& getShader(int n);
    static Texture2D& getItemTexture(itemType type);
    static Rectangle getItemTextureUV(itemType type);
    // Lighting update
    static void updateMachineLighting(Vector3 sunDirection, Color sunColor, 
                                    float ambientStrength, Color ambientColor, 
                                    float shiftIntensity, float shiftDisplacement);

    static Camera camera;
    // Update terrain shader uniforms
    static void updateTerrainLighting(Vector3 sunDirection, Vector3 sunColor,
                                     float ambientStrength, Vector3 ambientColor,
                                     float shiftIntensity, float shiftDisplacement);
    
    // Update water depth-based alpha parameters
    static void updateWaterDepthParams(float minDepth, float maxDepth, float minAlpha, float maxAlpha);
    
    // Update water time for displacement animation
    static void updateWaterTime(float time);
    
    // Update grass shader uniforms
    static void updateGrassUniforms(float time, Vector3 cameraPos,
                                    Vector3 sunDirection, Vector3 sunColor,
                                    float ambientStrength, Vector3 ambientColor,
                                    float shiftIntensity, float shiftDisplacement);
    
    // Get grass shader and material
    static Shader& getGrassShader();
    static Material& getGrassMaterial();
    static GrassShaderLocs& getGrassShaderLocs();

    static Texture2D terrainTexture;
    static Texture2D waterTexture;
    static Texture2D waterDisplacementTexture;
private:
    // Prevent instantiation
    resourceManager() = delete;

    // Cache shader locations on init
    static void cacheShaderLocations();

    static Texture2D itemTexture;
    static bool initialized;
    static std::unordered_map<itemType, itemTextureUVs> itemTextureUVsMap;
    static std::unordered_map<machineType, Model> machineModels;
    static std::unordered_map<machineType, Texture2D> machineTextures;
    static std::unordered_map<machineType, std::string> modelPaths;
    static std::unordered_map<machineType, std::string> texturePaths;
    static Shader terrainShader;
    static Shader waterShader;
    static Shader grassShader;
    static Material grassMaterial;
    
    // Cached uniform locations
    static TerrainShaderLocs terrainLocs;
    static WaterShaderLocs waterLocs;
    static GrassShaderLocs grassLocs;
};
