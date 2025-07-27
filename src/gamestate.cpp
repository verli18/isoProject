#include "../include/gamestate.hpp"
#include "rlgl.h"
#include <iostream>
#include "../libs/rlImGui/imgui/imgui.h"
#include "../libs/rlImGui/rlImGui.h"

Camera resourceManager::camera;

void gameState::init() {
    
    renderCanvas = LoadRenderTexture(GAMEWIDTH, GAMEHEIGHT);
    sunData = sun();    

    SetTargetFPS(60);

    //the direct consequences of putting multiple class definitions on the same file.. when am I gonna learn
    chunk.tiles.updateLighting(sunData.sunDirection, sunData.sunColor, sunData.ambientStrength, sunData.ambientColor, sunData.shiftIntensity, sunData.shiftDisplacement);

    resourceManager::initialize();
    rlImGuiSetup(true);
    machineManagement.addMachine(std::unique_ptr<droppedItem>(new droppedItem(Vector3{16, chunk.tiles.getTile(16, 16).tileHeight[0]+0.5f, 16}, IRON_ORE)));
    machineManagement.addMachine(std::unique_ptr<droppedItem>(new droppedItem(Vector3{16, chunk.tiles.getTile(16, 18).tileHeight[0]+0.5f, 18}, COPPER_ORE)));
    machineManagement.addMachine(std::unique_ptr<droppedItem>(new droppedItem(Vector3{18, chunk.tiles.getTile(18, 16).tileHeight[0]+0.5f, 16}, IRON_ORE)));
    
}

void gameState::update() {
    UpdateCamera(&camera, CAMERA_FREE);
    resourceManager::camera = camera;

    machineManagement.update();
        
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
        Vector3 hitVoxel = chunk.tiles.getTileIndexDDA(mouseRay);
        if (hitVoxel.x != -1 && buildMode) {
            std::cout << "Hit voxel: " << hitVoxel.x << " " << hitVoxel.y << " " << hitVoxel.z << std::endl;
            machineManagement.addMachine(std::make_unique<drillMk1>(Vector3{hitVoxel.x, chunk.tiles.getTile(hitVoxel.x, hitVoxel.y).tileHeight[0], hitVoxel.y}));
            chunk.tiles.placeMachine(hitVoxel.x, hitVoxel.y, machineManagement.previous);
        }
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
        rlImGuiBegin();
        
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(GAMEWIDTH, GAMEHEIGHT));
        ImGui::Begin("main UI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        if(ImGui::Button("build")) buildMode = !buildMode;
        ImGui::End();
        rlImGuiEnd();
    EndTextureMode();



    DrawTexturePro(renderCanvas.texture, Rectangle{0, 0, 320, -240}, Rectangle{0, 0, 320 * 4, 240 * 4}, Vector2{0, 0}, 0, WHITE);
    
    // Draw UI

    DrawText("G - Toggle wireframe", 10, 10, 20, WHITE);
    EndDrawing();
}
