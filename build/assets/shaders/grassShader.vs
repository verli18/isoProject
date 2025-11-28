#version 330

// Input vertex attributes - explicit locations to match rlgl setup
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;

// Instance transform matrix (mat4 uses 4 consecutive locations)
layout(location = 3) in mat4 instanceTransform;

// Per-instance blade color + diffuse (vec4: RGB + diffuse factor)
layout(location = 7) in vec4 instanceColorDiffuse;

// Uniforms
uniform mat4 mvp;
uniform float time;
uniform vec3 viewPos;  // Camera position for billboarding

// Wind parameters
uniform float windStrength;
uniform vec2 windDirection;

// Output to fragment shader
out vec2 fragTexCoord;
out vec3 fragWorldPos;
out float fragHeightFactor;
out vec3 fragBladeColor;
out float fragDiffuse;
out float fragWindBend;

void main()
{
    // Get the height factor from texture coord (0 at base, 1 at tip)
    fragHeightFactor = vertexTexCoord.y;
    
    // Get base position from instance transform (translation component)
    vec3 basePos = vec3(instanceTransform[3][0], instanceTransform[3][1], instanceTransform[3][2]);
    
    // Billboard: rotate blade to face camera (in XZ plane for isometric)
    vec3 toCamera = normalize(vec3(viewPos.x - basePos.x, 0.0, viewPos.z - basePos.z));
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), toCamera));
    
    // Apply billboard rotation to vertex position
    vec3 billboardPos = basePos;
    billboardPos += right * vertexPosition.x;
    billboardPos.y += vertexPosition.y;
    
    vec4 worldPos = vec4(billboardPos, 1.0);
    
    // Slow wind waves
    float windPhase = worldPos.x * 0.03 + worldPos.z * 0.02 + time * 0.15;
    float windOffset = sin(windPhase) * windStrength * fragHeightFactor * fragHeightFactor;
    
    float windPhase2 = worldPos.x * 0.05 - worldPos.z * 0.04 + time * 0.1;
    float windOffset2 = sin(windPhase2) * windStrength * 0.4 * fragHeightFactor;
    
    float totalBend = windOffset + windOffset2;
    fragWindBend = clamp(totalBend / (windStrength + 0.01), -1.0, 1.0);
    
    // Apply wind displacement
    worldPos.x += windOffset * windDirection.x + windOffset2 * windDirection.y;
    worldPos.z += windOffset * windDirection.y - windOffset2 * windDirection.x;
    worldPos.y -= abs(windOffset) * 0.15 * fragHeightFactor;
    
    fragWorldPos = worldPos.xyz;
    fragTexCoord = vertexTexCoord;
    fragBladeColor = instanceColorDiffuse.rgb;
    fragDiffuse = instanceColorDiffuse.a;  // Pre-computed diffuse from CPU
    
    gl_Position = mvp * worldPos;
}
