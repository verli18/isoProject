#include "../include/marchingCubes.hpp"
#include "../include/textureAtlas.hpp"
#include <iostream>
#include <map>
#include <raymath.h>
#include <vector>
#include <cmath>

MarchingCubes::MarchingCubes() {}
MarchingCubes::~MarchingCubes() {}

int MarchingCubes::determineSurfaceTexture(VoxelGrid& grid, int x, int y, int z) {
    // Sample in a 3x3x3 neighborhood around the triangle
    std::map<int, int> materialCount;
    
    for(int corner = 0; corner < 8; corner++) {
        int vx = x + (corner & 1);
        int vy = y + ((corner >> 2) & 1);
        int vz = z + ((corner >> 1) & 1);
        
        if(vx < (int)grid.getWidth() && vy < (int)grid.getHeight() && vz < (int)grid.getDepth()) {
            int type = grid.getVoxel(vx, vy, vz).type;
            if(type != AIR) {
                materialCount[type] += 1;
            }
        }
    }
    // Find the dominant material
    int dominantType = STONE;
    int maxCount = 0;
    for(const auto& [type, count] : materialCount) {
        if(count > maxCount) {
            maxCount = count;
            dominantType = type;
        }
    }
       
    return dominantType;
}

Mesh MarchingCubes::generateMeshFromGrid(VoxelGrid& grid) {

    std::vector<Vector3> mesh_vertices;
    std::vector<Vector2> mesh_texcoords;
    std::vector<Vector3> mesh_normals;

    Vector3 cornerOffsets[] = {
        {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1},
        {0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}
    };

    const float atlasWidth = 64.0f; // 5 textures * 16px width
    const float atlasHeight = 16.0f; // 16px height

    for(unsigned int x = 0; x < grid.getWidth()-1; x++) {
        for(unsigned int y = 0; y < grid.getHeight()-1; y++) {
            for(unsigned int z = 0; z < grid.getDepth()-1; z++) {
                int cubeIndex = 0;
                if(grid.getVoxel(x, y, z).type != 0) cubeIndex |= 1;
                if(grid.getVoxel(x+1, y, z).type != 0) cubeIndex |= 2;
                if(grid.getVoxel(x+1, y, z+1).type != 0) cubeIndex |= 4;
                if(grid.getVoxel(x, y, z+1).type != 0) cubeIndex |= 8;
                if(grid.getVoxel(x, y+1, z).type != 0) cubeIndex |= 16;
                if(grid.getVoxel(x+1, y+1, z).type != 0) cubeIndex |= 32;
                if(grid.getVoxel(x+1, y+1, z+1).type != 0) cubeIndex |= 64;
                if(grid.getVoxel(x, y+1, z+1).type != 0) cubeIndex |= 128;

                if(cubeIndex == 0 || cubeIndex == 255) continue;
            

                const int* triangleEdges = triangleTable[cubeIndex];

                for (int i = 0; triangleEdges[i] != -1; i += 3) {
                    Vector3 tri_vertices[3];
                    std::map<int, int> type_counts;

                    for (int j = 0; j < 3; j++) {
                        int edgeIndex = triangleEdges[i + j];
                        
                        std::pair<int, int> cornerIndices = edgeToVertices[edgeIndex];
                        
                        Vector3 p1 = { (float)x + cornerOffsets[cornerIndices.first].x, 
                                    (float)y + cornerOffsets[cornerIndices.first].y, 
                                    (float)z + cornerOffsets[cornerIndices.first].z };
                                    
                        Vector3 p2 = { (float)x + cornerOffsets[cornerIndices.second].x, 
                                    (float)y + cornerOffsets[cornerIndices.second].y, 
                                    (float)z + cornerOffsets[cornerIndices.second].z };

                        tri_vertices[j] = { (p1.x + p2.x) * 0.5f, 
                                            (p1.y + p2.y) * 0.5f, 
                                            (p1.z + p2.z) * 0.5f };

                    }
                    //Vector3 triangleCenter = {
                    //    (tri_vertices[0].x + tri_vertices[1].x + tri_vertices[2].x) / 3.0f,
                    //    (tri_vertices[0].y + tri_vertices[1].y + tri_vertices[2].y) / 3.0f,
                    //    (tri_vertices[0].z + tri_vertices[1].z + tri_vertices[2].z) / 3.0f
                    //};

                    Vector3 normal = Vector3Normalize(Vector3CrossProduct(
                        Vector3Subtract(tri_vertices[1], tri_vertices[0]), 
                        Vector3Subtract(tri_vertices[2], tri_vertices[0])
                    ));
                    
                    //do not ask me to explain this stuff, I followed a guide for it
                    textureAtlas texInfo = textures[determineSurfaceTexture(grid, x, y, z)];
                    float u_start = texInfo.uOffset / atlasWidth;
                    float v_start = texInfo.vOffset / atlasHeight;
                    float u_scale = texInfo.width / atlasWidth;
                    float v_scale = texInfo.height / atlasHeight;

                    Vector2 tri_uvs[3];
                    for(int j=0; j<3; j++) {
                        Vector3 p = tri_vertices[j];
                        Vector3 cube_origin = {(float)x, (float)y, (float)z};
                        Vector3 local_p = Vector3Subtract(p, cube_origin);

                        float wx = fabs(normal.x);
                        float wy = fabs(normal.y);
                        float wz = fabs(normal.z);
                        float sum = wx + wy + wz;
                        wx /= sum; wy /= sum; wz /= sum;

                        // Local projections onto each axis plane
                        Vector2 uvX = { local_p.y, local_p.z }; // Project onto YZ plane (X axis)
                        Vector2 uvY = { local_p.x, local_p.z }; // Project onto XZ plane (Y axis)
                        Vector2 uvZ = { local_p.x, local_p.y }; // Project onto XY plane (Z axis)

                        // Blend projections
                        Vector2 uv = {
                            uvX.x * wx + uvY.x * wy + uvZ.x * wz,
                            uvX.y * wx + uvY.y * wy + uvZ.y * wz
                        };

                        tri_uvs[j].x = u_start + uv.x * u_scale;
                        tri_uvs[j].y = v_start + uv.y * v_scale;
                    }

                    for(int j=0; j<3; j++) {
                        mesh_vertices.push_back(tri_vertices[j]);
                        mesh_texcoords.push_back(tri_uvs[j]);
                        mesh_normals.push_back(normal);
                    }
                }
            }
        }
    }

    int vertexCount = mesh_vertices.size();
    int triangleCount = vertexCount / 3;

    Mesh mesh = {
        vertexCount,
        triangleCount,
        (float*)MemAlloc(vertexCount * 3 * sizeof(float)),
        (float*)MemAlloc(vertexCount * 2 * sizeof(float)),
        nullptr,
        (float*)MemAlloc(vertexCount * 3 * sizeof(float)),
        nullptr,
        (unsigned char*)MemAlloc(vertexCount * 4 * sizeof(unsigned char)),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        0,
        0,
        0,
        0};
    
    // Simple directional lighting parameters
    const Vector3 sunDir = Vector3Normalize({ 1.0f, 1.0f, 0.5f });
    const float ambient = 0.2f;
    const float diffuse = 0.8f;

    for(int i = 0; i < mesh.vertexCount; i++) {
        mesh.vertices[i*3 + 0] = mesh_vertices[i].x;
        mesh.vertices[i*3 + 1] = mesh_vertices[i].y;
        mesh.vertices[i*3 + 2] = mesh_vertices[i].z;

        mesh.normals[i*3 + 0] = mesh_normals[i].x;
        mesh.normals[i*3 + 1] = mesh_normals[i].y;
        mesh.normals[i*3 + 2] = mesh_normals[i].z;

        mesh.texcoords[i*2 + 0] = mesh_texcoords[i].x;
        mesh.texcoords[i*2 + 1] = mesh_texcoords[i].y;
        
        // Compute simple Lambertian lighting
        float ndotl = Vector3DotProduct(mesh_normals[i], sunDir);
        if(ndotl < 0.0f) ndotl = 0.0f;
        float intensity = ambient + diffuse * ndotl;
        unsigned char lightVal = (unsigned char)(255.0f * intensity);
        mesh.colors[i*4 + 0] = lightVal;
        mesh.colors[i*4 + 1] = lightVal;
        mesh.colors[i*4 + 2] = lightVal;
        mesh.colors[i*4 + 3] = 255;
    }

    return mesh;
}