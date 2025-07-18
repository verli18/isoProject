#include "../include/3DvoxelGrid.hpp"
#include <raylib.h>

VoxelGrid::VoxelGrid(int width, int height, int depth) {
    this->width = width;
    this->height = height;
    this->depth = depth;
    grid.resize(width, std::vector<std::vector<bool>>(height, std::vector<bool>(depth, false)));
}

VoxelGrid::~VoxelGrid() {
}

void VoxelGrid::setVoxel(int x, int y, int z, bool value) {
    grid[x][y][z] = value;
}

bool VoxelGrid::getVoxel(int x, int y, int z) {
    return grid[x][y][z];
}

void VoxelGrid::generatePerlinTerrain(float scale, int offsetX, int offsetY) {
    perlinNoise = GenImagePerlinNoise(width, height, offsetX, offsetY,  1);
    Color sample;
    for (int x = 0; x < width; x++) {
        for (int z = 0; z < height; z++) {
            sample = GetImageColor(perlinNoise, x, z);
            for (int y = 0; y < std::min((int)(sample.r*scale), depth); y++) {
                setVoxel(x, y, z, true);
            }
        }
    }
    UnloadImage(perlinNoise);
}
void VoxelGrid::render() {
    for (int x = 0; x < width; x++) {
        for (int z = 0; z < height; z++) {
            for (int y = 0; y < depth; y++) {
                if (getVoxel(x, y, z)) {
                    DrawPoint3D({(float)x, (float)y, (float)z}, {128, 128, static_cast<unsigned char>(y*255/depth), 255});
                }
            }
        }
    }
}

unsigned int VoxelGrid::getWidth() { return width; }
unsigned int VoxelGrid::getHeight() { return height; }
unsigned int VoxelGrid::getDepth() { return depth; }
