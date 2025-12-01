#ifndef VISUAL_SETTINGS_HPP
#define VISUAL_SETTINGS_HPP

#include <raylib.h>
#include <string>

// Forward declarations
struct WorldGenConfig;
struct ErosionConfig;

/**
 * GrassSettings - Parameters for grass rendering
 */
struct GrassSettings {
    // === Base grass colors (warm/temperate biomes) ===
    Vector3 tipColor = {0.45f, 0.65f, 0.25f};     // Top of grass blade
    Vector3 baseColor = {0.25f, 0.40f, 0.12f};    // Bottom of grass blade
    
    // === Tundra colors (cold biomes, before full snow) ===
    Vector3 tundraTipColor = {0.55f, 0.50f, 0.35f};   // Golden/brown dead grass tips
    Vector3 tundraBaseColor = {0.35f, 0.30f, 0.20f};  // Dark brown base
    
    // === Snow/frozen colors (very cold biomes) ===
    Vector3 snowTipColor = {0.75f, 0.80f, 0.90f};     // Icy white-blue tips
    Vector3 snowBaseColor = {0.50f, 0.55f, 0.65f};    // Frosty blue-gray base
    
    // === Desert/sand colors (hot/dry biomes) ===
    Vector3 desertTipColor = {0.70f, 0.60f, 0.40f};   // Pale beige/tan tips
    Vector3 desertBaseColor = {0.50f, 0.40f, 0.25f};  // Darker tan base
    
    // === Temperature thresholds for biome blending ===
    float tundraStartTemp = 0.35f;    // Below this temp, start blending to tundra
    float tundraFullTemp = 0.25f;     // At this temp, fully tundra
    float snowStartTemp = 0.20f;      // Below this, start blending to snow grass
    float snowFullTemp = 0.10f;       // At this temp, fully snow (or no grass)
    float noGrassTemp = 0.05f;        // Below this, no grass at all (too frozen)
    
    // === Hot biome thresholds ===
    float desertStartTemp = 0.65f;    // Above this, start blending to desert
    float desertFullTemp = 0.80f;     // At this temp, fully desert grass
    
    // Dirt transition settings
    Vector3 dirtBlendColor = {0.35f, 0.28f, 0.18f};
    float dirtBlendDistance = 1.5f;
    float dirtBlendStrength = 0.6f;
    
    // Climate-based color variation (on top of biome blending)
    float temperatureInfluence = 0.08f;
    float moistureInfluence = 0.10f;
    float biologicalInfluence = 0.10f;
    
    // === Wind parameters ===
    float windStrength = 0.15f;
    float windSpeed = 2.0f;
    Vector2 windDirection = {0.7f, 0.7f};
    
    // === Blade geometry ===
    float baseHeight = 0.8f;
    float heightVariation = 0.4f;
    float bladeWidth = 0.06f;          // Width of grass blades
    float bladeTaper = 0.3f;           // How much blade narrows at tip (0-1)
    
    // === Per-biome height multipliers ===
    float tundraHeightMult = 0.6f;     // Tundra grass is shorter
    float snowHeightMult = 0.3f;       // Snow grass is very short (stubble)
    float desertHeightMult = 0.5f;     // Desert grass is sparse and short
    
    // === Density settings ===
    float bladesPerTile = 50.0f;       // Base density
    float moistureMultiplier = 0.5f;   // Extra density from moisture (0-1 adds 0-50%)
    float slopeReduction = 0.7f;       // Reduce density on steep slopes (erosion-based)
    float minDensity = 0.1f;           // Minimum grass density before skipping
    
    // === Per-biome density multipliers ===
    float tundraDensityMult = 0.5f;    // Tundra is sparser
    float snowDensityMult = 0.2f;      // Snow has very sparse grass
    float desertDensityMult = 0.15f;   // Desert has very sparse grass
    float stoneDensityMult = 0.05f;    // Stone has almost no grass
    
    // === Render distance and LOD ===
    float renderDistance = 80.0f;      // Max distance to render grass
    float fadeStartDistance = 60.0f;   // Start fading at this distance
    int lodLevels = 3;                 // Number of LOD levels (1-4)
    float lodReduction = 0.5f;         // Density multiplier per LOD level
};

/**
 * WaterSettings - Parameters for water rendering
 */
struct WaterSettings {
    // HSV modifications to base water color
    float hueShift = -0.1f;
    float saturationMult = 4.0f;
    float valueMult = 0.5f;
    
    // Depth-based alpha
    float minDepth = 0.0f;
    float maxDepth = 4.0f;
    float minAlpha = 0.4f;
    float maxAlpha = 1.0f;
    
    // Base color (RGB normalized)
    Vector3 shallowColor = {0.156f, 0.47f, 0.96f};
    Vector3 deepColor = {0.05f, 0.3f, 0.6f};
    
