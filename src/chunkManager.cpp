#include "../include/chunkManager.hpp"
#include "raylib.h"
#include "cmath"

chunkManager::chunkManager(int loadRadius) : radius(loadRadius) {}
chunkManager::~chunkManager() = default;

void chunkManager::update(const Camera& cam) {
    int centerX = static_cast<int>(cam.position.x) / CHUNKSIZE;
    int centerY = static_cast<int>(cam.position.z) / CHUNKSIZE;

    for(int dx = -radius; dx <= radius; ++dx) {
        for(int dy = -radius; dy <= radius; ++dy) {
            ensureChunk(centerX + dx, centerY + dy);
        }
    }

    unloadDistant({centerX, centerY});
}

void chunkManager::render() {
    for(auto& pair : chunks) {
        pair.second->render();
    }
}

void chunkManager::renderDataPoint() {
    for(auto& pair : chunks) {
        pair.second->tiles.renderDataPoint(pair.first.x * CHUNKSIZE, pair.first.y * CHUNKSIZE);
    }
}

void chunkManager::renderWires() {
    for(auto& pair : chunks) {
        pair.second->renderWires();
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
