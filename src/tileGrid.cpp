#include "../include/tileGrid.hpp"
#include "../include/textureAtlas.hpp"
#include <cfloat>  // for FLT_MAX
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include "../include/resourceManager.hpp"  // use shared shader
#include <chrono>
#include <FastNoise/FastNoise.h>

// Moved into tileGrid class

#include <map>

Color lerp(Color a, Color b, int t) {
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
    // Check if coordinates are valid
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    
    // Check if tile is already occupied
    if (grid[y][x].occupyingMachine != nullptr) {
        return false;
    }
    
    // Place the machine
    grid[y][x].occupyingMachine = machinePtr;
    return true;
}

machine* tileGrid::getMachineAt(int x, int y) {
    return grid[y][x].occupyingMachine;
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

    fnRegion->SetOctaveCount(2);
    fnRegion->SetGain(0.5f);
    fnRegion->SetLacunarity(2.0f);

    // Configure Domain Warp
    fnWarp->SetWarpAmplitude(30.0f);
    fnWarp->SetWarpFrequency(0.05f);

    // Generate noise data using FastNoise2
    std::vector<float> heightMap( (width + 1) * (height + 1));
    fnFractal->GenUniformGrid2D(heightMap.data(), baseGenOffset[0], baseGenOffset[1], width + 1, height + 1, 0.02f * scale, 1337);

    std::vector<float> regionMap( (width + 1) * (height + 1));
    fnRegion->GenUniformGrid2D(regionMap.data(), baseGenOffset[0], baseGenOffset[1], width + 1, height + 1, 0.002f * scale, 1337);

    std::vector<float> moistureMap(width * height);
    fnMoisture->GenUniformGrid2D(moistureMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.01f * scale, 1338);

    std::vector<float> temperatureMap(width * height);
    fnTemperature->GenUniformGrid2D(temperatureMap.data(), baseGenOffset[0], baseGenOffset[1], width, height, 0.01f * scale, 1339);


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

            // Modify by altitude
            const float altitudeEffect = avgH * 2.0f;
            t.moisture = static_cast<int>(baseMoisture - altitudeEffect);
            t.temperature = static_cast<int>(baseTemperature - altitudeEffect);

            // Clamp values
            t.moisture = std::max(0, std::min(255, t.moisture));
            t.temperature = std::max(0, std::min(255, t.temperature));

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


            const float waterPlane = (float)heightCo * 0.45f; 
            const float maxWaterLevel = (float)heightCo;      
            float desiredWaterY = waterPlane - 0.5f;
            float avgHeight = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            float minHeight = fminf(fminf(t.tileHeight[0], t.tileHeight[1]), fminf(t.tileHeight[2], t.tileHeight[3]));
            float effectiveHeight = (minHeight + avgHeight) / 2.0f;

            // Only place water if threshold is above the effective terrain height
            if (desiredWaterY > effectiveHeight - 0.5f) {
                float clampedY = fminf(desiredWaterY, maxWaterLevel);
                int quantized = (int)roundf(clampedY * 2.0f); // half-units
                t.waterLevel = quantized;
                
                if (effectiveHeight + 1 < desiredWaterY) t.type = STONE;
                else if (effectiveHeight - 0.75f < desiredWaterY) t.type = SAND;
            } else {
                t.waterLevel = 0;
            }

            // Default lighting placeholder
            t.lighting[0] = WHITE;
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
            DrawLine3D(v2, v1, t.lighting[0]);
            DrawLine3D(v3, v2, t.lighting[0]);
            DrawLine3D(v3, v0, t.lighting[0]);
        }
    }
}

void tileGrid::renderDataPoint(Color a, Color b, int tile::*dataMember, int chunkX, int chunkY) {
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
    using clock = std::chrono::high_resolution_clock;
#ifdef TILEGRID_PROFILE
    auto t0 = clock::now();
#endif
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

#ifdef TILEGRID_PROFILE
    auto t1 = clock::now();
    double meshMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    lastTimings.samplesMesh++;
    double nMesh = static_cast<double>(lastTimings.samplesMesh);
    lastTimings.meshGenMs += (meshMs - lastTimings.meshGenMs) / std::max(1.0, nMesh);
#endif
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
    using clock = std::chrono::high_resolution_clock;
#ifdef TILEGRID_PROFILE
    auto t0 = clock::now();
#endif
    // Build flat water surface at constant waterY with diagonal splitting
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    // Buffer to store underlying terrain height per water vertex
    std::vector<float> baseHeights;
    vertices.reserve(width * height * 6);
    normals.reserve(width * height * 6);
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t = getTile(x, y);
            if (t.waterLevel <= 0) continue;
            float waterY = 0.5f * (float)t.waterLevel;
            // Define water quad corners at flat waterY
            Vector3 v0 = {(float)x,     waterY+0.1f, (float)y};
            Vector3 v1 = {(float)x + 1, waterY+0.1f, (float)y};
            Vector3 v2 = {(float)x + 1, waterY+0.1f, (float)y + 1};
            Vector3 v3 = {(float)x,     waterY+0.1f, (float)y + 1};
            // Emit diagonal triangles where water plane exceeds terrain height at ANY corner
            // Triangle A: v2, v1, v0 (corners 2,1,0)
            if (waterY >= t.tileHeight[2] || waterY >= t.tileHeight[1] || waterY >= t.tileHeight[0]) {
                vertices.push_back(v2); vertices.push_back(v1); vertices.push_back(v0);
                normals.push_back({0,1,0}); normals.push_back({0,1,0}); normals.push_back({0,1,0});
                baseHeights.push_back(t.tileHeight[2]); baseHeights.push_back(t.tileHeight[1]); baseHeights.push_back(t.tileHeight[0]);
            }
            // Triangle B: v3, v2, v0 (corners 3,2,0)
            if (waterY >= t.tileHeight[3] || waterY >= t.tileHeight[2] || waterY >= t.tileHeight[0]) {
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

#ifdef TILEGRID_PROFILE
    auto t1 = clock::now();
    double waterMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    lastTimings.samplesWater++;
    double nWater = static_cast<double>(lastTimings.samplesWater);
    lastTimings.waterGenMs += (waterMs - lastTimings.waterGenMs) / std::max(1.0, nWater);
#endif
}
