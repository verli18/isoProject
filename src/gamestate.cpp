#include "../include/gamestate.hpp"
#include "rlgl.h"
#include <iostream>
#include <raylib.h>
#include "../libs/rlImGui/imgui/imgui.h"
#include "../libs/rlImGui/rlImGui.h"
#include "../include/worldGenerator.hpp"
#include "../include/biome.hpp"
#include "../include/worldMap.hpp"

Camera resourceManager::camera;
Vector3 cameraPosition = {32.0f, 32.0f, 32.0f};
Vector3 cameraTarget = {0.0f, 0.0f, 0.0f};

void gameState::init() {
    
    renderCanvas = LoadRenderTexture(GAMEWIDTH, GAMEHEIGHT);
    sunData = sun();    

    //DisableCursor();
    SetTargetFPS(60);

    // Initialize world generator first (before any chunk generation)
    WorldGenerator::getInstance().initialize(1337);
    BiomeManager::getInstance().initialize();

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
    
    // Handle terrain regeneration request
    if (shouldRegenerateTerrain) {
        WorldMap::getInstance().clear();
        world.clearAllChunks();
        shouldRegenerateTerrain = false;
    }
    
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
                // Update grass shader with lighting before rendering
                resourceManager::updateGrassUniforms(
                    static_cast<float>(GetTime()), camera.position,
                    sunData.sunDirection, sunData.sunColor,
                    sunData.ambientStrength, sunData.ambientColor,
                    sunData.shiftIntensity, sunData.shiftDisplacement
                );
                world.renderGrass(static_cast<float>(GetTime()), camera);
             break;
            case 2:
                switch (debugOpt) {
                    case 0: world.renderDataPoint({206,220,176,255}, {21,106,125,255}, &tile::moisture); break;
                    case 1: world.renderDataPoint({20,57,109,255}, {201,66,46,255}, &tile::temperature); break;
                    case 2: world.renderDataPoint({79,5,37,255}, {198,93,15,255}, &tile::magmaticPotential); break;
                    case 3: world.renderDataPoint({79,5,37,255}, {209,204,103,255}, &tile::sulfidePotential); break;
                    case 4: world.renderDataPoint({206,220,176,255}, {27,86,122,255}, &tile::hydrologicalPotential); break;
                    case 5: world.renderDataPoint({3,39,43,255}, {122,157,55,255}, &tile::biologicalPotential); break;
                    case 6: world.renderDataPoint({57,12,105,255}, {190,117,174,255}, &tile::crystalinePotential); break;
                }
             break;
        }
        EndMode3D();
        EndTextureMode();
        DrawTexturePro(renderCanvas.texture, Rectangle{0, 0, GAMEWIDTH, -GAMEHEIGHT}, Rectangle{0, 0, GAMEWIDTH * GAMESCALE, GAMEHEIGHT * GAMESCALE}, Vector2{0, 0}, 0, WHITE);
        
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
        ImGui::Combo("mode", &debugOpt, "moisture\0temperature\0magmatic potential\0sulfide potential\0hydrological potential\0biological potential\0crystaline potential\0");
        ImGui::Separator();
        ImGui::Checkbox("World Gen Debug", &showWorldGenDebug);
        ImGui::Separator();
        ImGui::Text("Grass blades: %zu", world.getTotalGrassBlades());
        ImGui::End();
        
        // World Gen Debug Window
        if (showWorldGenDebug) {
            renderWorldGenDebugUI();
        }

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

        
        
        
        DrawFPS(0, 0);
    EndDrawing();
}

