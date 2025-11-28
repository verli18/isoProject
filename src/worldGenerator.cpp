#include "../include/worldGenerator.hpp"
#include <cmath>
#include <algorithm>
#include <queue>

WorldGenerator& WorldGenerator::getInstance() {
    static WorldGenerator instance;
    return instance;
}

void WorldGenerator::initialize(int seed) {
    config.seed = seed;
    initialize(config);
}

void WorldGenerator::initialize(const WorldGenConfig& cfg) {
    config = cfg;
    rebuildNoiseGenerators();
    initialized = true;
}

void WorldGenerator::rebuildNoiseGenerators() {
    // === Terrain Height ===
    // Main height noise with domain warp for organic shapes
    // Using fewer octaves and lower gain for smoother terrain
    noiseHeight = FastNoise::New<FastNoise::FractalFBm>();
    noiseHeight->SetOctaveCount(3);      // Reduced from 4 - less high-frequency detail
    noiseHeight->SetGain(0.4f);          // Reduced from 0.5 - smoother falloff
    noiseHeight->SetLacunarity(2.2f);    // Slightly higher - more separation between octaves
    
    noiseWarp = FastNoise::New<FastNoise::DomainWarpGradient>();
    noiseWarp->SetSource(FastNoise::New<FastNoise::Simplex>());
    noiseWarp->SetWarpAmplitude(config.warpAmplitude);
    noiseWarp->SetWarpFrequency(config.warpFrequency);
    
    noiseHeight->SetSource(noiseWarp);
    
    // Region variation (large-scale continental shapes)
    noiseRegion = FastNoise::New<FastNoise::FractalFBm>();
    noiseRegion->SetSource(FastNoise::New<FastNoise::Simplex>());
    noiseRegion->SetOctaveCount(2);      // Reduced from 3 - broader regions
    noiseRegion->SetGain(0.4f);          // Reduced from 0.5
    noiseRegion->SetLacunarity(2.0f);
    
    // === Potentials ===
    
    // Magmatic: low frequency, represents tectonic/volcanic regions
    noiseMagmatic = createFBmNoise(2, 0.4f, 2.0f);    // Reduced octaves
    
    // Hydrological: medium frequency with some warp
    noiseHydrological = createFBmNoise(2, 0.4f, 2.5f); // Reduced octaves
    
    // Sulfide: deposits along fault lines
    noiseSulfide = createFBmNoise(2, 0.5f, 2.0f);      // Reduced from 4 octaves
    
    // Crystalline: ridged noise for vein-like patterns
    noiseCrystalline = FastNoise::New<FastNoise::FractalRidged>();
    noiseCrystalline->SetSource(FastNoise::New<FastNoise::Simplex>());
    noiseCrystalline->SetOctaveCount(2);  // Reduced from 3
    noiseCrystalline->SetGain(0.5f);      // Reduced from 0.6
    noiseCrystalline->SetLacunarity(2.0f);
    
    // Biological: mixed frequencies for varied ecosystems
    noiseBiological = createFBmNoise(2, 0.4f, 2.0f);   // Reduced from 4 octaves
    
    // === Climate ===
    // Very smooth climate gradients - only 1 octave for large coherent regions
    noiseTemperature = createFBmNoise(1, 0.5f, 2.0f);  // Single octave!
    noiseHumidity = createFBmNoise(1, 0.5f, 2.0f);     // Single octave!
}

FastNoise::SmartNode<FastNoise::FractalFBm> WorldGenerator::createFBmNoise(
    int octaves, float gain, float lacunarity
) const {
    auto noise = FastNoise::New<FastNoise::FractalFBm>();
    noise->SetSource(FastNoise::New<FastNoise::Simplex>());
    noise->SetOctaveCount(octaves);
    noise->SetGain(gain);
    noise->SetLacunarity(lacunarity);
    return noise;
}

