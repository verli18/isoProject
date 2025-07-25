#include "../include/chunk.hpp"
#include "../include/textureAtlas.hpp"
#include "../include/resourceManager.hpp"
#include <cmath>
#include <iostream>
#include <ostream>
#include <raylib.h>

int main() {
    InitWindow(320 * 4, 240 * 4, "Isometric Game");
    RenderTexture2D texture = LoadRenderTexture(320, 240);

    Chunk chunk(0, 0);

    // Set up lighting
    Vector3 sunDirection = {0.51f, -1.0f, 0.3f};  // Direction from sun to ground
    Vector3 sunColor = {1.0f, 0.9f, 0.7f};       // Warm sun color
    float ambientStrength = 0.5f;                 // Ambient light strength
    Vector3 ambientColor = {0.4f, 0.5f, 0.8f};   // Cool ambient color
    float shiftIntensity = -0.16f;
    float shiftDisplacement = 1.56f;
    
    chunk.tiles.updateLighting(sunDirection, sunColor, ambientStrength, ambientColor, shiftIntensity, shiftDisplacement);

    resourceManager::initialize();
    
    Camera3D camera = { {32.0f, 32.0f, 32.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 20.0f, CAMERA_ORTHOGRAPHIC};

    SetTargetFPS(60);
    bool showGrid = true;
    float sunAngle = 0.0f;  // For rotating the sun

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_PERSPECTIVE);
        // Update sun position based on time or input
        if (IsKeyDown(KEY_LEFT)) sunAngle -= 1.0f * GetFrameTime();
        if (IsKeyDown(KEY_RIGHT)) sunAngle += 1.0f * GetFrameTime();


        if (IsKeyDown(KEY_I)) shiftIntensity += 0.01f;
        if (IsKeyDown(KEY_K)) shiftIntensity -= 0.01f;
        if (IsKeyDown(KEY_J)) shiftDisplacement += 0.01f;
        if (IsKeyDown(KEY_L)) shiftDisplacement -= 0.01f;

        // Calculate sun direction based on angle
        Vector3 newSunDirection = {
            sinf(sunAngle) * 0.5f,
            -1.0f,
            cosf(sunAngle) * 0.3f
        };
        chunk.tiles.updateLighting(newSunDirection, sunColor, ambientStrength, ambientColor, shiftIntensity, shiftDisplacement);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
            Vector3 hitVoxel = chunk.tiles.getTileIndexDDA(mouseRay);
            std::cout << "Mouse position: " << GetMousePosition().x << " " << GetMousePosition().y << std::endl;
            std::cout << "Hit voxel: " << hitVoxel.x << " " << hitVoxel.y << " " << hitVoxel.z << std::endl;

            if (hitVoxel.x != -1) { // Check if we hit a valid voxel
                // Lower the hit voxel's four corners by 0.25
                tile t = chunk.tiles.getTile(hitVoxel.x, hitVoxel.y);
                for (int i = 0; i < 4; ++i) {
                    t.tileHeight[i] -= 0.25f;
                    if (t.tileHeight[i] < 0.0f) t.tileHeight[i] = 0.0f; // Clamp to non-negative
                }
                chunk.tiles.setTile(hitVoxel.x, hitVoxel.y, t);
                chunk.updateMesh();
            }
        }

        BeginDrawing();
        BeginTextureMode(texture);
            ClearBackground(BLACK);
            BeginMode3D(camera);

            if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
            if(!showGrid) DrawModel(chunk.model, {0, 0, 0}, 1.0f, WHITE);
            if (showGrid) DrawModelWires(chunk.model, {0, 0, 0}, 1.0f, WHITE);

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
