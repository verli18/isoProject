#include "../include/tileGrid.hpp"
#include "../include/textureAtlas.hpp"
#include <cfloat>  // for FLT_MAX
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include "../include/resourceManager.hpp"  // use shared shader
#include <chrono>
#include <FastNoise/FastNoise.h>


#pragma GCC diagnostic ignored "-Wchar-subscripts"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
// Moved into tileGrid class

#include <map>
#include <queue>
#include <algorithm>

Color lerp(Color a, Color b, uint8_t t) {
    return Color{static_cast<unsigned char>(a.r + (b.r - a.r) * t / 255), static_cast<unsigned char>(a.g + (b.g - a.g) * t / 255), static_cast<unsigned char>(a.b + (b.b - a.b) * t / 255), 255};
}

// Helper to determine the dominant tile type among four tiles
char getDominantType4(tile t1, tile t2, tile t3, tile t4) {
    std::map<char, int> counts;
    counts[t1.type]++;
    counts[t2.type]++;
    counts[t3.type]++;
    counts[t4.type]++;

    char dominantType = t1.type;
    int maxCount = 0;
    // In case of a tie, prefer the original tile's type
    for (auto const& [type, count] : counts) {
        if (count > maxCount) {
            maxCount = count;
            dominantType = type;
        } else if (count == maxCount) {
            if (type == t1.type) {
                dominantType = type;
            }
        }
    }
    return dominantType;
}

// Helper to determine the dominant type among three chars
char getDominantType3(char c1, char c2, char c3) {
    if (c1 == c2 || c1 == c3) return c1;
    if (c2 == c3) return c2;
    // No majority, return the first one (tie-breaker)
    return c1;
}
tileGrid::tileGrid(int width, int height) : width(width), height(height) {
    grid.resize(width, std::vector<tile>(height));

    // The final generator that will be used for the heightmap
    fnFractal = FastNoise::New<FastNoise::FractalFBm>();

    // The warp generator
    fnWarp = FastNoise::New<FastNoise::DomainWarpGradient>();
    fnWarp->SetSource(FastNoise::New<FastNoise::Simplex>());

    // Apply the warp to the fractal
    fnFractal->SetSource(fnWarp);

    // Moisture map generator
    fnMoisture = FastNoise::New<FastNoise::FractalFBm>();
    fnMoisture->SetSource(FastNoise::New<FastNoise::Simplex>());
    
    // Temperature map generator
    fnTemperature = FastNoise::New<FastNoise::FractalFBm>();
    fnTemperature->SetSource(FastNoise::New<FastNoise::Simplex>());

    // Region map generator
    fnRegion = FastNoise::New<FastNoise::FractalFBm>();
    fnRegion->SetSource(FastNoise::New<FastNoise::Simplex>());

    // Magmatic potential map generator
    fnMagmatic = FastNoise::New<FastNoise::FractalFBm>();
    fnMagmatic->SetSource(FastNoise::New<FastNoise::Simplex>());

    // Biological potential map generator
    fnBiological = FastNoise::New<FastNoise::FractalFBm>();
    fnBiological->SetSource(FastNoise::New<FastNoise::Simplex>());

    // Sulfide potential map generator
    fnSulfide = FastNoise::New<FastNoise::FractalFBm>();
    fnSulfide->SetSource(FastNoise::New<FastNoise::Simplex>());

    // Crystaline potential map generator - using Cellular noise for chain-like patterns
    fnCrystaline = FastNoise::New<FastNoise::FractalRidged>();
    fnCrystaline->SetSource(FastNoise::New<FastNoise::Simplex>());
}

tileGrid::~tileGrid() {
}

void tileGrid::setTile(int x, int y, tile tile) {
    grid[x][y] = tile;
}

tile tileGrid::getTile(int x, int y) {
    return grid[x][y];
}

bool tileGrid::placeMachine(int x, int y, machine* machinePtr) {

    // Check if tile is already occupied
    if (grid[y][x].occupyingMachine != nullptr) {
        return false;
    }
    
    // Place the machine
    for(machineTileOffset offset : machinePtr->tileOffsets) {
        if (x + offset.x < 0 || x + offset.x >= width || y + offset.y < 0 || y + offset.y >= height) {
            return false;
        }
        if (grid[y + offset.y][x + offset.x].occupyingMachine != nullptr) {
            return false;
        }
    }
    
    // Place the machine
    for(machineTileOffset offset : machinePtr->tileOffsets) {
        grid[y + offset.y][x + offset.x].occupyingMachine = machinePtr;
    }

    return true;
}

machine* tileGrid::getMachineAt(int x, int y) {
    return grid[y][x].occupyingMachine;
}

void tileGrid::removeMachine(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }

    machine* machinePtr = grid[y][x].occupyingMachine;
    if (machinePtr == nullptr) {
        return; // No machine to remove
    }

    // For multi-tile machines, this assumes the passed (x, y) is the root tile.
    // This holds true for the current 1x1 machines.
    for (const auto& offset : machinePtr->tileOffsets) {
        int currentX = x + offset.x;
        int currentY = y + offset.y;
        if (currentX >= 0 && currentX < width && currentY >= 0 && currentY < height) {
            grid[currentY][currentX].occupyingMachine = nullptr;
        }
    }
}

