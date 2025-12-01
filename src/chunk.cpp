#include "../include/chunk.hpp"
#include "../include/resourceManager.hpp"
#include "rlgl.h"
#include <chrono>

Chunk::Chunk(int x, int y) : tiles(CHUNKSIZE, CHUNKSIZE) {
    chunkX = x;
    chunkY = y;
    meshGenerated = false;
    generateMesh();
}

Chunk::~Chunk() {
    // tileGrid destructor handles all model/mesh cleanup
    // Chunk::model is just a copy of the reference, not a separate resource
    // GrassField destructor handles grass cleanup
}

// Draw opaque terrain
void Chunk::renderTerrain() {
    if (!meshGenerated) generateMesh();
    Vector3 pos = {(float)chunkX, 0.0f, (float)chunkY};
    DrawModel(model, pos, 1.0f, WHITE);
}

// Draw transparent water layer
void Chunk::renderWater() {
    Vector3 pos = {(float)chunkX, -0.2f, (float)chunkY};
    //rlDisableDepthMask();
    //rlDisableDepthTest();
    DrawModel(tiles.waterModel, pos, 1.0f, WHITE);
    //rlEnableDepthTest();
    //rlEnableDepthMask();
}

// Draw grass layer
void Chunk::renderGrass(float time) {
    grass.render(time);
}

// Draw terrain wireframe
void Chunk::renderWires() {
    if (!meshGenerated) generateMesh();
    DrawModelWires(model, {(float)chunkX, 0.0f, (float)chunkY}, 1.0f, WHITE);
}

// Draw water wireframe
void Chunk::renderWaterWires() {
    rlDisableBackfaceCulling();
    DrawModelWires(tiles.waterModel, {(float)chunkX, 0.0f, (float)chunkY}, 1.0f, WHITE);
    rlEnableBackfaceCulling();
}

void Chunk::generateMesh() {
    using clock = std::chrono::high_resolution_clock;

    int baseGenOffset[6] = {chunkX, chunkY, chunkX+1000, chunkY+1000, chunkX+2000, chunkY+2000};

    tiles.generatePerlinTerrain(0.75f, 90, 4, 0.25f, 2.0f, 1.2f, baseGenOffset);

    tiles.generateMesh();
    tiles.generateWaterMesh();

    // Generate grass
    generateGrassData();

    model = tiles.model;
    Shader& shader = resourceManager::getShader(0);
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = shader;
    }
    meshGenerated = true;
}

void Chunk::generateGrassData() {
    // Collect tile data for grass generation
    int w = CHUNKSIZE;
    int h = CHUNKSIZE;
    
    std::vector<float> heights;
    std::vector<uint8_t> types;
    std::vector<uint8_t> temps;
    std::vector<uint8_t> moists;
    std::vector<uint8_t> bios;
    std::vector<uint8_t> erosions;
    
    // Heights: (w+1) x (h+1) corner vertices
    heights.resize((w + 1) * (h + 1));
    for (int z = 0; z <= h; ++z) {
        for (int x = 0; x <= w; ++x) {
            // Get height from corner - tiles store 4 corners [0]=TL, [1]=TR, [2]=BR, [3]=BL
            int tx = std::min(x, w - 1);
            int tz = std::min(z, h - 1);
            tile t = tiles.getTile(tx, tz);
            
            // Figure out which corner this is
            int cornerIdx;
            if (x == tx && z == tz) cornerIdx = 0;       // Top-left
            else if (x == tx + 1 && z == tz) cornerIdx = 1;  // Top-right
            else if (x == tx + 1 && z == tz + 1) cornerIdx = 2; // Bottom-right
            else cornerIdx = 3; // Bottom-left
            
            heights[z * (w + 1) + x] = t.tileHeight[cornerIdx];
        }
    }
    
    // Per-tile data
    std::vector<BiomeType> biomes;
    biomes.resize(w * h);
    types.resize(w * h); // Kept for debug/other uses if needed, though grass doesn't use it now
    temps.resize(w * h);
    moists.resize(w * h);
    bios.resize(w * h);
    erosions.resize(w * h);
    
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            tile t = tiles.getTile(x, z);
            int idx = z * w + x;
            biomes[idx] = t.biome;
            types[idx] = static_cast<uint8_t>(t.type);
            temps[idx] = t.temperature;
            moists[idx] = t.moisture;
            bios[idx] = t.biologicalPotential;
            erosions[idx] = t.erosionFactor;
        }
    }
    
    grass.generate(chunkX, chunkY, w, h, heights, biomes, temps, moists, bios, erosions);
}

void Chunk::updateMesh() {
    tiles.generateMesh();
    tiles.generateWaterMesh();
    generateGrassData();
    model = tiles.model;
    // Reapply shared terrain shader
    Shader& shader = resourceManager::getShader(0);
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = shader;
    }
}