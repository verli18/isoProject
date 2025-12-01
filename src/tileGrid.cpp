#include "../include/tileGrid.hpp"
#include "../include/textureAtlas.hpp"
#include "../include/worldGenerator.hpp"
#include "../include/worldMap.hpp"
#include "../include/biome.hpp"
#include <cfloat>  // for FLT_MAX
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include "../include/resourceManager.hpp"  // use shared shader
#include <chrono>


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
tileGrid::tileGrid(int width, int height) : width(width), height(height), depth(0) {
    grid.resize(width, std::vector<tile>(height));
}

tileGrid::~tileGrid() {
    // Clean up mesh resources
    // Note: UnloadModel also unloads the associated mesh
    if (meshGenerated) {
        UnloadModel(model);
        UnloadModel(waterModel);
    }
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
    
    // Use the WorldMap for world-scale terrain data (erosion, water, etc.)
    WorldMap& worldMap = WorldMap::getInstance();
    BiomeManager& biomeMan = BiomeManager::getInstance();

    // Preallocate helpers for hydrology
    const int N = width * height;
    
    std::vector<float> groundElev(N, 0.0f);
    std::vector<uint8_t> moist(N, 0);
    std::vector<uint8_t> temp(N, 0);
    std::vector<uint8_t> biol(N, 0);

    auto idx = [this](int x, int y) { return y * this->width + x; };

    // === GET DATA FROM WORLDMAP (handles erosion, caching, etc.) ===
    
    // Get pre-eroded height grid from WorldMap
    std::vector<float> heightGrid;
    worldMap.getHeightGrid(heightGrid, baseGenOffset[0], baseGenOffset[1], width, height);
    
    // Get potentials grid from WorldMap
    std::vector<PotentialData> potentialsGrid;
    worldMap.getPotentialGrid(potentialsGrid, baseGenOffset[0], baseGenOffset[1], width, height);
    
    // Get water levels from WorldMap (computed at region scale)
    std::vector<float> waterGrid;
    worldMap.getWaterGrid(waterGrid, baseGenOffset[0], baseGenOffset[1], width, height);
    
    // Get river data from WorldMap
    std::vector<uint8_t> flowDirGrid;
    std::vector<uint8_t> riverWidthGrid;
    worldMap.getRiverGrid(flowDirGrid, riverWidthGrid, baseGenOffset[0], baseGenOffset[1], width, height);
    
    // Get erosion intensity data from WorldMap
    std::vector<uint8_t> erosionGrid;
    worldMap.getErosionGrid(erosionGrid, baseGenOffset[0], baseGenOffset[1], width, height);
    
    // Height grid indexer (width+1 columns)
    auto hidx = [this](int x, int y) { return y * (this->width + 1) + x; };

    // First pass: generate tiles using pre-computed grids
    // NOTE: Must iterate y (rows) first for row-major grid access
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            tile t;
            
            // Get potentials from pre-computed grid
            PotentialData& potentials = potentialsGrid[idx(x, y)];
            
            // Determine biome and blending using BiomeManager
            BiomeType primaryBiome, secondaryBiome;
            float primaryWeight;
            biomeMan.getBlendWeights(potentials, primaryBiome, secondaryBiome, primaryWeight);
            
            t.biome = primaryBiome;
            t.secondaryBiome = secondaryBiome;
            t.blendStrength = static_cast<uint8_t>((1.0f - primaryWeight) * 255.0f);
            
            // Get textures from BiomeManager
            t.type = biomeMan.getTopTexture(t.biome);
            t.secondaryType = biomeMan.getTopTexture(t.secondaryBiome);
            
            // If primary and secondary are the same, ensure blend strength is 0
            if (t.biome == t.secondaryBiome) {
                t.blendStrength = 0;
                t.secondaryType = t.type;
            }

            // Get height values for the 4 corners from pre-computed grid
            // Heights are already scaled, shaped, and eroded by WorldMap
            float heightTL = heightGrid[hidx(x, y)];
            float heightTR = heightGrid[hidx(x + 1, y)];
            float heightBL = heightGrid[hidx(x, y + 1)];
            float heightBR = heightGrid[hidx(x + 1, y + 1)];
            
            // World coordinates for each corner (for position-dependent features)
            float worldX = static_cast<float>(baseGenOffset[0] + x);
            float worldZ = static_cast<float>(baseGenOffset[1] + y);
            
            // Apply biome-specific terrain kernels to each corner
            heightTL = biomeMan.modifyHeight(heightTL, t.biome, potentials, worldX, worldZ);
            heightTR = biomeMan.modifyHeight(heightTR, t.biome, potentials, worldX + 1.0f, worldZ);
            heightBL = biomeMan.modifyHeight(heightBL, t.biome, potentials, worldX, worldZ + 1.0f);
            heightBR = biomeMan.modifyHeight(heightBR, t.biome, potentials, worldX + 1.0f, worldZ + 1.0f);

            // Convert to heights with quarter-unit quantization for smoother terrain
            // Multiplying by 4 then dividing by 4 gives 0.25 steps
            t.tileHeight[0] = std::round(heightTL * 2.0f) / 2.0f;
            t.tileHeight[1] = std::round(heightTR * 2.0f) / 2.0f;
            t.tileHeight[2] = std::round(heightBR * 2.0f) / 2.0f;
            t.tileHeight[3] = std::round(heightBL * 2.0f) / 2.0f;
            
            // Clamp extreme slopes - allow up to 0.75 difference before walls form
            // With quarter steps, 0.75 = 3 quarter-steps, still walkable-looking
            const float maxSlope = 5.0f;
            for (int i = 0; i < 4; ++i) {
                for (int j = i + 1; j < 4; ++j) {
                    float diff = t.tileHeight[i] - t.tileHeight[j];
                    if (std::abs(diff) > maxSlope) {
                        float mid = (t.tileHeight[i] + t.tileHeight[j]) / 2.0f;
                        t.tileHeight[i] = mid + (diff > 0 ? maxSlope/2 : -maxSlope/2);
                        t.tileHeight[j] = mid - (diff > 0 ? maxSlope/2 : -maxSlope/2);
                    }
                }
            }

            float avgH = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            
            // Use potentials from pre-computed grid 
            float baseMoisture = potentials.humidity * 255.0f;
            float baseTemperature = potentials.temperature * 255.0f;

            // Modify by altitude
            const float altitudeEffect = avgH * 2.0f;
            float finalMoisture = baseMoisture - altitudeEffect;
            float finalTemperature = baseTemperature - altitudeEffect;

            // Clamp values
            t.moisture = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, finalMoisture)));
            t.temperature = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, finalTemperature)));

            // Store geological potentials from pre-computed grid
            // Magmatic potential influenced by slope
            float min_h = t.tileHeight[0];
            float max_h = t.tileHeight[0];
            for(int i = 1; i < 4; ++i) {
                if(t.tileHeight[i] < min_h) min_h = t.tileHeight[i];
                if(t.tileHeight[i] > max_h) max_h = t.tileHeight[i];
            }
            float slope = max_h - min_h;

            float modifiedMagmatic = potentials.magmatic + slope * 0.35f;
            t.magmaticPotential = static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, modifiedMagmatic)) * 255.0f);

            t.sulfidePotential = static_cast<uint8_t>(std::round(std::clamp(potentials.sulfide, 0.0f, 1.0f) * 255.0f));

            // Crystalline potential with enhanced chain-like patterns
            float baseCrystaline = potentials.crystalline;
            baseCrystaline = std::pow(baseCrystaline + 0.2f, 2.5f);
            float magmaticFactor = t.magmaticPotential / 255.0f;
            float elevationFactor = std::max(0.0f, 1.0f - std::abs(avgH - 50.0f) / 100.0f);
            float finalCrystaline = baseCrystaline * (0.2f + 0.8f * magmaticFactor) * (0.7f + 0.3f * elevationFactor);
            t.crystalinePotential = static_cast<uint8_t>(std::round(std::clamp(finalCrystaline, 0.0f, 1.0f) * 255.0f));

            // Initialize water level to 0 (will be set by new water system if needed)
            t.waterLevel = 0;
            t.hydrologicalPotential = 0;
            
            // Assign erosion factor from pre-computed erosion simulation
            t.erosionFactor = erosionGrid[idx(x, y)];

            float avgHeight = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            groundElev[idx(x,y)] = avgHeight;
            moist[idx(x,y)] = t.moisture;
            temp[idx(x,y)] = t.temperature;

            setTile(x, y, t);
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = idx(x, y);
            tile t = getTile(x, y);
            
            float waterSurface = waterGrid[i];
            if (waterSurface > 0.0f) {
                // Water level is stored as 2x the actual Y coordinate
                int quantized = static_cast<int>(std::round(waterSurface * 2.0f));
                quantized = std::clamp(quantized, 1, 254);
                t.waterLevel = static_cast<uint8_t>(quantized);
            } else {
                t.waterLevel = 0;
            }
            
            // River data
            t.flowDir = flowDirGrid[i];
            t.riverWidth = riverWidthGrid[i];
            
            // Compute marching squares case for water shape (8-directional)
            // This determines smooth tile transitions for rivers and lakes
            if (t.waterLevel > 0 || t.riverWidth > 0) {
                uint8_t rcase = 0;
                // Check all 8 neighbors for water connectivity
                // Bit layout: 0=E, 1=SE, 2=S, 3=SW, 4=W, 5=NW, 6=N, 7=NE
                auto hasWater = [&](int nx, int ny) {
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) return false;
                    int ni = idx(nx, ny);
                    return waterGrid[ni] > 0 || riverWidthGrid[ni] > 0;
                };
                if (hasWater(x+1, y))   rcase |= 0x01;  // E
                if (hasWater(x+1, y+1)) rcase |= 0x02;  // SE
                if (hasWater(x, y+1))   rcase |= 0x04;  // S
                if (hasWater(x-1, y+1)) rcase |= 0x08;  // SW
                if (hasWater(x-1, y))   rcase |= 0x10;  // W
                if (hasWater(x-1, y-1)) rcase |= 0x20;  // NW
                if (hasWater(x, y-1))   rcase |= 0x40;  // N
                if (hasWater(x+1, y-1)) rcase |= 0x80;  // NE
                t.riverCase = rcase;
            } else {
                t.riverCase = 0;
            }
            
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
    std::vector<Color> colors;  // Store erosion data in vertex colors

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

            // Vertex color encodes terrain data for shader:
            // R = primary texture type (GRASS=1, SNOW=2, STONE=3, SAND=4)
            // G = secondary texture type (for blending)
            // B = blend strength (0=100% primary, 255=100% secondary)
            // A = erosion factor (0-255)
            Color tileDataColor = { 
                static_cast<unsigned char>(t.type),         // Primary texture
                static_cast<unsigned char>(t.secondaryType),// Secondary texture for blending
                t.blendStrength,                            // Blend strength
                t.erosionFactor                             // Erosion for dithering
            };
            
            // Use the tile's primary type for texture coordinates
            // The shader handles blending to secondary type via dithering
            textureAtlas texAtlas = textures[t.type];
            float uMin = (float)texAtlas.uOffset / atlasWidth;
            float vMin = (float)texAtlas.vOffset / atlasHeight;
            float uMax = (float)(texAtlas.uOffset + texAtlas.width) / atlasWidth;
            float vMax = (float)(texAtlas.vOffset + texAtlas.height) / atlasHeight;
            
            if (diag1_diff <= diag2_diff) {
                // --- Split with diagonal v0-v2 ---
                // Triangle 1: v0, v1, v2
                vertices.push_back(v0); vertices.push_back(v1); vertices.push_back(v2);
                texcoords.push_back(Vector2{uMin, vMin}); texcoords.push_back(Vector2{uMax, vMin}); texcoords.push_back(Vector2{uMax, vMax});
                Vector3 n1_edge1 = Vector3Subtract(v1, v0);
                Vector3 n1_edge2 = Vector3Subtract(v2, v0);
                Vector3 normal1 = Vector3Normalize(Vector3CrossProduct(n1_edge1, n1_edge2));
                normals.push_back(normal1); normals.push_back(normal1); normals.push_back(normal1);
                colors.push_back(tileDataColor); colors.push_back(tileDataColor); colors.push_back(tileDataColor);
                
                // Triangle 2: v0, v2, v3
                vertices.push_back(v0); vertices.push_back(v2); vertices.push_back(v3);
                texcoords.push_back(Vector2{uMin, vMin}); texcoords.push_back(Vector2{uMax, vMax}); texcoords.push_back(Vector2{uMin, vMax});
                Vector3 n2_edge1 = Vector3Subtract(v2, v0);
                Vector3 n2_edge2 = Vector3Subtract(v3, v0);
                Vector3 normal2 = Vector3Normalize(Vector3CrossProduct(n2_edge1, n2_edge2));
                normals.push_back(normal2); normals.push_back(normal2); normals.push_back(normal2);
                colors.push_back(tileDataColor); colors.push_back(tileDataColor); colors.push_back(tileDataColor);

            } else {
                // --- Split with diagonal v1-v3 ---
                // Triangle 1: v1, v2, v3
                vertices.push_back(v1); vertices.push_back(v2); vertices.push_back(v3);
                texcoords.push_back(Vector2{uMax, vMin}); texcoords.push_back(Vector2{uMax, vMax}); texcoords.push_back(Vector2{uMin, vMax});
                Vector3 n1_edge1 = Vector3Subtract(v2, v1);
                Vector3 n1_edge2 = Vector3Subtract(v3, v1);
                Vector3 normal1 = Vector3Normalize(Vector3CrossProduct(n1_edge1, n1_edge2));
                normals.push_back(normal1); normals.push_back(normal1); normals.push_back(normal1);
                colors.push_back(tileDataColor); colors.push_back(tileDataColor); colors.push_back(tileDataColor);

                // Triangle 2: v1, v3, v0
                vertices.push_back(v1); vertices.push_back(v3); vertices.push_back(v0);
                texcoords.push_back(Vector2{uMax, vMin}); texcoords.push_back(Vector2{uMin, vMax}); texcoords.push_back(Vector2{uMin, vMin});
                Vector3 n2_edge1 = Vector3Subtract(v3, v1);
                Vector3 n2_edge2 = Vector3Subtract(v0, v1);
                Vector3 normal2 = Vector3Normalize(Vector3CrossProduct(n2_edge1, n2_edge2));
                normals.push_back(normal2); normals.push_back(normal2); normals.push_back(normal2);
                colors.push_back(tileDataColor); colors.push_back(tileDataColor); colors.push_back(tileDataColor);
            }
            
            // Generate side faces (walls) where there are height differences
            // Calculate UV coordinates for side faces
            float sideUMin = (float)textures[t.type].sideUOffset / atlasWidth;
            float sideVMin = (float)textures[t.type].sideVOffset / atlasHeight;
            float sideUMax = (float)(textures[t.type].sideUOffset + textures[t.type].width) / atlasWidth;
            float sideVMax = (float)(textures[t.type].sideVOffset + textures[t.type].height) / atlasHeight;
            
            // Walls always show exposed rock texture (full erosion)
            Color wallColor = { 255, 255, 255, 255 };
            
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
                        colors.push_back(wallColor);
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
                        colors.push_back(wallColor);
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
                        colors.push_back(wallColor);
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
                        colors.push_back(wallColor);
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
    mesh.colors = (unsigned char*)MemAlloc(vertexCount * 4 * sizeof(unsigned char));  // RGBA colors for erosion
    
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
        
        // Copy color (RGBA)
        mesh.colors[i * 4 + 0] = colors[i].r;
        mesh.colors[i * 4 + 1] = colors[i].g;
        mesh.colors[i * 4 + 2] = colors[i].b;
        mesh.colors[i * 4 + 3] = colors[i].a;  // Erosion in alpha
    }
    
    UploadMesh(&mesh, true);
    
    // Create model from mesh and assign diffuse texture
    model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = resourceManager::terrainTexture;
    // Assign shared terrain shader
    Shader& shader = resourceManager::getShader(0);
    model.materials[0].shader = shader;
    
    meshGenerated = true;
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
// Rivers and lakes use full tile quads - the carved terrain provides the banks
void tileGrid::generateWaterMesh() {
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<float> baseHeights;
    std::vector<float> flowDirs;
    vertices.reserve(width * height * 6);
    normals.reserve(width * height * 6);
    baseHeights.reserve(width * height * 6);
    flowDirs.reserve(width * height * 6);

    const Vector3 upNormal = {0, 1, 0};
    
    // Lambda to add a triangle with consistent data
    auto addTri = [&](Vector3 a, Vector3 b, Vector3 c, float terrainH, float flowDir) {
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        normals.push_back(upNormal);
        normals.push_back(upNormal);
        normals.push_back(upNormal);
        baseHeights.push_back(terrainH);
        baseHeights.push_back(terrainH);
        baseHeights.push_back(terrainH);
        flowDirs.push_back(flowDir);
        flowDirs.push_back(flowDir);
        flowDirs.push_back(flowDir);
    };
    
    // Helper to get water height at a tile
    auto getWaterHeight = [this](int x, int y) -> float {
        if (x < 0 || x >= width || y < 0 || y >= height) return -1000.0f;
        tile t = getTile(x, y);
        if (t.waterLevel > 0) return 0.5f * t.waterLevel + 0.1f;
        if (t.riverWidth > 0) {
            // Rivers: water sits in carved channel, slightly above ground
            float minH = t.tileHeight[0];
            for (int i = 1; i < 4; i++) minH = std::min(minH, t.tileHeight[i]);
            return minH + 0.25f;  // Higher water level for visibility
        }
        return -1000.0f;
    };
    
    // Process each tile
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t = getTile(x, y);
            
            // Skip if no water at all
            if (t.waterLevel == 0 && t.riverWidth == 0) continue;
            
            // Calculate water height for this tile
            float waterY;
            if (t.waterLevel > 0) {
                waterY = 0.5f * t.waterLevel + 0.1f;
            } else {
                // River: use terrain-following water at higher level
                float minH = t.tileHeight[0];
                for (int i = 1; i < 4; i++) minH = std::min(minH, t.tileHeight[i]);
                waterY = minH + 0.25f;  // Match getWaterHeight
            }
            
            // Get flow direction as angle (0-7 maps to 0-2π)
            float flowAngle = (t.flowDir < 8) ? (t.flowDir * 0.785398f) : 0.0f;
            
            // Average terrain height for depth calculation
            float avgTerrainH = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            
            float fx = (float)x;
            float fy = (float)y;
            
            // Get water heights at neighboring tiles for corner interpolation
            float hN = getWaterHeight(x, y-1);
            float hS = getWaterHeight(x, y+1);
            float hE = getWaterHeight(x+1, y);
            float hW = getWaterHeight(x-1, y);
            float hNE = getWaterHeight(x+1, y-1);
            float hNW = getWaterHeight(x-1, y-1);
            float hSE = getWaterHeight(x+1, y+1);
            float hSW = getWaterHeight(x-1, y+1);
            
            // Calculate corner heights - average with valid neighbors for smooth transitions
            auto cornerHeight = [&](float h1, float h2, float h3) -> float {
                float sum = waterY;
                int count = 1;
                if (h1 > -500.0f) { sum += h1; count++; }
                if (h2 > -500.0f) { sum += h2; count++; }
                if (h3 > -500.0f) { sum += h3; count++; }
                return sum / count;
            };
            
            // Corner layout:  0--1  (NW--NE)  z=y
            //                 |  |
            //                 3--2  (SW--SE)  z=y+1
            float h0 = cornerHeight(hN, hW, hNW);
            float h1 = cornerHeight(hN, hE, hNE);
            float h2 = cornerHeight(hS, hE, hSE);
            float h3 = cornerHeight(hS, hW, hSW);
            
            Vector3 corners[4] = {
                {fx,     h0, fy},
                {fx + 1, h1, fy},
                {fx + 1, h2, fy + 1},
                {fx,     h3, fy + 1}
            };
            
            // Draw full quad as two triangles
            addTri(corners[2], corners[1], corners[0], avgTerrainH, flowAngle);
            addTri(corners[0], corners[3], corners[2], avgTerrainH, flowAngle);
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
        // Pack flow direction angle in texcoord.y for river animation
        waterMesh.texcoords[i*2+1] = flowDirs[i];
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