void gameState::renderWorldGenDebugUI() {
    ImGui::Begin("World Generation", &showWorldGenDebug);
    
    WorldGenerator& worldGen = WorldGenerator::getInstance();
    WorldGenConfig& config = worldGen.getConfig();
    
    // Current position info
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    
    // Sample potentials at camera position
    PotentialData potential = worldGen.getPotentialAt(cameraPosition.x, cameraPosition.z);
    BiomeType biome = BiomeManager::getInstance().getBiomeAt(potential);
    const BiomeData& biomeData = BiomeManager::getInstance().getBiomeData(biome);
    
    ImGui::Separator();
    ImGui::Text("Current Biome: %s", biomeData.name);
    
    // Potentials display
    ImGui::Separator();
    ImGui::Text("Potentials at Camera:");
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.1f, 1.0f));
    ImGui::ProgressBar(potential.magmatic, ImVec2(-1, 0), "Magmatic");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.1f, 0.4f, 0.8f, 1.0f));
    ImGui::ProgressBar(potential.hydrological, ImVec2(-1, 0), "Hydrological");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.7f, 0.1f, 1.0f));
    ImGui::ProgressBar(potential.sulfide, ImVec2(-1, 0), "Sulfide");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
    ImGui::ProgressBar(potential.crystalline, ImVec2(-1, 0), "Crystalline");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::ProgressBar(potential.biological, ImVec2(-1, 0), "Biological");
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    ImGui::Text("Climate:");
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.4f, 0.1f, 1.0f));
    ImGui::ProgressBar(potential.temperature, ImVec2(-1, 0), "Temperature");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
    ImGui::ProgressBar(potential.humidity, ImVec2(-1, 0), "Humidity");
    ImGui::PopStyleColor();
    
    // Config tweaking
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Generation Config")) {
        bool configChanged = false;
        
        configChanged |= ImGui::SliderInt("Seed", &config.seed, 0, 9999);
        configChanged |= ImGui::SliderFloat("Height Scale", &config.heightScale, 10.0f, 200.0f);
        configChanged |= ImGui::SliderFloat("Height Exponent", &config.heightExponent, 0.5f, 3.0f);
        configChanged |= ImGui::SliderFloat("Terrain Freq", &config.terrainFreq, 0.005f, 0.1f, "%.4f");
        configChanged |= ImGui::SliderFloat("Potential Freq", &config.potentialFreq, 0.001f, 0.05f, "%.4f");
        configChanged |= ImGui::SliderFloat("Climate Freq", &config.climateFreq, 0.001f, 0.02f, "%.4f");
        
        ImGui::Separator();
        ImGui::Text("Thresholds:");
        configChanged |= ImGui::SliderFloat("Geo Override", &config.geologicalOverrideThreshold, 0.5f, 0.95f);
        configChanged |= ImGui::SliderFloat("Sea Level", &config.seaLevel, 0.0f, 50.0f);
        
        ImGui::Separator();
        ImGui::Text("Feedback Loop:");
        configChanged |= ImGui::SliderFloat("Slope->Sulfide", &config.slopeToSulfide, 0.0f, 1.0f);
        configChanged |= ImGui::SliderFloat("Slope->Crystal", &config.slopeToCrystalline, 0.0f, 1.0f);
        configChanged |= ImGui::SliderFloat("Flow->Bio", &config.flowToBiological, 0.0f, 1.0f);
        configChanged |= ImGui::SliderFloat("Flow->Hydro", &config.flowToHydrological, 0.0f, 1.0f);
        
        if (configChanged) {
            // Rebuild noise generators with new config
            worldGen.rebuildNoiseGenerators();
        }
    }
    
    // WorldMap / Erosion Config
    if (ImGui::CollapsingHeader("Erosion & Water")) {
        WorldMap& worldMap = WorldMap::getInstance();
        ErosionConfig& erosion = worldMap.getErosionConfig();
        
        ImGui::Text("Erosion Simulation:");
        ImGui::SliderInt("Droplets", &erosion.numDroplets, 0, 20000);
        ImGui::SliderInt("Max Lifetime", &erosion.maxDropletLifetime, 10, 200);
        ImGui::SliderFloat("Inertia", &erosion.inertia, 0.0f, 1.0f);
        ImGui::SliderFloat("Sediment Capacity", &erosion.sedimentCapacity, 0.1f, 10.0f);
        ImGui::SliderFloat("Erode Speed", &erosion.erodeSpeed, 0.01f, 0.5f);
        ImGui::SliderFloat("Deposit Speed", &erosion.depositSpeed, 0.01f, 0.5f);
        ImGui::SliderFloat("Evaporate Speed", &erosion.evaporateSpeed, 0.001f, 0.1f);
        ImGui::SliderFloat("Gravity", &erosion.gravity, 0.5f, 10.0f);
        ImGui::SliderFloat("Max Erode/Step", &erosion.maxErodePerStep, 0.01f, 0.2f);
        ImGui::SliderInt("Erosion Radius", &erosion.erosionRadius, 1, 5);
        
        ImGui::Separator();
        ImGui::Text("Lakes:");
        ImGui::SliderFloat("Min Water Depth", &erosion.waterMinDepth, 0.1f, 2.0f);
        ImGui::SliderInt("Lake Dilation", &erosion.lakeDilation, 0, 5);
        
        ImGui::Separator();
        ImGui::Text("Rivers:");
        ImGui::SliderInt("Flow Threshold", &erosion.riverFlowThreshold, 10, 500);
        ImGui::SliderFloat("Width Scale", &erosion.riverWidthScale, 0.001f, 0.1f, "%.3f");
        ImGui::SliderInt("Max River Width", &erosion.maxRiverWidth, 1, 10);
        ImGui::SliderFloat("River Depth", &erosion.riverDepth, 0.1f, 1.0f);
        
        ImGui::Separator();
        if (ImGui::Button("Regenerate Terrain")) {
            // Set flag to regenerate on next update (after ImGui frame ends)
            shouldRegenerateTerrain = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Clears all chunks)");
    }
    
    // Biome list
    if (ImGui::CollapsingHeader("Biome List")) {
        for (int i = 0; i < static_cast<int>(BiomeType::COUNT); ++i) {
            const BiomeData& b = BiomeManager::getInstance().getBiomeData(static_cast<BiomeType>(i));
            if (b.name) {
                bool isCurrent = (static_cast<BiomeType>(i) == biome);
                if (isCurrent) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                }
                ImGui::BulletText("%s (H:%.1f, R:%.1f)", b.name, b.heightMultiplier, b.roughness);
                if (isCurrent) {
                    ImGui::PopStyleColor();
                }
            }
        }
    }
    
    ImGui::End();
}
