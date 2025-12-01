#include "../include/gamestate.hpp"
#include "rlgl.h"
#include <iostream>
#include <cstdio>
#include <raylib.h>
#include "../libs/rlImGui/imgui/imgui.h"
#include "../libs/rlImGui/rlImGui.h"
#include "../include/worldGenerator.hpp"
#include "../include/biome.hpp"
#include "../include/worldMap.hpp"
#include "../include/visualSettings.hpp"

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
    VisualSettings::getInstance().initialize();
    
    // Try to load default settings (includes visual, world gen, and erosion config)
    VisualSettings& vs = VisualSettings::getInstance();
    WorldGenConfig& worldGenConfig = WorldGenerator::getInstance().getConfig();
    ErosionConfig& erosionConfig = WorldMap::getInstance().getErosionConfig();
    
    if (vs.loadAllSettings(VisualSettings::DEFAULT_SETTINGS_FILE, &worldGenConfig, &erosionConfig)) {
        TraceLog(LOG_INFO, "Loaded default settings from %s", VisualSettings::DEFAULT_SETTINGS_FILE);
        // Sync sun data with loaded lighting settings
        LightingSettings& light = vs.getLightingSettings();
        sunData.sunDirection = light.sunDirection;
        sunData.sunColor = light.sunColor;
        sunData.ambientStrength = light.ambientStrength;
        sunData.ambientColor = light.ambientColor;
        sunData.shiftIntensity = light.shiftIntensity;
        sunData.shiftDisplacement = light.shiftDisplacement;
        // Rebuild noise generators with loaded config
        WorldGenerator::getInstance().rebuildNoiseGenerators();
        vs.markDirty();
    }

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
    
    // Apply visual settings if changed
    if (VisualSettings::getInstance().isDirty()) {
        resourceManager::applyVisualSettings();
    }
    
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
        ImGui::Checkbox("Settings Panel", &showVisualSettings);
        ImGui::Separator();
        ImGui::Text("Grass blades: %zu", world.getTotalGrassBlades());
        ImGui::End();
        
        // Unified Settings Window (combines visual + world gen)
        if (showVisualSettings) {
            renderSettingsUI();
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

void gameState::renderSettingsUI() {
    ImGui::Begin("Settings", &showVisualSettings);
    
    VisualSettings& vs = VisualSettings::getInstance();
    WorldGenerator& worldGen = WorldGenerator::getInstance();
    WorldGenConfig& config = worldGen.getConfig();
    WorldMap& worldMap = WorldMap::getInstance();
    ErosionConfig& erosion = worldMap.getErosionConfig();
    
    bool changed = false;
    
    // ==================== SAVE/LOAD SECTION (at top for easy access) ====================
    ImGui::Text("Settings File:");
    
    if (ImGui::Button("Save")) {
        if (vs.saveAllSettings("settings.ini", &config, &erosion)) {
            TraceLog(LOG_INFO, "All settings saved to settings.ini");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (vs.loadAllSettings("settings.ini", &config, &erosion)) {
            TraceLog(LOG_INFO, "All settings loaded from settings.ini");
            vs.markDirty();
            LightingSettings& light = vs.getLightingSettings();
            sunData.sunDirection = light.sunDirection;
            sunData.sunColor = light.sunColor;
            sunData.ambientStrength = light.ambientStrength;
            sunData.ambientColor = light.ambientColor;
            sunData.shiftIntensity = light.shiftIntensity;
            sunData.shiftDisplacement = light.shiftDisplacement;
            worldGen.rebuildNoiseGenerators();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Set as Default")) {
        if (vs.saveAllSettings(VisualSettings::DEFAULT_SETTINGS_FILE, &config, &erosion)) {
            TraceLog(LOG_INFO, "Settings saved as default");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Default")) {
        if (std::remove(VisualSettings::DEFAULT_SETTINGS_FILE) == 0) {
            TraceLog(LOG_INFO, "Cleared default settings");
        }
    }
    
    ImGui::Separator();
    
    // ==================== WORLD INFO ====================
    if (ImGui::CollapsingHeader("World Info")) {
        ImGui::Text("Camera: (%.1f, %.1f, %.1f)", cameraPosition.x, cameraPosition.y, cameraPosition.z);
        
        PotentialData potential = worldGen.getPotentialAt(cameraPosition.x, cameraPosition.z);
        BiomeType biome = BiomeManager::getInstance().getBiomeAt(potential);
        const BiomeData& biomeData = BiomeManager::getInstance().getBiomeData(biome);
        ImGui::Text("Current Biome: %s", biomeData.name);
        
        ImGui::Separator();
        ImGui::Text("Potentials:");
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
    }
    
    // ==================== WORLD GENERATION ====================
    if (ImGui::CollapsingHeader("World Generation")) {
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
            worldGen.rebuildNoiseGenerators();
        }
    }
    
    // ==================== EROSION & WATER ====================
    if (ImGui::CollapsingHeader("Erosion & Water")) {
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
    }
    
    // ==================== LIGHTING ====================
    if (ImGui::CollapsingHeader("Lighting")) {
        LightingSettings& light = vs.getLightingSettings();
        
        changed |= ImGui::SliderFloat3("Sun Direction", &light.sunDirection.x, -1.0f, 1.0f);
        changed |= ImGui::ColorEdit3("Sun Color", &light.sunColor.x);
        changed |= ImGui::SliderFloat("Ambient Strength", &light.ambientStrength, 0.0f, 1.0f);
        changed |= ImGui::ColorEdit3("Ambient Color", &light.ambientColor.x);
        
        ImGui::Separator();
        ImGui::Text("HSV Shift:");
        changed |= ImGui::SliderFloat("Shift Intensity", &light.shiftIntensity, -0.2f, 0.2f);
        changed |= ImGui::SliderFloat("Shift Displacement", &light.shiftDisplacement, 0.0f, 3.0f);
        
        if (changed) {
            sunData.sunDirection = light.sunDirection;
            sunData.sunColor = light.sunColor;
            sunData.ambientStrength = light.ambientStrength;
            sunData.ambientColor = light.ambientColor;
            sunData.shiftIntensity = light.shiftIntensity;
            sunData.shiftDisplacement = light.shiftDisplacement;
        }
    }
    
    // ==================== GRASS ====================
    if (ImGui::CollapsingHeader("Grass")) {
        GrassSettings& grass = vs.getGrassSettings();
        
        ImGui::Text("Warm Biome Colors:");
        changed |= ImGui::ColorEdit3("Tip Color", &grass.tipColor.x);
        changed |= ImGui::ColorEdit3("Base Color", &grass.baseColor.x);
        
        ImGui::Separator();
        ImGui::Text("Tundra Biome Colors:");
        changed |= ImGui::ColorEdit3("Tundra Tip", &grass.tundraTipColor.x);
        changed |= ImGui::ColorEdit3("Tundra Base", &grass.tundraBaseColor.x);
        
        ImGui::Separator();
        ImGui::Text("Snow Biome Colors:");
        changed |= ImGui::ColorEdit3("Snow Tip", &grass.snowTipColor.x);
        changed |= ImGui::ColorEdit3("Snow Base", &grass.snowBaseColor.x);
        
        ImGui::Separator();
        ImGui::Text("Temperature Thresholds:");
        ImGui::TextDisabled("Lower temp = colder. Range 0.0-1.0");
        changed |= ImGui::SliderFloat("Tundra Start##temp", &grass.tundraStartTemp, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Tundra Full##temp", &grass.tundraFullTemp, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Snow Start##temp", &grass.snowStartTemp, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Snow Full##temp", &grass.snowFullTemp, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("No Grass##temp", &grass.noGrassTemp, 0.0f, 0.2f);
        
        ImGui::Separator();
        ImGui::Text("Biome Density/Height Multipliers:");
        changed |= ImGui::SliderFloat("Tundra Height Mult", &grass.tundraHeightMult, 0.1f, 1.0f);
        changed |= ImGui::SliderFloat("Snow Height Mult", &grass.snowHeightMult, 0.1f, 1.0f);
        changed |= ImGui::SliderFloat("Tundra Density Mult", &grass.tundraDensityMult, 0.1f, 1.0f);
        changed |= ImGui::SliderFloat("Snow Density Mult", &grass.snowDensityMult, 0.05f, 1.0f);
        changed |= ImGui::SliderFloat("Min Density", &grass.minDensity, 0.0f, 0.5f);
        
        ImGui::Separator();
        ImGui::Text("Dirt Transition:");
        changed |= ImGui::ColorEdit3("Dirt Blend Color", &grass.dirtBlendColor.x);
        changed |= ImGui::SliderFloat("Blend Distance", &grass.dirtBlendDistance, 0.0f, 5.0f);
        changed |= ImGui::SliderFloat("Blend Strength", &grass.dirtBlendStrength, 0.0f, 1.0f);
        
        ImGui::Separator();
        ImGui::Text("Climate Influence:");
        changed |= ImGui::SliderFloat("Temperature##grass", &grass.temperatureInfluence, 0.0f, 0.3f);
        changed |= ImGui::SliderFloat("Moisture##grass", &grass.moistureInfluence, 0.0f, 0.3f);
        changed |= ImGui::SliderFloat("Biological##grass", &grass.biologicalInfluence, 0.0f, 0.3f);
        changed |= ImGui::SliderFloat("Moisture Mult##biomegrass", &grass.moistureMultiplier, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Slope Reduction", &grass.slopeReduction, 0.0f, 1.0f);
        
        ImGui::Separator();
        ImGui::Text("Wind:");
        changed |= ImGui::SliderFloat("Wind Strength", &grass.windStrength, 0.0f, 0.5f);
        changed |= ImGui::SliderFloat("Wind Speed", &grass.windSpeed, 0.5f, 5.0f);
        changed |= ImGui::SliderFloat2("Wind Direction", &grass.windDirection.x, -1.0f, 1.0f);
        
        ImGui::Separator();
        ImGui::Text("Geometry:");
        changed |= ImGui::SliderFloat("Base Height", &grass.baseHeight, 0.2f, 2.0f);
        changed |= ImGui::SliderFloat("Height Variation", &grass.heightVariation, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Blades Per Tile", &grass.bladesPerTile, 10.0f, 100.0f);
        changed |= ImGui::SliderFloat("Blade Width", &grass.bladeWidth, 0.02f, 0.2f);
        
        ImGui::Separator();
        ImGui::Text("Render Distance:");
        changed |= ImGui::SliderFloat("Render Distance##grass", &grass.renderDistance, 16.0f, 128.0f);
        changed |= ImGui::SliderFloat("Fade Start##grass", &grass.fadeStartDistance, 8.0f, 120.0f);
    }
    
    // ==================== WATER ====================
    if (ImGui::CollapsingHeader("Water Visuals")) {
        WaterSettings& water = vs.getWaterSettings();
        
        ImGui::Text("Color:");
        changed |= ImGui::SliderFloat("Hue Shift", &water.hueShift, -0.5f, 0.5f);
        changed |= ImGui::SliderFloat("Saturation##water", &water.saturationMult, 0.5f, 8.0f);
        changed |= ImGui::SliderFloat("Value##water", &water.valueMult, 0.1f, 2.0f);
        changed |= ImGui::ColorEdit3("Shallow Color", &water.shallowColor.x);
        changed |= ImGui::ColorEdit3("Deep Color", &water.deepColor.x);
        
        ImGui::Separator();
        ImGui::Text("Depth Alpha:");
        changed |= ImGui::SliderFloat("Min Depth##water", &water.minDepth, 0.0f, 2.0f);
        changed |= ImGui::SliderFloat("Max Depth##water", &water.maxDepth, 1.0f, 10.0f);
        changed |= ImGui::SliderFloat("Min Alpha", &water.minAlpha, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Max Alpha", &water.maxAlpha, 0.0f, 1.0f);
        
        ImGui::Separator();
        ImGui::Text("Animation:");
        changed |= ImGui::SliderFloat("Flow Speed", &water.flowSpeed, 0.0f, 0.5f);
        changed |= ImGui::SliderFloat("Ripple Speed", &water.rippleSpeed, 0.0f, 0.1f);
        changed |= ImGui::SliderFloat("Displacement", &water.displacementIntensity, 0.0f, 0.2f);
    }
    
    // ==================== TERRAIN ====================
    if (ImGui::CollapsingHeader("Terrain Visuals")) {
        TerrainSettings& terrain = vs.getTerrainSettings();
        
        ImGui::Text("Visualization Mode:");
        const char* visModes[] = { "Normal", "Erosion", "Moisture", "Temperature", "Slope", "Texture Type" };
        changed |= ImGui::Combo("Mode", &terrain.visualizationMode, visModes, IM_ARRAYSIZE(visModes));
        
        ImGui::Separator();
        ImGui::Text("Erosion Dithering:");
        changed |= ImGui::SliderFloat("Erosion Threshold", &terrain.erosionThreshold, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Full Expose", &terrain.erosionFullExpose, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Dither Intensity", &terrain.ditherIntensity, 0.0f, 1.0f);
        
        ImGui::Separator();
        ImGui::Text("Biome Exposed Textures (U offset):");
        ImGui::TextDisabled("16=dirt, 48=stone, 64=sand");
        changed |= ImGui::SliderInt("Grass Exposed", &terrain.grassExposedU, 0, 64);
        changed |= ImGui::SliderInt("Snow Exposed", &terrain.snowExposedU, 0, 64);
        changed |= ImGui::SliderInt("Sand Exposed", &terrain.sandExposedU, 0, 64);
        changed |= ImGui::SliderInt("Stone Exposed", &terrain.stoneExposedU, 0, 64);
        
        ImGui::Separator();
        ImGui::Text("Biome Transition Textures (U offset):");
        ImGui::TextDisabled("0=grass, 32=snow, 48=stone, 64=sand");
        changed |= ImGui::SliderInt("Tundra Texture", &terrain.tundraTextureU, 0, 64);
        changed |= ImGui::SliderInt("Snow Texture", &terrain.snowTextureU, 0, 64);
        
        ImGui::Separator();
        ImGui::Text("Color Adjustment:");
        changed |= ImGui::SliderFloat("Saturation##terrain", &terrain.colorSaturation, 0.0f, 2.0f);
        changed |= ImGui::SliderFloat("Brightness##terrain", &terrain.colorBrightness, 0.0f, 2.0f);
    }
    
    // ==================== BIOME LIST ====================
    if (ImGui::CollapsingHeader("Biome List")) {
        PotentialData potential = worldGen.getPotentialAt(cameraPosition.x, cameraPosition.z);
        BiomeType currentBiome = BiomeManager::getInstance().getBiomeAt(potential);
        
        for (int i = 0; i < static_cast<int>(BiomeType::COUNT); ++i) {
            const BiomeData& b = BiomeManager::getInstance().getBiomeData(static_cast<BiomeType>(i));
            if (b.name) {
                bool isCurrent = (static_cast<BiomeType>(i) == currentBiome);
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
    
    if (changed) {
        vs.markDirty();
    }
    
    // ==================== ACTIONS ====================
    ImGui::Separator();
    if (ImGui::Button("Regenerate Terrain")) {
        shouldRegenerateTerrain = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Clears all chunks)");
    
    ImGui::SameLine();
    if (ImGui::Button("Reset All")) {
        vs.resetToDefaults();
        config = WorldGenConfig{};
        erosion = ErosionConfig{};
        sunData = sun();
        worldGen.rebuildNoiseGenerators();
    }
    
    ImGui::End();
}