void tileGrid::generatePerlinTerrain(float scale, int heightCo,
                                    int octaves, float persistence, float lacunarity, float exponent, int baseGenOffset[6]) {

    // Configure noise generators
    fnFractal->SetOctaveCount(octaves);
    fnFractal->SetGain(persistence);
    fnFractal->SetLacunarity(lacunarity);

    fnMoisture->SetOctaveCount(2);
    fnMoisture->SetGain(0.4f);
    fnMoisture->SetLacunarity(2.5f);

    fnTemperature->SetOctaveCount(2);
    fnTemperature->SetGain(0.4f);
    fnTemperature->SetLacunarity(2.5f);

    fnRegion->SetOctaveCount(3);
    fnRegion->SetGain(0.5f);
    fnRegion->SetLacunarity(2.0f);

    fnMagmatic->SetOctaveCount(2);
    fnMagmatic->SetGain(0.5f);
    fnMagmatic->SetLacunarity(2.0f);

    fnBiological->SetOctaveCount(3);
    fnBiological->SetGain(0.5f);
    fnBiological->SetLacunarity(2.0f);

    fnSulfide->SetOctaveCount(3);
    fnSulfide->SetGain(0.5f);
    fnSulfide->SetLacunarity(2.0f);

    fnCrystaline->SetOctaveCount(3);
    fnCrystaline->SetGain(0.6f);
    fnCrystaline->SetLacunarity(2.0f);

    // Configure Domain Warp
    fnWarp->SetWarpAmplitude(10.0f);
    fnWarp->SetWarpFrequency(0.02f);

    // Generate noise data using FastNoise2
    std::vector<float> heightMap( (width + 1) * (height + 1));
    fnFractal->GenUniformGrid2D(heightMap.data(), baseGenOffset[0], baseGenOffset[1], width + 1, height + 1, 0.02f * scale, 1337);

    std::vector<float> regionMap( (width + 1) * (height + 1));
    fnRegion->GenUniformGrid2D(regionMap.data(), baseGenOffset[0], baseGenOffset[1], width + 1, height + 1, 0.002f * scale, 1337);

    std::vector<float> moistureMap(width * height);
    fnMoisture->GenUniformGrid2D(moistureMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.005f * scale, 1338);

    std::vector<float> temperatureMap(width * height);
    fnTemperature->GenUniformGrid2D(temperatureMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.01f * scale, 1339);

    std::vector<float> magmaticMap(width * height);
    fnMagmatic->GenUniformGrid2D(magmaticMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.02f * scale, 1340);

    std::vector<float> biologicalMap(width * height);
    fnBiological->GenUniformGrid2D(biologicalMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.05f * scale, 1341);

    std::vector<float> sulfideMap(width * height);
    fnSulfide->GenUniformGrid2D(sulfideMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.01f * scale, 1342);

    std::vector<float> crystalineMap(width * height);
    fnCrystaline->GenUniformGrid2D(crystalineMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.02f * scale, 1343);

    // Preallocate helpers for hydrology
    const int N = width * height;
    
    std::vector<float> groundElev(N, 0.0f);
    std::vector<uint8_t> moist(N, 0);
    std::vector<uint8_t> temp(N, 0);
    std::vector<uint8_t> biol(N, 0);


    auto idx = [this](int x, int y) { return y * this->width + x; };

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t;
            
            // Get height values for the 4 corners of the tile
            float heightTL = heightMap[(y * (width + 1)) + x] * regionMap[(y * (width + 1)) + x];
            float heightTR = heightMap[(y * (width + 1)) + (x + 1)] * regionMap[(y * (width + 1)) + (x + 1)];
            float heightBL = heightMap[((y + 1) * (width + 1)) + x] * regionMap[((y + 1) * (width + 1)) + x];
            float heightBR = heightMap[((y + 1) * (width + 1)) + (x + 1)] * regionMap[((y + 1) * (width + 1)) + (x + 1)];

            // Apply exponent
            float fTL = std::pow((heightTL + 1.0f) / 2.0f, exponent);
            float fTR = std::pow((heightTR + 1.0f) / 2.0f, exponent);
            float fBR = std::pow((heightBR + 1.0f) / 2.0f, exponent);
            float fBL = std::pow((heightBL + 1.0f) / 2.0f, exponent);

            // Convert to heights with half-unit quantization
            t.tileHeight[0] = std::round(fTL * heightCo * 2.0f) / 2.0f;
            t.tileHeight[1] = std::round(fTR * heightCo * 2.0f) / 2.0f;
            t.tileHeight[2] = std::round(fBR * heightCo * 2.0f) / 2.0f;
            t.tileHeight[3] = std::round(fBL * heightCo * 2.0f) / 2.0f;
            
            // Clamp slopes
            for (int i = 0; i < 4; ++i) {
                for (int j = i + 1; j < 4; ++j) {
                    if (std::abs(t.tileHeight[i] - t.tileHeight[j]) > 1) {
                        if (t.tileHeight[i] < t.tileHeight[j]) t.tileHeight[j] = t.tileHeight[i] + 1;
                        else t.tileHeight[i] = t.tileHeight[j] + 1;
                    }
                }
            }

            float avgH = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            
            // Base moisture and temperature from noise
            float baseMoisture = (moistureMap[y * width + x] + 1.0f) / 2.0f * 255.0f;
            float baseTemperature = (temperatureMap[y * width + x] + 1.0f) / 2.0f * 255.0f;


            // Base crystaline potential from noise - enhanced for chain-like patterns
            float baseCrystaline = (crystalineMap[y * width + x] + 1.0f) / 2.0f; // Normalized 0-1
            
            // Enhance chain-like patterns by increasing contrast
            baseCrystaline = std::pow(baseCrystaline+0.2f, 2.5f); // Square to create more extreme values
            
            // Crystals form with magmatic activity but are enhanced at specific conditions
            float magmaticFactor = t.magmaticPotential / 255.0f;
            
            // Add a bias toward mid elevations (around 40-60 units)
            float elevationFactor = std::max(0.0f, 1.0f - std::abs(avgH - 50.0f) / 100.0f);
            
            // Combine with a bias toward the magmatic factor
            float finalCrystaline = baseCrystaline * (0.2f + 0.8f * magmaticFactor) * (0.7f + 0.3f * elevationFactor);
            t.crystalinePotential = static_cast<uint8_t>(std::round(std::clamp(finalCrystaline, 0.0f, 1.0f) * 255.0f));
            // Modify by altitude
            const float altitudeEffect = avgH * 2.0f;
            float finalMoisture = baseMoisture - altitudeEffect;
            float finalTemperature = baseTemperature - altitudeEffect;

            // Clamp values
            t.moisture = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, finalMoisture)));
            t.temperature = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, finalTemperature)));

            // Magmatic potential influenced by slope
            float min_h = t.tileHeight[0];
            float max_h = t.tileHeight[0];
            for(int i = 1; i < 4; ++i) {
                if(t.tileHeight[i] < min_h) min_h = t.tileHeight[i];
                if(t.tileHeight[i] > max_h) max_h = t.tileHeight[i];
            }
            float slope = max_h - min_h;

            float baseMagmatic = (magmaticMap[y * width + x] + 1.0f) / 2.0f; // Normalized 0-1
            float modifiedMagmatic = baseMagmatic + slope * 0.35f; // Add 35% of the slope value
            t.magmaticPotential = static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, modifiedMagmatic)) * 255.0f);

            // Sulfide potential
            float baseSulfide = (sulfideMap[y * width + x] + 1.0f) / 2.0f; // Normalized 0-1
            t.sulfidePotential = static_cast<uint8_t>(std::round(std::clamp(baseSulfide, 0.0f, 1.0f) * 255.0f));

            // Biome selection
            if (t.temperature < 40) {
                t.type = SNOW; // Cold
            } else if (t.temperature > 200 && t.moisture < 70) {
                t.type = SAND; // Hot & Dry -> Desert
            } else if (t.moisture > 180 && t.temperature > 150) {
                t.type = GRASS; // Hot & Wet -> Jungle/Rainforest (using GRASS for now)
            } else {
                t.type = GRASS; // Default -> Temperate Plains/Forest
            }
            // Initialize; hydrology will set water/hydro later
            t.waterLevel = 0;
            t.hydrologicalPotential = 0;

            float avgHeight = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            groundElev[idx(x,y)] = avgHeight;
            moist[idx(x,y)] = t.moisture;
            temp[idx(x,y)] = t.temperature;

            setTile(x, y, t);
        }
    }

    // Hydrology pass: priority-flood depression filling on ground elevation
    struct PFCell { int x, y; float level; };
    struct Cmp { bool operator()(const PFCell& a, const PFCell& b) const { return a.level > b.level; } };
    std::priority_queue<PFCell, std::vector<PFCell>, Cmp> pq;
    std::vector<uint8_t> visited(N, 0);
    std::vector<float> filled(N, 0.0f);

    // Seed with border cells (act as outlets to outside world)
    for (int x = 0; x < width; ++x) {
        int i0 = idx(x, 0);
        int i1 = idx(x, height - 1);
        pq.push({x, 0, groundElev[i0]}); visited[i0] = 1; filled[i0] = groundElev[i0];
        pq.push({x, height - 1, groundElev[i1]}); visited[i1] = 1; filled[i1] = groundElev[i1];
    }
    for (int y = 1; y < height - 1; ++y) {
        int i0 = idx(0, y);
        int i1 = idx(width - 1, y);
        pq.push({0, y, groundElev[i0]}); visited[i0] = 1; filled[i0] = groundElev[i0];
        pq.push({width - 1, y, groundElev[i1]}); visited[i1] = 1; filled[i1] = groundElev[i1];
    }

    const int dx4[4] = {1,-1,0,0};
    const int dy4[4] = {0,0,1,-1};
    while (!pq.empty()) {
        PFCell c = pq.top(); pq.pop();
        for (int k = 0; k < 4; ++k) {
            int nx = c.x + dx4[k];
            int ny = c.y + dy4[k];
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
            int ni = idx(nx, ny);
            if (visited[ni]) continue;
            visited[ni] = 1;
            float g = groundElev[ni];
            float f = std::max(g, c.level);
            filled[ni] = f;
            pq.push({nx, ny, f});
        }
    }

    // Determine water presence using a climate-dependent minimum depth threshold
    std::vector<float> waterDepth(N, 0.0f);
    // Sea level mask (per-tile) to force baseline ocean water independent of local depressions
    std::vector<uint8_t> isSea(N, 0);
    float seaBase = waterParams.seaLevelThreshold; // Interpreted in same height units as tile heights
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            float rawDepth = std::max(0.0f, filled[i] - groundElev[i]);
            rawDepth *= std::max(0.0f, waterParams.waterAmountScale);
            float m = moist[i] / 255.0f;
            float tnorm = temp[i] / 255.0f;
            float evap = std::clamp((tnorm - waterParams.evapStart) / std::max(0.0001f, waterParams.evapRange), 0.0f, 1.0f);
            float dryness = (1.0f - m);
            // Threshold increases with dryness and evaporation pressure
            float minDepth = waterParams.minDepthBase
                           + waterParams.drynessCoeff * dryness
                           + waterParams.evapCoeff    * evap;
            waterDepth[i] = (rawDepth >= minDepth) ? rawDepth : 0.0f;
            // --- Baseline sea level application ---
            // If ground elevation is below configured sea level, guarantee water up to sea level.
            if (groundElev[i] < seaBase) {
                float baseline = seaBase - groundElev[i];
                if (baseline > waterDepth[i]) waterDepth[i] = baseline;
                if (baseline > 0.0f) isSea[i] = 1;
                biol[i] = 30;

            }
        }
    }

    // Flatten lakes: group contiguous water tiles (4-neigh) with similar candidate water surface and assign a plateau
    std::vector<float> waterSurfaceY(N, 0.0f);
    std::vector<int> comp(N, -1);
    int compId = 0;
    const float levelEps = 0.01f; // tolerance for grouping (in world height units)
    std::vector<int> q; q.reserve(N);
    
    // Candidate surface per cell: ground elevation + computed water depth (already includes sea baseline); ensures we never pull a lake below its filled spill level
    auto surfaceLevel = [&](int index)->float { return groundElev[index] + waterDepth[index]; };
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            if (waterDepth[i] <= 0.01f || comp[i] != -1) continue; // skip dry or already processed
            q.clear();
            q.push_back(i); comp[i] = compId;
            float sumLvl = 0.0f; int cnt = 0; size_t head = 0;
            while (head < q.size()) {
                int cur = q[head++];
                float curSurf = surfaceLevel(cur);
                sumLvl += curSurf; ++cnt;
                int cx = cur % width; int cy = cur / width;
                for (int k = 0; k < 4; ++k) {
                    int nx = cx + dx4[k];
                    int ny = cy + dy4[k];
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    int ni = idx(nx, ny);
                    if (comp[ni] != -1) continue;
                    if (waterDepth[ni] <= 0.01f) continue;
                    float nSurf = surfaceLevel(ni);
                    if (fabsf(nSurf - curSurf) <= levelEps) { // same lake plateau
                        comp[ni] = compId;
                        q.push_back(ni);
                    }
                }
            }
            float plateau = (cnt > 0) ? (sumLvl / (float)cnt) : surfaceLevel(i);
            plateau = roundf(plateau * 2.0f) / 2.0f; // quantize to half-unit increments
            for (int id : q) waterSurfaceY[id] = plateau;
            ++compId;
        }
    }

    // Apply padding to water levels to prevent exposed water tiles
    // Create a dilated version of water levels similar to mesh generation
    std::vector<uint8_t> waterLevelMap(N, 0);
    std::vector<uint8_t> dilatedWaterLevels(N, 0);
    
    // First pass: assign base water levels
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            if (waterDepth[i] > 0.01f) {
                float waterY = (comp[i] >= 0) ? waterSurfaceY[i] : surfaceLevel(i);
                int quantized = (int)roundf(waterY * 2.0f); // store in half units
                quantized = std::clamp(quantized, 0, heightCo * 2);
                waterLevelMap[i] = (uint8_t)quantized - 1;
                biol[i] = 30;
            } else {
                waterLevelMap[i] = 0;
            }
        }
    }
    
    // Apply morphological dilation for padding (similar to water mesh generation)
    const int waterPadding = std::max(1, waterParams.waterPad); // At least 1 tile of padding
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            uint8_t maxLevel = waterLevelMap[i];
            
            // If this tile doesn't have water, check neighbors for water to extend
            if (maxLevel == 0) {
                for (int ox = -waterPadding; ox <= waterPadding; ++ox) {
                    for (int oy = -waterPadding; oy <= waterPadding; ++oy) {
                        int nx = x + ox, ny = y + oy;
                        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                        uint8_t neighborLevel = waterLevelMap[idx(nx, ny)];
                        if (neighborLevel > maxLevel) {
                            maxLevel = neighborLevel;
                        }
                    }
                }
            }
            dilatedWaterLevels[i] = maxLevel;
        }
    }
    
    // Assign final water levels back to tiles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            tile t = getTile(x, y);
            t.waterLevel = dilatedWaterLevels[i];
            setTile(x, y, t);
        }
    }

    // Flow accumulation (D8) on filled surface to derive hydrologicalPotential
    const int dx8[8] = {1,1,0,-1,-1,-1,0,1};
    const int dy8[8] = {0,1,1,1,0,-1,-1,-1};
    std::vector<int> receiver(N, -1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            float elev = filled[i];
            float bestDrop = 0.0f;
            int bestIdx = -1;
            for (int k = 0; k < 8; ++k) {
                int nx = x + dx8[k];
                int ny = y + dy8[k];
                if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                int ni = idx(nx, ny);
                float drop = elev - filled[ni];
                if (drop > bestDrop) { bestDrop = drop; bestIdx = ni; }
            }                     
            receiver[i] = bestIdx; // -1 indicates pit/outlet
        }
    }

    // Topological order by filled elevation ascending
    std::vector<int> order(N);
    for (int i = 0; i < N; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b){ return filled[a] < filled[b]; });

    std::vector<float> acc(N, 1.0f); // uniform rainfall
    for (int i = 0; i < N; ++i) {
        int c = order[i];
        int r = receiver[c];
        if (r >= 0) acc[r] += acc[c];
    }

    float maxAcc = 0.0f; for (float a : acc) if (a > maxAcc) maxAcc = a;
    float logMax = logf(maxAcc + 1.0f);

    // Add slope weighting to make channels more coherent
    std::vector<float> slopeW(N, 0.0f);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            float here = groundElev[i];
            float minN = here;
            for (int k = 0; k < 8; ++k) {
                int nx = x + dx8[k];
                int ny = y + dy8[k];
                if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                minN = std::min(minN, groundElev[idx(nx, ny)]);
            }
            float s = std::max(0.0f, here - minN); // local drop
            slopeW[i] = std::clamp(s / 2.0f, 0.0f, 1.0f); // normalize assuming <=2 units typical between neighbors
        }
    }

    std::vector<float> hydroN(N, 0.0f);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            float norm = (logf(acc[i] + 1.0f) / (logMax > 0 ? logMax : 1.0f));
            float m = moist[i] / 255.0f;
            float hydro = norm * (0.5f + 0.5f * m) * (0.4f + 0.6f * slopeW[i]);
            hydroN[i] = std::clamp(hydro, 0.0f, 1.0f);
        }
    }

    // Light 3x3 blur to smooth speckles (1 iteration)
    std::vector<float> hydroBlur(N, 0.0f);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f; int cnt = 0;
            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    int nx = x + ox, ny = y + oy;
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    sum += hydroN[idx(nx, ny)]; cnt++;
                }
            }
            hydroBlur[idx(x, y)] = sum / std::max(1, cnt);
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            tile t = getTile(x, y);
            t.hydrologicalPotential = (uint8_t)std::round(std::clamp(hydroBlur[idx(x, y)], 0.0f, 1.0f) * 255.0f) + biol[idx(x, y)];

            // --- Biological Potential ---
            float baseBio = (biologicalMap[idx(x, y)] + 1.0f) / 2.0f; // 0-1 from noise
            float moistureFactor = t.moisture / 127.0f; // 0-1 moisture factor
            float hydroFactor = t.hydrologicalPotential / 127.0f;

            // Temperature suitability (ideal around 128, falls off towards 0 and 255)
            float tempDist = std::abs(t.temperature - 128.0f) / 128.0f; // 0 at ideal, 1 at extremes
            float tempFactor = 1.0f - tempDist;

            // Combine factors
            float finalBio = baseBio * moistureFactor * (0.5f + hydroFactor * 0.5f) * tempFactor;
            
            t.biologicalPotential = (uint8_t)std::round(std::clamp(finalBio, 0.0f, 1.0f) * 255.0f);

            setTile(x, y, t);
        }
    }
}

