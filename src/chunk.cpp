#include "../include/chunk.hpp"

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
    DrawModel(model, {(float)chunkX, (float)chunkY, 0}, 1.0f, WHITE);
}

void Chunk::renderWires() {
    if (!meshGenerated) generateMesh();
    DrawModelWires(model, {(float)chunkX, (float)chunkY, 0}, 1.0f, WHITE);
}

void Chunk::generateMesh() {
    tiles.generatePerlinTerrain(1.0f, chunkX, chunkY, 50);
    tiles.generateMesh();
    model = tiles.model;
    meshGenerated = true;
}

void Chunk::updateMesh() {
    tiles.generateMesh();
    model = tiles.model;
}