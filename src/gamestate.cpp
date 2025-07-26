#include "../include/gamestate.hpp"
#include "rlgl.h"
#include <iostream>


void gameState::init() {
    
    renderCanvas = LoadRenderTexture(GAMEWIDTH, GAMEHEIGHT);
    sunData = sun();    

    SetTargetFPS(60);

    //the direct consequences of putting multiple class definitions on the same file.. when am I gonna learn
    chunk.tiles.updateLighting(sunData.sunDirection, sunData.sunColor, sunData.ambientStrength, sunData.ambientColor, sunData.shiftIntensity, sunData.shiftDisplacement);

    resourceManager::initialize();
}

void gameState::update() {
    UpdateCamera(&camera, CAMERA_PERSPECTIVE);
    machineManagement.update();
        
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
        Vector3 hitVoxel = chunk.tiles.getTileIndexDDA(mouseRay);
        std::cout << "Hit voxel: " << hitVoxel.x << " " << hitVoxel.y << " " << hitVoxel.z << std::endl;
        if (hitVoxel.x != -1) machineManagement.addMachine(std::make_unique<drillMk1>(Vector3{hitVoxel.x, chunk.tiles.getTile(hitVoxel.x, hitVoxel.y).tileHeight[0], hitVoxel.y}));

    }

}

void gameState::render() {

    BeginDrawing();
        BeginTextureMode(renderCanvas);
        ClearBackground(BLACK);
        BeginMode3D(camera);
        rlDisableBackfaceCulling();
        machineManagement.render();
        if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
        if(!showGrid) chunk.render();
        if (showGrid) chunk.renderWires();

        EndMode3D();
    EndTextureMode();



    DrawTexturePro(renderCanvas.texture, Rectangle{0, 0, 320, -240}, Rectangle{0, 0, 320 * 4, 240 * 4}, Vector2{0, 0}, 0, WHITE);
    
    // Draw UI
    DrawText("G - Toggle wireframe", 10, 10, 20, WHITE);
    EndDrawing();
}