// Ray-based tile picking: intersect ray with mesh of tile surfaces
Vector3 tileGrid::getTileIndexDDA(Ray ray) {
    Vector3 bestHitPos = { -1, -1, -1 };
    float minDistance = FLT_MAX;
    // Iterate over all tiles
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t = getTile(x, y);
            // Build tile surface triangles
            Vector3 v0 = { (float)x,     (float)t.tileHeight[0], (float)y     };
            Vector3 v1 = { (float)x + 1, (float)t.tileHeight[1], (float)y     };
            Vector3 v2 = { (float)x + 1, (float)t.tileHeight[2], (float)y + 1 };
            Vector3 v3 = { (float)x,     (float)t.tileHeight[3], (float)y + 1 };
            // Check collision for first triangle
            RayCollision hit = GetRayCollisionTriangle(ray, v0, v1, v2);
            if (hit.hit && hit.distance < minDistance) {
                minDistance = hit.distance;
                bestHitPos = hit.point;
            }
            // Check col-   lision for second triangle
            hit = GetRayCollisionTriangle(ray, v0, v2, v3);
            if (hit.hit && hit.distance < minDistance) {
                minDistance = hit.distance;
                bestHitPos = hit.point;
            }
        }
    }
    // If we hit a tile, convert world hit position to grid indices
    if (bestHitPos.x >= 0) {
        int ix = (int)std::floor(bestHitPos.x);
        int iy = (int)std::floor(bestHitPos.z);
        // Return discrete tile indices (z unused)
        return Vector3{ (float)ix, (float)iy, 0.0f };
    }
    // No hit: return original sentinel
    return bestHitPos;
}

