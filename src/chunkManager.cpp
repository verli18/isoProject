#include "../include/chunkManager.hpp"
#include "raylib.h"
#include "cmath"
#include <algorithm>

chunkManager::chunkManager(int loadRadius) : radius(loadRadius), lastCenter({-99999, -99999}) {}
chunkManager::~chunkManager() = default;

void chunkManager::update(const Camera& cam) {
    int centerX = static_cast<int>(floor(cam.position.x / CHUNKSIZE));
    int centerY = static_cast<int>(floor(cam.position.z / CHUNKSIZE));

    ChunkCoord currentCenter{centerX, centerY};
    if (currentCenter == lastCenter) return;

    for(int dx = -radius; dx <= radius; ++dx) {
        for(int dy = -radius; dy <= radius; ++dy) {
            ensureChunk(currentCenter.x + dx, currentCenter.y + dy);
        }
    }

    unloadDistant(currentCenter);
    lastCenter = currentCenter;
}

void chunkManager::render() {
    // First pass: draw all opaque terrain
    for (auto& pair : chunks) {
        pair.second->renderTerrain();
    }
    // Second pass: draw all transparent water layers in a stable order
    // Gather chunks into a vector and sort by chunk coordinates
    std::vector<std::pair<ChunkCoord, Chunk*>> items;
    items.reserve(chunks.size());
    for (auto& pair : chunks) {
        items.emplace_back(pair.first, pair.second.get());
    }
    std::sort(items.begin(), items.end(), [](auto& a, auto& b) {
        if (a.first.x != b.first.x) return a.first.x < b.first.x;
        return a.first.y < b.first.y;
    });
    for (auto& entry : items) {
        entry.second->renderWater();
    }
}

void chunkManager::renderGrass(float time, const Camera& cam) {
    // Distance culling - only render grass for nearby chunks
    const float grassCullDistance = 100.0f;  // Max distance for grass rendering
    const float grassCullDistSq = grassCullDistance * grassCullDistance;
    
    for (auto& pair : chunks) {
        // Calculate chunk center in world coords
        float chunkCenterX = (pair.first.x + 0.5f) * CHUNKSIZE;
        float chunkCenterZ = (pair.first.y + 0.5f) * CHUNKSIZE;
        
        // Distance from camera to chunk center (XZ plane only)
        float dx = chunkCenterX - cam.position.x;
        float dz = chunkCenterZ - cam.position.z;
        float distSq = dx*dx + dz*dz;
        
        if (distSq < grassCullDistSq) {
            pair.second->renderGrass(time);
        }
    }
}

void chunkManager::renderDataPoint(Color a, Color b, uint8_t tile::*dataMember) {
    for(auto& pair : chunks) {
        pair.second->tiles.renderDataPoint(a, b, dataMember, pair.first.x * CHUNKSIZE, pair.first.y * CHUNKSIZE);
    }
}

void chunkManager::renderWires() {
    // Draw terrain wireframes
    for (auto& pair : chunks) {
        pair.second->renderWires();
    }
    // Draw water wireframes
    for (auto& pair : chunks) {
        pair.second->renderWaterWires();
    }
}

Chunk* chunkManager::getChunk(int cx, int cy) {
    return ensureChunk(cx, cy);
}

Chunk* chunkManager::ensureChunk(int cx, int cy) {
    ChunkCoord key{cx, cy};
    auto it = chunks.find(key);
    if(it != chunks.end()) return it->second.get();

    // Create and store new chunk; actual mesh generation deferred to Chunk::render
    auto ptr = std::make_unique<Chunk>(cx * CHUNKSIZE, cy * CHUNKSIZE);
    Chunk* raw = ptr.get();
    chunks.emplace(key, std::move(ptr));
    return raw;
}

void chunkManager::unloadDistant(const ChunkCoord& center) {
    for(auto it = chunks.begin(); it != chunks.end();) {
        int dx = it->first.x - center.x;
        int dy = it->first.y - center.y;
        if(abs(dx) > radius || abs(dy) > radius) {
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void chunkManager::clearAllChunks() {
    chunks.clear();
    lastCenter = {-99999, -99999};  // Force reload on next update
}

size_t chunkManager::getTotalGrassBlades() const {
    size_t total = 0;
    for (const auto& pair : chunks) {
        total += pair.second->grass.getBladeCount();
    }
    return total;
}
