#include "../include/chunk.hpp"
#include "../include/resourceManager.hpp"  // for shared shader reference
#include "../include/resourceManager.hpp"
#include "rlgl.h"
#include <chrono>

#define CHUNKSIZE 32

Chunk::Chunk(int x, int y) : tiles(CHUNKSIZE, CHUNKSIZE) {
    chunkX = x;
    chunkY = y;
    meshGenerated = false;
    generateMesh();
}

Chunk::~Chunk() {
}

// Draw opaque terrain
void Chunk::renderTerrain() {
    if (!meshGenerated) generateMesh();
    Vector3 pos = {(float)chunkX, 0.0f, (float)chunkY};
    DrawModel(model, pos, 1.0f, WHITE);
}

// Draw transparent water layer
void Chunk::renderWater() {
    Vector3 pos = {(float)chunkX, 0.0f, (float)chunkY};
    //rlDisableDepthMask();
    //rlDisableDepthTest();
    DrawModel(tiles.waterModel, pos, 1.0f, WHITE);
    //rlEnableDepthTest();
    //rlEnableDepthMask();
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

#ifdef TILEGRID_PROFILE
    auto t0 = clock::now();
#endif
    tiles.generatePerlinTerrain(0.75f, 100, 4, 0.25f, 2.0f, 1.1f, baseGenOffset);
#ifdef TILEGRID_PROFILE
    auto t1 = clock::now();
#endif

#ifdef TILEGRID_PROFILE
    auto tMesh0 = clock::now();
#endif
    tiles.generateMesh();
#ifdef TILEGRID_PROFILE
    auto tMesh1 = clock::now();
#endif

#ifdef TILEGRID_PROFILE
    auto tWater0 = clock::now();
#endif
    tiles.generateWaterMesh();
#ifdef TILEGRID_PROFILE
    auto tWater1 = clock::now();
    // Accumulate per-chunk snapshot into tileGrid's lastTimings fields that were set internally
    tiles.lastTimings.meshGenMs = std::chrono::duration<double, std::milli>(tMesh1 - tMesh0).count();
    tiles.lastTimings.samplesMesh++;
    // Terrain phase (noise+index) already captured inside tileGrid::generatePerlinTerrain
    // Water mesh timing just measured here:
    tiles.lastTimings.waterGenMs = std::chrono::duration<double, std::milli>(tWater1 - tWater0).count();
    tiles.lastTimings.samplesWater++;
#endif

    model = tiles.model;
    Shader& shader = resourceManager::getShader(0);
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = shader;
    }
    meshGenerated = true;
}

void Chunk::updateMesh() {
    tiles.generateMesh();
    tiles.generateWaterMesh();
    model = tiles.model;
    // Reapply shared terrain shader
    Shader& shader = resourceManager::getShader(0);
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = shader;
    }
}