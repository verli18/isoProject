#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;
in vec4 fragColor;  // Tile data: R=primary tex, G=secondary tex, B=blend strength, A=erosion

// Outputs
out vec4 finalColor;

// Uniforms
uniform sampler2D texture0;    // Diffuse texture atlas
uniform vec3 sunDirection;     // Direction to the sun (normalized)
uniform vec3 sunColor;         // Color of the sun light
uniform float ambientStrength; // Ambient light strength
uniform vec3 ambientColor;     // Ambient light color
uniform float shiftIntensity;
uniform float shiftDisplacement;

// Erosion dithering uniforms
uniform float erosionThreshold;   // Start showing exposed ground at this erosion level (0-1)
uniform float erosionFullExpose;  // Fully exposed at this erosion level (0-1)
uniform float ditherIntensity;    // How much dithering to apply (0 = sharp, 1 = full dither)

// Per-biome exposed texture U offsets (normalized 0-1)
uniform float grassExposedU;   // For grass biomes -> dirt
uniform float snowExposedU;    // For snow/tundra -> stone
uniform float sandExposedU;    // For desert/beach -> stone
uniform float stoneExposedU;   // For volcanic/crystalline -> stone

// Visualization mode (0=normal, 1=erosion, 2=blend strength, 3=secondary type, 4=slope, 5=biome)
uniform int visualizationMode;

// Texture atlas info (80x16 atlas, each tile is 16x16)
const float ATLAS_WIDTH = 80.0;
const float ATLAS_HEIGHT = 16.0;
const float TILE_SIZE = 16.0;

// Stone/dirt texture offset in atlas (tile index 3: stone at 48,0)
const vec2 STONE_UV_OFFSET = vec2(48.0 / ATLAS_WIDTH, 0.0);
const vec2 TILE_UV_SIZE = vec2(TILE_SIZE / ATLAS_WIDTH, TILE_SIZE / ATLAS_HEIGHT);

// 4x4 Bayer dither matrix (values 0-15, normalized to 0-1)
const float bayerMatrix[16] = float[16](
     0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
    12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
     3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
    15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
);

float getBayerValue(vec2 screenPos) {
    int x = int(mod(screenPos.x, 4.0));
    int y = int(mod(screenPos.y, 4.0));
    return bayerMatrix[y * 4 + x];
}

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Texture type constants (must match enum in textureAtlas.hpp)
// These are what t.type actually stores - texture indices, not BiomeType
const int TEX_AIR = 0;
const int TEX_GRASS = 1;
const int TEX_SNOW = 2;
const int TEX_STONE = 3;
const int TEX_SAND = 4;

// Get the exposed texture U offset based on tile texture type
float getExposedU(int texType) {
    // Grass tiles use dirt texture when eroded
    if (texType == TEX_GRASS) {
        return grassExposedU;
    }
    // Snow tiles use stone when eroded
    if (texType == TEX_SNOW) {
        return snowExposedU;
    }
    // Sand tiles use stone when eroded
    if (texType == TEX_SAND) {
        return sandExposedU;
    }
    // Stone and others use stone
    return stoneExposedU;
}

// Visualization mode colors
vec3 getVisualizationColor(int mode, float primaryType, float secondaryType, float blendStrength, float erosion, float slope) {
    if (mode == 1) {
        // Erosion: black (none) to red (high)
        return vec3(erosion, 0.0, 0.0);
    } else if (mode == 2) {
        // Blend strength: black (no blend) to green (full blend)
        return vec3(0.0, blendStrength, 0.0);
    } else if (mode == 3) {
        // Secondary type visualization
        int t = int(secondaryType * 255.0 + 0.5);
        if (t == TEX_AIR) return vec3(0.0, 0.0, 0.0);
        if (t == TEX_GRASS) return vec3(0.4, 0.7, 0.3);
        if (t == TEX_SNOW) return vec3(0.9, 0.95, 1.0);
        if (t == TEX_STONE) return vec3(0.5, 0.5, 0.5);
        if (t == TEX_SAND) return vec3(0.9, 0.8, 0.5);
        return vec3(1.0, 0.0, 1.0);
    } else if (mode == 4) {
        // Slope: green (flat) to white (steep)
        return mix(vec3(0.2, 0.6, 0.2), vec3(1.0, 1.0, 1.0), slope);
    } else if (mode == 5) {
        // Primary texture type visualization (add 0.5 for rounding)
        int t = int(primaryType * 255.0 + 0.5);
        if (t == TEX_AIR) return vec3(0.0, 0.0, 0.0);
        if (t == TEX_GRASS) return vec3(0.4, 0.7, 0.3);
        if (t == TEX_SNOW) return vec3(0.9, 0.95, 1.0);
        if (t == TEX_STONE) return vec3(0.5, 0.5, 0.5);
        if (t == TEX_SAND) return vec3(0.9, 0.8, 0.5);
        return vec3(1.0, 0.0, 1.0); // Unknown - magenta
    }
    return vec3(1.0); // Should not reach here
}

