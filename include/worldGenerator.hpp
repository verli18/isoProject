#ifndef WORLDGENERATOR_HPP
#define WORLDGENERATOR_HPP

#include <cstdint>
#include <vector>
#include <raylib.h>
#include "FastNoise/FastNoise.h"
#include "FastNoise/Generators/DomainWarp.h"

// Forward declarations
class tileGrid;

// Fundamental potentials for terrain generation
struct PotentialData {
    float magmatic;      // 0-1: volcanic activity, heat sources
    float hydrological;  // 0-1: water table, aquifer pressure  
    float sulfide;       // 0-1: sulfur deposits, acidic soil
    float crystalline;   // 0-1: mineral density, gem formation
    float biological;    // 0-1: biomass density, organic matter
    float temperature;   // 0-1: normalized temperature
    float humidity;      // 0-1: normalized humidity
};

// Terrain analysis for feedback loop
struct TerrainAnalysis {
    float slope;         // 0-1: steepness (0 = flat, 1 = cliff)
    float curvature;     // -1 to 1: negative = valley, positive = ridge
    float flowAccum;     // Accumulated water flow (higher = river path)
};

// Generation config - tweak these for different world feels
struct WorldGenConfig {
    // Seed
    int seed = 1337;
    
    // Height settings
    float heightScale = 50.0f;          // Range of height variation
    float heightBase = 15.0f;           // Base height offset (keeps terrain above water)
    float heightExponent = 1.3f;        // Shape the height distribution
    
    // Noise frequencies (lower = larger features)
    float terrainFreq = 0.008f;         // Lowered for larger terrain features
    float regionFreq = 0.0007f;         // Larger continental regions
    float potentialFreq = 0.002f;       // Geological features span more area
    float climateFreq = 0.0005f;        // Biomes span even larger areas
    
    // Domain warp
    float warpAmplitude = 20.0f;        // More organic coastlines (increased)
    float warpFrequency = 0.008f;       // Larger-scale warping
    
    // Biome thresholds
    float geologicalOverrideThreshold = 0.85f;  // Fewer geological overrides
    float seaLevel = 0.0f;              // Disabled until water system ready
    
    // Feedback loop strengths
    float slopeToSulfide = 0.4f;
    float slopeToCrystalline = 0.3f;
    float flowToBiological = 0.3f;
    float flowToHydrological = 0.2f;
};

/**
 * WorldGenerator - Singleton managing all procedural generation
 * 
 * Centralizes noise generators and provides consistent world generation.
 * Call initialize() once at startup with a seed.
 */
class WorldGenerator {
public:
    // Singleton access
    static WorldGenerator& getInstance();
    
    // Delete copy/move
    WorldGenerator(const WorldGenerator&) = delete;
    WorldGenerator& operator=(const WorldGenerator&) = delete;
    
    // Initialize with seed (call once at startup)
    void initialize(int seed);
    void initialize(const WorldGenConfig& config);
    
    // Check if initialized
    bool isInitialized() const { return initialized; }
    
    // Get current config (for ImGui tweaking)
    WorldGenConfig& getConfig() { return config; }
    const WorldGenConfig& getConfig() const { return config; }
    
    // Re-initialize noise generators with current config (after config changes)
    void rebuildNoiseGenerators();
    
    // === Primary Generation API ===
    
    // Generate potentials for a region (world coordinates)
    PotentialData getPotentialAt(float worldX, float worldZ) const;
    
    // Generate potentials for a grid (more efficient for chunks)
    void generatePotentialGrid(
        std::vector<PotentialData>& out,
        int startX, int startZ,
        int width, int height
    ) const;
    
    // Get base terrain height at world position (before biome modification)
    float getBaseHeightAt(float worldX, float worldZ) const;
    
    // Get height grid for chunk (4 corners per tile for smooth terrain)
    void generateHeightGrid(
        std::vector<float>& out,
        int startX, int startZ,
        int width, int height  // +1 for corner vertices
    ) const;
    
    // === Water System ===
    
    // Get water surface level at world position (0 = no water)
    // Uses hydrological potential and terrain to determine water presence
    float getWaterLevelAt(float worldX, float worldZ, float groundHeight) const;
    
    // Generate water level grid for a chunk
    // Returns water surface Y for each tile (0 = no water)
    void generateWaterGrid(
        std::vector<float>& waterLevels,
        const std::vector<float>& groundHeights,
        const std::vector<PotentialData>& potentials,
        int startX, int startZ,
        int width, int height
    ) const;
    
    // === Erosion System ===
    
    // Apply particle-based hydraulic erosion to a height grid
    // Modifies heights in-place. Grid is (width x height) corner vertices.
    // startX, startZ: world coordinates of the grid origin (for cross-chunk sampling)
    // numDroplets: number of water droplets to simulate
    void applyErosion(
        std::vector<float>& heights,
        int startX, int startZ,
        int width, int height,
        int numDroplets = 5000
    ) const;
    
    // === Terrain Analysis ===
    
    // Analyze terrain for feedback loop
    TerrainAnalysis analyzeTerrainAt(
        const std::vector<float>& heights,
        int x, int y,
        int gridWidth
    ) const;
    
    // Apply feedback loop to modify potentials based on terrain
    void applyFeedbackLoop(
        std::vector<PotentialData>& potentials,
        const std::vector<float>& heights,
        int width, int height
    ) const;
    
    // === Utility ===
    
    // Sample raw noise (for debugging/visualization)
    float sampleNoise(int noiseType, float x, float z, float frequency) const;
    
private:
    WorldGenerator() = default;
    ~WorldGenerator() = default;
    
    bool initialized = false;
    WorldGenConfig config;
    
    // === Noise Generators ===
    
    // Terrain height
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseHeight;
    FastNoise::SmartNode<FastNoise::DomainWarpGradient> noiseWarp;
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseRegion;
    
    // Potentials
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseMagmatic;
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseHydrological;
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseSulfide;
    FastNoise::SmartNode<FastNoise::FractalRidged> noiseCrystalline;
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseBiological;
    
    // Climate
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseTemperature;
    FastNoise::SmartNode<FastNoise::FractalFBm> noiseHumidity;
    
    // Helper to create configured FBm noise
    FastNoise::SmartNode<FastNoise::FractalFBm> createFBmNoise(
        int octaves = 4,
        float gain = 0.5f,
        float lacunarity = 2.0f
    ) const;
};

#endif // WORLDGENERATOR_HPP
