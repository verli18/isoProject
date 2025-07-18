#include "../include/3DvoxelGrid.hpp"

VoxelGrid::VoxelGrid(int width, int height, int depth) {
    this->width = width;
    this->height = height;
    this->depth = depth;
    grid.resize(width, std::vector<std::vector<Voxels>>(height, std::vector<Voxels>(depth)));
}

VoxelGrid::~VoxelGrid() {
}

void VoxelGrid::setVoxel(int x, int y, int z, Voxels voxel) {
    grid[x][y][z] = voxel;
}

Voxels VoxelGrid::getVoxel(int x, int y, int z) {
    return grid[x][y][z];
}

void VoxelGrid::generatePerlinTerrain(float scale, int offsetX, int offsetY) {
    perlinNoise = GenImagePerlinNoise(width, height, offsetX, offsetY,  1);
    Color sample;
    for (int x = 0; x < width; x++) {
        for (int z = 0; z < height; z++) {
            sample = GetImageColor(perlinNoise, x, z);
            for (int y = 0; y < std::min((int)(sample.r*scale), depth); y++) {
                setVoxel(x, y, z, Voxels{1, {255, 255, 255, 255}, true, false});
            }
        }
    }
    UnloadImage(perlinNoise);
}

Vector3 VoxelGrid::getVoxelIndexDDA(Ray ray) {
    Vector3 rayStart = ray.position;
    Vector3 rayDir = ray.direction;

    // Current voxel coordinates (integer)
    Vector3 currentVoxel = { (float)floor(rayStart.x), (float)floor(rayStart.y), (float)floor(rayStart.z) };

    // Direction to step in x, y, z (either +1 or -1)
    Vector3 step = { (rayDir.x >= 0) ? 1.0f : -1.0f, (rayDir.y >= 0) ? 1.0f : -1.0f, (rayDir.z >= 0) ? 1.0f : -1.0f };

    // Distance to travel along the ray to cross one voxel unit in x, y, z
    Vector3 tDelta = { fabsf(1.0f / rayDir.x), fabsf(1.0f / rayDir.y), fabsf(1.0f / rayDir.z) };

    // Distance to the next voxel boundary in x, y, z
    float nextX = (rayDir.x >= 0) ? (currentVoxel.x + 1.0f - rayStart.x) : (rayStart.x - currentVoxel.x);
    float nextY = (rayDir.y >= 0) ? (currentVoxel.y + 1.0f - rayStart.y) : (rayStart.y - currentVoxel.y);
    float nextZ = (rayDir.z >= 0) ? (currentVoxel.z + 1.0f - rayStart.z) : (rayStart.z - currentVoxel.z);
    Vector3 tMax = { tDelta.x * nextX, tDelta.y * nextY, tDelta.z * nextZ };

    int maxSteps = 100; // Safety break to prevent infinite loops

    for (int i = 0; i < maxSteps; ++i) {
        // Check if current voxel is inside grid bounds
        if (currentVoxel.x >= 0 && currentVoxel.x < width &&
            currentVoxel.y >= 0 && currentVoxel.y < height &&
            currentVoxel.z >= 0 && currentVoxel.z < depth) {
            
            // If the current voxel is solid, we've found our target
            if (getVoxel(currentVoxel.x, currentVoxel.y, currentVoxel.z).type != 0) {
                return currentVoxel; // Return the coordinates of the hit voxel
            }
        }

        // Advance to the next voxel
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                currentVoxel.x += step.x;
                tMax.x += tDelta.x;
            } else {
                currentVoxel.z += step.z;
                tMax.z += tDelta.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                currentVoxel.y += step.y;
                tMax.y += tDelta.y;
            } else {
                currentVoxel.z += step.z;
                tMax.z += tDelta.z;
            }
        }
    }

    // If no solid voxel was found within maxSteps, return an invalid position
    return {-1, -1, -1};
}

void VoxelGrid::render() {
    for (int x = 0; x < width; x++) {
        for (int z = 0; z < height; z++) {
            for (int y = 0; y < depth; y++) {
                if (getVoxel(x, y, z).type != 0) {
                    DrawPoint3D({(float)x, (float)y, (float)z}, {128, 128, static_cast<unsigned char>(y*255/depth), 255});
                }
            }
        }
    }
}

unsigned int VoxelGrid::getWidth() { return width; }
unsigned int VoxelGrid::getHeight() { return height; }
unsigned int VoxelGrid::getDepth() { return depth; }
