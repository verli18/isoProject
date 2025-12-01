#pragma once

#include "raylib.h"
#include "chunkManager.hpp"
#include "resourceManager.hpp"
#include "machineManager.hpp"
#include "worldGenerator.hpp"
#include "biome.hpp"
#include "visualSettings.hpp"

#define GAMEWIDTH 480
#define GAMEHEIGHT 270
#define GAMESCALE 3

class gameState{
    public:
    sun sunData;
    Camera3D camera = { {32.0f, 32.0f, 32.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 20.0f, CAMERA_PERSPECTIVE};
    int renderMode = 1;
    int debugOpt = 0;
    gameState() : world(5) {
        init();
    }

    bool buildMode = false;
    bool showVisualSettings = false; // Toggle for unified settings panel
    bool shouldRegenerateTerrain = false;  // Flag to trigger regeneration
    void init();
    void update();
    void render();
    
    // Unified settings UI (combines visual + world gen + erosion)
    void renderSettingsUI();
    
    chunkManager world;
    machineManager machineManagement;
    RenderTexture2D renderCanvas;

    private:
        // Machine placement variables
        machineType placementType = DRILLMK1;
        direction placementDirection = NORTH;

        // Machine inspection variable
        machine* inspectedMachine = nullptr;
};