void tileGrid::renderWires() {
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            tile t = getTile(x, y);
            // Define the four corner vertices in 3D space
            Vector3 v0 = {(float)x,     (float)t.tileHeight[0], (float)y};
            Vector3 v1 = {(float)x + 1, (float)t.tileHeight[1], (float)y};
            Vector3 v2 = {(float)x + 1, (float)t.tileHeight[2], (float)y + 1};
            Vector3 v3 = {(float)x,     (float)t.tileHeight[3], (float)y + 1};
            // Draw two triangles for the top face
            DrawLine3D(v2, v1, WHITE);
            DrawLine3D(v3, v2, WHITE);
            DrawLine3D(v3, v0, WHITE);
        }
    }
}

void tileGrid::renderDataPoint(Color a, Color b, uint8_t tile::*dataMember, int chunkX, int chunkY) {
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            tile t = getTile(x, y);
            Vector3 v0 = {(float)x + chunkX,     (float)t.tileHeight[0], (float)y + chunkY};
            Vector3 v1 = {(float)x + 1 + chunkX, (float)t.tileHeight[1], (float)y + chunkY};
            Vector3 v2 = {(float)x + 1 + chunkX, (float)t.tileHeight[2], (float)y + 1 + chunkY};
            Vector3 v3 = {(float)x + chunkX,     (float)t.tileHeight[3], (float)y + 1 + chunkY};
            
            DrawLine3D(v2, v1, lerp(a, b, t.*dataMember));
            DrawLine3D(v3, v2, lerp(a, b, t.*dataMember));
            DrawLine3D(v3, v0, lerp(a, b, t.*dataMember));
        }
    }
}

