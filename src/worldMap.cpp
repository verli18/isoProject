#include "../include/worldMap.hpp"
#include "../include/worldGenerator.hpp"
#include <cmath>
#include <algorithm>
#include <queue>

// ============================================================
// RegionData Implementation
// ============================================================

float RegionData::getHeight(float localX, float localZ) const {
    int x0 = static_cast<int>(std::floor(localX));
    int z0 = static_cast<int>(std::floor(localZ));
    
    // Clamp to valid range
    x0 = std::clamp(x0, 0, width - 1);
    z0 = std::clamp(z0, 0, height - 1);
    int x1 = std::min(x0 + 1, width);
    int z1 = std::min(z0 + 1, height);
    
    float fx = std::clamp(localX - x0, 0.0f, 1.0f);
    float fz = std::clamp(localZ - z0, 0.0f, 1.0f);
    
    int stride = width + 1;
    float h00 = heights[z0 * stride + x0];
    float h10 = heights[z0 * stride + x1];
    float h01 = heights[z1 * stride + x0];
    float h11 = heights[z1 * stride + x1];
    
    return h00 * (1 - fx) * (1 - fz) +
           h10 * fx * (1 - fz) +
           h01 * (1 - fx) * fz +
           h11 * fx * fz;
}

float RegionData::getHeightAt(int localX, int localZ) const {
    localX = std::clamp(localX, 0, width);
    localZ = std::clamp(localZ, 0, height);
    return heights[localZ * (width + 1) + localX];
}

bool RegionData::contains(int localX, int localZ) const {
    return localX >= 0 && localX < width && localZ >= 0 && localZ < height;
}

// ============================================================
// WorldMap Implementation
// ============================================================

WorldMap& WorldMap::getInstance() {
    static WorldMap instance;
    return instance;
}

void WorldMap::initialize() {
    if (!WorldGenerator::getInstance().isInitialized()) {
        return;  // Wait for WorldGenerator
    }
    initialized = true;
}

void WorldMap::clear() {
    std::lock_guard<std::recursive_mutex> lock(regionMutex);
    regions.clear();
}

int64_t WorldMap::worldToRegionKey(int worldX, int worldZ) const {
    // Divide by region size to get region coordinates
    // Use integer division that rounds toward negative infinity
    int regionX = (worldX >= 0) ? (worldX / REGION_SIZE) : ((worldX - REGION_SIZE + 1) / REGION_SIZE);
    int regionZ = (worldZ >= 0) ? (worldZ / REGION_SIZE) : ((worldZ - REGION_SIZE + 1) / REGION_SIZE);
    
    // Pack into 64-bit key
    return (static_cast<int64_t>(regionX) << 32) | (static_cast<uint32_t>(regionZ));
}

void WorldMap::getRegionOrigin(int worldX, int worldZ, int& regionX, int& regionZ) const {
    // Round down to region boundary
    regionX = (worldX >= 0) ? (worldX / REGION_SIZE) * REGION_SIZE 
                            : ((worldX - REGION_SIZE + 1) / REGION_SIZE) * REGION_SIZE;
    regionZ = (worldZ >= 0) ? (worldZ / REGION_SIZE) * REGION_SIZE 
                            : ((worldZ - REGION_SIZE + 1) / REGION_SIZE) * REGION_SIZE;
}

RegionData& WorldMap::getRegion(int worldX, int worldZ) {
    int64_t key = worldToRegionKey(worldX, worldZ);
    
    std::lock_guard<std::recursive_mutex> lock(regionMutex);
    
    auto it = regions.find(key);
    if (it != regions.end()) {
        return *it->second;
    }
    
    // Create new region
    auto region = std::make_unique<RegionData>();
    getRegionOrigin(worldX, worldZ, region->worldX, region->worldZ);
    region->width = REGION_SIZE;
    region->height = REGION_SIZE;
    
    RegionData& ref = *region;
    regions[key] = std::move(region);
    return ref;
}

void WorldMap::ensureRegionReady(RegionData& region) {
    if (!region.heightsGenerated) {
        generateHeights(region);
    }
    if (!region.eroded) {
        applyErosion(region);
    }
    if (!region.potentialsGenerated) {
        generatePotentials(region);
    }
    if (!region.waterGenerated) {
        generateWater(region);
    }
}

