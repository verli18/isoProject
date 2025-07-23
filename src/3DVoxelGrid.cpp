#include "../include/3DvoxelGrid.hpp"
#include "../include/textureAtlas.hpp"
#include <cfloat>  // for FLT_MAX
#include <cmath>
#include <cstring>
#include <raymath.h>

VoxelGrid::VoxelGrid(int width, int height) {
    this->width = width;
    this->height = height;
    grid.resize(width, std::vector<tile>(height));
}

VoxelGrid::~VoxelGrid() {
}

void VoxelGrid::setVoxel(int x, int y, tile voxel) {
    grid[x][y] = voxel;
}

tile VoxelGrid::getVoxel(int x, int y) {
    return grid[x][y];
}

void VoxelGrid::generatePerlinTerrain(float scale, int offsetX, int offsetY, int heightCo) {
    // Generate noise image with extra row/column to share corner samples between tiles
    perlinNoise = GenImagePerlinNoise(width + 1, height + 1, offsetX, offsetY, (int)scale);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            tile t;
            // Sample the four tile corners
            Color cTL = GetImageColor(perlinNoise, x,     y);
            Color cTR = GetImageColor(perlinNoise, x + 1, y);
            Color cBL = GetImageColor(perlinNoise, x,     y + 1);
            Color cBR = GetImageColor(perlinNoise, x + 1, y + 1);
            // Use red channel value as height for now
            t.tileHeight[0] = cTL.r / heightCo;
            t.tileHeight[1] = cTR.r / heightCo;
            t.tileHeight[2] = cBR.r / heightCo;
            t.tileHeight[3] = cBL.r / heightCo;

            // Clamp slopes
            for (int i = 0; i < 4; ++i) {
                for (int j = i + 1; j < 4; ++j) {
                    if (std::abs(t.tileHeight[i] - t.tileHeight[j]) > 1) {
                        if (t.tileHeight[i] < t.tileHeight[j]) {
                            t.tileHeight[j] = t.tileHeight[i] + 1;
                        } else {
                            t.tileHeight[i] = t.tileHeight[j] + 1;
                        }
                    }
                }
            }

            // Assign grass type
            t.type = 1;
            // Default lighting placeholder
            t.lighting = WHITE;
            setVoxel(x, y, t);
        }
    }
    UnloadImage(perlinNoise);
}

// Ray-based tile picking: intersect ray with mesh of tile surfaces
Vector3 VoxelGrid::getVoxelIndexDDA(Ray ray) {
    Vector3 bestHitPos = { -1, -1, -1 };
    float minDistance = FLT_MAX;
    // Iterate over all tiles
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tile t = getVoxel(x, y);
            // Build tile surface triangles
            Vector3 v0 = { (float)x,     (float)t.tileHeight[0], (float)y     };
            Vector3 v1 = { (float)x + 1, (float)t.tileHeight[1], (float)y     };
            Vector3 v2 = { (float)x + 1, (float)t.tileHeight[2], (float)y + 1 };
            Vector3 v3 = { (float)x,     (float)t.tileHeight[3], (float)y + 1 };
            // Check collision for first triangle
            RayCollision hit = GetRayCollisionTriangle(ray, v0, v1, v2);
            if (hit.hit && hit.distance < minDistance) {
                minDistance = hit.distance;
                bestHitPos = hit.point;
            }
            // Check collision for second triangle
            hit = GetRayCollisionTriangle(ray, v0, v2, v3);
            if (hit.hit && hit.distance < minDistance) {
                minDistance = hit.distance;
                bestHitPos = hit.point;
            }
        }
    }
    return bestHitPos;
}

void VoxelGrid::renderWires() {
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            tile t = getVoxel(x, y);
            // Define the four corner vertices in 3D space
            Vector3 v0 = {(float)x,     (float)t.tileHeight[0], (float)y};
            Vector3 v1 = {(float)x + 1, (float)t.tileHeight[1], (float)y};
            Vector3 v2 = {(float)x + 1, (float)t.tileHeight[2], (float)y + 1};
            Vector3 v3 = {(float)x,     (float)t.tileHeight[3], (float)y + 1};
            // Draw two triangles for the top face
            DrawLine3D(v2, v1, t.lighting);
            DrawLine3D(v3, v2, t.lighting);
            DrawLine3D(v3, v0, t.lighting);
        }
    }
}

