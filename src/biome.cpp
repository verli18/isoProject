#include "../include/biome.hpp"
#include "../include/textureAtlas.hpp"
#include <cmath>
#include <algorithm>

BiomeManager& BiomeManager::getInstance() {
    static BiomeManager instance;
    return instance;
}

void BiomeManager::initialize() {
    if (initialized) return;
    setupBiomeData();
    initialized = true;
}

void BiomeManager::setupBiomeData() {
    // Default all biomes first
    for (int i = 0; i < static_cast<int>(BiomeType::COUNT); ++i) {
        biomes[i] = {};
        biomes[i].type = static_cast<BiomeType>(i);
        biomes[i].heightMultiplier = 1.0f;
        biomes[i].heightOffset = 0.0f;
        biomes[i].roughness = 1.0f;
        biomes[i].featureScale = 1.0f;
        biomes[i].dominantPotential = -1;
        biomes[i].potentialThreshold = 1.0f;  // Disable by default
        biomes[i].textures = {GRASS, STONE, GRASS};
    }
    
    // ============================================
    // SIMPLE BIOMES - Just 5 basic terrain types
    // ============================================
    
    // === Plains (default, temperate) ===
    auto& plains = biomes[static_cast<int>(BiomeType::TEMPERATE_GRASSLAND)];
    plains.name = "Plains";
    plains.textures = {GRASS, STONE, GRASS};
    plains.heightMultiplier = 1.0f;
    plains.heightOffset = 0.0f;
    plains.roughness = 1.0f;
    
    // === Desert (hot, dry) ===
    auto& desert = biomes[static_cast<int>(BiomeType::ARID_DESERT)];
    desert.name = "Desert";
    desert.textures = {SAND, SAND, SAND};
    desert.heightMultiplier = 1.0f;
    desert.heightOffset = 0.0f;
    desert.roughness = 1.0f;
    
    // === Cold Plains (cold, moderate humidity) ===
    auto& coldPlains = biomes[static_cast<int>(BiomeType::BOREAL_FOREST)];
    coldPlains.name = "Cold Plains";
    coldPlains.textures = {GRASS, STONE, GRASS};
    coldPlains.heightMultiplier = 1.0f;
    coldPlains.heightOffset = 0.0f;
    coldPlains.roughness = 1.0f;
    
    // === Snowy Plains (very cold) ===
    auto& snow = biomes[static_cast<int>(BiomeType::TUNDRA)];
    snow.name = "Snowy Plains";
    snow.textures = {SNOW, STONE, SNOW};
    snow.heightMultiplier = 1.0f;
    snow.heightOffset = 0.0f;
    snow.roughness = 1.0f;
    
    // === Mountains (high altitude via magmatic potential for now) ===
    auto& mountains = biomes[static_cast<int>(BiomeType::VOLCANIC_WASTES)];
    mountains.name = "Mountains";
    mountains.textures = {STONE, STONE, STONE};
    mountains.heightMultiplier = 1.0f;
    mountains.heightOffset = 0.0f;
    mountains.roughness = 1.0f;
    
    // Copy defaults to unused biomes so they don't cause issues
    biomes[static_cast<int>(BiomeType::OCEAN)] = plains;
    biomes[static_cast<int>(BiomeType::TEMPERATE_FOREST)] = plains;
    biomes[static_cast<int>(BiomeType::SAVANNA)] = desert;
    biomes[static_cast<int>(BiomeType::RAINFOREST)] = plains;
    biomes[static_cast<int>(BiomeType::CRYSTALLINE_FIELDS)] = mountains;
    biomes[static_cast<int>(BiomeType::SULFURIC_VENTS)] = mountains;
    biomes[static_cast<int>(BiomeType::WETLANDS)] = plains;
    biomes[static_cast<int>(BiomeType::LUSH_VALLEY)] = plains;
    biomes[static_cast<int>(BiomeType::BEACH)] = desert;
    biomes[static_cast<int>(BiomeType::RIVER_BANK)] = plains;
}

BiomeType BiomeManager::checkGeologicalOverride(const PotentialData& p) const {
    // DISABLED - return no override
    (void)p;
    return BiomeType::COUNT;
}

BiomeType BiomeManager::getClimateBiome(float temperature, float humidity) const {
    // Simple 5-biome selection based on temperature only (for now)
    // Temperature: 0 = cold, 1 = hot
    (void)humidity;  // Ignore humidity for simplicity
    
    if (temperature < 0.25f) {
        return BiomeType::TUNDRA;  // Snow
    }
    else if (temperature < 0.45f) {
        return BiomeType::BOREAL_FOREST;  // Cold plains
    }
    else if (temperature < 0.7f) {
        return BiomeType::TEMPERATE_GRASSLAND;  // Plains
    }
    else {
        return BiomeType::ARID_DESERT;  // Desert
    }
}

BiomeType BiomeManager::getBiomeAt(const PotentialData& potential) const {
    // No geological overrides for now
    return getClimateBiome(potential.temperature, potential.humidity);
}

const BiomeData& BiomeManager::getBiomeData(BiomeType type) const {
    return biomes[static_cast<int>(type)];
}

float BiomeManager::modifyHeight(
    float baseHeight,
    BiomeType biome,
    const PotentialData& potential,
    float worldX, float worldZ
) const {
    // SIMPLIFIED: Just return the base height with no modifications
    // This ensures terrain is purely from the noise generator
    (void)biome;
    (void)potential;
    (void)worldX;
    (void)worldZ;
    
    return baseHeight;
}

uint8_t BiomeManager::getTopTexture(BiomeType biome) const {
    return getBiomeData(biome).textures.topTexture;
}

uint8_t BiomeManager::getSideTexture(BiomeType biome) const {
    return getBiomeData(biome).textures.sideTexture;
}

void BiomeManager::getBlendWeights(
    const PotentialData& potential,
    BiomeType& primary, BiomeType& secondary,
    float& primaryWeight
) const {
    primary = getBiomeAt(potential);
    secondary = primary;
    primaryWeight = 1.0f;
}