PotentialData WorldGenerator::getPotentialAt(float worldX, float worldZ) const {
    PotentialData p;
    
    // All noise returns -1 to 1, normalize to 0-1
    auto norm = [](float v) { return (v + 1.0f) * 0.5f; };
    
    // Sample each potential with different frequency offsets for variation
    float magVal = noiseMagmatic->GenSingle2D(worldX * config.potentialFreq, worldZ * config.potentialFreq, config.seed);
    float hydroVal = noiseHydrological->GenSingle2D(worldX * config.potentialFreq * 0.8f, worldZ * config.potentialFreq * 0.8f, config.seed + 1000);
    float sulfVal = noiseSulfide->GenSingle2D(worldX * config.potentialFreq * 1.5f, worldZ * config.potentialFreq * 1.5f, config.seed + 2000);
    float crystVal = noiseCrystalline->GenSingle2D(worldX * config.potentialFreq * 1.2f, worldZ * config.potentialFreq * 1.2f, config.seed + 3000);
    float bioVal = noiseBiological->GenSingle2D(worldX * config.potentialFreq, worldZ * config.potentialFreq, config.seed + 4000);
    
    // Climate
    float tempVal = noiseTemperature->GenSingle2D(worldX * config.climateFreq, worldZ * config.climateFreq, config.seed + 5000);
    float humVal = noiseHumidity->GenSingle2D(worldX * config.climateFreq, worldZ * config.climateFreq, config.seed + 6000);
    
    p.magmatic = norm(magVal);
    p.hydrological = norm(hydroVal);
    p.sulfide = norm(sulfVal);
    p.crystalline = norm(crystVal);
    p.biological = norm(bioVal);
    p.temperature = norm(tempVal);
    p.humidity = norm(humVal);
    
    return p;
}

void WorldGenerator::generatePotentialGrid(
    std::vector<PotentialData>& out,
    int startX, int startZ,
    int width, int height
) const {
    const int count = width * height;
    out.resize(count);
    
    // Generate all noise maps in bulk for efficiency
    std::vector<float> magmaticMap(count);
    std::vector<float> hydrologicalMap(count);
    std::vector<float> sulfideMap(count);
    std::vector<float> crystallineMap(count);
    std::vector<float> biologicalMap(count);
    std::vector<float> temperatureMap(count);
    std::vector<float> humidityMap(count);
    
    noiseMagmatic->GenUniformGrid2D(magmaticMap.data(), startX, startZ, width, height, 
                                     config.potentialFreq, config.seed);
    noiseHydrological->GenUniformGrid2D(hydrologicalMap.data(), startX, startZ, width, height,
                                         config.potentialFreq * 0.8f, config.seed + 1000);
    noiseSulfide->GenUniformGrid2D(sulfideMap.data(), startX, startZ, width, height,
                                    config.potentialFreq * 1.5f, config.seed + 2000);
    noiseCrystalline->GenUniformGrid2D(crystallineMap.data(), startX, startZ, width, height,
                                        config.potentialFreq * 1.2f, config.seed + 3000);
    noiseBiological->GenUniformGrid2D(biologicalMap.data(), startX, startZ, width, height,
                                       config.potentialFreq, config.seed + 4000);
    noiseTemperature->GenUniformGrid2D(temperatureMap.data(), startX, startZ, width, height,
                                        config.climateFreq, config.seed + 5000);
    noiseHumidity->GenUniformGrid2D(humidityMap.data(), startX, startZ, width, height,
                                     config.climateFreq, config.seed + 6000);
    
    // Normalize and pack into PotentialData
    for (int i = 0; i < count; ++i) {
        out[i].magmatic = (magmaticMap[i] + 1.0f) * 0.5f;
        out[i].hydrological = (hydrologicalMap[i] + 1.0f) * 0.5f;
        out[i].sulfide = (sulfideMap[i] + 1.0f) * 0.5f;
        out[i].crystalline = (crystallineMap[i] + 1.0f) * 0.5f;
        out[i].biological = (biologicalMap[i] + 1.0f) * 0.5f;
        out[i].temperature = (temperatureMap[i] + 1.0f) * 0.5f;
        out[i].humidity = (humidityMap[i] + 1.0f) * 0.5f;
    }
}