    // Animation
    float flowSpeed = 0.15f;
    float rippleSpeed = 0.02f;
    float displacementIntensity = 0.06f;
};

/**
 * TerrainSettings - Parameters for terrain rendering
 */
struct TerrainSettings {
    // These could hold per-biome color tints in the future
    float colorSaturation = 1.0f;
    float colorBrightness = 1.0f;
    
    // Erosion-based texture blending
    // When erosionFactor > erosionThreshold, start showing exposed ground
    float erosionThreshold = 0.05f;       // Start blending at 5% erosion (lower = more visible)
    float erosionFullExpose = 0.4f;       // Fully exposed at 40% erosion
    
    // Per-biome exposed texture U offsets in atlas (pixel offset, 0-80)
    // Atlas is 80x16, each tile is 16x16
    // GRASS side (dirt) at 16,0
    // STONE at 48,0
    // SAND at 64,0
    int grassExposedU = 16;    // Dirt texture for grass biomes
    int snowExposedU = 48;     // Stone for snow biome
    int sandExposedU = 48;     // Stone for desert
    int stoneExposedU = 48;    // Stone for cliffs
    
    // Dither intensity (0 = hard edge, 1 = full Bayer dither)
    float ditherIntensity = 1.0f;
    
    // Debug visualization modes
    int visualizationMode = 0;
    // 0: Normal rendering
    // 1: Erosion (red intensity)
    // 2: Moisture (blue intensity)
    // 3: Temperature (red-blue gradient)
    // 4: Slope/steepness (white intensity)
    // 5: Biome type (color coded)
    
    // Biome transition texture U offsets (for grass->tundra->snow dithering)
    // These specify which textures to blend into as temperature drops
    int tundraTextureU = 32;   // Snow texture (32) used for tundra transition - or could add a tundra grass
    int snowTextureU = 32;     // Snow texture (32) for frozen areas
};

/**
 * LightingSettings - Sun and ambient lighting
 */
struct LightingSettings {
    Vector3 sunDirection = {0.59f, -1.0f, -0.8f};
    Vector3 sunColor = {1.0f, 0.9f, 0.7f};
    float ambientStrength = 0.5f;
    Vector3 ambientColor = {0.4f, 0.5f, 0.8f};
    
    // HSV shift based on lighting
    float shiftIntensity = -0.05f;
    float shiftDisplacement = 1.86f;
};

/**
 * VisualSettings - Centralized visual configuration
 * 
 * Singleton pattern for easy access throughout the codebase.
 * All visual parameters are tweakable at runtime via ImGui.
 */
class VisualSettings {
public:
    // Singleton access
    static VisualSettings& getInstance() {
        static VisualSettings instance;
        return instance;
    }
    
    // Delete copy/move
    VisualSettings(const VisualSettings&) = delete;
    VisualSettings& operator=(const VisualSettings&) = delete;
    
    // Initialize with default values (already done in struct initializers)
    void initialize();
    
    // Reset to defaults
    void resetToDefaults();
    
    // Save/Load settings to file
    bool saveToFile(const std::string& filename = "visual_settings.ini") const;
    bool loadFromFile(const std::string& filename = "visual_settings.ini");
    
    // Save/Load with generation config (full settings including world gen)
    bool saveAllSettings(const std::string& filename, 
                         const WorldGenConfig* worldGen = nullptr,
                         const ErosionConfig* erosion = nullptr) const;
    bool loadAllSettings(const std::string& filename,
                         WorldGenConfig* worldGen = nullptr,
                         ErosionConfig* erosion = nullptr);
    
    // Default settings file (auto-loaded on startup)
    static constexpr const char* DEFAULT_SETTINGS_FILE = "default_settings.ini";
    
    // Settings accessors
    GrassSettings& getGrassSettings() { return grass; }
    const GrassSettings& getGrassSettings() const { return grass; }
    
    WaterSettings& getWaterSettings() { return water; }
    const WaterSettings& getWaterSettings() const { return water; }
    
    TerrainSettings& getTerrainSettings() { return terrain; }
    const TerrainSettings& getTerrainSettings() const { return terrain; }
    
    LightingSettings& getLightingSettings() { return lighting; }
    const LightingSettings& getLightingSettings() const { return lighting; }
    
    // Flag to notify systems that settings changed
    bool isDirty() const { return dirty; }
    void markDirty() { dirty = true; }
    void clearDirty() { dirty = false; }
    
private:
    VisualSettings() = default;
    ~VisualSettings() = default;
    
    GrassSettings grass;
    WaterSettings water;
    TerrainSettings terrain;
    LightingSettings lighting;
    
    bool dirty = true;  // Start dirty so initial values are applied
    bool initialized = false;
};

#endif // VISUAL_SETTINGS_HPP