unsigned int tileGrid::getWidth() { return width; }
unsigned int tileGrid::getHeight() { return height; }
unsigned int tileGrid::getDepth() { return depth; }

void tileGrid::generateMesh() {
    // Assuming texture atlas dimensions (you may want to make these configurable)
    const float atlasWidth = 80.0f;  // Total atlas width in pixels
    const float atlasHeight = 16.0f; // Total atlas height in pixels
    
    std::vector<Vector3> vertices;
    std::vector<Vector2> texcoords;
    std::vector<Vector3> normals;

    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            tile t = getTile(x, y);
            
            // Skip air tiles
            if(t.type == AIR) continue;
            
            Vector3 v0 = {(float)x,     (float)t.tileHeight[0], (float)y};
            Vector3 v1 = {(float)x + 1, (float)t.tileHeight[1], (float)y};
            Vector3 v2 = {(float)x + 1, (float)t.tileHeight[2], (float)y + 1};
            Vector3 v3 = {(float)x,     (float)t.tileHeight[3], (float)y + 1};
            
            // Choose the mesh diagonal with the smallest height difference for a smoother look
            float diag1_diff = fabsf(t.tileHeight[0] - t.tileHeight[2]);
            float diag2_diff = fabsf(t.tileHeight[1] - t.tileHeight[3]);

            // --- Determine corner types ---
            tile current = getTile(x, y);
            tile top = (y > 0) ? getTile(x, y - 1) : current;
            tile left = (x > 0) ? getTile(x - 1, y) : current;
            tile topLeft = (x > 0 && y > 0) ? getTile(x - 1, y - 1) : current;
            tile right = (x < width - 1) ? getTile(x + 1, y) : current;
            tile topRight = (x < width - 1 && y > 0) ? getTile(x + 1, y - 1) : current;
            tile bottom = (y < height - 1) ? getTile(x, y + 1) : current;
            tile bottomLeft = (x > 0 && y < height - 1) ? getTile(x - 1, y + 1) : current;
            tile bottomRight = (x < width - 1 && y < height - 1) ? getTile(x + 1, y + 1) : current;

            char cornerTypeTL = getDominantType4(current, top, left, topLeft);
            char cornerTypeTR = getDominantType4(current, top, right, topRight);
            char cornerTypeBL = getDominantType4(current, bottom, left, bottomLeft);
            char cornerTypeBR = getDominantType4(current, bottom, right, bottomRight);

            if (diag1_diff <= diag2_diff) {
                // --- Split with diagonal v0-v2 ---
                textureAtlas t1 = textures[cornerTypeTR];
                textureAtlas t2 = textures[cornerTypeBL];

                float uMin1 = (float)t1.uOffset / atlasWidth, vMin1 = (float)t1.vOffset / atlasHeight;
                float uMax1 = (float)(t1.uOffset + t1.width) / atlasWidth, vMax1 = (float)(t1.vOffset + t1.height) / atlasHeight;
                float uMin2 = (float)t2.uOffset / atlasWidth, vMin2 = (float)t2.vOffset / atlasHeight;
                float uMax2 = (float)(t2.uOffset + t2.width) / atlasWidth, vMax2 = (float)(t2.vOffset + t2.height) / atlasHeight;

                // Triangle 1: v0, v1, v2
                vertices.push_back(v0); vertices.push_back(v1); vertices.push_back(v2);
                texcoords.push_back(Vector2{uMin1, vMin1}); texcoords.push_back(Vector2{uMax1, vMin1}); texcoords.push_back(Vector2{uMax1, vMax1});
                
                // Triangle 2: v0, v2, v3
                vertices.push_back(v0); vertices.push_back(v2); vertices.push_back(v3);
                texcoords.push_back(Vector2{uMin2, vMin2}); texcoords.push_back(Vector2{uMax2, vMax2}); texcoords.push_back(Vector2{uMin2, vMax2});

            } else {
                // --- Split with diagonal v1-v3 ---
                textureAtlas t1 = textures[cornerTypeBR];
                textureAtlas t2 = textures[cornerTypeTL];

                float uMin1 = (float)t1.uOffset / atlasWidth, vMin1 = (float)t1.vOffset / atlasHeight;
                float uMax1 = (float)(t1.uOffset + t1.width) / atlasWidth, vMax1 = (float)(t1.vOffset + t1.height) / atlasHeight;
                float uMin2 = (float)t2.uOffset / atlasWidth, vMin2 = (float)t2.vOffset / atlasHeight;
                float uMax2 = (float)(t2.uOffset + t2.width) / atlasWidth, vMax2 = (float)(t2.vOffset + t2.height) / atlasHeight;

                // Triangle 1: v1, v2, v3
                vertices.push_back(v1); vertices.push_back(v2); vertices.push_back(v3);
                texcoords.push_back(Vector2{uMax1, vMin1}); texcoords.push_back(Vector2{uMax1, vMax1}); texcoords.push_back(Vector2{uMin1, vMax1});

                // Triangle 2: v1, v3, v0
                vertices.push_back(v1); vertices.push_back(v3); vertices.push_back(v0);
                texcoords.push_back(Vector2{uMax2, vMin2}); texcoords.push_back(Vector2{uMin2, vMax2}); texcoords.push_back(Vector2{uMin2, vMin2});
            }

            // Normals are calculated once for the whole quad and applied to both triangles
            Vector3 n_edge1 = Vector3Subtract(v1, v0);
            Vector3 n_edge2 = Vector3Subtract(v3, v0);
            Vector3 normal = Vector3Normalize(Vector3CrossProduct(n_edge1, n_edge2));
            for(int i=0; i<6; ++i) normals.push_back(normal);
            
            // Generate side faces (walls) where there are height differences
            // Calculate UV coordinates for side faces
            float sideUMin = (float)textures[t.type].sideUOffset / atlasWidth;
            float sideVMin = (float)textures[t.type].sideVOffset / atlasHeight;
            float sideUMax = (float)(textures[t.type].sideUOffset + textures[t.type].width) / atlasWidth;
            float sideVMax = (float)(textures[t.type].sideVOffset + textures[t.type].height) / atlasHeight;
            
            // Check each edge for height differences and generate wall faces only where needed
            
            // Edge 0->1 (front edge) - check neighbor tile at y-1
            if (y > 0) {
                tile neighborTile = getTile(x, y - 1);
                // Create wall if this tile's edge is higher than neighbor's opposite edge
                if (t.tileHeight[0] > neighborTile.tileHeight[3] || t.tileHeight[1] > neighborTile.tileHeight[2]) {
                    Vector3 w0 = {(float)x,     (float)neighborTile.tileHeight[3], (float)y};
                    Vector3 w1 = {(float)x + 1, (float)neighborTile.tileHeight[2], (float)y};
                    
                    // Wall face (2 triangles)
                    vertices.push_back(w1); vertices.push_back(w0); vertices.push_back(v0);
                    vertices.push_back(v0); vertices.push_back(v1); vertices.push_back(w1);
                    
                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMax});
                    
                    // Wall normal (facing negative Z)
                    Vector3 wallNormal = {0, 0, 1};
                    for(int i = 0; i < 6; i++) {
                        normals.push_back(wallNormal);
                    }
                }
            }
            
            // Edge 1->2 (right edge) - check neighbor tile at x+1
            if (x < width - 1) {
                tile neighborTile = getTile(x + 1, y);
                // Create wall if this tile's edge is higher than neighbor's opposite edge
                if (t.tileHeight[1] > neighborTile.tileHeight[0] || t.tileHeight[2] > neighborTile.tileHeight[3]) {
                    Vector3 w1 = {(float)x + 1, (float)neighborTile.tileHeight[0], (float)y};
                    Vector3 w2 = {(float)x + 1, (float)neighborTile.tileHeight[3], (float)y + 1};
                    
                    // Wall face (2 triangles)
                    vertices.push_back(v1); vertices.push_back(v2); vertices.push_back(w2);
                    vertices.push_back(w2); vertices.push_back(w1); vertices.push_back(v1);

                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMax, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMin});
                    
                    // Wall normal (facing positive X)
                    Vector3 wallNormal = {-1, 0, 0};
                    for(int i = 0; i < 6; i++) {
                        normals.push_back(wallNormal);
                    }
                }
            }
            
            // Edge 2->3 (back edge) - check neighbor tile at y+1
            if (y < height - 1) {
                tile neighborTile = getTile(x, y + 1);
                // Create wall if this tile's edge is higher than neighbor's opposite edge
                if (t.tileHeight[2] > neighborTile.tileHeight[1] || t.tileHeight[3] > neighborTile.tileHeight[0]) {
                    Vector3 w2 = {(float)x + 1, (float)neighborTile.tileHeight[1], (float)y + 1};
                    Vector3 w3 = {(float)x,     (float)neighborTile.tileHeight[0], (float)y + 1};
                    
                    // Wall face (2 triangles)
                    vertices.push_back(v2); vertices.push_back(v3); vertices.push_back(w3);
                    vertices.push_back(w3); vertices.push_back(w2); vertices.push_back(v2);

                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMax, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMin});
                    
                    // Wall normal (facing positive Z)
                    Vector3 wallNormal = {0, 0, -1};
                    for(int i = 0; i < 6; i++) {
                        normals.push_back(wallNormal);
                    }
                }
            }
            
            // Edge 3->0 (left edge) - check neighbor tile at x-1
            if (x > 0) {
                tile neighborTile = getTile(x - 1, y);
                // Create wall if this tile's edge is higher than neighbor's opposite edge
                if (t.tileHeight[3] > neighborTile.tileHeight[2] || t.tileHeight[0] > neighborTile.tileHeight[1]) {
                    Vector3 w3 = {(float)x, (float)neighborTile.tileHeight[2], (float)y + 1};
                    Vector3 w0 = {(float)x, (float)neighborTile.tileHeight[1], (float)y};
                    
                    // Wall face (2 triangles)
                    vertices.push_back(v3); vertices.push_back(v0); vertices.push_back(w0);
                    vertices.push_back(w0); vertices.push_back(w3); vertices.push_back(v3);

                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMin}); texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMax, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMax}); texcoords.push_back(Vector2{sideUMin, sideVMin});
                    
                    // Wall normal (facing negative X)
                    Vector3 wallNormal = {1, 0, 0};
                    for(int i = 0; i < 6; i++) {
                        normals.push_back(wallNormal);
                    }
                }
            }
        }
    }
    
    int vertexCount = vertices.size();
    int triangleCount = vertexCount / 3;

    // Create mesh structure and allocate memory
    mesh = {0};
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = triangleCount;
    
    // Allocate and copy vertex data
    mesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
    mesh.normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    
    // Copy vertex data
    for(int i = 0; i < vertexCount; i++) {
        mesh.vertices[i * 3 + 0] = vertices[i].x;
        mesh.vertices[i * 3 + 1] = vertices[i].y;
        mesh.vertices[i * 3 + 2] = vertices[i].z;
        
        mesh.texcoords[i * 2 + 0] = texcoords[i].x;
        mesh.texcoords[i * 2 + 1] = texcoords[i].y;
        
        mesh.normals[i * 3 + 0] = normals[i].x;
        mesh.normals[i * 3 + 1] = normals[i].y;
        mesh.normals[i * 3 + 2] = normals[i].z;
    }
    
    UploadMesh(&mesh, true);
    
    // Create model from mesh and assign diffuse texture
    model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = resourceManager::terrainTexture;
    // Assign shared terrain shader
    Shader& shader = resourceManager::getShader(0);
    model.materials[0].shader = shader;
}

