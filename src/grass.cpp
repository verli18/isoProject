#include "../include/grass.hpp"
#include "../include/resourceManager.hpp"
#include "../include/textureAtlas.hpp"
#include "../include/visualSettings.hpp"
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

// Linear interpolation helper
static float lerpf(float a, float b, float t) {
    return a + t * (b - a);
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
    vboInstanceTemp = 0;
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
    if (vboInstanceTemp != 0) {
        rlUnloadVertexBuffer(vboInstanceTemp);
        vboInstanceTemp = 0;
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
    // Get grass settings from VisualSettings
    const GrassSettings& settings = VisualSettings::getInstance().getGrassSettings();
    
    // Start with the configurable base color (use tipColor as the main grass color)
    float r = settings.tipColor.x;
    float g = settings.tipColor.y;
    float b = settings.tipColor.z;
    
    // Subtle variation based on biome parameters
    float tempNorm = temperature / 255.0f;
    float moistNorm = moisture / 255.0f;
    float bioNorm = biological / 255.0f;
    
    // Temperature: warm = slight yellow shift, cool = slight blue shift
    float tempShift = (tempNorm - 0.5f) * settings.temperatureInfluence;
    r += tempShift * 0.4f;
    b -= tempShift * 0.2f;
    
    // Moisture: wetter = slightly more saturated
    float satBoost = 0.95f + moistNorm * settings.moistureInfluence;
    g *= satBoost;
    
    // Biological: healthier = slightly brighter
    float brightness = 0.95f + bioNorm * settings.biologicalInfluence;
    r *= brightness;
    g *= brightness;
    b *= brightness;
    
    // Clamp to reasonable range around the base color
    r = std::clamp(r, settings.tipColor.x - 0.15f, settings.tipColor.x + 0.15f);
    g = std::clamp(g, settings.tipColor.y - 0.15f, settings.tipColor.y + 0.15f);
    b = std::clamp(b, settings.tipColor.z - 0.15f, settings.tipColor.z + 0.15f);
    
    // Final clamp to valid range
    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);
    
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
    const std::vector<BiomeType>& biomes,
    const std::vector<uint8_t>& temperatures,
    const std::vector<uint8_t>& moistures,
    const std::vector<uint8_t>& biologicalPotentials,
    const std::vector<uint8_t>& erosionFactors
) {
    clear();
    generateBladeMesh();
    
    // Get settings
    const GrassSettings& settings = VisualSettings::getInstance().getGrassSettings();
    const BiomeManager& biomeMan = BiomeManager::getInstance();
    
    // Estimate max blades and reserve
    int bladesPerTile = static_cast<int>(settings.bladesPerTile);
    int maxBlades = width * height * bladesPerTile;
    transforms.reserve(maxBlades);
    bladeData.reserve(maxBlades);
    
    auto idx = [width](int x, int z) { return z * width + x; };
    
    // Helper: check if tile at (x,z) is dirt-like (sand, stone, etc.)
    // We use the biome's primary texture to determine this
    auto isDirtLike = [&](int x, int z) -> bool {
        if (x < 0 || x >= width || z < 0 || z >= height) return false;
        BiomeType b = biomes[idx(x, z)];
        uint8_t tex = biomeMan.getTopTexture(b);
        return tex == SAND || tex == STONE || tex == SNOW;
    };
    
    // Helper: calculate minimum distance to nearest dirt-like tile edge
    auto distToDirt = [&](int tx, int tz, float localX, float localZ) -> float {
        float minDist = 999.0f;
        
        // Check all 8 neighbors + center tile edges
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (isDirtLike(tx + dx, tz + dz)) {
                    // Calculate distance from position to this neighbor
                    float nx = tx + dx + 0.5f;
                    float nz = tz + dz + 0.5f;
                    float dist = sqrtf((localX - nx) * (localX - nx) + (localZ - nz) * (localZ - nz));
                    // Subtract 0.5 to get distance from edge rather than center
                    dist = std::max(0.0f, dist - 0.7f);
                    minDist = std::min(minDist, dist);
                }
            }
        }
        return minDist;
    };
    
    for (int tz = 0; tz < height; ++tz) {
        for (int tx = 0; tx < width; ++tx) {
            int tileIdx = idx(tx, tz);
            BiomeType biomeType = biomes[tileIdx];
            const BiomeData& biomeData = biomeMan.getBiomeData(biomeType);
            
            // Skip if biome doesn't have grass
            if (!biomeData.grass.enabled) continue;
            
            uint8_t temp = temperatures[tileIdx];
            uint8_t moist = moistures[tileIdx];
            uint8_t bio = biologicalPotentials[tileIdx];
            uint8_t erosionRaw = erosionFactors[tileIdx];
            
            // Normalized values
            float tempNorm = temp / 255.0f;
            float moistNorm = moist / 255.0f;
            float bioNorm = bio / 255.0f;
            
            // === Use actual erosion data from erosion simulation ===
            float erosionFactor = erosionRaw / 255.0f;
            
            // Get terrain settings for erosion thresholds
            const TerrainSettings& terrainSettings = VisualSettings::getInstance().getTerrainSettings();
            
            // Apply same thresholds as shader
            float blendRange = std::max(0.01f, terrainSettings.erosionFullExpose - terrainSettings.erosionThreshold);
            float erosionBlend = std::clamp((erosionFactor - terrainSettings.erosionThreshold) / blendRange, 0.0f, 1.0f);
            
            // Grass density multiplier from erosion
            float erosionMult = 1.0f - erosionBlend * settings.slopeReduction;
            
            // === Calculate density based on BiomeData ===
            float density = biomeData.grass.densityBase;
            
            // Apply patchiness
            if (biomeData.grass.patchiness > 0.0f) {
                // Use simplex noise for patches
                // We don't have simplex noise handy here, so let's use a simple noise function
                // or just reuse the hash function for a grid-based noise
                float scale = std::max(1.0f, biomeData.grass.patchScale);
                float nx = (chunkWorldX + tx) / scale;
                float nz = (chunkWorldZ + tz) / scale;
                
                // Simple value noise
                int ix = (int)floor(nx);
                int iz = (int)floor(nz);
                float fx = nx - ix;
                float fz = nz - iz;
                
                auto noise = [](int x, int z) {
                    return hashFloat(hash(x + z * 57));
                };
                
                float n00 = noise(ix, iz);
                float n10 = noise(ix + 1, iz);
                float n01 = noise(ix, iz + 1);
                float n11 = noise(ix + 1, iz + 1);
                
                float n = lerpf(lerpf(n00, n10, fx), lerpf(n01, n11, fx), fz);
                
                // Threshold for patches
                // High patchiness means we only keep grass where noise is high
                float threshold = biomeData.grass.patchiness * 0.8f; // 0.8 max threshold
                if (n < threshold) {
                    density *= 0.1f; // Very sparse in gaps
                } else {
                    // Smooth transition at edges of patches
                    float edge = (n - threshold) / 0.2f;
                    density *= std::min(1.0f, edge * 2.0f + 0.1f);
                }
            }
            
            // Biological potential boost
            density += bioNorm * biomeData.grass.densityVariation;
            
            // Moisture boost for dry biomes (if density is low)
            if (biomeData.grass.densityBase < 0.3f) {
                density += moistNorm * biomeData.grass.densityVariation;
            }
            
            density *= erosionMult;
            density = std::clamp(density, 0.0f, 1.0f);
            
            // Skip if too sparse
            if (density < settings.minDensity) continue;
            
            // === Calculate height multiplier based on biome ===
            float biomeHeightMult = biomeData.grass.heightMultiplier;
            
            int bladesToPlace = static_cast<int>(settings.bladesPerTile * density);
            if (bladesToPlace < 1) bladesToPlace = 1;
            
            // Compute base grass color from biome data
            // We use the biome's configured colors instead of the global settings
            // but we still apply some variation based on temp/moist
            Vector3 tipColor = biomeData.grass.tipColor;
            Vector3 baseColor = biomeData.grass.baseColor;
            
            // Apply subtle variation
            tipColor.x += (tempNorm - 0.5f) * 0.1f; // Temp affects red
            tipColor.y += (moistNorm - 0.5f) * 0.1f; // Moist affects green
            
            // Place blades within this tile
            for (int b = 0; b < bladesToPlace; ++b) {
                // Deterministic pseudo-random position within tile
                uint32_t seed = hash(chunkWorldX + tx + (chunkWorldZ + tz) * 65537 + b * 31337);
                
                float localX = tx + hashFloat(seed);
                float localZ = tz + hashFloat(seed + 1);
                float worldX = chunkWorldX + localX;
                float worldZ = chunkWorldZ + localZ;
                
                // Calculate dirt blending for this blade position
                float dirtDist = distToDirt(tx, tz, localX, localZ);
                float dirtBlend = 0.0f;
                if (dirtDist < settings.dirtBlendDistance) {
                    // Smoothstep falloff for natural transition
                    float t = dirtDist / settings.dirtBlendDistance;
                    t = t * t * (3.0f - 2.0f * t);  // Smoothstep
                    dirtBlend = (1.0f - t) * settings.dirtBlendStrength;
                }
                
                // Color for this blade
                float r = tipColor.x;
                float g = tipColor.y;
                float blueVal = tipColor.z;
                
                if (dirtBlend > 0.0f) {
                    r = r * (1.0f - dirtBlend) + settings.dirtBlendColor.x * dirtBlend;
                    g = g * (1.0f - dirtBlend) + settings.dirtBlendColor.y * dirtBlend;
                    blueVal = blueVal * (1.0f - dirtBlend) + settings.dirtBlendColor.z * dirtBlend;
                }
                
                // Get height at this position
                float y = getHeightAt(localX, localZ, tx, tz, width, tileHeights);
                
                // Random rotation around Y axis
                float rotation = hashFloat(seed + 2) * 2.0f * PI;
                
                // Height variation from settings, apply biome multiplier
                float heightVar = settings.heightVariation;
                float baseHeightVar = (1.0f - heightVar/2.0f) + hashFloat(seed + 3) * heightVar;
                float heightScale = baseHeightVar * biomeHeightMult * erosionMult * (settings.baseHeight / BLADE_BASE_HEIGHT);
                
                // Base angle (slight lean)
                float leanAmount = 0.1f + erosionFactor * 0.25f;
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
                
                // Compute diffuse lighting on CPU
                Vector3 lightDir = Vector3Normalize({-0.59f, 1.0f, 0.8f});  // Negated = toward sun
                float diffuse = std::max(0.0f, Vector3DotProduct(terrainNormal, lightDir));
                
                // Build transform matrix (for billboard, we mainly need position)
                Matrix scale = MatrixScale(1.0f, heightScale, 1.0f);
                Matrix translate = MatrixTranslate(worldX, y, worldZ);
                Matrix transform = MatrixMultiply(scale, translate);
                
                transforms.push_back(transform);
                
                // Store blade data with pre-computed diffuse (using blended colors)
                GrassBlade blade;
                blade.r = r;
                blade.g = g;
                blade.b = blueVal;
                blade.diffuse = diffuse;
                blade.temperature = temp / 255.0f;  // Normalized temperature for shader
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
    
    // Build per-instance temperature data (float per blade)
    std::vector<float> tempData(bladeCount);
    for (size_t i = 0; i < bladeCount; ++i) {
        tempData[i] = bladeData[i].temperature;
    }
    
    // Clean up old temp buffer if exists
    if (vboInstanceTemp != 0) {
        rlUnloadVertexBuffer(vboInstanceTemp);
    }
    
    // Create instance temperature VBO at location 8 (float)
    vboInstanceTemp = rlLoadVertexBuffer(tempData.data(),
                                          tempData.size() * sizeof(float),
                                          false);
    rlEnableVertexBuffer(vboInstanceTemp);
    rlEnableVertexAttribute(8);
    rlSetVertexAttribute(8, 1, RL_FLOAT, false, 0, 0);  // single float, no stride, no offset
    rlSetVertexAttributeDivisor(8, 1);  // 1 = advance once per instance
    
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    
    resourcesLoaded = true;
    TraceLog(LOG_INFO, "GRASS: Instance data uploaded - %zu instances, transform VBO: %u, color VBO: %u, temp VBO: %u", 
             bladeCount, vboInstanceTransforms, vboInstanceColors, vboInstanceTemp);
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