float WorldMap::getHeight(float worldX, float worldZ) {
    RegionData& region = getRegion(static_cast<int>(worldX), static_cast<int>(worldZ));
    
    if (!region.heightsGenerated) {
        generateHeights(region);
    }
    if (!region.eroded) {
        applyErosion(region);
    }
    
    float localX = worldX - region.worldX;
    float localZ = worldZ - region.worldZ;
    return region.getHeight(localX, localZ);
}

void WorldMap::getHeightGrid(
    std::vector<float>& out,
    int chunkWorldX, int chunkWorldZ,
    int width, int height
) {
    out.resize((width + 1) * (height + 1));
    
    // May need to sample from multiple regions
    for (int z = 0; z <= height; ++z) {
        for (int x = 0; x <= width; ++x) {
            int worldX = chunkWorldX + x;
            int worldZ = chunkWorldZ + z;
            out[z * (width + 1) + x] = getHeight(static_cast<float>(worldX), static_cast<float>(worldZ));
        }
    }
}

void WorldMap::getPotentialGrid(
    std::vector<PotentialData>& out,
    int chunkWorldX, int chunkWorldZ,
    int width, int height
) {
    out.resize(width * height);
    
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            int worldX = chunkWorldX + x;
            int worldZ = chunkWorldZ + z;
            
            RegionData& region = getRegion(worldX, worldZ);
            ensureRegionReady(region);
            
            int localX = worldX - region.worldX;
            int localZ = worldZ - region.worldZ;
            int idx = localZ * region.width + localX;
            
            out[z * width + x] = region.potentials[idx];
        }
    }
}

void WorldMap::getWaterGrid(
    std::vector<float>& out,
    int chunkWorldX, int chunkWorldZ,
    int width, int height
) {
    out.resize(width * height);
    
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            int worldX = chunkWorldX + x;
            int worldZ = chunkWorldZ + z;
            
            RegionData& region = getRegion(worldX, worldZ);
            ensureRegionReady(region);
            
            int localX = worldX - region.worldX;
            int localZ = worldZ - region.worldZ;
            int idx = localZ * region.width + localX;
            
            out[z * width + x] = region.waterLevels[idx];
        }
    }
}

void WorldMap::getRiverGrid(
    std::vector<uint8_t>& flowDirOut,
    std::vector<uint8_t>& riverWidthOut,
    int chunkWorldX, int chunkWorldZ,
    int width, int height
) {
    int N = width * height;
    flowDirOut.resize(N);
    riverWidthOut.resize(N);
    
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            int worldX = chunkWorldX + x;
            int worldZ = chunkWorldZ + z;
            
            RegionData& region = getRegion(worldX, worldZ);
            ensureRegionReady(region);
            
            int localX = worldX - region.worldX;
            int localZ = worldZ - region.worldZ;
            int idx = localZ * region.width + localX;
            int outIdx = z * width + x;
            
            flowDirOut[outIdx] = region.flowDir[idx];
            riverWidthOut[outIdx] = region.riverWidth[idx];
        }
    }
}

void WorldMap::getErosionGrid(
    std::vector<uint8_t>& erosionOut,
    int chunkWorldX, int chunkWorldZ,
    int width, int height
) {
    int N = width * height;
    erosionOut.resize(N);
    
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            int worldX = chunkWorldX + x;
            int worldZ = chunkWorldZ + z;
            
            RegionData& region = getRegion(worldX, worldZ);
            ensureRegionReady(region);
            
            int localX = worldX - region.worldX;
            int localZ = worldZ - region.worldZ;
            int idx = localZ * region.width + localX;
            
            erosionOut[z * width + x] = region.erosionIntensity[idx];
        }
    }
}

void WorldMap::preloadAround(int worldX, int worldZ, int radiusInRegions) {
    for (int rz = -radiusInRegions; rz <= radiusInRegions; ++rz) {
        for (int rx = -radiusInRegions; rx <= radiusInRegions; ++rx) {
            int regionWorldX = worldX + rx * REGION_SIZE;
            int regionWorldZ = worldZ + rz * REGION_SIZE;
            RegionData& region = getRegion(regionWorldX, regionWorldZ);
            ensureRegionReady(region);
        }
    }
}

// ============================================================
// Internal Generation Functions
// ============================================================