float WorldGenerator::getBaseHeightAt(float worldX, float worldZ) const {
    // Sample height with region modulation
    float heightVal = noiseHeight->GenSingle2D(worldX * config.terrainFreq, worldZ * config.terrainFreq, config.seed);
    float regionVal = noiseRegion->GenSingle2D(worldX * config.regionFreq, worldZ * config.regionFreq, config.seed);
    
    // Combine: region modulates height amplitude
    float combined = heightVal * ((regionVal + 1.0f) * 0.5f);
    
    // Normalize to 0-1, apply exponent, scale
    float normalized = (combined + 1.0f) * 0.5f;
    float shaped = std::pow(normalized, config.heightExponent);
    
    return config.heightBase + shaped * config.heightScale;
}

void WorldGenerator::generateHeightGrid(
    std::vector<float>& out,
    int startX, int startZ,
    int width, int height
) const {
    const int count = width * height;
    out.resize(count);
    
    std::vector<float> heightMap(count);
    std::vector<float> regionMap(count);
    
    noiseHeight->GenUniformGrid2D(heightMap.data(), startX, startZ, width, height,
                                   config.terrainFreq, config.seed);
    noiseRegion->GenUniformGrid2D(regionMap.data(), startX, startZ, width, height,
                                   config.regionFreq, config.seed);
    
    for (int i = 0; i < count; ++i) {
        float combined = heightMap[i] * ((regionMap[i] + 1.0f) * 0.5f);
        float normalized = (combined + 1.0f) * 0.5f;
        float shaped = std::pow(normalized, config.heightExponent);
        out[i] = config.heightBase + shaped * config.heightScale;
    }
}

TerrainAnalysis WorldGenerator::analyzeTerrainAt(
    const std::vector<float>& heights,
    int x, int y,
    int gridWidth
) const {
    TerrainAnalysis analysis = {0.0f, 0.0f, 0.0f};
    
    auto idx = [gridWidth](int x, int y) { return y * gridWidth + x; };
    int gridHeight = heights.size() / gridWidth;
    
    // Bounds check
    if (x <= 0 || x >= gridWidth - 1 || y <= 0 || y >= gridHeight - 1) {
        return analysis;
    }
    
    float center = heights[idx(x, y)];
    float left = heights[idx(x - 1, y)];
    float right = heights[idx(x + 1, y)];
    float up = heights[idx(x, y - 1)];
    float down = heights[idx(x, y + 1)];
    
    // Slope: magnitude of gradient
    float dx = (right - left) * 0.5f;
    float dz = (down - up) * 0.5f;
    analysis.slope = std::sqrt(dx * dx + dz * dz);
    analysis.slope = std::min(analysis.slope / 2.0f, 1.0f);  // Normalize, max slope ~2 units
    
    // Curvature: second derivative (laplacian)
    float laplacian = (left + right + up + down) - 4.0f * center;
    analysis.curvature = std::clamp(laplacian / 4.0f, -1.0f, 1.0f);
    
    // Flow accumulation: simplified - just use how much lower neighbors are
    float flowIn = 0.0f;
    if (left > center) flowIn += left - center;
    if (right > center) flowIn += right - center;
    if (up > center) flowIn += up - center;
    if (down > center) flowIn += down - center;
    analysis.flowAccum = std::min(flowIn / 4.0f, 1.0f);
    
    return analysis;
}

