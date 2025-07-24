#include "../include/3DvoxelGrid.hpp"
#include <cmath>
#include <raylib.h>

int main() {
    InitWindow(320 * 4, 240 * 4, "Isometric Game");
    RenderTexture2D texture = LoadRenderTexture(320, 240);

    bool disableCursor = false;
    if (disableCursor) DisableCursor();
    
    VoxelGrid voxelGrid(32, 32);
    voxelGrid.generatePerlinTerrain(1.0f, 0, 0, 12);
    voxelGrid.generateMesh();

    // Set up lighting
    Vector3 sunDirection = {0.51f, -1.0f, 0.3f};  // Direction from sun to ground
    Vector3 sunColor = {1.0f, 0.9f, 0.7f};       // Warm sun color
    float ambientStrength = 0.5f;                 // Ambient light strength
    Vector3 ambientColor = {0.4f, 0.5f, 0.8f};   // Cool ambient color
    float shiftIntensity = -0.16f;
    float shiftDisplacement = 1.56f;
    
    voxelGrid.updateLighting(sunDirection, sunColor, ambientStrength, ambientColor, shiftIntensity, shiftDisplacement);

    Camera3D camera = { {16.0f, 16.0f, 16.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 32.0f, CAMERA_PERSPECTIVE};

    SetTargetFPS(60);
    bool showGrid = true;
    float sunAngle = 0.0f;  // For rotating the sun

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);
        
        // Update sun position based on time or input
        if (IsKeyDown(KEY_LEFT)) sunAngle -= 1.0f * GetFrameTime();
        if (IsKeyDown(KEY_RIGHT)) sunAngle += 1.0f * GetFrameTime();


        if (IsKeyDown(KEY_I)) shiftIntensity += 0.01f;
        if (IsKeyDown(KEY_K)) shiftIntensity -= 0.01f;
        if (IsKeyDown(KEY_J)) shiftDisplacement += 0.01f;
        if (IsKeyDown(KEY_L)) shiftDisplacement -= 0.01f;
        if (IsKeyDown(KEY_C)) disableCursor = !disableCursor;
        if(disableCursor) DisableCursor();
        else EnableCursor();
        // Calculate sun direction based on angle
        Vector3 newSunDirection = {
            sinf(sunAngle) * 0.5f,
            -1.0f,
            cosf(sunAngle) * 0.3f
        };
        voxelGrid.updateLighting(newSunDirection, sunColor, ambientStrength, ambientColor, shiftIntensity, shiftDisplacement);

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
        
        // Draw UI
        DrawText("G - Toggle wireframe", 10, 10, 20, WHITE);
        DrawText("Left/Right arrows - Rotate sun", 10, 30, 20, WHITE);
        DrawText(TextFormat("Sun angle: %.2f", sunAngle), 10, 50, 20, WHITE);
        DrawText(TextFormat("Shift intensity: %.2f", shiftIntensity), 10, 70, 20, WHITE);
        DrawText(TextFormat("Shift displacement: %.2f", shiftDisplacement), 10, 90, 20, WHITE);
        
        EndDrawing();
    }
    
}
