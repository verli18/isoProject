#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "init.c"
#include "logic.c"

void main()
{
    init(); 
    Texture2D cube = LoadTexture("cube.png");

    struct chunk mainChunk = initChunk();
    genTerrain(&mainChunk);
    SetTargetFPS(60);
    while(!WindowShouldClose())
    {
        update();

        BeginDrawing();

        ClearBackground(BLACK);
        drawWorld(cube, &mainChunk);
        
        DrawRectangle(0,0,100,24,BLACK);
        DrawFPS(0,0);
        EndDrawing();
    }
}