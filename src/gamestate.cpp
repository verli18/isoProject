#include "../include/gamestate.hpp"
#include "rlgl.h"
#include <iostream>
#include <raylib.h>
#include "../libs/rlImGui/imgui/imgui.h"
#include "../libs/rlImGui/rlImGui.h"

Camera resourceManager::camera;
Vector3 cameraPosition = {32.0f, 32.0f, 32.0f};
Vector3 cameraTarget = {0.0f, 0.0f, 0.0f};
void gameState::init() {
    
    renderCanvas = LoadRenderTexture(GAMEWIDTH, GAMEHEIGHT);
    sunData = sun();    

    //DisableCursor();
    SetTargetFPS(60);

    // Initialize resources (shader, textures) before chunk generation
    resourceManager::initialize();
    rlImGuiSetup(true);

    // Load initial chunks and setup center chunk
    world.update(camera);
    Chunk* center = world.getChunk(0, 0);
    center->tiles.updateLighting(
        sunData.sunDirection, sunData.sunColor,
        sunData.ambientStrength, sunData.ambientColor,
        sunData.shiftIntensity, sunData.shiftDisplacement
    );

    // Initial dropped items on center chunk
    machineManagement.addMachine(std::make_unique<droppedItem>(
        Vector3{16, center->tiles.getTile(16, 16).tileHeight[0] + 0.5f, 16}, IRON_ORE
    ));
    machineManagement.addMachine(std::make_unique<droppedItem>(
        Vector3{16, center->tiles.getTile(16, 18).tileHeight[0] + 0.5f, 18}, COPPER_ORE
    ));
    machineManagement.addMachine(std::make_unique<droppedItem>(
        Vector3{18, center->tiles.getTile(18, 16).tileHeight[0] + 0.5f, 16}, IRON_ORE
    ));
    
}

void gameState::update() {
    //UpdateCamera(&camera, CAMERA_ORTHOGRAPHIC);
    resourceManager::camera = camera;
    // Load or unload chunks based on camera movement
    world.update(camera);
    // Update terrain shader with current lighting
    resourceManager::updateTerrainLighting(
        sunData.sunDirection, sunData.sunColor,
        sunData.ambientStrength, sunData.ambientColor,
        sunData.shiftIntensity, sunData.shiftDisplacement
    );

    machineManagement.update();
        
    if(IsKeyDown(KEY_W)) { cameraPosition.z -= 1 * GetFrameTime() * 10; cameraTarget.z -= 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_S)) { cameraPosition.z += 1 * GetFrameTime() * 10; cameraTarget.z += 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_A)) { cameraPosition.x -= 1 * GetFrameTime() * 10; cameraTarget.x -= 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_D)) { cameraPosition.x += 1 * GetFrameTime() * 10; cameraTarget.x += 1 * GetFrameTime() * 10; }

    camera.target = cameraTarget;
    camera.position = cameraPosition;
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        // Ray-pick across loaded chunks
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
        Vector3 hitVoxel = world.getChunk(0, 0)->tiles.getTileIndexDDA(mouseRay);
        if (hitVoxel.x != -1 && buildMode) {
            std::cout << "Hit voxel: " << hitVoxel.x << " " << hitVoxel.y << " " << hitVoxel.z << std::endl;
            machineManagement.addMachine(std::make_unique<conveyorMk1>(Vector3{hitVoxel.x, world.getChunk(1, 0)->tiles.getTile(hitVoxel.x, hitVoxel.y).tileHeight[0], hitVoxel.y}));
            world.getChunk(0, 0)->tiles.placeMachine(hitVoxel.x, hitVoxel.y, machineManagement.previous);
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
        if (IsKeyPressed(KEY_E)) showDebug = !showDebug;
        if (!showGrid) world.render();
        else world.renderWires();
        if (showDebug) world.renderDataPoint();

        EndMode3D();
        rlImGuiBegin();
    
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(GAMEWIDTH, GAMEHEIGHT));
        ImGui::Begin("main UI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        if(ImGui::Button("build")) buildMode = !buildMode;

        ImGui::End(); 
        ImGui::Begin("debug", NULL);
        ImGui::End();
        rlImGuiEnd();
        EndTextureMode();
        
        
        
        DrawTexturePro(renderCanvas.texture, Rectangle{0, 0, 320, -240}, Rectangle{0, 0, 320 * GAMESCALE, 240 * GAMESCALE}, Vector2{0, 0}, 0, WHITE);
        
        DrawText("G - Toggle wireframe", 10, 10, 20, WHITE);
        DrawText("E - Toggle debug", 10, 30, 20, WHITE);
        DrawFPS(10, 40);
    EndDrawing();
}
