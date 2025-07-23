#include "../include/3DvoxelGrid.hpp"
#include <raylib.h>
#include <iostream>

int main() {
    InitWindow(320 * 4, 240 * 4, "Isometric Game");
    RenderTexture2D texture = LoadRenderTexture(320, 240);

    VoxelGrid voxelGrid(16, 16);
    voxelGrid.generatePerlinTerrain(1.0f, 0, 0, 16);
    voxelGrid.generateMesh();

    Camera3D camera = { {16.0f, 16.0f, 16.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 32.0f, CAMERA_PERSPECTIVE};

    SetTargetFPS(60);
    bool showGrid = true;

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
            Vector3 hitVoxel = voxelGrid.getVoxelIndexDDA(mouseRay);

            if (hitVoxel.x != -1) { // Check if we hit a valid voxel

            }
        }

        BeginDrawing();
        BeginTextureMode(texture);
            ClearBackground(BLACK);
            BeginMode3D(camera);

            if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
            if(!showGrid) DrawModel(voxelGrid.model, {0, 0, 0}, 1.0f, WHITE);
            if (showGrid) DrawModelWires(voxelGrid.model, {0, 0, 0}, 1.0f, WHITE);
            EndMode3D();
        EndTextureMode();

        DrawTexturePro(texture.texture, Rectangle{0, 0, 320, -240}, Rectangle{0, 0, 320 * 4, 240 * 4}, Vector2{0, 0}, 0, WHITE);
        EndDrawing();
    }
    
}
