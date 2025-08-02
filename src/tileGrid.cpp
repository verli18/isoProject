#include "../include/tileGrid.hpp"
#include "../include/textureAtlas.hpp"
#include <cfloat>  // for FLT_MAX
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include "../include/resourceManager.hpp"  // use shared shader

Color lerp(Color a, Color b, int t) {
    return Color{static_cast<unsigned char>(a.r + (b.r - a.r) * t / 255), static_cast<unsigned char>(a.g + (b.g - a.g) * t / 255), static_cast<unsigned char>(a.b + (b.b - a.b) * t / 255), 255};
}
tileGrid::tileGrid(int width, int height) {
    this->width = width;
    this->height = height;
    grid.resize(width, std::vector<tile>(height));
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
    // Generate multiple Perlin noise images for fractal noise (octaves)
    std::vector<Image> noiseImages;
    noiseImages.reserve(octaves);

    Image moistureMap = GenImagePerlinNoise(width+1, height+1, baseGenOffset[2], baseGenOffset[3], scale);
    Image temperatureMap = GenImagePerlinNoise(width+1, height+1, baseGenOffset[4], baseGenOffset[5], scale);
    for (int i = 0; i < octaves; ++i) {
        float octaveScale = scale * std::pow(lacunarity, (float)i);
        noiseImages.push_back(
            GenImagePerlinNoise(width+1, height+1, baseGenOffset[0], baseGenOffset[1], octaveScale)
        );
    }

    // Compute normalization factor
    float weightSum = 0.0f;
    for (int i = 0; i < octaves; ++i) weightSum += std::pow(persistence, (float)i);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t;
            float sumTL = 0.0f, sumTR = 0.0f, sumBR = 0.0f, sumBL = 0.0f;
            // Accumulate weighted noise per corner
            for (int i = 0; i < octaves; ++i) {
                float weight = std::pow(persistence, (float)i);
                Color colTL = GetImageColor(noiseImages[i], x, y);
                Color colTR = GetImageColor(noiseImages[i], x+1, y);
                Color colBR = GetImageColor(noiseImages[i], x+1, y+1);
                Color colBL = GetImageColor(noiseImages[i], x, y+1);
                sumTL += (colTL.r / 255.0f) * weight;
                sumTR += (colTR.r / 255.0f) * weight;
                sumBR += (colBR.r / 255.0f) * weight;
                sumBL += (colBL.r / 255.0f) * weight;
            }
            // Normalize and apply exponent
            float fTL = std::pow(sumTL / weightSum, exponent);
            float fTR = std::pow(sumTR / weightSum, exponent);
            float fBR = std::pow(sumBR / weightSum, exponent);
            float fBL = std::pow(sumBL / weightSum, exponent);

            // Convert to tile heights with half-step rounding
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

            t.moisture = GetImageColor(moistureMap, x, y).r;
            t.temperature = GetImageColor(temperatureMap, x, y).r;

            float avgH = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            t.type = (avgH > heightCo * 0.5f) ? SNOW : GRASS;

            const float waterPlane = (float)heightCo * 0.45f; 
            const float maxWaterLevel = (float)heightCo;      
            float desiredWaterY = waterPlane - 0.5f;
            float avgHeight = (t.tileHeight[0] + t.tileHeight[1] + t.tileHeight[2] + t.tileHeight[3]) / 4.0f;
            float minHeight = fminf(fminf(t.tileHeight[0], t.tileHeight[1]), fminf(t.tileHeight[2], t.tileHeight[3]));
            float effectiveHeight = (minHeight + avgHeight) / 2.0f;

            // Only place water if threshold is above the effective terrain height
            if (desiredWaterY > effectiveHeight) {
                float clampedY = fminf(desiredWaterY, maxWaterLevel);
                int quantized = (int)roundf(clampedY * 2.0f); // half-units
                t.waterLevel = quantized;
                if (effectiveHeight + 1 < desiredWaterY) t.type = DIRT;
            } else {
                t.waterLevel = 0;
            }

            // Default lighting placeholder
            t.lighting[0] = WHITE;
            setTile(x, y, t);
        }
    }
    // Unload all noise images
    for (auto &img : noiseImages) UnloadImage(img);
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
    // Assuming texture atlas dimensions (you may want to make these configurable)
    const float atlasWidth = 64.0f;  // Total atlas width in pixels
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
            
            // Get texture atlas info for this tile type
            textureAtlas atlas = textures[t.type];
            
            // Calculate UV coordinates for the top face
            float uMin = (float)atlas.uOffset / atlasWidth;
            float vMin = (float)atlas.vOffset / atlasHeight;
            float uMax = (float)(atlas.uOffset + atlas.width) / atlasWidth;
            float vMax = (float)(atlas.vOffset + atlas.height) / atlasHeight;
            
            // First triangle: v2, v1, v0 (counter-clockwise)
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v0);

            // Second triangle: v3, v2, v0 (counter-clockwise)
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v0);
            
            // Texture coordinates for each vertex (6 vertices total)
            // First triangle: v2, v1, v0
            texcoords.push_back(Vector2{uMax, vMax});  // v2 (bottom-right)
            texcoords.push_back(Vector2{uMax, vMin});  // v1 (top-right)
            texcoords.push_back(Vector2{uMin, vMin});  // v0 (top-left)
            
            // Second triangle: v3, v2, v0
            texcoords.push_back(Vector2{uMin, vMax});  // v3 (bottom-left)
            texcoords.push_back(Vector2{uMax, vMax});  // v2 (bottom-right)
            texcoords.push_back(Vector2{uMin, vMin});  // v0 (top-left)
            
            // Calculate normal for the first triangle (v2, v1, v0)
            Vector3 n1_edge1 = Vector3Subtract(v1, v0);
            Vector3 n1_edge2 = Vector3Subtract(v2, v0);
            Vector3 normal1 = Vector3Normalize(Vector3CrossProduct(n1_edge1, n1_edge2));
            // Add normals for first triangle vertices
            normals.push_back(normal1);
            normals.push_back(normal1);
            normals.push_back(normal1);

            // Calculate normal for the second triangle (v3, v2, v0)
            Vector3 n2_edge1 = Vector3Subtract(v2, v0);
            Vector3 n2_edge2 = Vector3Subtract(v3, v0);
            Vector3 normal2 = Vector3Normalize(Vector3CrossProduct(n2_edge1, n2_edge2));
            // Add normals for second triangle vertices
            normals.push_back(normal2);
            normals.push_back(normal2);
            normals.push_back(normal2);
            
            // Generate side faces (walls) where there are height differences
            // Calculate UV coordinates for side faces
            float sideUMin = (float)atlas.sideUOffset / atlasWidth;
            float sideVMin = (float)atlas.sideVOffset / atlasHeight;
            float sideUMax = (float)(atlas.sideUOffset + atlas.width) / atlasWidth;
            float sideVMax = (float)(atlas.sideVOffset + atlas.height) / atlasHeight;
            
            // Check each edge for height differences and generate wall faces only where needed
            
            // Edge 0->1 (front edge) - check neighbor tile at y-1
            if (y > 0) {
                tile neighborTile = getTile(x, y - 1);
                // Create wall if this tile's edge is higher than neighbor's opposite edge
                if (t.tileHeight[0] > neighborTile.tileHeight[3] || t.tileHeight[1] > neighborTile.tileHeight[2]) {
                    Vector3 w0 = {(float)x,     (float)neighborTile.tileHeight[3], (float)y};
                    Vector3 w1 = {(float)x + 1, (float)neighborTile.tileHeight[2], (float)y};
                    
                    // Wall face (corrected winding order)
                    vertices.push_back(w1);
                    vertices.push_back(w0);
                    vertices.push_back(v0);
                    vertices.push_back(v1);
                    vertices.push_back(w1);
                    vertices.push_back(v0);
                    
                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMin, sideVMax});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMax, sideVMin});
                    
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
                    
                    // Wall face (corrected winding order)
                    vertices.push_back(v1);
                    vertices.push_back(v2);
                    vertices.push_back(w2);
                    vertices.push_back(v1);
                    vertices.push_back(w2);
                    vertices.push_back(w1);
                    
                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMin, sideVMax});
                    
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
                    
                    // Wall face (corrected winding order)
                    vertices.push_back(v2);
                    vertices.push_back(v3);
                    vertices.push_back(w3);
                    vertices.push_back(v2);
                    vertices.push_back(w3);
                    vertices.push_back(w2);
                    
                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMin, sideVMax});
                    
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
                    
                    // Wall face (corrected winding order)
                    vertices.push_back(v3);
                    vertices.push_back(w0);
                    vertices.push_back(w3);
                    vertices.push_back(v3);
                    vertices.push_back(v0);
                    vertices.push_back(w0);
                    
                    // Side texture coordinates
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    texcoords.push_back(Vector2{sideUMin, sideVMax});
                    texcoords.push_back(Vector2{sideUMin, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMin});
                    texcoords.push_back(Vector2{sideUMax, sideVMax});
                    
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
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("textures.png");
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
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t = getTile(x, y);
            if (t.waterLevel <= 0) continue;
            float waterY = 0.5f * (float)t.waterLevel;
            // Define water quad corners at flat waterY
            Vector3 v0 = {(float)x,     waterY, (float)y};
            Vector3 v1 = {(float)x + 1, waterY, (float)y};
            Vector3 v2 = {(float)x + 1, waterY, (float)y + 1};
            Vector3 v3 = {(float)x,     waterY, (float)y + 1};
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

    // If empty, create an empty mesh/model with translucent tint
    waterMesh = {0};
    waterModel = LoadModelFromMesh(waterMesh);
    Color tint = { 40, 120, 220, 140 };

    if (vertexCount == 0) {
        for (int i = 0; i < waterModel.materialCount; ++i) {
            waterModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = tint;
            waterModel.materials[i].shader = resourceManager::getShader(0);
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
        // pack underlying terrain height in texcoord.x, leave y unused
        waterMesh.texcoords[i*2+0] = baseHeights[i];
        waterMesh.texcoords[i*2+1] = 0.0f;
    }

    UploadMesh(&waterMesh, true);

    // Create model and set translucent blue color
    waterModel = LoadModelFromMesh(waterMesh);
    for (int i = 0; i < waterModel.materialCount; ++i) {
        waterModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = tint;
        waterModel.materials[i].shader = resourceManager::getShader(1);
    }
}
