#include "../include/chunk.hpp"
#include <raylib.h>
#include <iostream>

int main() {
    InitWindow(320 * 4, 240 * 4, "Isometric Game");
    RenderTexture2D texture = LoadRenderTexture(320, 240);


    Chunk chunk(0, 0, 0);
    chunk.generateMesh();

    Camera3D camera = { {16.0f, 16.0f, 16.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 32.0f, CAMERA_ORTHOGRAPHIC};

    SetTargetFPS(60);
    bool showGrid = true;


    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORTHOGRAPHIC);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
            Vector3 hitVoxel = chunk.voxelGrid.getVoxelIndexDDA(mouseRay);

            if (hitVoxel.x != -1) { // Check if we hit a valid voxel
                chunk.voxelGrid.setVoxel(hitVoxel.x, hitVoxel.y, hitVoxel.z, Voxels{0, {255, 255, 255, 255}, false, false});
                chunk.updateMesh();
            }
        }

        BeginDrawing();
        BeginTextureMode(texture);
            ClearBackground(BLACK);
            BeginMode3D(camera);

            if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
            if(!showGrid) chunk.render();
            if (showGrid) {
                chunk.renderWires();
                chunk.voxelGrid.render();
            }
            EndMode3D();
        EndTextureMode();

        DrawTexturePro(texture.texture, Rectangle{0, 0, 320, -240}, Rectangle{0, 0, 320 * 4, 240 * 4}, Vector2{0, 0}, 0, WHITE);
        EndDrawing();
    }
    
}
