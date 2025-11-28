#include "../include/grass.hpp"
#include "../include/resourceManager.hpp"
#include "../include/textureAtlas.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// Simple deterministic hash for pseudo-random values
static uint32_t hash(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static float hashFloat(uint32_t seed) {
    return static_cast<float>(hash(seed) & 0xFFFF) / 65535.0f;
}

GrassField::GrassField() {
    bladeMesh = {0};
    grassMaterial = {0};
    vaoId = 0;
    vboPositions = 0;
    vboTexcoords = 0;
    vboNormals = 0;
    vboInstanceTransforms = 0;
    vboInstanceColors = 0;
}

GrassField::~GrassField() {
    clear();
    if (meshGenerated) {
        // Clean up our manually created buffers
        if (vboPositions != 0) rlUnloadVertexBuffer(vboPositions);
        if (vboTexcoords != 0) rlUnloadVertexBuffer(vboTexcoords);
        if (vboNormals != 0) rlUnloadVertexBuffer(vboNormals);
        if (vaoId != 0) rlUnloadVertexArray(vaoId);
    }
}

void GrassField::clear() {
    transforms.clear();
    bladeData.clear();
    bladeCount = 0;
    
    if (vboInstanceTransforms != 0) {
        rlUnloadVertexBuffer(vboInstanceTransforms);
        vboInstanceTransforms = 0;
    }
    if (vboInstanceColors != 0) {
        rlUnloadVertexBuffer(vboInstanceColors);
        vboInstanceColors = 0;
    }
}

void GrassField::generateBladeMesh() {
    if (meshGenerated) return;
    
    // Simple billboard grass blade - single tapered triangle (double-sided)
    // The billboard rotation is done in the vertex shader
    // This is a simple 2D blade that faces the camera
    //
    //       tip (0, height)
    //        /\
    //       /  \
    //      /    \
    //     /______\
    //   (-w,0)  (w,0)
    //
    
    const float halfWidth = BLADE_WIDTH * 0.5f;
    const float height = BLADE_BASE_HEIGHT;
    
    vertexCount = 6;  // 2 faces x 3 vertices
    
    // Single triangle blade (front and back)
    float positions[] = {
        // Front face
        -halfWidth, 0.0f, 0.0f,   // bottom-left
         halfWidth, 0.0f, 0.0f,   // bottom-right
         0.0f, height, 0.0f,      // tip
        // Back face (reversed winding)
         halfWidth, 0.0f, 0.0f,
        -halfWidth, 0.0f, 0.0f,
         0.0f, height, 0.0f
    };
    
    float texcoords[] = {
        // Front
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.5f, 1.0f,
        // Back
        1.0f, 0.0f,
        0.0f, 0.0f,
        0.5f, 1.0f
    };
    
    float normals[] = {
        // Front (+Z)
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        // Back (-Z)
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f
    };
    
    // Create VAO
    vaoId = rlLoadVertexArray();
    rlEnableVertexArray(vaoId);
    
    // Load vertex buffers and set up attributes
    // Attribute 0: positions (vec3)
    vboPositions = rlLoadVertexBuffer(positions, sizeof(positions), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
    rlEnableVertexAttribute(0);
    
    // Attribute 1: texcoords (vec2)
    vboTexcoords = rlLoadVertexBuffer(texcoords, sizeof(texcoords), false);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, 0, 0);
    rlEnableVertexAttribute(1);
    
    // Attribute 2: normals (vec3)
    vboNormals = rlLoadVertexBuffer(normals, sizeof(normals), false);
    rlSetVertexAttribute(2, 3, RL_FLOAT, false, 0, 0);
    rlEnableVertexAttribute(2);
    
    rlDisableVertexArray();
    
    meshGenerated = true;
    TraceLog(LOG_INFO, "GRASS: Billboard blade mesh created - VAO: %u, vertices: %d", vaoId, vertexCount);
}

float GrassField::getHeightAt(float localX, float localZ, int tileX, int tileZ,
                              int width, const std::vector<float>& heights) const {
    // Heights array is (width+1) x (height+1) for corner vertices
    int stride = width + 1;
    
    // Get the four corner heights
    float h00 = heights[tileZ * stride + tileX];
    float h10 = heights[tileZ * stride + tileX + 1];
    float h01 = heights[(tileZ + 1) * stride + tileX];
    float h11 = heights[(tileZ + 1) * stride + tileX + 1];
    
    // Bilinear interpolation
    float fx = localX - tileX;
    float fz = localZ - tileZ;
    
    return h00 * (1 - fx) * (1 - fz) +
           h10 * fx * (1 - fz) +
           h01 * (1 - fx) * fz +
           h11 * fx * fz;
}

Color GrassField::computeGrassColor(uint8_t temperature, uint8_t moisture,
                                    uint8_t biological, uint8_t tileType) const {
    // Base color from grass texture: #497a22 = RGB(73, 122, 34)
    // Normalized: (0.286, 0.478, 0.133)
    float r = 0.286f;
    float g = 0.478f;  
    float b = 0.133f;
    
    // Subtle variation based on biome parameters
    float tempNorm = temperature / 255.0f;
    float moistNorm = moisture / 255.0f;
    float bioNorm = biological / 255.0f;
    
    // Temperature: warm = slight yellow shift, cool = slight blue shift
    float tempShift = (tempNorm - 0.5f) * 0.08f;
    r += tempShift * 0.4f;
    b -= tempShift * 0.2f;
    
    // Moisture: wetter = slightly more saturated
    float satBoost = 0.95f + moistNorm * 0.1f;
    g *= satBoost;
    
    // Biological: healthier = slightly brighter
    float brightness = 0.95f + bioNorm * 0.1f;
    r *= brightness;
    g *= brightness;
    b *= brightness;
    
    // Keep colors close to base texture
    r = std::clamp(r, 0.20f, 0.40f);
    g = std::clamp(g, 0.40f, 0.60f);
    b = std::clamp(b, 0.08f, 0.22f);
    
    return Color{
        static_cast<unsigned char>(r * 255),
        static_cast<unsigned char>(g * 255),
        static_cast<unsigned char>(b * 255),
        255
    };
}

void GrassField::generate(
    int chunkWorldX, int chunkWorldZ,
    int width, int height,
    const std::vector<float>& tileHeights,
    const std::vector<uint8_t>& tileTypes,
    const std::vector<uint8_t>& temperatures,
    const std::vector<uint8_t>& moistures,
    const std::vector<uint8_t>& biologicalPotentials
) {
    clear();
    generateBladeMesh();
    
    // Estimate max blades and reserve
    int maxBlades = width * height * BLADES_PER_TILE;
    transforms.reserve(maxBlades);
    bladeData.reserve(maxBlades);
    
    auto idx = [width](int x, int z) { return z * width + x; };
    auto heightIdx = [width](int x, int z) { return z * (width + 1) + x; };
    
    for (int tz = 0; tz < height; ++tz) {
        for (int tx = 0; tx < width; ++tx) {
            int tileIdx = idx(tx, tz);
            uint8_t tileType = tileTypes[tileIdx];
            
            // Skip non-grass tiles (sand, snow, water, etc.)
            if (tileType != GRASS) continue;
            
            uint8_t temp = temperatures[tileIdx];
            uint8_t moist = moistures[tileIdx];
            uint8_t bio = biologicalPotentials[tileIdx];
            
            // === Estimate erosion from height gradient (steeper = more water flow = more erosion) ===
            // Get the 4 corner heights
            float h00 = tileHeights[heightIdx(tx, tz)];
            float h10 = tileHeights[heightIdx(tx + 1, tz)];
            float h01 = tileHeights[heightIdx(tx, tz + 1)];
            float h11 = tileHeights[heightIdx(tx + 1, tz + 1)];
            
            // Calculate max height difference (slope indicator)
            float maxDiff = std::max({
                std::abs(h00 - h10), std::abs(h00 - h01), std::abs(h00 - h11),
                std::abs(h10 - h01), std::abs(h10 - h11), std::abs(h01 - h11)
            });
            
            // Erosion factor: steeper slopes = more water flow = more erosion = LESS grass
            // Normalize: 0 = flat (best for grass), 1 = very steep (bad for grass)
            float erosionFactor = std::clamp(maxDiff / 1.5f, 0.0f, 1.0f);
            // Flat areas get 100% grass, steep areas get only 10%
            float grassMultiplier = 1.0f - erosionFactor * 0.9f;
            
            // === Calculate grass density ===
            // For carpet-like effect, start with high base density
            float density = 0.7f;  // High base density
            
            if (bio > 0) {
                // Boost with biological potential
                float bioNorm = bio / 255.0f;
                density = 0.6f + 0.4f * bioNorm;
            } else {
                // Fallback: mainly moisture-driven
                float moistNorm = moist / 255.0f;
                density = 0.5f + 0.5f * moistNorm;
            }
            
            // Low erosion (flat terrain) = more grass (carpet effect)
            density *= grassMultiplier;
            density = std::clamp(density, 0.0f, 1.0f);
            
            // Skip if too sparse
            if (density < 0.1f) continue;
            
            int bladesToPlace = static_cast<int>(BLADES_PER_TILE * density);
            if (bladesToPlace < 1) bladesToPlace = 1;
            
            // Compute grass color
            Color grassColor = computeGrassColor(temp, moist, bio > 0 ? bio : moist, tileType);
            
            // Place blades within this tile
            for (int b = 0; b < bladesToPlace; ++b) {
                // Deterministic pseudo-random position within tile
                uint32_t seed = hash(chunkWorldX + tx + (chunkWorldZ + tz) * 65537 + b * 31337);
                
                float localX = tx + hashFloat(seed);
                float localZ = tz + hashFloat(seed + 1);
                float worldX = chunkWorldX + localX;
                float worldZ = chunkWorldZ + localZ;
                
                // Get height at this position
                float y = getHeightAt(localX, localZ, tx, tz, width, tileHeights);
                
                // Random rotation around Y axis
                float rotation = hashFloat(seed + 2) * 2.0f * PI;
                
                // Height variation - more uniform for carpet look
                float baseHeightVar = 0.8f + hashFloat(seed + 3) * 0.4f;  // 0.8 to 1.2
                float heightScale = baseHeightVar * grassMultiplier;
                
                // Base angle (slight lean) - more lean on slopes
                float leanAmount = 0.1f + erosionFactor * 0.25f;  // More lean on steep slopes
                float baseAngle = (hashFloat(seed + 4) - 0.5f) * leanAmount * 2.0f;
                
                // Stiffness variation
                float stiffness = 0.3f + hashFloat(seed + 5) * 0.5f;
                
                // Compute terrain normal at this position (from height gradient)
                float eps = 0.1f;
                float hL = getHeightAt(std::max(0.0f, localX - eps), localZ, tx, tz, width, tileHeights);
                float hR = getHeightAt(std::min((float)width, localX + eps), localZ, tx, tz, width, tileHeights);
                float hD = getHeightAt(localX, std::max(0.0f, localZ - eps), tx, tz, width, tileHeights);
                float hU = getHeightAt(localX, std::min((float)height, localZ + eps), tx, tz, width, tileHeights);
                
                Vector3 terrainNormal = Vector3Normalize({
                    (hL - hR) / (2.0f * eps),
                    1.0f,
                    (hD - hU) / (2.0f * eps)
                });
                
                // Compute diffuse lighting on CPU (matches terrain shader)
                // Sun direction stored as {0.59f, -1.0f, -0.8f} (from sun to ground, pointing DOWN)
                // Terrain normals are computed via cross product and point DOWN due to winding order
                // Our terrainNormal points UP (Y=1.0), so we negate the sun direction to get
                // the light direction (from ground to sun) for correct diffuse calculation
                Vector3 lightDir = Vector3Normalize({-0.59f, 1.0f, 0.8f});  // Negated = toward sun
                float diffuse = std::max(0.0f, Vector3DotProduct(terrainNormal, lightDir));
                
                // Build transform matrix (for billboard, we mainly need position)
                Matrix scale = MatrixScale(1.0f, heightScale, 1.0f);
                Matrix translate = MatrixTranslate(worldX, y, worldZ);
                Matrix transform = MatrixMultiply(scale, translate);
                
                transforms.push_back(transform);
                
                // Store blade data with pre-computed diffuse
                GrassBlade blade;
                blade.r = grassColor.r / 255.0f;
                blade.g = grassColor.g / 255.0f;
                blade.b = grassColor.b / 255.0f;
                blade.diffuse = diffuse;
                blade.heightScale = heightScale;
                blade.baseAngle = baseAngle;
                blade.stiffness = stiffness;
                
                bladeData.push_back(blade);
            }
        }
    }
    
    bladeCount = transforms.size();
    
    if (bladeCount > 0) {
        uploadInstanceData();
    }
}

void GrassField::uploadInstanceData() {
    if (bladeCount == 0 || vaoId == 0) return;
    
    // Convert Matrix array to float16 array (raylib's MatrixToFloatV format)
    std::vector<float> instanceData(bladeCount * 16);
    for (size_t i = 0; i < bladeCount; ++i) {
        float16 m = MatrixToFloatV(transforms[i]);
        memcpy(&instanceData[i * 16], m.v, 16 * sizeof(float));
    }
    
    // Build per-instance color+diffuse data (vec4 per blade: RGB + diffuse)
    std::vector<float> colorData(bladeCount * 4);
    for (size_t i = 0; i < bladeCount; ++i) {
        colorData[i * 4 + 0] = bladeData[i].r;
        colorData[i * 4 + 1] = bladeData[i].g;
        colorData[i * 4 + 2] = bladeData[i].b;
        colorData[i * 4 + 3] = bladeData[i].diffuse;
    }
    
    // Bind our VAO to add the instance buffers
    rlEnableVertexArray(vaoId);
    
    // Clean up old instance buffers if they exist
    if (vboInstanceTransforms != 0) {
        rlUnloadVertexBuffer(vboInstanceTransforms);
    }
    if (vboInstanceColors != 0) {
        rlUnloadVertexBuffer(vboInstanceColors);
    }
    
    // Create instance transform VBO
    vboInstanceTransforms = rlLoadVertexBuffer(instanceData.data(), 
                                                instanceData.size() * sizeof(float), 
                                                false);
    
    // IMPORTANT: Enable the VBO we just created before setting its attributes
    rlEnableVertexBuffer(vboInstanceTransforms);
    
    // A mat4 takes up 4 consecutive attribute slots (each slot is vec4)
    // We use locations 3, 4, 5, 6 for instanceTransform (after pos=0, uv=1, normal=2)
    for (int i = 0; i < 4; i++) {
        int attribLoc = 3 + i;
        rlEnableVertexAttribute(attribLoc);
        rlSetVertexAttribute(attribLoc, 4, RL_FLOAT, false, sizeof(Matrix), i * sizeof(Vector4));
        rlSetVertexAttributeDivisor(attribLoc, 1);  // 1 = advance once per instance
    }
    
    // Create instance color+diffuse VBO at location 7 (vec4: rgb + diffuse)
    vboInstanceColors = rlLoadVertexBuffer(colorData.data(),
                                            colorData.size() * sizeof(float),
                                            false);
    rlEnableVertexBuffer(vboInstanceColors);
    rlEnableVertexAttribute(7);
    rlSetVertexAttribute(7, 4, RL_FLOAT, false, 0, 0);  // vec4, no stride, no offset
    rlSetVertexAttributeDivisor(7, 1);  // 1 = advance once per instance
    
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    
    resourcesLoaded = true;
    TraceLog(LOG_INFO, "GRASS: Instance data uploaded - %zu instances, transform VBO: %u, color VBO: %u", 
             bladeCount, vboInstanceTransforms, vboInstanceColors);
}

void GrassField::render(float time) {
    if (!meshGenerated || bladeCount == 0 || vaoId == 0) return;
    
    // Get the grass shader
    Shader& shader = resourceManager::getGrassShader();
    
    // Enable our shader
    rlEnableShader(shader.id);
    
    // Set up matrices - get current MVP from rlgl
    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();
    Matrix matModel = rlGetMatrixTransform();
    Matrix matMVP = MatrixMultiply(MatrixMultiply(matModel, matView), matProjection);
    
    // Set shader uniforms
    int mvpLoc = shader.locs[SHADER_LOC_MATRIX_MVP];
    if (mvpLoc != -1) {
        rlSetUniformMatrix(mvpLoc, matMVP);
    }
    
    // Set time uniform for wind animation
    GrassShaderLocs& locs = resourceManager::getGrassShaderLocs();
    if (locs.time != -1) {
        rlSetUniform(locs.time, &time, SHADER_UNIFORM_FLOAT, 1);
    }
    
    // Disable backface culling for grass (visible from both sides)
    rlDisableBackfaceCulling();
    
    // Bind our VAO and draw instanced
    if (rlEnableVertexArray(vaoId)) {
        rlDrawVertexArrayInstanced(0, vertexCount, static_cast<int>(bladeCount));
    }
    
    rlDisableVertexArray();
    rlEnableBackfaceCulling();
    rlDisableShader();
}
