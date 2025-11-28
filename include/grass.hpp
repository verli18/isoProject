#ifndef GRASS_HPP
#define GRASS_HPP

#include <raylib.h>
#include <vector>
#include <cstdint>

/**
 * GrassBlade - Per-instance data for a single grass blade
 * 
 * This is packed efficiently for GPU upload. Each blade needs:
 * - Position (from instance matrix)
 * - Color tint (RGB, derived from biome/temperature/humidity)
 * - Height scale (0-1, affects blade length)
 * - Bend angle (base bend direction for wind)
 * - Phase offset (for wind animation desync, can derive from position)
 */
struct GrassBlade {
    // Position is encoded in the transform matrix
    // These are per-blade attributes packed for the shader
    
    // Color tint (normalized 0-1) + diffuse lighting factor in alpha
    float r, g, b, diffuse;
    
    // Height multiplier (0.5 - 1.5 typical range)
    float heightScale;
    
    // Base angle in radians (which way the blade leans)
    float baseAngle;
    
    // Stiffness (how much it resists wind, 0-1)
    float stiffness;
};

/**
 * GrassField - Manages grass instances for a chunk
 * 
 * Handles:
 * - Generating blade positions and properties from tile data
 * - Building instanced mesh data
 * - Rendering with instancing
 */
class GrassField {
public:
    GrassField();
    ~GrassField();
    
    // Generate grass blades for a chunk based on tile properties
    // chunkWorldX/Z: world position of chunk origin
    // width/height: chunk dimensions in tiles
    // tileHeights: height data for tile corners (width+1 x height+1)
    // tileData: per-tile biome info (temperature, moisture, type, potentials)
    void generate(
        int chunkWorldX, int chunkWorldZ,
        int width, int height,
        const std::vector<float>& tileHeights,
        const std::vector<uint8_t>& tileTypes,
        const std::vector<uint8_t>& temperatures,
        const std::vector<uint8_t>& moistures,
        const std::vector<uint8_t>& biologicalPotentials
    );
    
    // Clear all grass data
    void clear();
    
    // Render all grass blades using instancing
    // time: current time for wind animation
    void render(float time);
    
    // Get number of grass blades
    size_t getBladeCount() const { return bladeCount; }
    
    // Configuration - BILLBOARD GRASS (slim blades facing camera)
    static constexpr int BLADES_PER_TILE = 50;  // More blades since they're slimmer
    static constexpr float BLADE_BASE_HEIGHT = 0.8f;  // Tall blades
    static constexpr float BLADE_WIDTH = 0.15f;  // Slim billboards
    
private:
    // Instance data
    std::vector<Matrix> transforms;      // Per-instance transform matrices
    std::vector<GrassBlade> bladeData;   // Per-instance blade properties
    size_t bladeCount = 0;
    
    // GPU resources - using raw rlgl for proper instancing control
    Mesh bladeMesh;           // Reference only, we manage VBOs ourselves
    Material grassMaterial;   // Material with grass shader
    bool meshGenerated = false;
    bool resourcesLoaded = false;
    
    // OpenGL buffer IDs (managed via rlgl)
    unsigned int vaoId = 0;                    // Vertex Array Object
    unsigned int vboPositions = 0;             // Vertex positions VBO
    unsigned int vboTexcoords = 0;             // Texture coordinates VBO
    unsigned int vboNormals = 0;               // Normals VBO
    unsigned int vboInstanceTransforms = 0;    // Per-instance transforms VBO
    unsigned int vboInstanceColors = 0;        // Per-instance color+diffuse VBO (vec4)
    int vertexCount = 0;                       // Number of vertices in blade mesh
    
    // Generate the base blade mesh (a simple quad or triangle strip)
    void generateBladeMesh();
    
    // Upload instance data to GPU
    void uploadInstanceData();
    
    // Helper: get interpolated height at a position within a tile
    float getHeightAt(float localX, float localZ, int tileX, int tileZ,
                      int width, const std::vector<float>& heights) const;
    
    // Helper: compute grass color from tile properties
    Color computeGrassColor(uint8_t temperature, uint8_t moisture, 
                           uint8_t biological, uint8_t tileType) const;
};

#endif // GRASS_HPP