void VoxelGrid::renderSurface() {
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            tile t = getVoxel(x, y);
            Vector3 v0 = {(float)x,     (float)t.tileHeight[0], (float)y};
            Vector3 v1 = {(float)x + 1, (float)t.tileHeight[1], (float)y};
            Vector3 v2 = {(float)x + 1, (float)t.tileHeight[2], (float)y + 1};
            Vector3 v3 = {(float)x,     (float)t.tileHeight[3], (float)y + 1};
            
            DrawTriangle3D(v2, v1, v0, t.lighting);
            DrawTriangle3D(v3, v2, v1, t.lighting);
        }
    }
}

unsigned int VoxelGrid::getWidth() { return width; }
unsigned int VoxelGrid::getHeight() { return height; }
unsigned int VoxelGrid::getDepth() { return depth; }

void VoxelGrid::generateMesh() {
    // Assuming texture atlas dimensions (you may want to make these configurable)
    const float atlasWidth = 64.0f;  // Total atlas width in pixels
    const float atlasHeight = 16.0f; // Total atlas height in pixels
    
    std::vector<Vector3> vertices;
    std::vector<Vector2> texcoords;
    std::vector<Vector3> normals;

    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            tile t = getVoxel(x, y);
            
            // Skip air tiles
            if(t.type == AIR) continue;
            
            Vector3 v0 = {(float)x,     (float)t.tileHeight[0], (float)y};
            Vector3 v1 = {(float)x + 1, (float)t.tileHeight[1], (float)y};
            Vector3 v2 = {(float)x + 1, (float)t.tileHeight[2], (float)y + 1};
            Vector3 v3 = {(float)x,     (float)t.tileHeight[3], (float)y + 1};
            
            // Get texture atlas info for this tile type
            textureAtlas atlas = textures[t.type];
            
            // Calculate UV coordinates for the top face
            float uMin = (float)atlas.uOffset / atlasWidth;
            float vMin = (float)atlas.vOffset / atlasHeight;
            float uMax = (float)(atlas.uOffset + atlas.width) / atlasWidth;
            float vMax = (float)(atlas.vOffset + atlas.height) / atlasHeight;
            
            // First triangle: v2, v1, v0 (counter-clockwise)
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v0);
            
            // Second triangle: v3, v2, v0 (counter-clockwise)
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v0);
            
            // Texture coordinates for each vertex (6 vertices total)
            // First triangle: v2, v1, v0
            texcoords.push_back(Vector2{uMax, vMax});  // v2 (bottom-right)
            texcoords.push_back(Vector2{uMax, vMin});  // v1 (top-right)
            texcoords.push_back(Vector2{uMin, vMin});  // v0 (top-left)
            
            // Second triangle: v3, v2, v0
            texcoords.push_back(Vector2{uMin, vMax});  // v3 (bottom-left)
            texcoords.push_back(Vector2{uMax, vMax});  // v2 (bottom-right)
            texcoords.push_back(Vector2{uMin, vMin});  // v0 (top-left)
            
            // Calculate face normal for both triangles
            Vector3 edge1 = Vector3Subtract(v1, v0);
            Vector3 edge2 = Vector3Subtract(v2, v0);
            Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
            
            // Add normals for all 6 vertices
            for(int i = 0; i < 6; i++) {
                normals.push_back(normal);
            }
        }
    }
    
    int vertexCount = vertices.size();
    int triangleCount = vertexCount / 3;

    // Create mesh structure and allocate memory
    mesh = {0};
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = triangleCount;
    
    // Allocate and copy vertex data
    mesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
    mesh.normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    
    // Copy vertex data
    for(int i = 0; i < vertexCount; i++) {
        mesh.vertices[i * 3 + 0] = vertices[i].x;
        mesh.vertices[i * 3 + 1] = vertices[i].y;
        mesh.vertices[i * 3 + 2] = vertices[i].z;
        
        mesh.texcoords[i * 2 + 0] = texcoords[i].x;
        mesh.texcoords[i * 2 + 1] = texcoords[i].y;
        
        mesh.normals[i * 3 + 0] = normals[i].x;
        mesh.normals[i * 3 + 1] = normals[i].y;
        mesh.normals[i * 3 + 2] = normals[i].z;
    }
    
    // Upload mesh to GPU
    UploadMesh(&mesh, false);
    
    // Create model from mesh
    model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("textures.png");
}

