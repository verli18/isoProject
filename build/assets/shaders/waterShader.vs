#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 matView;
uniform sampler2D texture1; // Displacement texture (using texture1 slot)
uniform float time;          // Time for animation
uniform float displacementStrength; // Displacement intensity

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPosition;
out vec3 waterPosViewSpace;
out vec3 terrainPosViewSpace;

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    
    // Calculate displacement UV coordinates based on world position and time
    vec2 displacementUV1 = vec2(vertexPosition.x * 0.1 + time * 0.02, vertexPosition.z * 0.1 + time * 0.015);
    vec2 displacementUV2 = vec2(vertexPosition.x * 0.15 - time * 0.01, vertexPosition.z * 0.15 + time * 0.025);
    
    // Sample displacement texture at two different scales and combine
    float displacement1 = texture(texture1, displacementUV1).r;
    float displacement2 = texture(texture1, displacementUV2).r;
    float combinedDisplacement = (displacement1 + displacement2 * 0.5) - 0.75; // Center around 0
    
    // Apply displacement to vertex position (only Y axis for water surface waves)
    vec3 displacedPosition = vertexPosition;
    displacedPosition.y += combinedDisplacement * displacementStrength;
    
    // Transform normal to world space
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    
    // Transform displaced vertex position to world space
    fragPosition = vec3(matModel * vec4(displacedPosition, 1.0));
    
    // Calculate world positions for water surface and terrain
    vec3 waterWorldPos = fragPosition;
    vec3 terrainWorldPos = vec3(fragPosition.x, vertexTexCoord.x, fragPosition.z);
    
    // Transform both positions to view space for depth calculation
    waterPosViewSpace = (matView * vec4(waterWorldPos, 1.0)).xyz;
    terrainPosViewSpace = (matView * vec4(terrainWorldPos, 1.0)).xyz;
    
    // Calculate final vertex position with displacement
    gl_Position = mvp * vec4(displacedPosition, 1.0);
}