void WorldGenerator::applyFeedbackLoop(
    std::vector<PotentialData>& potentials,
    const std::vector<float>& heights,
    int width, int height
) const {
    // Heights grid is (width+1) x (height+1) for corners
    int heightGridWidth = width + 1;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int potIdx = y * width + x;
            
            // Analyze terrain at tile center (approximate with corner)
            TerrainAnalysis analysis = analyzeTerrainAt(heights, x + 1, y + 1, heightGridWidth);
            
            PotentialData& p = potentials[potIdx];
            
            // Erosion exposure: steep slopes reveal sulfide and crystalline
            p.sulfide = std::clamp(p.sulfide + analysis.slope * config.slopeToSulfide, 0.0f, 1.0f);
            p.crystalline = std::clamp(p.crystalline + analysis.slope * config.slopeToCrystalline, 0.0f, 1.0f);
            
            // Sediment deposition: valleys boost biological and hydrological
            if (analysis.curvature < 0) {  // Concave = valley
                float valleyBoost = -analysis.curvature;
                p.biological = std::clamp(p.biological + valleyBoost * config.flowToBiological, 0.0f, 1.0f);
                p.hydrological = std::clamp(p.hydrological + valleyBoost * config.flowToHydrological, 0.0f, 1.0f);
            }
            
            // Flow accumulation boosts hydrological and biological
            p.hydrological = std::clamp(p.hydrological + analysis.flowAccum * config.flowToHydrological, 0.0f, 1.0f);
            p.biological = std::clamp(p.biological + analysis.flowAccum * config.flowToBiological * 0.5f, 0.0f, 1.0f);
            
            // Peaks reduce biological potential
            if (analysis.curvature > 0.3f) {
                p.biological *= (1.0f - analysis.curvature * 0.5f);
            }
        }
    }
}

float WorldGenerator::sampleNoise(int noiseType, float x, float z, float frequency) const {
    switch (noiseType) {
        case 0: return noiseHeight->GenSingle2D(x * frequency, z * frequency, config.seed);
        case 1: return noiseRegion->GenSingle2D(x * frequency, z * frequency, config.seed);
        case 2: return noiseMagmatic->GenSingle2D(x * frequency, z * frequency, config.seed);
        case 3: return noiseHydrological->GenSingle2D(x * frequency, z * frequency, config.seed + 1000);
        case 4: return noiseSulfide->GenSingle2D(x * frequency, z * frequency, config.seed + 2000);
        case 5: return noiseCrystalline->GenSingle2D(x * frequency, z * frequency, config.seed + 3000);
        case 6: return noiseBiological->GenSingle2D(x * frequency, z * frequency, config.seed + 4000);
        case 7: return noiseTemperature->GenSingle2D(x * frequency, z * frequency, config.seed + 5000);
        case 8: return noiseHumidity->GenSingle2D(x * frequency, z * frequency, config.seed + 6000);
        default: return 0.0f;
    }
}

float WorldGenerator::getWaterLevelAt(float worldX, float worldZ, float groundHeight) const {
    // Simple per-tile water check - main logic is in generateWaterGrid
    float hydroVal = noiseHydrological->GenSingle2D(
        worldX * config.potentialFreq * 0.5f, 
        worldZ * config.potentialFreq * 0.5f, 
        config.seed + 1000
    );
    float hydro = (hydroVal + 1.0f) * 0.5f;
    
    // Very low threshold for testing - more water
    if (hydro < 0.35f) {
        return 0.0f;
    }
    
    // Water at ground level + small offset
    return groundHeight + 0.5f;
}

