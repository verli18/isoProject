#include "../include/chunk.hpp"
#include "../include/resourceManager.hpp"  // for shared shader reference
#include "../include/resourceManager.hpp"

#define CHUNKSIZE 32

Chunk::Chunk(int x, int y) : tiles(CHUNKSIZE, CHUNKSIZE) {
    chunkX = x;
    chunkY = y;
    meshGenerated = false;
    generateMesh();
}

Chunk::~Chunk() {
}

void Chunk::render() {
    if (!meshGenerated) generateMesh();
    // Place chunk on XZ plane (Y is up)
    DrawModel(model, {(float)chunkX, 0.0f, (float)chunkY}, 1.0f, WHITE);
}

void Chunk::renderWires() {
    if (!meshGenerated) generateMesh();
    // Place wireframe on XZ plane
    DrawModelWires(model, {(float)chunkX, 0.0f, (float)chunkY}, 1.0f, WHITE);
}

void Chunk::generateMesh() {
    tiles.generatePerlinTerrain(1.0f,
                                chunkX,
                                chunkY,
                                18);
    tiles.generateMesh();
    model = tiles.model;
    // Apply shared terrain shader
    Shader& shader = resourceManager::getShader();
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = shader;
    }
    meshGenerated = true;
}

void Chunk::updateMesh() {
    tiles.generateMesh();
    model = tiles.model;
    // Reapply shared terrain shader
    Shader& shader = resourceManager::getShader();
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = shader;
    }
}