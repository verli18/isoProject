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

    // Link managers
    machineManagement.world = &world;

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
    //UpdateCamera(&camera, CAMERA_FREE);
    resourceManager::camera = camera;
    // Load or unload chunks based on camera movement
    world.update(camera);
    // Update terrain shader with current lighting
    resourceManager::updateTerrainLighting(
        sunData.sunDirection, sunData.sunColor,
        sunData.ambientStrength, sunData.ambientColor,
        sunData.shiftIntensity, sunData.shiftDisplacement
    );

    if (IsKeyPressed(KEY_R)) {
        placementDirection = static_cast<direction>((placementDirection + 1) % 4);
    }

    // Machine Inspection Logic
    if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
        Vector3 hitVoxel = world.getChunk(0, 0)->tiles.getTileIndexDDA(mouseRay);
        if (hitVoxel.x != -1) {
            inspectedMachine = machineManagement.getMachineAt({(int)hitVoxel.x, (int)hitVoxel.y});
        } else {
            inspectedMachine = nullptr;
        }
    }

    // Machine Deletion Logic
    if (IsKeyPressed(KEY_X)) {
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
        Vector3 hitVoxel = world.getChunk(0, 0)->tiles.getTileIndexDDA(mouseRay);
        if (hitVoxel.x != -1) {
            machineManagement.removeMachineAt({(int)hitVoxel.x, (int)hitVoxel.y});
        }
    }

    machineManagement.update();
        
    if(IsKeyDown(KEY_W)) { cameraPosition.z -= 1 * GetFrameTime() * 10; cameraTarget.z -= 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_S)) { cameraPosition.z += 1 * GetFrameTime() * 10; cameraTarget.z += 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_A)) { cameraPosition.x -= 1 * GetFrameTime() * 10; cameraTarget.x -= 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_D)) { cameraPosition.x += 1 * GetFrameTime() * 10; cameraTarget.x += 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_SPACE)) { cameraPosition.y += 1 * GetFrameTime() * 10; cameraTarget.y += 1 * GetFrameTime() * 10; }
    if(IsKeyDown(KEY_LEFT_CONTROL)) { cameraPosition.y -= 1 * GetFrameTime() * 10; cameraTarget.y -= 1 * GetFrameTime() * 10; }
    
    camera.target = cameraTarget;
    camera.position = cameraPosition;
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (!ImGui::GetIO().WantCaptureMouse) {
            std::cout << "Left-click registered in-game." << std::endl;
            // Ray-pick across loaded chunks
            Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
            Vector3 hitVoxel = world.getChunk(0, 0)->tiles.getTileIndexDDA(mouseRay);
            if (hitVoxel.x != -1 && buildMode) {
                std::cout << "Build conditions met. Placing machine at: " << hitVoxel.x << ", " << hitVoxel.y << std::endl;
                std::unique_ptr<machine> newMachine;
                if (placementType == DRILLMK1) {
                    newMachine = std::make_unique<drillMk1>(Vector3{hitVoxel.x, world.getChunk(0, 0)->tiles.getTile(hitVoxel.x, hitVoxel.y).tileHeight[0], hitVoxel.y});
                } else {
                    newMachine = std::make_unique<conveyorMk1>(Vector3{hitVoxel.x, world.getChunk(0, 0)->tiles.getTile(hitVoxel.x, hitVoxel.y).tileHeight[0], hitVoxel.y});
                }

                newMachine->dir = placementDirection;
                // For now, assuming placement is on center chunk, so local coords are global coords.
                newMachine->globalPos = {(int)hitVoxel.x, (int)hitVoxel.y};

                if(world.getChunk(0, 0)->tiles.placeMachine(hitVoxel.x, hitVoxel.y, newMachine.get())){
                    machineManagement.addMachine(std::move(newMachine));
                }
            }
        }
    }

}

void gameState::render() {


    BeginDrawing();
        BeginTextureMode(renderCanvas);
        ClearBackground(BLACK);
        
        // Update water shader time for displacement animation
        resourceManager::updateWaterTime(GetTime());
        
        BeginMode3D(camera);
        rlDisableBackfaceCulling();
        switch(renderMode) {
            case 0: 
                rlEnableWireMode();
                machineManagement.render();
                world.render();
                rlDisableWireMode();
             break;
            case 1:
                machineManagement.render();
                world.render();
             break;
            case 2:
                switch (debugOpt) {
                    case 0: world.renderDataPoint({206,220,176,255}, {21,106,125,255}, &tile::moisture); break;
                    case 1: world.renderDataPoint({20,57,109,255}, {201,66,46,255}, &tile::temperature); break;
                    case 2: world.renderDataPoint({79,5,37,255}, {198,93,15,255}, &tile::magmaticPotential); break;
                    case 3: world.renderDataPoint({79,5,37,255}, {209,204,103,255}, &tile::sulfidePotential); break;
                    case 4: world.renderDataPoint({206,220,176,255}, {27,86,122,255}, &tile::hydrologicalPotential); break;
                    case 5: world.renderDataPoint({3,39,43,255}, {122,157,55,255}, &tile::biologicalPotential); break;
                }
             break;
        }
        EndMode3D();
        rlImGuiBegin();
    
        /*
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(GAMEWIDTH, GAMEHEIGHT));
        ImGui::Begin("main UI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        if(ImGui::Button("build")) buildMode = !buildMode;
        ImGui::End();
        */
        
        ImGui::Begin("debug", NULL);
        ImGui::RadioButton("wireframe", &renderMode, 0);
        ImGui::RadioButton("mesh", &renderMode, 1);
        ImGui::RadioButton("debug", &renderMode, 2);
        ImGui::Combo("mode", &debugOpt, "moisture\0temperature\0magmatic potential\0sulfide potential\0hydrological potential\0biological potential\0");

        ImGui::End();

        ImGui::Begin("Build");
        const char* direction_names[] = {"NORTH", "EAST", "SOUTH", "WEST"};
        ImGui::Text("Rotation: %s", direction_names[placementDirection]);
        ImGui::Separator();
        ImGui::RadioButton("Drill", (int*)&placementType, DRILLMK1);
        ImGui::SameLine();
        ImGui::RadioButton("Conveyor", (int*)&placementType, CONVEYORMK1);
        ImGui::Checkbox("Build Mode", &buildMode);
        ImGui::End();

        if (inspectedMachine) {
            ImGui::Begin("Machine Inventory");
            Inventory* inv = inspectedMachine->getInventory();
            if (inv) {
                const char* item_names[] = {"IRON_ORE", "COPPER_ORE"};
                const auto& slots = inv->getSlots();
                for (size_t i = 0; i < slots.size(); ++i) {
                    const auto& slot = slots[i];
                    if (slot.currentItem.quantity > 0) {
                        ImGui::Text("Slot %zu: %d x %s", i, slot.currentItem.quantity, item_names[slot.currentItem.type]);
                    } else {
                        ImGui::Text("Slot %zu: Empty", i);
                    }
                }
            } else {
                ImGui::Text("This machine has no inventory.");
            }
            ImGui::End();
        }
        rlImGuiEnd();
        EndTextureMode();
        
        
        DrawTexturePro(renderCanvas.texture, Rectangle{0, 0, GAMEWIDTH, -GAMEHEIGHT}, Rectangle{0, 0, GAMEWIDTH * GAMESCALE, GAMEHEIGHT * GAMESCALE}, Vector2{0, 0}, 0, WHITE);
        
        DrawFPS(0, 0);
    EndDrawing();
}