void WorldMap::generateHeights(RegionData& region) {
    WorldGenerator& gen = WorldGenerator::getInstance();
    
    // Generate height grid for corner vertices
    gen.generateHeightGrid(
        region.heights,
        region.worldX, region.worldZ,
        region.width + 1, region.height + 1
    );
    
    region.heightsGenerated = true;
}

void WorldMap::applyErosion(RegionData& region) {
    if (!region.heightsGenerated) {
        generateHeights(region);
    }
    
    // Use config for all parameters
    const ErosionConfig& cfg = erosionConfig;
    
    const int numDroplets = cfg.numDroplets;
    const int maxLifetime = cfg.maxDropletLifetime;
    const float inertia = cfg.inertia;
    const float sedimentCapacityFactor = cfg.sedimentCapacity;
    const float minSedimentCapacity = cfg.minSedimentCapacity;
    const float erodeSpeed = cfg.erodeSpeed;
    const float depositSpeed = cfg.depositSpeed;
    const float evaporateSpeed = cfg.evaporateSpeed;
    const float gravity = cfg.gravity;
    const float maxErode = cfg.maxErodePerStep;
    const int erosionRadius = cfg.erosionRadius;
    const float initialWater = 1.0f;
    
    int width = region.width + 1;
    int height = region.height + 1;
    
    // Initialize erosion intensity tracking for tiles (not vertices)
    // We'll accumulate erosion amount per tile, then normalize to 0-255
    std::vector<float> erosionAccum(region.width * region.height, 0.0f);
    
    // Precompute erosion brush
    std::vector<std::pair<int, int>> brushOffsets;
    std::vector<float> brushWeights;
    float weightSum = 0.0f;
    
    for (int dz = -erosionRadius; dz <= erosionRadius; ++dz) {
        for (int dx = -erosionRadius; dx <= erosionRadius; ++dx) {
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist <= erosionRadius) {
                brushOffsets.push_back({dx, dz});
                float w = std::max(0.0f, erosionRadius - dist);
                brushWeights.push_back(w);
                weightSum += w;
            }
        }
    }
    for (float& w : brushWeights) w /= weightSum;
    
    // Helpers
    auto inBounds = [&](int x, int z) {
        return x >= 0 && x < width && z >= 0 && z < height;
    };
    
    auto getH = [&](float x, float z) -> float {
        int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, width - 2);
        int z0 = std::clamp(static_cast<int>(std::floor(z)), 0, height - 2);
        float fx = std::clamp(x - x0, 0.0f, 1.0f);
        float fz = std::clamp(z - z0, 0.0f, 1.0f);
        
        float h00 = region.heights[z0 * width + x0];
        float h10 = region.heights[z0 * width + x0 + 1];
        float h01 = region.heights[(z0 + 1) * width + x0];
        float h11 = region.heights[(z0 + 1) * width + x0 + 1];
        
        return h00 * (1 - fx) * (1 - fz) +
               h10 * fx * (1 - fz) +
               h01 * (1 - fx) * fz +
               h11 * fx * fz;
    };
    
    auto getGrad = [&](float x, float z) -> std::pair<float, float> {
        int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, width - 2);
        int z0 = std::clamp(static_cast<int>(std::floor(z)), 0, height - 2);
        float fx = std::clamp(x - x0, 0.0f, 1.0f);
        float fz = std::clamp(z - z0, 0.0f, 1.0f);
        
        float h00 = region.heights[z0 * width + x0];
        float h10 = region.heights[z0 * width + x0 + 1];
        float h01 = region.heights[(z0 + 1) * width + x0];
        float h11 = region.heights[(z0 + 1) * width + x0 + 1];
        
        float gx = (h10 - h00) * (1 - fz) + (h11 - h01) * fz;
        float gz = (h01 - h00) * (1 - fx) + (h11 - h10) * fx;
        return {gx, gz};
    };
    
    // RNG based on region position
    const WorldGenConfig& genCfg = WorldGenerator::getInstance().getConfig();
    uint32_t rng = static_cast<uint32_t>(genCfg.seed + region.worldX * 73856093 + region.worldZ * 19349663);
    auto nextRand = [&rng]() -> float {
        rng = rng * 1103515245 + 12345;
        return static_cast<float>((rng >> 16) & 0x7FFF) / 32767.0f;
    };
    
    // Margin from edges
    const int margin = erosionRadius + 2;
    const float spawnW = static_cast<float>(width - 2 * margin);
    const float spawnH = static_cast<float>(height - 2 * margin);
    
    if (spawnW <= 0 || spawnH <= 0) {
        region.erosionIntensity.resize(region.width * region.height, 0);
        region.eroded = true;
        return;
    }
    
    // Simulate droplets
    for (int drop = 0; drop < numDroplets; ++drop) {
        float posX = nextRand() * spawnW + margin;
        float posZ = nextRand() * spawnH + margin;
        float dirX = 0.0f, dirZ = 0.0f;
        float speed = 1.0f;  // Start with some speed
        float water = initialWater;
        float sediment = 0.0f;
        
        for (int life = 0; life < maxLifetime; ++life) {
            int nodeX = static_cast<int>(posX);
            int nodeZ = static_cast<int>(posZ);
            float cellX = posX - nodeX;
            float cellZ = posZ - nodeZ;
            
            if (!inBounds(nodeX, nodeZ) || !inBounds(nodeX + 1, nodeZ + 1)) break;
            
            float hOld = getH(posX, posZ);
            auto [gx, gz] = getGrad(posX, posZ);
            
            dirX = dirX * inertia - gx * (1 - inertia);
            dirZ = dirZ * inertia - gz * (1 - inertia);
            
            float len = std::sqrt(dirX * dirX + dirZ * dirZ);
            if (len < 0.0001f) {
                float angle = nextRand() * 6.28318f;
                dirX = std::cos(angle);
                dirZ = std::sin(angle);
            } else {
                dirX /= len;
                dirZ /= len;
            }
            
            float newX = posX + dirX;
            float newZ = posZ + dirZ;
            
            if (newX < 0 || newX >= width - 1 || newZ < 0 || newZ >= height - 1) break;
            
            float hNew = getH(newX, newZ);
            float dh = hNew - hOld;
            
            // Capacity based on speed and slope
            float capacity = std::max(-dh, 0.0f) * speed * water * sedimentCapacityFactor + minSedimentCapacity;
            
            if (sediment > capacity || dh > 0) {
                // Deposit sediment
                float deposit = (dh > 0) ? std::min(dh, sediment) : (sediment - capacity) * depositSpeed;
                deposit = std::max(0.0f, deposit);  // Never negative
                sediment -= deposit;
                
                int idx00 = nodeZ * width + nodeX;
                region.heights[idx00] += deposit * (1 - cellX) * (1 - cellZ);
                region.heights[idx00 + 1] += deposit * cellX * (1 - cellZ);
                region.heights[idx00 + width] += deposit * (1 - cellX) * cellZ;
                region.heights[idx00 + width + 1] += deposit * cellX * cellZ;
            } else {
                // Erode terrain - cap the maximum erosion per step
                float erode = std::min((capacity - sediment) * erodeSpeed, maxErode);
                erode = std::max(0.0f, erode);  // Never negative
                
                for (size_t i = 0; i < brushOffsets.size(); ++i) {
                    int ex = nodeX + brushOffsets[i].first;
                    int ez = nodeZ + brushOffsets[i].second;
                    if (inBounds(ex, ez)) {
                        float amt = erode * brushWeights[i];
                        region.heights[ez * width + ex] -= amt;
                        sediment += amt;
                        
                        // Track erosion intensity per tile
                        // Convert vertex coords to tile coords (vertices are at tile corners)
                        int tileX = std::clamp(ex, 0, region.width - 1);
                        int tileZ = std::clamp(ez, 0, region.height - 1);
                        int tileIdx = tileZ * region.width + tileX;
                        // Weight by droplet speed (faster = more visible erosion channels)
                        erosionAccum[tileIdx] += amt * speed;
                    }
                }
            }
            
            // Update speed based on height change
            speed = std::sqrt(std::max(0.01f, speed * speed + dh * gravity));
            water *= (1 - evaporateSpeed);
            posX = newX;
            posZ = newZ;
            
            if (water < 0.01f) break;
        }
    }
    
    // Normalize erosion accumulator to 0-255 range
    float maxErosion = 0.0f;
    for (float e : erosionAccum) {
        if (e > maxErosion) maxErosion = e;
    }
    
    region.erosionIntensity.resize(region.width * region.height);
    if (maxErosion > 0.001f) {
        for (size_t i = 0; i < erosionAccum.size(); ++i) {
            // Apply sqrt to make erosion more visible in lower ranges
            float normalized = std::sqrt(erosionAccum[i] / maxErosion);
            region.erosionIntensity[i] = static_cast<uint8_t>(std::clamp(normalized * 255.0f, 0.0f, 255.0f));
        }
    } else {
        std::fill(region.erosionIntensity.begin(), region.erosionIntensity.end(), 0);
    }
    
    // Also factor in slope - steep areas show more exposed rock even without erosion simulation
    // This ensures cliffs always look rocky
    for (int z = 0; z < region.height; ++z) {
        for (int x = 0; x < region.width; ++x) {
            int hIdx = z * (region.width + 1) + x;
            float h00 = region.heights[hIdx];
            float h10 = region.heights[hIdx + 1];
            float h01 = region.heights[hIdx + region.width + 1];
            float h11 = region.heights[hIdx + region.width + 2];
            
            float maxDiff = std::max({
                std::abs(h00 - h10), std::abs(h00 - h01), std::abs(h00 - h11),
                std::abs(h10 - h01), std::abs(h10 - h11), std::abs(h01 - h11)
            });
            
            // Slope factor: 0 for flat, 1 for very steep (>1.5 units difference)
            float slopeFactor = std::clamp(maxDiff / 1.5f, 0.0f, 1.0f);
            
            int tileIdx = z * region.width + x;
            // Combine erosion simulation with slope
            float combined = region.erosionIntensity[tileIdx] / 255.0f;
            combined = std::max(combined, slopeFactor * 0.8f);  // Steep slopes = at least 80% exposed
            region.erosionIntensity[tileIdx] = static_cast<uint8_t>(std::clamp(combined * 255.0f, 0.0f, 255.0f));
        }
    }
    
    region.eroded = true;
}

