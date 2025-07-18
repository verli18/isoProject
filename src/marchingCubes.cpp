#include "../include/marchingCubes.hpp"
#include <iostream>
#include <map>
#include <raymath.h>

Mesh MarchingCubes::generateMeshFromGrid(VoxelGrid& grid) {

    std::vector<Vector3> vertices;
    std::vector<unsigned short> indices;
    std::map<long long, int> vertexMap;

    Vector3 cornerOffsets[] = {
        {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1},
        {0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}
    };

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
                    for (int j = 0; j < 3; j++) {
                        int edgeIndex = triangleEdges[i + j];
                        
                        std::pair<int, int> cornerIndices = edgeToVertices[edgeIndex];
                        
                        Vector3 p1 = { (float)x + cornerOffsets[cornerIndices.first].x, 
                                    (float)y + cornerOffsets[cornerIndices.first].y, 
                                    (float)z + cornerOffsets[cornerIndices.first].z };
                                    
                        Vector3 p2 = { (float)x + cornerOffsets[cornerIndices.second].x, 
                                    (float)y + cornerOffsets[cornerIndices.second].y, 
                                    (float)z + cornerOffsets[cornerIndices.second].z };

                        Vector3 midpoint = { (p1.x + p2.x) * 0.5f, 
                                            (p1.y + p2.y) * 0.5f, 
                                            (p1.z + p2.z) * 0.5f };

                        long long key = ((long long)x << 32) | (y << 16) | z | ((long long)edgeIndex << 48);

                        if (vertexMap.find(key) != vertexMap.end()) {
                            indices.push_back(vertexMap[key]);
                        } else {
                            unsigned short newIndex = vertices.size();
                            vertices.push_back(midpoint);
                            vertexMap[key] = newIndex;
                            indices.push_back(newIndex);
                        }
                    }
                }
            }
        }
    }
    Mesh mesh = {
        (int)vertices.size(),
        (int)indices.size() / 3,
        (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float)),
        nullptr,
        nullptr,
        (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float)),
        nullptr,
        (unsigned char*)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char)),
        (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short)),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        0,
        0,
        0,
        0};

    for(unsigned int i = 0; i < vertices.size(); i++) {
        mesh.vertices[i*3] = vertices[i].x;
        mesh.vertices[i*3+1] = vertices[i].y;
        mesh.vertices[i*3+2] = vertices[i].z;
    }

    for(unsigned int i = 0; i < indices.size(); i++) {
        mesh.indices[i] = indices[i];
    }

    

    // Calculate min/max Y for height-based coloring
    float minY = vertices[0].y;
    float maxY = vertices[0].y;
    for (unsigned int i = 0; i < vertices.size(); i++) {
        if (vertices[i].y < minY) minY = vertices[i].y;
        if (vertices[i].y > maxY) maxY = vertices[i].y;
    }

    // Assign height-based colors
    for (unsigned int i = 0; i < vertices.size(); i++) {
        float normalizedY = (vertices[i].y - minY) / (maxY - minY); // 0.0 to 1.0

        Color color;
        if (normalizedY < 0.5f) {
            // Blend from blue to green
            color.r = (unsigned char)(0 * (1.0f - normalizedY * 2) + 0 * (normalizedY * 2));
            color.g = (unsigned char)(0 * (1.0f - normalizedY * 2) + 255 * (normalizedY * 2));
            color.b = (unsigned char)(255 * (1.0f - normalizedY * 2) + 0 * (normalizedY * 2));
        } else {
            // Blend from green to yellow
            color.r = (unsigned char)(0 * (1.0f - (normalizedY - 0.5f) * 2) + 255 * ((normalizedY - 0.5f) * 2));
            color.g = (unsigned char)(255 * (1.0f - (normalizedY - 0.5f) * 2) + 255 * ((normalizedY - 0.5f) * 2));
            color.b = (unsigned char)(0 * (1.0f - (normalizedY - 0.5f) * 2) + 0 * ((normalizedY - 0.5f) * 2));
        }
        color.a = 255; // Full opacity

        mesh.colors[i*4] = color.r;
        mesh.colors[i*4+1] = color.g;
        mesh.colors[i*4+2] = color.b;
        mesh.colors[i*4+3] = color.a;
    }

    for(int i = 0; i < mesh.triangleCount; i++) {
        unsigned short i1 = mesh.indices[i*3];
        unsigned short i2 = mesh.indices[i*3+1];
        unsigned short i3 = mesh.indices[i*3+2];

        Vector3 v1 = {mesh.vertices[i1*3], mesh.vertices[i1*3+1], mesh.vertices[i1*3+2]};
        Vector3 v2 = {mesh.vertices[i2*3], mesh.vertices[i2*3+1], mesh.vertices[i2*3+2]};
        Vector3 v3 = {mesh.vertices[i3*3], mesh.vertices[i3*3+1], mesh.vertices[i3*3+2]};

        Vector3 normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(v2, v1), Vector3Subtract(v3, v1)));

        mesh.normals[i1*3] += normal.x;
        mesh.normals[i1*3+1] += normal.y;
        mesh.normals[i1*3+2] += normal.z;
        mesh.normals[i2*3] += normal.x;
        mesh.normals[i2*3+1] += normal.y;
        mesh.normals[i2*3+2] += normal.z;
        mesh.normals[i3*3] += normal.x;
        mesh.normals[i3*3+1] += normal.y;
        mesh.normals[i3*3+2] += normal.z;
    }

    for(int i = 0; i < mesh.vertexCount; i++) {
        Vector3 normal = {mesh.normals[i*3], mesh.normals[i*3+1], mesh.normals[i*3+2]};
        normal = Vector3Normalize(normal);
        mesh.normals[i*3] = normal.x;
        mesh.normals[i*3+1] = normal.y;
        mesh.normals[i*3+2] = normal.z;
    }

    return mesh;
}

