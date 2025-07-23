#include "../include/chunk.hpp"

#define CHUNKSIZE 16

Chunk::Chunk(int x, int y, int z) : voxelGrid(CHUNKSIZE, CHUNKSIZE, CHUNKSIZE) {
    chunkX = x;
    chunkY = y;
    chunkZ = z;
    meshGenerated = false;
    textureAtlas = LoadTexture("textures.png");
}

Chunk::~Chunk() {
    UnloadTexture(textureAtlas);
    UnloadModel(model);
}

void Chunk::render() {
    if (!meshGenerated) generateMesh();
    DrawModel(model, {(float)chunkX, (float)chunkY, (float)chunkZ}, 1.0f, WHITE);
}

void Chunk::renderWires() {
    if (!meshGenerated) generateMesh();
    DrawModelWires(model, {0, 0, 0}, 1.0f, WHITE);
    DrawModelWires(model, {(float)chunkX, (float)chunkY, (float)chunkZ}, 1.0f, WHITE);
}

void Chunk::generateMesh() {
    voxelGrid.generatePerlinTerrain(0.07f, chunkX, chunkZ, 10);
    voxelGrid.generateMesh();
    meshGenerated = true;

    UploadMesh(&mesh, false);
    model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textureAtlas;
}

void Chunk::updateMesh() {
    UnloadModel(model);
    voxelGrid.generateMesh();

    UploadMesh(&mesh, false);
    model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textureAtlas;
}