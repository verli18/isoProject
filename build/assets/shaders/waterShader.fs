#version 330

in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoord;

// Outputs
out vec4 finalColor;

// Uniforms
uniform sampler2D texture0;      // Water texture
uniform sampler2D texture1;      // Displacement texture (normal map slot)
uniform float waterHue;
uniform float waterSaturation;
uniform float waterValue;
uniform float time;              // Time for displacement animation

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
    float terrainHeight = fragTexCoord.x;  // Terrain height stored in texcoord.x
    float flowAngle = fragTexCoord.y;      // Flow direction angle (0-2π) stored in texcoord.y
    float waterHeight = fragPosition.y;    // Current water surface Y position
    float depth = waterHeight - terrainHeight;
    
    // Calculate depth-based transparency
    float alpha = minAlpha + (maxAlpha - minAlpha) * clamp((depth - minDepth) / (maxDepth - minDepth), 0.0, 1.0);
    
    // Base water color (blue) with HSV shifts applied
    vec3 baseColor = vec3(0.156, 0.47, 0.96);
    vec3 hsv = rgb2hsv(baseColor);
    hsv.x = clamp(hsv.x + waterHue, 0.0, 1.0);
    hsv.y = clamp(hsv.y * waterSaturation, 0.0, 1.0);
    hsv.z = clamp(hsv.z * waterValue, 0.0, 1.0);
    vec3 tintedBaseColor = hsv2rgb(hsv);
    
    // Sample texture using world position for UV coordinates with time-based movement
    float textureScale = 0.5; // Fixed texture scale
    
    // Compute flow direction vector from angle
    // flowAngle: 0=E, π/4=SE, π/2=S, etc.
    vec2 flowDir = vec2(cos(flowAngle), sin(flowAngle));
    
    // For lakes/no flow (flowAngle near 0 with no valid direction), use ambient ripples
    bool hasFlow = (flowAngle > 0.01);
    
    // Flow-based UV movement for rivers, ambient ripples for lakes
    vec2 flowOffset;
    if (hasFlow) {
        // River flow - move UVs in flow direction
        float flowSpeed = 0.15;
        flowOffset = time * flowSpeed * flowDir;
    } else {
        // Lake/still water - slow ambient movement
        flowOffset = time * vec2(0.02, 0.015);
    }
    
    vec2 baseTextureUV = vec2(fragPosition.x * textureScale, fragPosition.z * textureScale) + flowOffset;
    
    // Sample displacement texture for wobbling effect
    float displacementScale = 0.8; // Scale factor for displacement texture
    vec2 displacementUV = vec2(fragPosition.x * displacementScale + time * 0.05, fragPosition.z * displacementScale + time * 0.03);
    vec3 displacement = texture(texture1, displacementUV).rgb;
    
    // Convert displacement to offset (-1 to 1 range)
    vec2 displacementOffset = (displacement.xy - 0.5) * 2.0;
    
    // Apply displacement with depth-based intensity to the flowing texture
    // Rivers have less displacement (more directional), lakes have more (ripples)
    float displacementIntensity = hasFlow ? 0.03 : 0.06;
    float depthDisplacementFactor = clamp((depth - 0.5) / 2.0, 0.0, 1.0);
    vec2 finalTextureUV = baseTextureUV + displacementOffset * displacementIntensity * (0.3 + depthDisplacementFactor * 0.7);
    
    vec4 textureColorWithAlpha = texture(texture0, finalTextureUV);
    vec3 textureColor = textureColorWithAlpha.rgb;
    
    // Calculate blend factor based on depth
    float depthBlendFactor = clamp((depth - 1.0) / 1.0, 0.0, 1.0);
    
    // 2x2 Bayer dither matrix with displacement
    mat2 ditherMatrix = mat2(
        0.0/4.0, 2.0/4.0,
        3.0/4.0, 1.0/4.0
    );
    
    // Get screen position for dithering with displacement offset
    vec2 displacedScreenPos = gl_FragCoord.xy + displacementOffset * 8.0;
    ivec2 screenPos = ivec2(displacedScreenPos);
    int x = screenPos.x % 2;
    int y = screenPos.y % 2;
    
    // Get dither threshold from matrix
    float ditherThreshold = ditherMatrix[y][x];
    
    // Use dithering to choose between base color and textured overlay
    vec3 finalColor3;
    float finalAlpha;
    
    if (depthBlendFactor > ditherThreshold) {
        // Show textured water with depth-based darkening and blue shift
        
        // Calculate depth factor for color modification (0 = shallow, 1 = deep)
        float depthFactor = clamp((depth - 1.0) / 3.0, 0.0, 1.0);
        
        // Darken the texture based on depth
        float darkenFactor = 1.0 - (depthFactor * 0.20);
        vec3 darkenedTexture = textureColor * darkenFactor;
        
        // Shift texture towards deeper blue in deep areas
        vec3 deepBlue = vec3(0.05, 0.5, 0.8);
        vec3 blueShiftedTexture = mix(darkenedTexture, darkenedTexture * deepBlue, depthFactor * 0.7);
        
        finalColor3 = blueShiftedTexture;
        finalAlpha = alpha * textureColorWithAlpha.a;
    } else {
        // Show base tinted color
        finalColor3 = tintedBaseColor;
        finalAlpha = alpha;
    }
    
    // Apply depth-based alpha
    finalColor = vec4(finalColor3, finalAlpha);
}