void main()
{
    // Extract tile data from vertex color
    // R = primary texture type (TEX_GRASS=1, TEX_SNOW=2, etc.)
    // G = secondary texture type (for biome blending)
    // B = blend strength (0=100% primary, 1=100% secondary)
    // A = erosion factor (0-1)
    float primaryTypeNorm = fragColor.r;     // Primary texture type (0-1, maps to 0-255)
    float secondaryTypeNorm = fragColor.g;   // Secondary texture type for blending
    float blendStrength = fragColor.b;       // Blend strength (0-1)
    float erosion = fragColor.a;             // Erosion factor (0-1)
    
    // Calculate slope from normal (for visualization)
    vec3 normal = normalize(fragNormal);
    float slope = 1.0 - max(dot(normal, vec3(0.0, 1.0, 0.0)), 0.0);
    
    // Handle visualization modes
    if (visualizationMode > 0 && visualizationMode <= 5) {
        vec3 visColor = getVisualizationColor(visualizationMode, primaryTypeNorm, secondaryTypeNorm, blendStrength, erosion, slope);
        // Apply basic lighting to visualization
        float diff = max(dot(normal, sunDirection), 0.0);
        vec3 lighting = ambientStrength * ambientColor + diff * sunColor;
        finalColor = vec4(lighting * visColor, 1.0);
        return;
    }
    
    // Normal rendering mode
    // Sample the main texture (surface texture like grass, sand, snow)
    vec4 surfaceColor = texture(texture0, fragTexCoord);
    
    // Get the exposed texture offset based on primary texture type
    // Add 0.5 before truncating to handle floating point precision issues
    int primaryType = int(primaryTypeNorm * 255.0 + 0.5);
    int secondaryType = int(secondaryTypeNorm * 255.0 + 0.5);
    float exposedU = getExposedU(primaryType);
    
    // Get local UV within current tile (0-1 range) for texture lookups
    vec2 localUV = fract(fragTexCoord / TILE_UV_SIZE);
    
    // Sample the exposed ground texture (dirt/stone based on texture type)
    vec2 exposedUV = vec2(exposedU, 0.0) + localUV * TILE_UV_SIZE;
    vec4 exposedColor = texture(texture0, exposedUV);
    
    // === Biome texture blending using Bayer dithering ===
    // Get Bayer dither value for stylized transitions
    float bayer = getBayerValue(gl_FragCoord.xy);
    float ditherThreshold = mix(0.5, bayer, ditherIntensity);
    
    // Get secondary texture U offset based on type
    float secondaryU = 0.0;
    if (secondaryType == TEX_GRASS) secondaryU = 0.0 / ATLAS_WIDTH;
    else if (secondaryType == TEX_SNOW) secondaryU = 32.0 / ATLAS_WIDTH;
    else if (secondaryType == TEX_STONE) secondaryU = 48.0 / ATLAS_WIDTH;
    else if (secondaryType == TEX_SAND) secondaryU = 64.0 / ATLAS_WIDTH;
    
    // Sample secondary texture
    vec2 secondaryUV = vec2(secondaryU, 0.0) + localUV * TILE_UV_SIZE;
    vec4 secondaryColor = texture(texture0, secondaryUV);
    
    // Apply dithered blending between primary and secondary
    vec4 biomeBlendedSurface = surfaceColor;
    if (blendStrength > ditherThreshold) {
        biomeBlendedSurface = secondaryColor;
    }
    
    // Calculate erosion blend factor
    float blendRange = max(0.001, erosionFullExpose - erosionThreshold);
    float blendFactor = clamp((erosion - erosionThreshold) / blendRange, 0.0, 1.0);
    
    // Choose between biome-blended surface and exposed based on erosion
    vec4 texelColor;
    if (blendFactor > ditherThreshold) {
        texelColor = exposedColor;
    } else {
        texelColor = biomeBlendedSurface;
    }
    
    // Apply subtle color tinting for cold areas (when blending to snow)
    vec3 tintedColor = texelColor.rgb;
    
    // For tiles blending toward snow, add frozen look
    if (secondaryType == TEX_SNOW && blendStrength > 0.0) {
        // Desaturate slightly for frozen look
        float desatAmount = blendStrength * 0.25;
        float gray = dot(tintedColor, vec3(0.299, 0.587, 0.114));
        tintedColor = mix(tintedColor, vec3(gray), desatAmount);
        
        // Add subtle blue tint
        tintedColor = mix(tintedColor, tintedColor * vec3(0.92, 0.95, 1.08), blendStrength * 0.4);
    }
    
    // Calculate diffuse lighting
    float diff = max(dot(normal, sunDirection), 0.0);
    vec3 diffuse = diff * sunColor;
    
    // Calculate ambient lighting
    vec3 ambient = ambientStrength * ambientColor;
    
    // Total lighting intensity (used for hue shifting)
    float lightIntensity = length(diffuse + ambient);
    
    // Convert texture color to HSV for hue manipulation
    vec3 hsv = rgb2hsv(tintedColor);
    
    // Hue shift based on lighting conditions
    float hueShift = (lightIntensity - shiftDisplacement) * shiftIntensity;
    hsv.x = fract(hsv.x + hueShift);
    
    // Slightly enhance saturation in well-lit areas (reduced when blending to snow)
    float satMult = 1.0 + diff * 0.2;
    if (secondaryType == TEX_SNOW) {
        satMult *= mix(1.0, 0.7, blendStrength); // Less saturation boost in cold areas
    }
    hsv.y = clamp(hsv.y * satMult, 0.0, 1.0);
    
    // Convert back to RGB
    vec3 shiftedColor = hsv2rgb(hsv);
    
    // Combine lighting with the hue-shifted color
    vec3 lighting = ambient + diffuse;
    
    // Apply lighting to the hue-shifted texture color
    finalColor = vec4(lighting * shiftedColor.rgb, texelColor.a);
}
