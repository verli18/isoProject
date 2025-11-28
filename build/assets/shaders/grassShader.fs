#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec3 fragWorldPos;
in float fragHeightFactor;
in vec3 fragBladeColor;
in float fragDiffuse;  // Pre-computed diffuse from terrain normal
in float fragWindBend;

// Output
out vec4 finalColor;

// Lighting uniforms (same as terrain)
uniform vec3 sunColor;
uniform float ambientStrength;
uniform vec3 ambientColor;
uniform float shiftIntensity;
uniform float shiftDisplacement;

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
    
    // HSV manipulation (same as terrain shader)
    vec3 hsv = rgb2hsv(fragBladeColor);
    
    // Hue shift based on lighting
    float hueShift = (lightIntensity - shiftDisplacement) * shiftIntensity;
    hsv.x = fract(hsv.x + hueShift);
    
    // Enhance saturation in well-lit areas
    hsv.y = hsv.y * (1.0 + diff * 0.2);
    hsv.y = clamp(hsv.y, 0.0, 1.0);
    
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
    
    finalColor = vec4(grassColor, 1.0);
}
