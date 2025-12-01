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

// Helper to set up a default biome
void BiomeManager::setupBiomeData() {
    auto setDefault = [&](BiomeType type, const char* name) {
        biomes[static_cast<int>(type)] = {};
        auto& b = biomes[static_cast<int>(type)];
        b.type = type;
        b.name = name;
        b.heightMultiplier = 1.0f;
        b.heightOffset = 0.0f;
        b.roughness = 1.0f;
        b.featureScale = 1.0f;
        b.dominantPotential = -1;
        b.potentialThreshold = 1.0f;
        
        // Default textures
        b.textures.primaryTexture = GRASS;
        b.textures.secondaryTexture = STONE;
        b.textures.transitionTexture = GRASS;
        
        // Default grass
        b.grass.enabled = true;
        b.grass.densityBase = 0.8f;
        b.grass.densityVariation = 0.2f;
        b.grass.heightMultiplier = 1.0f;
        b.grass.patchiness = 0.0f;
        b.grass.patchScale = 1.0f;
        b.grass.tipColor = {0.25f, 0.40f, 0.12f};
        b.grass.baseColor = {0.15f, 0.30f, 0.05f};
        
        // Default climate (wide range)
        b.minTemp = 0.0f; b.maxTemp = 1.0f;
        b.minHumidity = 0.0f; b.maxHumidity = 1.0f;
        
        // Default transitions (none)
        for(int i=0; i<4; i++) {
            b.blendTargets[i] = BiomeType::COUNT;
            b.blendThresholds[i] = 0.0f;
        }
    };

    // Initialize all with defaults first
    for (int i = 0; i < static_cast<int>(BiomeType::COUNT); ++i) {
        setDefault(static_cast<BiomeType>(i), "Unknown");
    }
    
    // ============================================
    // DETAILED BIOME SETUP
    // ============================================
    
    // === TEMPERATE GRASSLAND (Plains) ===
    {
        auto& b = biomes[static_cast<int>(BiomeType::TEMPERATE_GRASSLAND)];
        b.name = "Temperate Grassland";
        b.textures = {GRASS, STONE, GRASS};
        b.minTemp = 0.45f; b.maxTemp = 0.7f;
        b.minHumidity = 0.3f; b.maxHumidity = 0.7f;
        
        // Lush green grass
        b.grass.densityBase = 0.9f;
        b.grass.tipColor = {0.25f, 0.45f, 0.12f};
        b.grass.baseColor = {0.15f, 0.35f, 0.05f};
        
        // Blends to:
        b.blendTargets[0] = BiomeType::BOREAL_FOREST; // Cold edge
        b.blendThresholds[0] = 0.45f; // Blend when temp drops near this
        
        b.blendTargets[1] = BiomeType::ARID_DESERT;   // Hot/Dry edge
        b.blendThresholds[1] = 0.7f;  // Blend when temp rises near this
    }
    
    // === ARID DESERT ===
    {
        auto& b = biomes[static_cast<int>(BiomeType::ARID_DESERT)];
        b.name = "Arid Desert";
        b.textures = {SAND, SAND, SAND};
        b.minTemp = 0.7f; b.maxTemp = 1.0f;
        b.minHumidity = 0.0f; b.maxHumidity = 0.3f;
        
        // Sparse, patchy, dry grass
        b.grass.densityBase = 0.05f;
        b.grass.densityVariation = 0.8f; // Highly dependent on potential
        b.grass.heightMultiplier = 0.4f;
        b.grass.patchiness = 0.9f;       // Very patchy
        b.grass.patchScale = 8.0f;
        b.grass.tipColor = {0.70f, 0.60f, 0.40f};
        b.grass.baseColor = {0.50f, 0.40f, 0.25f};
        
        // Blends to:
        b.blendTargets[0] = BiomeType::TEMPERATE_GRASSLAND; // Wetter edge
        b.blendThresholds[0] = 0.3f; // Blend when humidity rises
    }
    
    // === BOREAL FOREST (Cold Plains) ===
    {
        auto& b = biomes[static_cast<int>(BiomeType::BOREAL_FOREST)];
        b.name = "Boreal Forest";
        b.textures = {GRASS, STONE, GRASS};
        b.minTemp = 0.25f; b.maxTemp = 0.45f;
        
        // Darker, cooler grass
        b.grass.densityBase = 0.7f;
        b.grass.tipColor = {0.20f, 0.35f, 0.25f};
        b.grass.baseColor = {0.10f, 0.25f, 0.15f};
        
        // Blends to:
        b.blendTargets[0] = BiomeType::TUNDRA; // Colder edge
        b.blendThresholds[0] = 0.25f;
        
        b.blendTargets[1] = BiomeType::TEMPERATE_GRASSLAND; // Warmer edge
        b.blendThresholds[1] = 0.45f;
    }
    
    // === TUNDRA (Snowy Plains) ===
    {
        auto& b = biomes[static_cast<int>(BiomeType::TUNDRA)];
        b.name = "Tundra";
        b.textures = {SNOW, STONE, SNOW};
        b.minTemp = 0.0f; b.maxTemp = 0.25f;
        
        // Very sparse, frozen grass
        b.grass.densityBase = 0.2f;
        b.grass.heightMultiplier = 0.3f;
        b.grass.tipColor = {0.40f, 0.45f, 0.50f};
        b.grass.baseColor = {0.30f, 0.35f, 0.40f};
        
        // Blends to:
        b.blendTargets[0] = BiomeType::BOREAL_FOREST; // Warmer edge
        b.blendThresholds[0] = 0.25f;
    }
    
    // === VOLCANIC WASTES (Mountains) ===
    {
        auto& b = biomes[static_cast<int>(BiomeType::VOLCANIC_WASTES)];
        b.name = "Volcanic Wastes";
        b.textures = {STONE, STONE, STONE};
        b.dominantPotential = 0; // Magmatic
        b.potentialThreshold = 0.6f;
        
        // Ash-covered grass? Mostly none
        b.grass.enabled = false;
        b.grass.densityBase = 0.0f;
    }
    
    // Copy to similar types
    biomes[static_cast<int>(BiomeType::SAVANNA)] = biomes[static_cast<int>(BiomeType::ARID_DESERT)];
    biomes[static_cast<int>(BiomeType::SAVANNA)].name = "Savanna";
    biomes[static_cast<int>(BiomeType::SAVANNA)].grass.densityBase = 0.4f; // More grass than desert
    
    biomes[static_cast<int>(BiomeType::TEMPERATE_FOREST)] = biomes[static_cast<int>(BiomeType::TEMPERATE_GRASSLAND)];
    biomes[static_cast<int>(BiomeType::TEMPERATE_FOREST)].name = "Temperate Forest";
}

