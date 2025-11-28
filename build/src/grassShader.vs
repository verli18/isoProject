#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;      // Instance color (if provided)

// Input instance attributes (from instancing)
in mat4 instanceTransform;

// Uniforms
uniform mat4 mvp;
uniform mat4 matNormal;
uniform float time;

// Wind parameters
uniform float windStrength;
uniform vec2 windDirection;
uniform float windSpeed;

// Output to fragment shader
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;
out float fragHeightFactor;

void main()
{
    // Get the height factor from texture coord (0 at base, 1 at tip)
    fragHeightFactor = vertexTexCoord.y;
    
    // Apply instance transform to get world position
    vec4 worldPos = instanceTransform * vec4(vertexPosition, 1.0);
    
    // Wind effect - stronger at tip, based on world position for variation
    float windPhase = worldPos.x * 0.5 + worldPos.z * 0.3 + time * windSpeed;
    float windOffset = sin(windPhase) * windStrength * fragHeightFactor * fragHeightFactor;
    
    // Secondary wind wave for more organic movement
    float windPhase2 = worldPos.x * 0.8 - worldPos.z * 0.6 + time * windSpeed * 1.3;
    float windOffset2 = sin(windPhase2) * windStrength * 0.5 * fragHeightFactor;
    
    // Apply wind displacement in wind direction
    worldPos.x += windOffset * windDirection.x + windOffset2 * windDirection.y;
    worldPos.z += windOffset * windDirection.y - windOffset2 * windDirection.x;
    
    // Slight vertical compression when bending
    worldPos.y -= abs(windOffset) * 0.2 * fragHeightFactor;
    
    fragWorldPos = worldPos.xyz;
    fragTexCoord = vertexTexCoord;
    
    // Transform normal (simplified - just use up vector for grass)
    fragNormal = vec3(0.0, 1.0, 0.0);
    
    // Final position
    gl_Position = mvp * worldPos;
}