void WorldGenerator::generateWaterGrid(
    std::vector<float>& waterLevels,
    const std::vector<float>& groundHeights,
    const std::vector<PotentialData>& potentials,
    int startX, int startZ,
    int width, int height
) const {
    const int count = width * height;
    waterLevels.resize(count);
    
    // First pass: compute average ground heights for each tile
    std::vector<float> tileGroundHeight(count);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            int cornerIdx = y * (width + 1) + x;
            tileGroundHeight[i] = (groundHeights[cornerIdx] + 
                                   groundHeights[cornerIdx + 1] +
                                   groundHeights[cornerIdx + width + 1] +
                                   groundHeights[cornerIdx + width + 2]) * 0.25f;
        }
    }
    
    // Generate hydrological potential grid efficiently using bulk generation
    std::vector<float> hydroGrid(count);
    std::vector<float> tempGrid(count);
    std::vector<float> humidGrid(count);
    
    noiseHydrological->GenUniformGrid2D(
        hydroGrid.data(), startX, startZ, width, height,
        config.potentialFreq * 0.5f, config.seed + 1000
    );
    noiseTemperature->GenUniformGrid2D(
        tempGrid.data(), startX, startZ, width, height,
        config.climateFreq, config.seed + 5000
    );
    noiseHumidity->GenUniformGrid2D(
        humidGrid.data(), startX, startZ, width, height,
        config.climateFreq, config.seed + 6000
    );
    
    // Second pass: determine water presence based on hydrological potential
    std::vector<bool> hasWater(count, false);
    for (int i = 0; i < count; ++i) {
        float hydro = (hydroGrid[i] + 1.0f) * 0.5f;
        float temperature = (tempGrid[i] + 1.0f) * 0.5f;
        float humidity = (humidGrid[i] + 1.0f) * 0.5f;
        
        // Climate-based threshold - lower = more water
        float evaporation = std::max(0.0f, (temperature - 0.5f) * 1.0f);
        float dryness = 1.0f - humidity;
        float waterThreshold = 0.40f + dryness * 0.1f + evaporation * 0.1f;
        
        hasWater[i] = (hydro >= waterThreshold);
    }
    
    // Third pass: flood-fill to find connected water regions and their levels
    // Use priority-flood from edges to fill depressions
    struct PFCell { int x, y; float level; };
    struct PFCmp { 
        bool operator()(const PFCell& a, const PFCell& b) const { 
            return a.level > b.level; 
        } 
    };
    std::priority_queue<PFCell, std::vector<PFCell>, PFCmp> pq;
    std::vector<bool> visited(count, false);
    std::vector<float> filledLevel(count, 0.0f);
    
    auto idx = [width](int x, int y) { return y * width + x; };
    
    // Seed edges
    for (int x = 0; x < width; ++x) {
        int i0 = idx(x, 0);
        int i1 = idx(x, height - 1);
        pq.push({x, 0, tileGroundHeight[i0]});
        pq.push({x, height - 1, tileGroundHeight[i1]});
        visited[i0] = visited[i1] = true;
        filledLevel[i0] = tileGroundHeight[i0];
        filledLevel[i1] = tileGroundHeight[i1];
    }
    for (int y = 1; y < height - 1; ++y) {
        int i0 = idx(0, y);
        int i1 = idx(width - 1, y);
        pq.push({0, y, tileGroundHeight[i0]});
        pq.push({width - 1, y, tileGroundHeight[i1]});
        visited[i0] = visited[i1] = true;
        filledLevel[i0] = tileGroundHeight[i0];
        filledLevel[i1] = tileGroundHeight[i1];
    }
    
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    
    while (!pq.empty()) {
        PFCell c = pq.top();
        pq.pop();
        
        for (int k = 0; k < 4; ++k) {
            int nx = c.x + dx[k];
            int ny = c.y + dy[k];
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
            
            int ni = idx(nx, ny);
            if (visited[ni]) continue;
            visited[ni] = true;
            
            float ground = tileGroundHeight[ni];
            float filled = std::max(ground, c.level);
            filledLevel[ni] = filled;
            pq.push({nx, ny, filled});
        }
    }
    
    // Fourth pass: assign water levels where there are depressions AND hydrological potential
    for (int i = 0; i < count; ++i) {
        float depth = filledLevel[i] - tileGroundHeight[i];
        
        if (hasWater[i] && depth > 0.2f) {
            // Water surface is the filled level, quantized
            float waterSurface = std::round(filledLevel[i] * 2.0f) / 2.0f;
            waterLevels[i] = waterSurface;
        } else {
            waterLevels[i] = 0.0f;
        }
    }
}

