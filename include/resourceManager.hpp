#pragma once

#include <raylib.h>
#include <sys/stat.h>
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
    static Shader& getShader();
    static Texture2D& getItemTexture(itemType type);
    static Rectangle getItemTextureUV(itemType type);
    // Lighting update
    static void updateMachineLighting(Vector3 sunDirection, Color sunColor, 
                                    float ambientStrength, Color ambientColor, 
                                    float shiftIntensity, float shiftDisplacement);

    static Camera camera;

private:
    // Prevent instantiation
    resourceManager() = delete;

    static Texture2D itemTexture;
    static bool initialized;
    static std::unordered_map<itemType, itemTextureUVs> itemTextureUVsMap;
    static std::unordered_map<machineType, Model> machineModels;
    static std::unordered_map<machineType, Texture2D> machineTextures;
    static std::unordered_map<machineType, std::string> modelPaths;
    static std::unordered_map<machineType, std::string> texturePaths;
    static Shader terrainShader;
};
