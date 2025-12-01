#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;
in vec4 fragColor;  // Tile data: R=biome, G=moisture, B=temperature, A=erosion

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

// Visualization mode (0=normal, 1=erosion, 2=moisture, 3=temperature, 4=slope, 5=biome)
uniform int visualizationMode;

// Temperature-based biome texture transitions
uniform float tundraStartTemp;  // Below this, start blending to tundra texture
uniform float tundraFullTemp;   // At this temp, fully tundra texture
uniform float snowStartTemp;    // Below this, start blending to snow texture
uniform float snowFullTemp;     // At this temp, fully snow texture

// Biome transition texture U offsets (normalized 0-1)
uniform float tundraTextureU;   // Tundra grass texture (dried/brown grass)
uniform float snowTextureU;     // Snow texture for frozen areas

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
vec3 getVisualizationColor(int mode, float texType, float moisture, float temperature, float erosion, float slope) {
    if (mode == 1) {
        // Erosion: black (none) to red (high)
        return vec3(erosion, 0.0, 0.0);
    } else if (mode == 2) {
        // Moisture: brown (dry) to blue (wet)
        return mix(vec3(0.6, 0.4, 0.2), vec3(0.0, 0.3, 0.8), moisture);
    } else if (mode == 3) {
        // Temperature: blue (cold) to red (hot)
        return mix(vec3(0.0, 0.2, 0.8), vec3(0.9, 0.2, 0.0), temperature);
    } else if (mode == 4) {
        // Slope: green (flat) to white (steep)
        return mix(vec3(0.2, 0.6, 0.2), vec3(1.0, 1.0, 1.0), slope);
    } else if (mode == 5) {
        // Texture type visualization (add 0.5 for rounding)
        int t = int(texType * 255.0 + 0.5);
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
    // R = texture type (TEX_GRASS=1, TEX_SNOW=2, etc.)
    // G = moisture, B = temperature, A = erosion
    float texTypeNorm = fragColor.r;     // Texture type (0-1, maps to 0-255)
    float moisture = fragColor.g;        // Moisture (0-1)
    float temperature = fragColor.b;     // Temperature (0-1)
    float erosion = fragColor.a;         // Erosion factor (0-1)
    
    // Calculate slope from normal (for visualization)
    vec3 normal = normalize(fragNormal);
    float slope = 1.0 - max(dot(normal, vec3(0.0, 1.0, 0.0)), 0.0);
    
    // Handle visualization modes
    if (visualizationMode > 0 && visualizationMode <= 5) {
        vec3 visColor = getVisualizationColor(visualizationMode, texTypeNorm, moisture, temperature, erosion, slope);
        // Apply basic lighting to visualization
        float diff = max(dot(normal, sunDirection), 0.0);
        vec3 lighting = ambientStrength * ambientColor + diff * sunColor;
        finalColor = vec4(lighting * visColor, 1.0);
        return;
    }
    
    // Normal rendering mode
    // Sample the main texture (surface texture like grass, sand, snow)
    vec4 surfaceColor = texture(texture0, fragTexCoord);
    
    // Get the exposed texture offset based on texture type
    // Add 0.5 before truncating to handle floating point precision issues
    int texType = int(texTypeNorm * 255.0 + 0.5);
    float exposedU = getExposedU(texType);
    
    // Get local UV within current tile (0-1 range) for texture lookups
    vec2 localUV = fract(fragTexCoord / TILE_UV_SIZE);
    
    // Sample the exposed ground texture (dirt/stone based on texture type)
    vec2 exposedUV = vec2(exposedU, 0.0) + localUV * TILE_UV_SIZE;
    vec4 exposedColor = texture(texture0, exposedUV);
    
    // === Temperature-based biome texture transitions (for grass tiles) ===
    // Calculate blend factors for biome transitions
    float tundraBlend = 0.0;
    if (temperature < tundraStartTemp) {
        tundraBlend = clamp((tundraStartTemp - temperature) / max(0.001, tundraStartTemp - tundraFullTemp), 0.0, 1.0);
    }
    
    float snowBlend = 0.0;
    if (temperature < snowStartTemp) {
        snowBlend = clamp((snowStartTemp - temperature) / max(0.001, snowStartTemp - snowFullTemp), 0.0, 1.0);
    }
    
    // For grass tiles, apply dithered texture transitions based on temperature
    vec4 biomeBlendedSurface = surfaceColor;
    if (texType == TEX_GRASS) {
        // Sample tundra texture (dried/brown grass or similar)
        vec2 tundraUV = vec2(tundraTextureU, 0.0) + localUV * TILE_UV_SIZE;
        vec4 tundraColor = texture(texture0, tundraUV);
        
        // Sample snow texture for very cold areas
        vec2 snowUV = vec2(snowTextureU, 0.0) + localUV * TILE_UV_SIZE;
        vec4 snowColor = texture(texture0, snowUV);
        
        // Get Bayer dither value for stylized transitions
        float bayer = getBayerValue(gl_FragCoord.xy);
        float ditherThreshold = mix(0.5, bayer, ditherIntensity);
        
        // First transition: grass -> tundra
        vec4 afterTundra = surfaceColor;
        if (tundraBlend > ditherThreshold) {
            afterTundra = tundraColor;
        }
        
        // Second transition: tundra -> snow (on top of first)
        biomeBlendedSurface = afterTundra;
        if (snowBlend > ditherThreshold) {
            biomeBlendedSurface = snowColor;
        }
    }
    
    // Calculate erosion blend factor
    float blendRange = max(0.001, erosionFullExpose - erosionThreshold);
    float blendFactor = clamp((erosion - erosionThreshold) / blendRange, 0.0, 1.0);
    
    // Apply Bayer dithering for erosion transition
    float bayer = getBayerValue(gl_FragCoord.xy);
    float threshold = mix(0.5, bayer, ditherIntensity);
    
    // Choose between biome-blended surface and exposed based on erosion
    vec4 texelColor;
    if (blendFactor > threshold) {
        texelColor = exposedColor;
    } else {
        texelColor = biomeBlendedSurface;
    }
    
    // Apply subtle color tinting for cold areas (on top of texture swap)
    vec3 tintedColor = texelColor.rgb;
    
    // Desaturate slightly in cold areas for a frozen look
    float desatAmount = tundraBlend * 0.15 + snowBlend * 0.25;
    float gray = dot(tintedColor, vec3(0.299, 0.587, 0.114));
    tintedColor = mix(tintedColor, vec3(gray), desatAmount);
    
    // Add subtle blue tint in snow areas
    tintedColor = mix(tintedColor, tintedColor * vec3(0.92, 0.95, 1.08), snowBlend * 0.4);
    
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
    
    // Slightly enhance saturation in well-lit areas (reduced in cold biomes)
    float satMult = 1.0 + diff * 0.2;
    satMult *= mix(1.0, 0.7, snowBlend); // Less saturation boost in cold areas
    hsv.y = clamp(hsv.y * satMult, 0.0, 1.0);
    
    // Convert back to RGB
    vec3 shiftedColor = hsv2rgb(hsv);
    
    // Combine lighting with the hue-shifted color
    vec3 lighting = ambient + diffuse;
    
    // Apply lighting to the hue-shifted texture color
    finalColor = vec4(lighting * shiftedColor.rgb, texelColor.a);
}
