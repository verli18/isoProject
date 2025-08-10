#include "raylib.h"
#include "chunkManager.hpp"
#include "resourceManager.hpp"

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
    void init();
    void update();
    void render();
    
    chunkManager world;
    machineManager machineManagement;
    RenderTexture2D renderCanvas; //I will always use this naming convention, even though I haven't used lua in years by now lol
    private:

};
    