BiomeType BiomeManager::checkGeologicalOverride(const PotentialData& p) const {
    // Check for high potentials that override climate
    if (p.magmatic > 0.7f) return BiomeType::VOLCANIC_WASTES;
    // Add others here...
    return BiomeType::COUNT;
}

BiomeType BiomeManager::getClimateBiome(float temperature, float humidity) const {
    // Simple selection logic matching the setup ranges
    if (temperature < 0.25f) return BiomeType::TUNDRA;
    if (temperature < 0.45f) return BiomeType::BOREAL_FOREST;
    if (temperature < 0.7f) return BiomeType::TEMPERATE_GRASSLAND;
    
    // Hot biomes split by humidity
    if (humidity < 0.3f) return BiomeType::ARID_DESERT;
    return BiomeType::SAVANNA; // Or rainforest if very wet
}

BiomeType BiomeManager::getBiomeAt(const PotentialData& potential) const {
    BiomeType override = checkGeologicalOverride(potential);
    if (override != BiomeType::COUNT) return override;
    
    return getClimateBiome(potential.temperature, potential.humidity);
}

const BiomeData& BiomeManager::getBiomeData(BiomeType type) const {
    if (static_cast<int>(type) >= static_cast<int>(BiomeType::COUNT)) {
        return biomes[0]; // Fallback
    }
    return biomes[static_cast<int>(type)];
}

