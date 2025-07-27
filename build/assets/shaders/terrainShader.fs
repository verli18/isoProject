#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;

// Outputs
out vec4 finalColor;

// Uniforms
uniform sampler2D texture0;    // Diffuse texture
uniform vec3 sunDirection;     // Direction to the sun (normalized)
uniform vec3 sunColor;         // Color of the sun light
uniform float ambientStrength; // Ambient light strength
uniform vec3 ambientColor;     // Ambient light color
uniform float shiftIntensity;
uniform float shiftDisplacement;

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
    // Sample the texture
    vec4 texelColor = texture(texture0, fragTexCoord);
    
    // Normalize the fragment normal
    vec3 normal = normalize(fragNormal);
    
    // Calculate diffuse lighting
    float diff = max(dot(normal, sunDirection), 0.0);
    vec3 diffuse = diff * sunColor;
    
    // Calculate ambient lighting
    vec3 ambient = ambientStrength * ambientColor;
    
    // Total lighting intensity (used for hue shifting)
    float lightIntensity = length(diffuse + ambient);
    
    // Convert texture color to HSV for hue manipulation
    vec3 hsv = rgb2hsv(texelColor.rgb);
    
    // Hue shift based on lighting conditions:
    // - Areas in shadow (low light) shift towards cooler colors (blue/purple)
    // - Areas in bright light shift towards warmer colors (orange/red)
    float hueShift = (lightIntensity - shiftDisplacement) * shiftIntensity;
    hsv.x = fract(hsv.x + hueShift);
    
    // Slightly enhance saturation in well-lit areas
    hsv.y = hsv.y * (1.0 + diff * 0.2);
    hsv.y = clamp(hsv.y, 0.0, 1.0);
    
    // Convert back to RGB
    vec3 shiftedColor = hsv2rgb(hsv);
    
    // Combine lighting with the hue-shifted color
    vec3 lighting = ambient + diffuse;
    
    // Apply lighting to the hue-shifted texture color
    finalColor = vec4(lighting * shiftedColor.rgb, texelColor.a);
}