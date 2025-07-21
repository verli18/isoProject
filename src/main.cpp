#include "../include/marchingCubes.hpp"
#include <raylib.h>
#include <iostream>
/**
 * @brief Main entry point of the application.
 *
 * This function initializes the Raylib window, generates a voxel grid with Perlin noise,
 * renders the voxel grid as a 3D mesh and displays it in the window.
 *
 * The user can move the camera with the mouse and keyboard, and remove voxels by
 * clicking on them. The rendered mesh is updated in real-time when voxels are removed.
 *
 * The voxel grid can be toggled on and off by pressing the 'G' key.
 */
int main() {
    InitWindow(320 * 4, 240 * 4, "Isometric Game");
    RenderTexture2D texture = LoadRenderTexture(320, 240);

    DisableCursor();
    VoxelGrid voxelGrid(16, 16, 16);
    voxelGrid.generatePerlinTerrain(0.07f, 16, 16);

    MarchingCubes marchingCubes;
    Mesh mesh = marchingCubes.generateMeshFromGrid(voxelGrid);
    UploadMesh(&mesh, true);
    Model model = LoadModelFromMesh(mesh);

    Camera3D camera = { {16.0f, 16.0f, 16.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 60.0f, CAMERA_PERSPECTIVE };

    SetTargetFPS(60);
    bool showGrid = true;


    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORTHOGRAPHIC);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            std::cout << "Mouse clicked" << std::endl;
        
            Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
            std::cout << "Mouse ray: " << mouseRay.position.x << ", " << mouseRay.position.y << ", " << mouseRay.position.z << std::endl;
            std::cout << "Mouse ray direction: " << mouseRay.direction.x << ", " << mouseRay.direction.y << ", " << mouseRay.direction.z << std::endl;
            Vector3 hitVoxel = voxelGrid.getVoxelIndexDDA(mouseRay);
            std::cout << "Hit voxel: " << hitVoxel.x << ", " << hitVoxel.y << ", " << hitVoxel.z << std::endl;

            if (hitVoxel.x != -1) { // Check if we hit a valid voxel
                // We hit a voxel, let's remove it
                voxelGrid.setVoxel(hitVoxel.x, hitVoxel.y, hitVoxel.z, Voxels{0, {255, 255, 255, 255}, false, false});

                // Now, we MUST update the mesh to see the change
                UnloadModel(model); // Unload old model data from VRAM
                mesh = marchingCubes.generateMeshFromGrid(voxelGrid);
                UploadMesh(&mesh, true); // Re-upload the new mesh data
                model = LoadModelFromMesh(mesh);
            }
        }

        BeginDrawing();
        BeginTextureMode(texture);
            //draw center cursor
            
            ClearBackground(BLACK);
            BeginMode3D(camera);
            if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
            if(!showGrid) DrawModel(model, {0,0,-0}, 1.0f, WHITE);
            if (showGrid) {
                DrawModelWires(model, {0,0,0}, 1.0f, WHITE);
                voxelGrid.render();
            }
            EndMode3D();
            DrawPixelV({160, 120}, WHITE);
            EndTextureMode();

        DrawTexturePro(texture.texture, Rectangle{0, 0, 320, -240}, Rectangle{0, 0, 320 * 4, 240 * 4}, Vector2{0, 0}, 0, WHITE);
        EndDrawing();
    }
    
}