void WorldMap::generatePotentials(RegionData& region) {
    WorldGenerator& gen = WorldGenerator::getInstance();
    
    gen.generatePotentialGrid(
        region.potentials,
        region.worldX, region.worldZ,
        region.width, region.height
    );
    
    region.potentialsGenerated = true;
}

void WorldMap::generateWater(RegionData& region) {
    if (!region.heightsGenerated || !region.eroded) {
        ensureRegionReady(region);
    }
    
    const int W = region.width;
    const int H = region.height;
    const int N = W * H;
    
    region.waterLevels.resize(N, 0.0f);
    region.flowAccum.resize(N, 0);
    region.flowDir.resize(N, 255);
    region.riverWidth.resize(N, 0);
    
    auto idx = [W](int x, int z) { return z * W + x; };
    const int hStride = W + 1;
    
    // D8 directions: E, SE, S, SW, W, NW, N, NE
    const int dx8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    const int dz8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    const float dist8[8] = {1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f};
    const int dx4[4] = {1, -1, 0, 0};
    const int dz4[4] = {0, 0, 1, -1};
    
    // ========================================
    // STEP 1: Compute ground heights per tile
    // ========================================
    std::vector<float> ground(N);
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, z);
            float h00 = region.heights[z * hStride + x];
            float h10 = region.heights[z * hStride + x + 1];
            float h01 = region.heights[(z + 1) * hStride + x];
            float h11 = region.heights[(z + 1) * hStride + x + 1];
            ground[i] = (h00 + h10 + h01 + h11) * 0.25f;
        }
    }
    
    // Helper: get height including from neighboring regions
    auto getHeight = [&](int lx, int lz) -> float {
        if (lx >= 0 && lx < W && lz >= 0 && lz < H) {
            return ground[idx(lx, lz)];
        }
        int wx = region.worldX + lx;
        int wz = region.worldZ + lz;
        return this->getHeight(static_cast<float>(wx), static_cast<float>(wz));
    };
    
    // ========================================
    // STEP 2: D8 flow directions
    // ========================================
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, z);
            float h = ground[i];
            float maxSlope = 0.0f;
            int bestDir = 255;
            
            for (int d = 0; d < 8; ++d) {
                float nh = getHeight(x + dx8[d], z + dz8[d]);
                float slope = (h - nh) / dist8[d];
                if (slope > maxSlope) {
                    maxSlope = slope;
                    bestDir = d;
                }
            }
            region.flowDir[i] = static_cast<uint8_t>(bestDir);
        }
    }
    
    // ========================================
    // STEP 3: Flow accumulation (high to low)
    // ========================================
    std::vector<std::pair<float, int>> sorted(N);
    for (int i = 0; i < N; ++i) {
        sorted[i] = {ground[i], i};
        region.flowAccum[i] = 1;
    }
    std::sort(sorted.begin(), sorted.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    for (const auto& [h, i] : sorted) {
        uint8_t dir = region.flowDir[i];
        if (dir >= 8) continue;
        int x = i % W;
        int z = i / W;
        int nx = x + dx8[dir];
        int nz = z + dz8[dir];
        if (nx >= 0 && nx < W && nz >= 0 && nz < H) {
            region.flowAccum[idx(nx, nz)] += region.flowAccum[i];
        }
    }
    
    // ========================================
    // STEP 4: Priority-flood for lakes
    // ========================================
    struct Cell { int x, z; float level; };
    auto cmp = [](const Cell& a, const Cell& b) { return a.level > b.level; };
    std::priority_queue<Cell, std::vector<Cell>, decltype(cmp)> pq(cmp);
    
    std::vector<bool> visited(N, false);
    std::vector<float> filled(N, 0.0f);
    
    // Initialize from edges
    for (int x = 0; x < W; ++x) {
        for (int z : {0, H-1}) {
            int i = idx(x, z);
            pq.push({x, z, ground[i]});
            visited[i] = true;
            filled[i] = ground[i];
        }
    }
    for (int z = 1; z < H-1; ++z) {
        for (int x : {0, W-1}) {
            int i = idx(x, z);
            pq.push({x, z, ground[i]});
            visited[i] = true;
            filled[i] = ground[i];
        }
    }
    
    while (!pq.empty()) {
        Cell c = pq.top();
        pq.pop();
        for (int k = 0; k < 4; ++k) {
            int nx = c.x + dx4[k];
            int nz = c.z + dz4[k];
            if (nx < 0 || nx >= W || nz < 0 || nz >= H) continue;
            int ni = idx(nx, nz);
            if (visited[ni]) continue;
            visited[ni] = true;
            filled[ni] = std::max(ground[ni], c.level);
            pq.push({nx, nz, filled[ni]});
        }
    }
    
    // ========================================
    // STEP 5: Identify lakes
    // ========================================
    const float minLakeDepth = erosionConfig.waterMinDepth;
    std::vector<bool> isLake(N, false);
    
    for (int i = 0; i < N; ++i) {
        float depth = filled[i] - ground[i];
        if (depth >= minLakeDepth) {
            isLake[i] = true;
            region.waterLevels[i] = filled[i];
        }
    }
    
    // Remove tiny lakes (< 6 tiles)
    std::vector<int> lakeLabel(N, -1);
    std::vector<int> lakeSizes;
    int nextLabel = 0;
    
    for (int i = 0; i < N; ++i) {
        if (!isLake[i] || lakeLabel[i] >= 0) continue;
        
        std::queue<int> q;
        q.push(i);
        lakeLabel[i] = nextLabel;
        int size = 0;
        
        while (!q.empty()) {
            int cur = q.front();
            q.pop();
            size++;
            int cx = cur % W, cz = cur / W;
            for (int k = 0; k < 4; ++k) {
                int nx = cx + dx4[k];
                int nz = cz + dz4[k];
                if (nx < 0 || nx >= W || nz < 0 || nz >= H) continue;
                int ni = idx(nx, nz);
                if (isLake[ni] && lakeLabel[ni] < 0) {
                    lakeLabel[ni] = nextLabel;
                    q.push(ni);
                }
            }
        }
        lakeSizes.push_back(size);
        nextLabel++;
    }
    
    for (int i = 0; i < N; ++i) {
        if (lakeLabel[i] >= 0 && lakeSizes[lakeLabel[i]] < 6) {
            isLake[i] = false;
            region.waterLevels[i] = 0.0f;
        }
    }
    
    // ========================================
    // STEP 6: Rivers disabled for now
    // ========================================
    // River generation was causing visual artifacts and discontinuities.
    // Lakes work well, rivers need a rethink.
    
    // Just ensure all river-related data is zeroed
    std::fill(region.riverWidth.begin(), region.riverWidth.end(), 0);
    
    region.waterGenerated = true;
}