float BiomeManager::modifyHeight(
    float baseHeight,
    BiomeType biome,
    const PotentialData& potential,
    float worldX, float worldZ
) const {
    // TODO: Implement biome-specific height kernels
    return baseHeight;
}

uint8_t BiomeManager::getTopTexture(BiomeType biome) const {
    return getBiomeData(biome).textures.primaryTexture;
}

uint8_t BiomeManager::getSideTexture(BiomeType biome) const {
    return getBiomeData(biome).textures.secondaryTexture;
}

void BiomeManager::getBlendWeights(
    const PotentialData& potential,
    BiomeType& primary, BiomeType& secondary,
    float& primaryWeight
) const {
    primary = getBiomeAt(potential);
    secondary = primary;
    primaryWeight = 1.0f;
    
    const BiomeData& data = getBiomeData(primary);
    
    // Check blend targets
    // This is a simplified blend logic. A robust one would check all neighbors in 2D space,
    // but here we just check if we are close to the threshold of a neighbor biome
    // based on the current potential values.
    
    float temp = potential.temperature;
    float humid = potential.humidity;
    
    // Check against the hardcoded thresholds we used in setup
    // This should ideally be data-driven from the blendTargets array, 
    // but for now we'll implement the specific logic for the main transitions.
    
    // TUNDRA <-> BOREAL (Threshold 0.25)
    if (primary == BiomeType::TUNDRA) {
        if (temp > 0.20f) { // Approaching Boreal
            secondary = BiomeType::BOREAL_FOREST;
            // 0.20 -> 0.25 range maps to 1.0 -> 0.5 weight (start blending)
            // Actually we want primaryWeight to decrease as we get closer to the other biome
            // Let's say blend zone is 0.05 wide
            float t = (temp - 0.20f) / 0.10f; // 0.20->0.30 transition
            primaryWeight = std::clamp(1.0f - t, 0.0f, 1.0f);
        }
    }
    else if (primary == BiomeType::BOREAL_FOREST) {
        if (temp < 0.30f) { // Approaching Tundra
            secondary = BiomeType::TUNDRA;
            float t = (0.30f - temp) / 0.10f;
            primaryWeight = std::clamp(1.0f - t, 0.0f, 1.0f);
        }
        else if (temp > 0.40f) { // Approaching Grassland (0.45)
            secondary = BiomeType::TEMPERATE_GRASSLAND;
            float t = (temp - 0.40f) / 0.10f;
            primaryWeight = std::clamp(1.0f - t, 0.0f, 1.0f);
        }
    }
    else if (primary == BiomeType::TEMPERATE_GRASSLAND) {
        if (temp < 0.50f) { // Approaching Boreal (0.45)
            secondary = BiomeType::BOREAL_FOREST;
            float t = (0.50f - temp) / 0.10f;
            primaryWeight = std::clamp(1.0f - t, 0.0f, 1.0f);
        }
        else if (temp > 0.65f) { // Approaching Desert (0.7)
            secondary = BiomeType::ARID_DESERT;
            float t = (temp - 0.65f) / 0.10f;
            primaryWeight = std::clamp(1.0f - t, 0.0f, 1.0f);
        }
    }
    else if (primary == BiomeType::ARID_DESERT) {
        if (temp < 0.75f && humid > 0.25f) { // Approaching Grassland
            secondary = BiomeType::TEMPERATE_GRASSLAND;
            float t = (0.75f - temp) / 0.10f;
            primaryWeight = std::clamp(1.0f - t, 0.0f, 1.0f);
        }
    }
}