void WorldGenerator::applyErosion(
    std::vector<float>& heights,
    int startX, int startZ,
    int width, int height,
    int numDroplets
) const {
    // Erosion parameters
    const float inertia = 0.1f;           // How much velocity is preserved (0-1)
    const float sedimentCapacityFactor = 4.0f;  // Multiplier for sediment capacity
    const float minSedimentCapacity = 0.01f;
    const float erodeSpeed = 0.3f;        // Erosion rate
    const float depositSpeed = 0.3f;      // Deposition rate
    const float evaporateSpeed = 0.02f;   // Water evaporation per step
    const float gravity = 4.0f;           // Affects velocity from height difference
    const int maxDropletLifetime = 50;    // Max steps per droplet
    const float initialWaterVolume = 1.0f;
    const int erosionRadius = 3;          // Radius of erosion brush
    
    // Precompute erosion brush weights (gaussian-like falloff)
    std::vector<std::pair<int, int>> brushOffsets;
    std::vector<float> brushWeights;
    float weightSum = 0.0f;
    
    for (int dy = -erosionRadius; dy <= erosionRadius; ++dy) {
        for (int dx = -erosionRadius; dx <= erosionRadius; ++dx) {
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= erosionRadius) {
                brushOffsets.push_back({dx, dy});
                float w = std::max(0.0f, erosionRadius - dist);
                brushWeights.push_back(w);
                weightSum += w;
            }
        }
    }
    // Normalize weights
    for (float& w : brushWeights) {
        w /= weightSum;
    }
    
    // Helper to check if a local grid position is within bounds
    auto inBounds = [&](int x, int y) -> bool {
        return x >= 0 && x < width && y >= 0 && y < height;
    };
    
    // Helper to get height with bilinear interpolation (local grid only now)
    auto getHeight = [&](float localX, float localY) -> float {
        int x0 = static_cast<int>(std::floor(localX));
        int y0 = static_cast<int>(std::floor(localY));
        
        // Clamp to valid range for interpolation
        x0 = std::clamp(x0, 0, width - 2);
        y0 = std::clamp(y0, 0, height - 2);
        
        float fx = std::clamp(localX - x0, 0.0f, 1.0f);
        float fy = std::clamp(localY - y0, 0.0f, 1.0f);
        
        float h00 = heights[y0 * width + x0];
        float h10 = heights[y0 * width + x0 + 1];
        float h01 = heights[(y0 + 1) * width + x0];
        float h11 = heights[(y0 + 1) * width + x0 + 1];
        
        return h00 * (1 - fx) * (1 - fy) + 
               h10 * fx * (1 - fy) + 
               h01 * (1 - fx) * fy + 
               h11 * fx * fy;
    };
    
    // Helper to compute gradient at position
    auto computeGradient = [&](float localX, float localY) -> std::pair<float, float> {
        int x0 = static_cast<int>(std::floor(localX));
        int y0 = static_cast<int>(std::floor(localY));
        
        x0 = std::clamp(x0, 0, width - 2);
        y0 = std::clamp(y0, 0, height - 2);
        
        float fx = std::clamp(localX - x0, 0.0f, 1.0f);
        float fy = std::clamp(localY - y0, 0.0f, 1.0f);
        
        float h00 = heights[y0 * width + x0];
        float h10 = heights[y0 * width + x0 + 1];
        float h01 = heights[(y0 + 1) * width + x0];
        float h11 = heights[(y0 + 1) * width + x0 + 1];
        
        // Analytical gradient from bilinear interpolation
        float gx = (h10 - h00) * (1 - fy) + (h11 - h01) * fy;
        float gy = (h01 - h00) * (1 - fx) + (h11 - h10) * fx;
        
        return {gx, gy};
    };
    
    // Simple random number generator (deterministic based on config seed AND chunk position)
    uint32_t rngState = static_cast<uint32_t>(config.seed + startX * 73856093 + startZ * 19349663);
    auto nextRandom = [&rngState]() -> float {
        rngState = rngState * 1103515245 + 12345;
        return static_cast<float>((rngState >> 16) & 0x7FFF) / 32767.0f;
    };
    
    // Margin from edges where droplets can spawn (erosion radius + 1)
    const int spawnMargin = erosionRadius + 1;
    const float spawnWidth = static_cast<float>(width - 2 * spawnMargin);
    const float spawnHeight = static_cast<float>(height - 2 * spawnMargin);
    
    if (spawnWidth <= 0 || spawnHeight <= 0) return;  // Grid too small
    
    // Simulate droplets
    for (int drop = 0; drop < numDroplets; ++drop) {
        // Random starting position (uniform across spawnable area)
        float posX = nextRandom() * spawnWidth + spawnMargin;
        float posY = nextRandom() * spawnHeight + spawnMargin;
        float dirX = 0.0f;
        float dirY = 0.0f;
        float speed = 0.0f;  // Start with zero speed - gravity will accelerate
        float water = initialWaterVolume;
        float sediment = 0.0f;
        
        for (int lifetime = 0; lifetime < maxDropletLifetime; ++lifetime) {
            int nodeX = static_cast<int>(posX);
            int nodeY = static_cast<int>(posY);
            float cellOffsetX = posX - nodeX;
            float cellOffsetY = posY - nodeY;
            
            // Check if we're still in bounds for modification
            if (!inBounds(nodeX, nodeY) || !inBounds(nodeX + 1, nodeY + 1)) {
                break;
            }
            
            // Get current height and gradient
            float heightOld = getHeight(posX, posY);
            auto [gradX, gradY] = computeGradient(posX, posY);
            
            // Update direction with inertia (blend between current direction and downhill)
            dirX = dirX * inertia - gradX * (1 - inertia);
            dirY = dirY * inertia - gradY * (1 - inertia);
            
            // Normalize direction
            float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
            if (dirLen < 0.0001f) {
                // Random direction if on flat terrain
                float angle = nextRandom() * 6.28318f;
                dirX = std::cos(angle);
                dirY = std::sin(angle);
                dirLen = 1.0f;
            } else {
                dirX /= dirLen;
                dirY /= dirLen;
            }
            
            // Move to new position
            float newPosX = posX + dirX;
            float newPosY = posY + dirY;
            
            // Check bounds before sampling new height
            if (newPosX < 0 || newPosX >= width - 1 || 
                newPosY < 0 || newPosY >= height - 1) {
                break;
            }
            
            // Get new height
            float heightNew = getHeight(newPosX, newPosY);
            float deltaHeight = heightNew - heightOld;
            
            // Calculate sediment capacity (based on slope and speed)
            float sedimentCapacity = std::max(
                -deltaHeight * speed * water * sedimentCapacityFactor,
                minSedimentCapacity
            );
            
            // Erode or deposit
            if (sediment > sedimentCapacity || deltaHeight > 0) {
                // Deposit sediment (going uphill or over capacity)
                float amountToDeposit = (deltaHeight > 0) 
                    ? std::min(deltaHeight, sediment)
                    : (sediment - sedimentCapacity) * depositSpeed;
                
                sediment -= amountToDeposit;
                
                // Deposit at the 4 corners of the cell (bilinear distribution)
                int idx00 = nodeY * width + nodeX;
                heights[idx00] += amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY);
                heights[idx00 + 1] += amountToDeposit * cellOffsetX * (1 - cellOffsetY);
                heights[idx00 + width] += amountToDeposit * (1 - cellOffsetX) * cellOffsetY;
                heights[idx00 + width + 1] += amountToDeposit * cellOffsetX * cellOffsetY;
            } else {
                // Erode terrain (going downhill with capacity)
                float amountToErode = std::min(
                    (sedimentCapacity - sediment) * erodeSpeed,
                    -deltaHeight  // Don't erode more than the height difference
                );
                
                // Apply erosion using brush
                for (size_t i = 0; i < brushOffsets.size(); ++i) {
                    int erodeX = nodeX + brushOffsets[i].first;
                    int erodeY = nodeY + brushOffsets[i].second;
                    
                    if (inBounds(erodeX, erodeY)) {
                        int idx = erodeY * width + erodeX;
                        float erodeAmount = amountToErode * brushWeights[i];
                        heights[idx] -= erodeAmount;
                        sediment += erodeAmount;
                    }
                }
            }
            
            // Update speed based on height change (gravity acceleration)
            speed = std::sqrt(std::max(0.0f, speed * speed - deltaHeight * gravity));
            
            // Evaporate water
            water *= (1 - evaporateSpeed);
            
            // Update position
            posX = newPosX;
            posY = newPosY;
            
            // Stop if too little water
            if (water < 0.01f) {
                break;
            }
        }
    }
}
