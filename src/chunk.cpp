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

    tiles.generatePerlinTerrain(0.75f, 70, 4, 0.25f, 2.0f, 1.2f, baseGenOffset);

    tiles.generateMesh();
    tiles.generateWaterMesh();


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