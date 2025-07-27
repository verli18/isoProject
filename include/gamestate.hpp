#include "raylib.h"
#include "../include/chunk.hpp"
#include "resourceManager.hpp"

#define GAMEWIDTH 320
#define GAMEHEIGHT 240
#define GAMESCALE 4
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
        shiftIntensity = -0.16f;
        shiftDisplacement = 1.56f;
    }
};


class gameState{
    public:
    sun sunData;
    Camera3D camera = { {32.0f, 32.0f, 32.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 20.0f, CAMERA_PERSPECTIVE};
    bool showGrid = true;
    gameState() : chunk(0, 0){
        init();
    }

    bool buildMode = false;
    void init();
    void update();
    void render();
    
    Chunk chunk;
    machineManager machineManagement;
    RenderTexture2D renderCanvas; //I will always use this naming convention, even though I haven't used lua in years by now lol
    private:

};
    