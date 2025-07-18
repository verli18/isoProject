#include "../include/marchingCubes.hpp"
#include <raylib.h>

int main() {
    InitWindow(320 * 4, 240 * 4, "Isometric Game");
    RenderTexture2D texture = LoadRenderTexture(320, 240);

    VoxelGrid voxelGrid(64, 64, 64);
    voxelGrid.generatePerlinTerrain(0.25f, 16, 16);

    MarchingCubes marchingCubes;
    Mesh mesh = marchingCubes.generateMeshFromGrid(voxelGrid);
    UploadMesh(&mesh, true);
    Model model = LoadModelFromMesh(mesh);

    Camera3D camera = { {64.0f, 64.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 60.0f, CAMERA_PERSPECTIVE };

    SetTargetFPS(60);
    bool showGrid = true;


    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);
        BeginDrawing();
        BeginTextureMode(texture);
            ClearBackground(BLACK);
            BeginMode3D(camera);
                if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
                if(!showGrid) DrawModel(model, {-32,0,-32}, 1.0f, WHITE);
                if (showGrid) voxelGrid.render();
            EndMode3D();
        EndTextureMode();

        DrawTexturePro(texture.texture, Rectangle{0, 0, 320, -240}, Rectangle{0, 0, 320 * 4, 240 * 4}, Vector2{0, 0}, 0, WHITE);
        EndDrawing();
    }
    
}
