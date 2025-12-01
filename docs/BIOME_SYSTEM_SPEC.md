# Biome System Overhaul Specification

## Current Problems

### 1. Scattered Biome Logic
- **tileGrid.cpp**: Hardcoded temperature thresholds for blend calculations
- **grass.cpp**: Duplicate temperature thresholds for grass appearance
- **terrainShader.fs**: Yet another set of thresholds for texture blending
- **grassShader.fs**: Same thresholds duplicated again
- **resourceManager.cpp**: Per-biome uniform locations (doesn't scale)

### 2. Texture Type vs Biome Type Confusion
- `BiomeType` enum (biome.hpp): 15+ biomes (TUNDRA, ARID_DESERT, SAVANNA, etc.)
- Texture enum (textureAtlas.hpp): Only 5 types (AIR, GRASS, SNOW, STONE, SAND)
- Current code conflates these - a "biome" isn't the same as a "texture"

### 3. Broken Blend Logic
```cpp
// Current broken code in tileGrid.cpp:
if (t.type == SAND) {
    if (tempNorm < 0.75f || moistNorm > 0.4f) {  // Almost always true!
        t.secondaryType = GRASS;
        // This makes deserts look like grass
    }
}
```

### 4. No Grass Patching System
- Grass density is uniform per biome
- No support for sparse patches in deserts
- Biological potential exists but isn't used for clustering

### 5. Shader Uniform Explosion
Adding a new biome requires:
1. Add to `GrassSettings` struct
2. Add to `GrassShaderLocs` struct  
3. Add `GetShaderLocation()` calls
4. Add `SetShaderValue()` calls in multiple places
5. Add uniforms to shader files
6. Add blend logic in shader

---

## Proposed Architecture

### Core Principle: Single Source of Truth

```
BiomeManager (C++)
       │
       ├── Determines biome from PotentialData
       ├── Provides texture IDs for each biome
       ├── Provides grass properties per biome
       ├── Provides blend rules
       │
       ▼
   Tile Data
       │
       ├── biomeId (uint8_t) - Primary biome
       ├── secondaryBiomeId (uint8_t) - For transitions
       ├── blendFactor (uint8_t) - 0-255 blend amount
       │
       ▼
   Shaders (minimal logic)
       │
       └── Use biomeId to index into properties
```

### 1. Enhanced BiomeData Structure

```cpp
// In biome.hpp
struct BiomeData {
    BiomeType type;
    const char* name;
    
    // === TEXTURES ===
    uint8_t primaryTexture;      // Index into texture atlas (GRASS=1, SNOW=2, etc.)
    uint8_t secondaryTexture;    // For erosion/exposed ground
    uint8_t transitionTexture;   // Optional, for special transitions
    
    // === GRASS PROPERTIES ===
    struct GrassProps {
        bool enabled;              // Does this biome have grass?
        float densityBase;         // Base density (0-1)
        float densityVariation;    // How much noise affects density
        float heightMultiplier;    // Blade height scale
        float patchiness;          // 0 = uniform, 1 = very patchy
        float patchScale;          // Size of patches in world units
        Vector3 tipColor;          // Grass tip color
        Vector3 baseColor;         // Grass base color
    } grass;
    
    // === CLIMATE RANGES (for biome selection) ===
    float minTemp, maxTemp;
    float minHumidity, maxHumidity;
    
    // === TRANSITION RULES ===
    BiomeType blendTargets[4];     // Which biomes this can blend to
    float blendThresholds[4];      // At what parameter delta to blend
};
```

### 2. Simplified Tile Data

```cpp
// In tileGrid.hpp
struct tile {
    // Heights (unchanged)
    float tileHeight[4];
    
    // === NEW: Biome-centric data ===
    BiomeType biome;              // Primary biome (from BiomeManager)
    BiomeType secondaryBiome;     // For edge blending
    uint8_t blendStrength;        // 0=100% primary, 255=100% secondary
    
    // === Derived texture (computed from biome) ===
    uint8_t textureId;            // Cached from BiomeManager
    
    // === Climate data (for reference/debugging) ===
    uint8_t temperature;
    uint8_t moisture;
    
    // === Other potentials (unchanged) ===
    uint8_t erosionFactor;
    uint8_t biologicalPotential;
    // ... etc
};
```

### 3. Vertex Color Encoding (GPU)

```
R = textureId (primary)      - Which texture to sample
G = secondaryTextureId       - For dithered blending
B = blendStrength            - How much to blend (0-255)
A = erosionFactor            - For exposed ground dithering
```

The shader doesn't need to know about biomes - it just blends textures.

### 4. Grass Instance Data

```cpp
struct GrassBladeData {
    // Position encoded in transform matrix
    
    float r, g, b;      // Pre-computed color from BiomeManager
    float diffuse;      // Pre-computed lighting
    
    // NEW: Biome info for shader
    uint8_t biomeId;    // For any biome-specific effects
    uint8_t flags;      // Frozen, dried, etc.
};
```

### 5. Shader Simplification

**Before (current):**
```glsl
uniform vec3 grassTipColor;
uniform vec3 grassBaseColor;
uniform vec3 tundraTipColor;
uniform vec3 tundraBaseColor;
uniform vec3 snowTipColor;
uniform vec3 snowBaseColor;
uniform vec3 desertTipColor;
uniform vec3 desertBaseColor;
uniform float tundraStartTemp;
uniform float tundraFullTemp;
// ... 20+ uniforms per biome type
```

**After (proposed):**
```glsl
// Option A: Hardcoded switch (simple, fast)
vec3 getBiomeColor(int biomeId, float height) {
    switch(biomeId) {
        case BIOME_GRASSLAND: return mix(vec3(0.25, 0.40, 0.12), vec3(0.45, 0.65, 0.25), height);
        case BIOME_DESERT:    return mix(vec3(0.50, 0.40, 0.25), vec3(0.70, 0.60, 0.40), height);
        case BIOME_TUNDRA:    return mix(vec3(0.35, 0.30, 0.20), vec3(0.55, 0.50, 0.35), height);
        case BIOME_SNOW:      return mix(vec3(0.50, 0.55, 0.65), vec3(0.75, 0.80, 0.90), height);
        default:              return vec3(0.5);
    }
}

// Option B: Texture lookup (more flexible, slightly slower)
uniform sampler1D biomeColorLUT;  // 16 biomes × 2 colors (base/tip) = 32 texels
vec3 getBiomeColor(int biomeId, float height) {
    vec3 base = texture(biomeColorLUT, (biomeId * 2 + 0) / 32.0).rgb;
    vec3 tip  = texture(biomeColorLUT, (biomeId * 2 + 1) / 32.0).rgb;
    return mix(base, tip, height);
}
```

---

## Grass Patching System

### Concept
Use noise + biological potential to create natural-looking grass distribution:

```cpp
float getGrassDensity(int tileX, int tileZ, const BiomeData& biome, const tile& t) {
    // Base density from biome
    float density = biome.grass.densityBase;
    
    // Patchiness: use noise to create clusters
    if (biome.grass.patchiness > 0) {
        float noise = simplexNoise2D(
            tileX / biome.grass.patchScale,
            tileZ / biome.grass.patchScale
        );
        // Remap noise: -1..1 -> 0..1, then apply patchiness
        float patchFactor = (noise + 1.0f) * 0.5f;
        patchFactor = pow(patchFactor, 1.0f + biome.grass.patchiness * 3.0f);
        density *= patchFactor;
    }
    
    // Biological potential boost (oases in desert, etc.)
    float bio = t.biologicalPotential / 255.0f;
    density += bio * biome.grass.densityVariation;
    
    // Moisture boost for dry biomes
    if (biome.type == BiomeType::ARID_DESERT) {
        float moist = t.moisture / 255.0f;
        density *= 0.1f + moist * 2.0f;  // Low base, high near water
    }
    
    return std::clamp(density, 0.0f, 1.0f);
}
```

### Desert Grass Example
```cpp
BiomeData desert = {
    .type = BiomeType::ARID_DESERT,
    .grass = {
        .enabled = true,
        .densityBase = 0.05f,        // Very sparse base
        .densityVariation = 0.8f,    // High variation from bio potential
        .heightMultiplier = 0.4f,    // Short stubby grass
        .patchiness = 0.9f,          // Extremely patchy
        .patchScale = 8.0f,          // Large patches (oases)
        .tipColor = {0.70, 0.60, 0.40},  // Pale beige
        .baseColor = {0.50, 0.40, 0.25}
    }
};
```

---

## Implementation Plan

### Phase 1: Fix Immediate Issues (Quick)
1. **Fix desert blend bug** - Remove the overly aggressive SAND→GRASS blend
2. **Respect biome type** - Don't override SAND with GRASS based on temperature

### Phase 2: Centralize Biome Data (Medium)
1. Extend `BiomeData` struct with texture and grass properties
2. Initialize all biome data in `BiomeManager::setupBiomeData()`
3. Remove hardcoded thresholds from tileGrid.cpp
4. Use `BiomeManager::getBlendWeights()` instead of manual calculation

### Phase 3: Update Tile Generation (Medium)
1. Store `BiomeType` in tile (not just texture ID)
2. Compute texture ID from biome via `BiomeManager`
3. Use `BiomeManager` for blend decisions

### Phase 4: Simplify Shaders (Medium)
1. Remove per-biome uniforms from terrain/grass shaders
2. Add biome color lookup (switch or texture)
3. Pass biome ID through vertex data or uniform

### Phase 5: Add Grass Patching (Medium)
1. Add patchiness parameters to `BiomeData::GrassProps`
2. Implement noise-based density in grass generation
3. Add biological potential influence

### Phase 6: Cleanup (Small)
1. Remove unused uniforms from `GrassShaderLocs`, `TerrainShaderLocs`
2. Remove duplicate threshold constants
3. Update debug UI to use BiomeManager

---

## File Changes Summary

| File | Changes |
|------|---------|
| `include/biome.hpp` | Extend BiomeData with textures, grass props |
| `src/biome.cpp` | Initialize all biome data centrally |
| `include/tileGrid.hpp` | Add BiomeType to tile struct |
| `src/tileGrid.cpp` | Use BiomeManager, remove hardcoded thresholds |
| `src/grass.cpp` | Use BiomeManager for density/color, add patching |
| `assets/shaders/grassShader.fs` | Remove per-biome uniforms, add lookup |
| `assets/shaders/terrainShader.fs` | Simplify blend logic |
| `include/resourceManager.hpp` | Remove per-biome shader locs |
| `src/resourceManager.cpp` | Remove per-biome uniform setup |
| `include/visualSettings.hpp` | Keep settings but link to BiomeManager |

---

## Benefits

1. **Single Source of Truth**: All biome data in BiomeManager
2. **Easy to Add Biomes**: Just add entry to biome array
3. **Consistent Behavior**: Same thresholds everywhere
4. **Scalable**: No uniform explosion as biomes grow
5. **Debuggable**: Can inspect all biome data in one place
6. **Flexible Grass**: Patchy distribution looks natural

---

## Questions to Resolve

1. Should we use texture LUT or switch statement in shaders?
   - LUT: More flexible, can be edited at runtime
   - Switch: Simpler, slightly faster, compile-time

2. Should grass color be computed on CPU or GPU?
   - CPU: Pre-baked, no shader complexity
   - GPU: Dynamic, responds to lighting better

3. How to handle biome transitions for grass?
   - Option A: Grass follows primary biome only
   - Option B: Blend grass color at biome edges
   - Option C: Both biomes spawn grass, creates natural overlap

---

## Appendix: Current Texture Atlas Layout

```
| Offset | Texture | Used By Biomes |
|--------|---------|----------------|
| 0      | GRASS   | Grassland, Forest, etc. |
| 16     | (side)  | Grass sides |
| 32     | SNOW    | Tundra, Snow |
| 48     | STONE   | Volcanic, Mountains |
| 64     | SAND    | Desert, Beach |

Atlas size: 80×16 pixels
Tile size: 16×16 pixels
```
