#ifndef CHUNKMANAGER_HPP
#define CHUNKMANAGER_HPP

#include <unordered_map>
#include <memory>
#include "chunk.hpp"
#include "raylib.h"

struct ChunkCoord {
    int x;
    int y;
    bool operator==(const ChunkCoord& other) const { return x == other.x && y == other.y; }
};

namespace std {
    template<>
    struct hash<ChunkCoord> {
        size_t operator()(const ChunkCoord& coord) const noexcept {
            return (std::hash<int>()(coord.x) << 1) ^ std::hash<int>()(coord.y);
        }
    };
}

class chunkManager {
public:
    explicit chunkManager(int loadRadius = 2);
    ~chunkManager();

    void update(const Camera& cam);
    void render();
    void renderWires();
    void renderDataPoint(Color a, Color b, uint8_t tile::*dataMember);
    Chunk* getChunk(int cx, int cy);
    // Ray-pick across loaded chunks around camera; returns (globalX, globalZ, localHeight) or (-1,-1,-1) if none
    Vector3 pickTile(const Ray& ray, const Camera& cam);

private:
    Chunk* ensureChunk(int cx, int cy);
    void unloadDistant(const ChunkCoord& center);

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> chunks;
    int radius;
    ChunkCoord lastCenter; // last camera chunk to avoid redundant updates
};

#endif // CHUNKMANAGER_HPP
