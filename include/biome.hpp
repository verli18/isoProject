#ifndef BIOME_HPP
#define BIOME_HPP

#include <cstdint>
#include <string>
#include "worldGenerator.hpp"

// Biome types - ordered by priority for texture blending
enum class BiomeType : uint8_t {
    // Ocean/Water (handled separately)
    OCEAN = 0,
    
    // Climate-based biomes
    TEMPERATE_GRASSLAND,
    TEMPERATE_FOREST,
    BOREAL_FOREST,
    TUNDRA,
    ARID_DESERT,
    SAVANNA,
    RAINFOREST,
    
    // Geological override biomes (high potential values)
    VOLCANIC_WASTES,      // High magmatic
    CRYSTALLINE_FIELDS,   // High crystalline  
    SULFURIC_VENTS,       // High sulfide
    WETLANDS,             // High hydrological
    LUSH_VALLEY,          // High biological
    
    // Transition/special
    BEACH,
    RIVER_BANK,
    
    COUNT
};

// Texture indices for each biome (maps to textureAtlas)
// Texture indices for each biome (maps to textureAtlas)
struct BiomeTextures {
    uint8_t primaryTexture;      // Top surface (index into texture atlas)
    uint8_t secondaryTexture;    // For erosion/exposed ground
    uint8_t transitionTexture;   // Optional, for special transitions
};

// Grass properties for the biome
struct GrassProps {
    bool enabled;              // Does this biome have grass?
    float densityBase;         // Base density (0-1)
    float densityVariation;    // How much noise/potential affects density
    float heightMultiplier;    // Blade height scale
    float patchiness;          // 0 = uniform, 1 = very patchy
    float patchScale;          // Size of patches in world units
    Vector3 tipColor;          // Grass tip color
    Vector3 baseColor;         // Grass base color
};

// Complete biome definition
struct BiomeData {
    BiomeType type;
    const char* name;
    
    // Textures
    BiomeTextures textures;
    
    // Grass configuration
    GrassProps grass;
    
    // Terrain generation parameters
    float heightMultiplier;    // Scale base height
    float heightOffset;        // Add/subtract from base
    float roughness;           // Detail noise strength
    float featureScale;        // Scale of biome-specific features
    
    // Feature generation
    bool hasVolcanoes;
    bool hasSinkholes;
    bool hasCrystalSpires;
    bool hasGeysers;
    
    // Climate ranges (for biome selection)
    float minTemp, maxTemp;
    float minHumidity, maxHumidity;
    
    // Dominant potential (for geological override)
    // -1 = no override, 0-4 = magmatic/hydro/sulfide/crystal/bio
    int dominantPotential;
    float potentialThreshold;
    
    // Transition rules
    BiomeType blendTargets[4];     // Which biomes this can blend to
    float blendThresholds[4];      // At what parameter delta to blend
};

/**
 * BiomeManager - Handles biome selection and terrain modification
 */
class BiomeManager {
public:
    // Singleton access
    static BiomeManager& getInstance();
    
    // Delete copy/move
    BiomeManager(const BiomeManager&) = delete;
    BiomeManager& operator=(const BiomeManager&) = delete;
    
    // Initialize biome data
    void initialize();
    
    // Get biome at world position
    BiomeType getBiomeAt(const PotentialData& potential) const;
    
    // Get biome data
    const BiomeData& getBiomeData(BiomeType type) const;
    
    // Get all biome data (for debug UI)
    const BiomeData* getAllBiomes() const { return biomes; }
    
    // Modify height based on biome
    float modifyHeight(float baseHeight, BiomeType biome, const PotentialData& potential, float worldX, float worldZ) const;
    
    // Get texture for biome
    uint8_t getTopTexture(BiomeType biome) const;
    uint8_t getSideTexture(BiomeType biome) const;
    
    // Biome blending weight (0-1) at position
    // Returns how much of each biome to blend at edges
    void getBlendWeights(
        const PotentialData& potential,
        BiomeType& primary, BiomeType& secondary,
        float& primaryWeight
    ) const;
    
private:
    BiomeManager() = default;
    ~BiomeManager() = default;
    
    void setupBiomeData();
    
    // Check if a geological potential overrides climate biome
    BiomeType checkGeologicalOverride(const PotentialData& p) const;
    
    // Get climate-based biome
    BiomeType getClimateBiome(float temperature, float humidity) const;
    
    BiomeData biomes[static_cast<int>(BiomeType::COUNT)];
    bool initialized = false;
};

// Convenience function
inline BiomeType getBiome(const PotentialData& p) {
    return BiomeManager::getInstance().getBiomeAt(p);
}

#endif // BIOME_HPP
