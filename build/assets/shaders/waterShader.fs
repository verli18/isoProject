#version 330

in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoord;

// Outputs
out vec4 finalColor;

// Uniforms
uniform sampler2D texture0;
uniform float waterHue;
uniform float waterSaturation;
uniform float waterValue;

// Depth-based alpha parameters
uniform float minDepth;      // Minimum depth for alpha calculation
uniform float maxDepth;      // Maximum depth for full alpha
uniform float minAlpha;      // Minimum alpha value (for shallow water)
uniform float maxAlpha;      // Maximum alpha value (for deep water)

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

void main() {
    // Calculate water depth: water Y position - terrain height
    float terrainHeight = fragTexCoord.x;  // Terrain height stored in texcoord.x
    float waterHeight = fragPosition.y;    // Current water surface Y position
    float depth = waterHeight - terrainHeight;
    
    // Calculate depth-based alpha
    float alpha = minAlpha + (maxAlpha - minAlpha) * clamp((depth - minDepth) / (maxDepth - minDepth), 0.0, 1.0);
    
    // Base water color (blue)
    vec3 baseColor = vec3(0.156, 0.47, 0.96);
    // Convert to HSV and shift
    vec3 hsv = rgb2hsv(baseColor);
    hsv.x = clamp(hsv.x + waterHue, 0.0, 1.0);
    hsv.y = clamp(hsv.y * waterSaturation, 0.0, 1.0);
    hsv.z = clamp(hsv.z * waterValue, 0.0, 1.0);
    vec3 shiftedColor = hsv2rgb(hsv);
    
    // Apply depth-based alpha
    finalColor = vec4(shiftedColor, alpha);
}