void tileGrid::updateLighting(Vector3 sunDirection, Vector3 sunColor, float ambientStrength, Vector3 ambientColor, float shiftIntensity, float shiftDisplacement) {
    // Forward to shared shader uniform updater
    resourceManager::updateTerrainLighting(
        Vector3Normalize(sunDirection), sunColor,
        ambientStrength, ambientColor,
        shiftIntensity, shiftDisplacement
    );
}

// Build a separate flat translucent water surface model
void tileGrid::generateWaterMesh() {
    // Build flat water surface at constant waterY with diagonal splitting
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    // Buffer to store underlying terrain height per water vertex
    std::vector<float> baseHeights;
    vertices.reserve(width * height * 6);
    normals.reserve(width * height * 6);

    // Expand water outward by a small padding to hide seams against terrain
    const int pad = waterParams.waterPad; // tiles of outward expansion
    const float skirtEpsilon = waterParams.waterSkirt; // allow water to overlap slightly onto shore
    const int N = width * height;
    auto idx2 = [this](int x, int y){ return y * this->width + x; };
    std::vector<uint8_t> waterLvl(N, 0);
    std::vector<uint8_t> dilatedLvl(N, 0);
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t = getTile(x, y);
            waterLvl[idx2(x,y)] = t.waterLevel; // 0..(heightCo*2)
        }
    }
    // Morphological dilation (max level within pad neighborhood)
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            uint8_t maxLevel = waterLvl[idx2(x,y)];
            if (maxLevel == 0) {
                for (int ox = -pad; ox <= pad; ++ox) {
                    for (int oy = -pad; oy <= pad; ++oy) {
                        int nx = x + ox, ny = y + oy;
                        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                        uint8_t wl = waterLvl[idx2(nx, ny)];
                        if (wl > maxLevel) maxLevel = wl;
                    }
                }
            }
            dilatedLvl[idx2(x,y)] = maxLevel;
        }
    }
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t = getTile(x, y);
            uint8_t level = dilatedLvl[idx2(x,y)];
            if (level == 0) continue;
            float waterY = 0.5f * (float)level;
            // Define water quad corners at flat waterY
            Vector3 v0 = {(float)x,     waterY+0.1f, (float)y};
            Vector3 v1 = {(float)x + 1, waterY+0.1f, (float)y};
            Vector3 v2 = {(float)x + 1, waterY+0.1f, (float)y + 1};
            Vector3 v3 = {(float)x,     waterY+0.1f, (float)y + 1};
            // Emit diagonal triangles where water plane exceeds terrain height at ANY corner
            // Triangle A: v2, v1, v0 (corners 2,1,0)
            if (waterY + skirtEpsilon >= t.tileHeight[2] || waterY + skirtEpsilon >= t.tileHeight[1] || waterY + skirtEpsilon >= t.tileHeight[0]) {
                vertices.push_back(v2); vertices.push_back(v1); vertices.push_back(v0);
                normals.push_back({0,1,0}); normals.push_back({0,1,0}); normals.push_back({0,1,0});
                baseHeights.push_back(t.tileHeight[2]); baseHeights.push_back(t.tileHeight[1]); baseHeights.push_back(t.tileHeight[0]);
            }
            // Triangle B: v3, v2, v0 (corners 3,2,0)
            if (waterY + skirtEpsilon >= t.tileHeight[3] || waterY + skirtEpsilon >= t.tileHeight[2] || waterY + skirtEpsilon >= t.tileHeight[0]) {
                vertices.push_back(v3); vertices.push_back(v2); vertices.push_back(v0);
                normals.push_back({0,1,0}); normals.push_back({0,1,0}); normals.push_back({0,1,0});
                baseHeights.push_back(t.tileHeight[3]); baseHeights.push_back(t.tileHeight[2]); baseHeights.push_back(t.tileHeight[0]);
            }
        }
    }

    int vertexCount = (int)vertices.size();
    int triangleCount = vertexCount / 3;

    // Initialize empty mesh
    waterMesh = {0};
    Color tint = { 40, 120, 220, 140 };

    if (vertexCount == 0) {
        // Create an empty model for chunks with no water
        waterModel = LoadModelFromMesh(waterMesh);
        for (int i = 0; i < waterModel.materialCount; ++i) {
            waterModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = tint;
            waterModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = resourceManager::waterTexture;
            waterModel.materials[i].maps[MATERIAL_MAP_NORMAL].texture = resourceManager::waterDisplacementTexture; // Use normal map slot for displacement
            waterModel.materials[i].shader = resourceManager::getShader(1);
        }
        return;
    }

    // Allocate arrays (positions, normals, and use texcoords.x for base height)
    waterMesh.vertexCount = vertexCount;
    waterMesh.triangleCount = triangleCount;
    waterMesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    waterMesh.normals  = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    waterMesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));

    for (int i = 0; i < vertexCount; ++i) {
        waterMesh.vertices[i*3+0] = vertices[i].x;
        waterMesh.vertices[i*3+1] = vertices[i].y;
        waterMesh.vertices[i*3+2] = vertices[i].z;
        waterMesh.normals[i*3+0] = normals[i].x;
        waterMesh.normals[i*3+1] = normals[i].y;
        waterMesh.normals[i*3+2] = normals[i].z;
        // Pack underlying terrain height in texcoord.x for depth calculation
        waterMesh.texcoords[i*2+0] = baseHeights[i];
        // Store texture scale factor in texcoord.y (we'll use world position in shader for UV)
        waterMesh.texcoords[i*2+1] = 0.5f; // Texture scale factor - higher value = more repetitions
    }

    UploadMesh(&waterMesh, true);

    // Create model and set translucent blue color
    waterModel = LoadModelFromMesh(waterMesh);
    for (int i = 0; i < waterModel.materialCount; ++i) {
        waterModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = tint;
        waterModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = resourceManager::waterTexture;
        waterModel.materials[i].maps[MATERIAL_MAP_NORMAL].texture = resourceManager::waterDisplacementTexture; // Use normal map slot for displacement
        waterModel.materials[i].shader = resourceManager::getShader(1);
    }
}
