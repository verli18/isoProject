#include "raylib.h"
#include "chunkManager.hpp"
#include "resourceManager.hpp"

#define GAMEWIDTH 320
#define GAMEHEIGHT 240
#define GAMESCALE 3

class gameState{
    public:
    sun sunData;
    Camera3D camera = { {32.0f, 32.0f, 32.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 20.0f, CAMERA_PERSPECTIVE};
    bool showGrid = false; // default to surface rendering
    bool showDebug = false;
    gameState() : world(2) {
        init();
    }

    bool buildMode = false;
    void init();
    void update();
    void render();
    
    chunkManager world;
    machineManager machineManagement;
    RenderTexture2D renderCanvas; //I will always use this naming convention, even though I haven't used lua in years by now lol
    private:

};
    