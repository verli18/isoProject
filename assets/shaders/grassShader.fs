#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec3 fragWorldPos;
in float fragHeightFactor;
in vec3 fragBladeColor;
in float fragDiffuse;  // Pre-computed diffuse from terrain normal
in float fragWindBend;
in float fragTemperature;  // Temperature value (0-1) for biome blending

// Output
out vec4 finalColor;

// Lighting uniforms (same as terrain)
uniform vec3 sunColor;
uniform float ambientStrength;
uniform vec3 ambientColor;
uniform float shiftIntensity;
uniform float shiftDisplacement;

// Grass color uniforms - warm biome gradient
uniform vec3 grassTipColor;
uniform vec3 grassBaseColor;

// Tundra colors (cold but not frozen)
uniform vec3 tundraTipColor;
uniform vec3 tundraBaseColor;

// Snow/frozen colors (very cold)
uniform vec3 snowTipColor;
uniform vec3 snowBaseColor;

// Desert colors (hot/dry)
uniform vec3 desertTipColor;
uniform vec3 desertBaseColor;

// Temperature thresholds - cold
uniform float tundraStartTemp;  // Below this, start blending to tundra
uniform float tundraFullTemp;   // At this, fully tundra
uniform float snowStartTemp;    // Below this, start blending to snow
uniform float snowFullTemp;     // At this, fully snow grass

// Temperature thresholds - hot
uniform float desertStartTemp;  // Above this, start blending to desert
uniform float desertFullTemp;   // At this, fully desert grass

// HSV conversion functions (same as terrain)
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

void main()
{
    // Use pre-computed diffuse from CPU (based on terrain normal)
    float diff = fragDiffuse;
    vec3 diffuse = diff * sunColor;
    
    // Ambient lighting
    vec3 ambient = ambientStrength * ambientColor;
    
    // Total lighting intensity for hue shifting
    float lightIntensity = length(diffuse + ambient);
    
    // === Temperature-based biome color blending ===
    float temp = fragTemperature;
    
    // Calculate blend factors for each biome transition
    // Warm -> Tundra transition (cold)
    float tundraBlend = 0.0;
    if (temp < tundraStartTemp) {
        tundraBlend = clamp((tundraStartTemp - temp) / (tundraStartTemp - tundraFullTemp), 0.0, 1.0);
    }
    
    // Tundra -> Snow transition (very cold)
    float snowBlend = 0.0;
    if (temp < snowStartTemp) {
        snowBlend = clamp((snowStartTemp - temp) / (snowStartTemp - snowFullTemp), 0.0, 1.0);
    }
    
    // Warm -> Desert transition (hot)
    float desertBlend = 0.0;
    if (temp > desertStartTemp) {
        desertBlend = clamp((temp - desertStartTemp) / (desertFullTemp - desertStartTemp), 0.0, 1.0);
    }
    
    // Blend base colors: start with warm grass
    vec3 blendedBaseColor = grassBaseColor;
    vec3 blendedTipColor = grassTipColor;
    
    // Cold biomes: blend warm -> tundra -> snow
    blendedBaseColor = mix(blendedBaseColor, tundraBaseColor, tundraBlend);
    blendedTipColor = mix(blendedTipColor, tundraTipColor, tundraBlend);
    
    blendedBaseColor = mix(blendedBaseColor, snowBaseColor, snowBlend);
    blendedTipColor = mix(blendedTipColor, snowTipColor, snowBlend);
    
    // Hot biomes: blend warm -> desert (mutually exclusive with cold)
    blendedBaseColor = mix(blendedBaseColor, desertBaseColor, desertBlend);
    blendedTipColor = mix(blendedTipColor, desertTipColor, desertBlend);
    
    // Create height gradient with blended colors
    vec3 gradientColor = mix(blendedBaseColor, blendedTipColor, fragHeightFactor);
    
    // Apply the per-instance color tint (includes dirt blending from CPU)
    // Normalize by warm tipColor to preserve relative tinting
    vec3 bladeColor = fragBladeColor * gradientColor / grassTipColor;
    
    // HSV manipulation (same as terrain shader)
    vec3 hsv = rgb2hsv(bladeColor);
    
    // Hue shift based on lighting
    float hueShift = (lightIntensity - shiftDisplacement) * shiftIntensity;
    hsv.x = fract(hsv.x + hueShift);
    
    // Enhance saturation in well-lit areas (less saturation in cold areas)
    float satMult = 1.0 + diff * 0.2;
    // Cold biomes are less saturated (more gray/muted)
    satMult *= mix(1.0, 0.7, snowBlend);
    hsv.y = clamp(hsv.y * satMult, 0.0, 1.0);
    
    // Convert back to RGB
    vec3 shiftedColor = hsv2rgb(hsv);
    
    // Apply lighting (same formula as terrain)
    vec3 lighting = ambient + diffuse;
    vec3 grassColor = lighting * shiftedColor;
    
    // Subtle wind brightness variation
    float windShade = 1.0 - fragWindBend * 0.08;
    grassColor *= windShade;
    
    // Ambient occlusion at the base
    float ao = mix(0.85, 1.0, fragHeightFactor);
    grassColor *= ao;
    
    // Add subtle frost shimmer in cold areas
    float frost = snowBlend * 0.1 * (0.5 + 0.5 * sin(fragWorldPos.x * 5.0 + fragWorldPos.z * 7.0));
    grassColor += vec3(frost * 0.5, frost * 0.6, frost * 0.8);
    
    finalColor = vec4(grassColor, 1.0);
}
