#ifndef WORLDMAP_HPP
#define WORLDMAP_HPP

#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>
#include "worldGenerator.hpp"

/**
 * WorldMap - Manages world-scale terrain data with caching and simulation
 * 
 * This class solves the fundamental problem of chunk-based generation:
 * operations like erosion, water bodies, and rivers need to work across
 * chunk boundaries seamlessly.
 * 
 * Architecture:
 * - World is divided into "regions" (larger than chunks, e.g. 4x4 chunks)
 * - Each region caches heights, erosion results, water levels, etc.
 * - Regions overlap slightly to eliminate seams
 * - Chunks query the WorldMap for their data instead of generating it
 * 
 * The WorldMap is a singleton accessed via getInstance().
 */

// Forward declarations
struct PotentialData;

// Region size in tiles (should be multiple of chunk size)
// Larger = better erosion quality, more memory, slower initial generation
constexpr int REGION_SIZE = 128;  // 4x 32-tile chunks

// Overlap between regions to eliminate seams
constexpr int REGION_OVERLAP = 16;

/**
 * ErosionConfig - Tweakable erosion parameters
 */
struct ErosionConfig {
    // Erosion simulation
    int numDroplets = 5000;           // Number of water droplets per region
    int maxDropletLifetime = 60;      // Max steps per droplet
    float inertia = 0.3f;             // Droplet direction smoothing (0-1)
    float sedimentCapacity = 2.0f;    // How much sediment water can carry
    float minSedimentCapacity = 0.01f;
    float erodeSpeed = 0.15f;         // Erosion rate
    float depositSpeed = 0.15f;       // Deposition rate
    float evaporateSpeed = 0.01f;     // Water evaporation rate
    float gravity = 4.0f;             // Affects droplet speed on slopes
    float maxErodePerStep = 0.05f;    // Cap on erosion per step
    int erosionRadius = 2;            // Brush radius for erosion
    
    // Water detection (lakes)
    float waterMinDepth = 0.2f;       // Minimum depression depth for water
    int lakeDilation = 2;             // Dilate lakes by this many tiles
    
    // River detection - more aggressive settings for visible rivers
    int riverFlowThreshold = 15;      // Min flow accumulation for river (lower = more rivers)
    float riverWidthScale = 0.05f;    // How flow affects river width
    int maxRiverWidth = 6;            // Maximum river width in tiles
    float riverDepth = 0.5f;          // How deep rivers carve (increased!)
};

/**
 * RegionData - Cached data for a world region
 */
struct RegionData {
    // Region bounds in world coordinates
    int worldX, worldZ;  // Top-left corner
    int width, height;   // Always REGION_SIZE
    
    // Height data (corner vertices, so width+1 x height+1)
    std::vector<float> heights;
    
    // Erosion has been applied
    bool eroded = false;
    
    // Per-tile data
    std::vector<PotentialData> potentials;
    std::vector<float> waterLevels;      // 0 = no water, >0 = water surface Y
    std::vector<uint16_t> flowAccum;     // Flow accumulation (higher = more upstream area)
    std::vector<uint8_t> flowDir;        // D8 flow direction (0-7, 255 = pit)
    std::vector<uint8_t> riverWidth;     // River width at this tile (0 = no river)
    
    // Generation state
    bool heightsGenerated = false;
    bool potentialsGenerated = false;
    bool waterGenerated = false;
    
    // Get height at local coordinates (with bilinear interpolation)
    float getHeight(float localX, float localZ) const;
    
    // Get height at local integer coordinates
    float getHeightAt(int localX, int localZ) const;
    
    // Check if local coordinates are within this region
    bool contains(int localX, int localZ) const;
};

/**
 * WorldMap Singleton
 */
class WorldMap {
public:
    static WorldMap& getInstance();
    
    // Delete copy/move
    WorldMap(const WorldMap&) = delete;
    WorldMap& operator=(const WorldMap&) = delete;
    
    // Initialize (call after WorldGenerator is initialized)
    void initialize();
    
    // Clear all cached regions (call when seed changes or config changes)
    void clear();
    
    // Get erosion config for tweaking
    ErosionConfig& getErosionConfig() { return erosionConfig; }
    const ErosionConfig& getErosionConfig() const { return erosionConfig; }
    
    // === Primary Query API ===
    
    // Get terrain height at world position (will generate/cache as needed)
    float getHeight(float worldX, float worldZ);
    
    // Get heights for a chunk area (more efficient than individual queries)
    // Output is (width+1) x (height+1) corner vertices
    void getHeightGrid(
        std::vector<float>& out,
        int chunkWorldX, int chunkWorldZ,
        int width, int height
    );
    
    // Get potentials for a chunk area
    void getPotentialGrid(
        std::vector<PotentialData>& out,
        int chunkWorldX, int chunkWorldZ,
        int width, int height
    );
    
    // Get water levels for a chunk area
    void getWaterGrid(
        std::vector<float>& out,
        int chunkWorldX, int chunkWorldZ,
        int width, int height
    );
    
    // Get river data for a chunk area (flowDir, riverWidth, riverCase)
    void getRiverGrid(
        std::vector<uint8_t>& flowDirOut,
        std::vector<uint8_t>& riverWidthOut,
        int chunkWorldX, int chunkWorldZ,
        int width, int height
    );
    
    // === Region Management ===
    
    // Get or create a region containing the given world position
    RegionData& getRegion(int worldX, int worldZ);
    
    // Ensure a region is fully generated (heights + erosion + water)
    void ensureRegionReady(RegionData& region);
    
    // Preload regions around a position (for smooth gameplay)
    void preloadAround(int worldX, int worldZ, int radiusInRegions = 1);
    
private:
    WorldMap() = default;
    ~WorldMap() = default;
    
    bool initialized = false;
    ErosionConfig erosionConfig;
    
    // Region cache (key = packed region coordinates)
    std::unordered_map<int64_t, std::unique_ptr<RegionData>> regions;
    std::recursive_mutex regionMutex;  // Recursive to allow nested lookups during generation
    
    // Convert world position to region key
    int64_t worldToRegionKey(int worldX, int worldZ) const;
    
    // Get region origin for a world position
    void getRegionOrigin(int worldX, int worldZ, int& regionX, int& regionZ) const;
    
    // Internal generation functions
    void generateHeights(RegionData& region);
    void applyErosion(RegionData& region);
    void generatePotentials(RegionData& region);
    void generateWater(RegionData& region);
};

#endif // WORLDMAP_HPP
