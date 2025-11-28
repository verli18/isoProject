#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;
in float fragHeightFactor;

// Uniforms
uniform vec4 colDiffuse;    // Base color from material
uniform vec3 viewPos;       // Camera position
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float ambientStrength;
uniform vec3 ambientColor;

// Output
out vec4 finalColor;

void main()
{
    // Base grass color from material
    vec3 baseColor = colDiffuse.rgb;
    
    // Darken at base, lighter at tip (ambient occlusion effect)
    float aoFactor = 0.6 + 0.4 * fragHeightFactor;
    
    // Simple directional lighting
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(-sunDirection);
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    // Subsurface scattering approximation for grass
    // Light coming from behind the blade
    vec3 viewDir = normalize(viewPos - fragWorldPos);
    float backLight = max(dot(viewDir, sunDirection), 0.0);
    float sss = pow(backLight, 2.0) * 0.3 * fragHeightFactor;
    
    // Combine lighting
    vec3 ambient = ambientColor * ambientStrength;
    vec3 diffuse = sunColor * NdotL;
    vec3 subsurface = sunColor * sss;
    
    vec3 litColor = baseColor * (ambient + diffuse + subsurface) * aoFactor;
    
    // Add slight color variation based on height
    litColor = mix(litColor * 0.8, litColor * 1.1, fragHeightFactor);
    
    finalColor = vec4(litColor, 1.0);
}
