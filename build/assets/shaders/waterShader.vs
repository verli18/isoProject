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
    
    // Transform normal to world space
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    
    // Transform vertex position to world space
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    
    // Calculate world positions for water surface and terrain
    vec3 waterWorldPos = fragPosition;
    vec3 terrainWorldPos = vec3(fragPosition.x, vertexTexCoord.x, fragPosition.z);
    
    // Transform both positions to view space for depth calculation
    waterPosViewSpace = (matView * vec4(waterWorldPos, 1.0)).xyz;
    terrainPosViewSpace = (matView * vec4(terrainWorldPos, 1.0)).xyz;
    
    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
