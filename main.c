#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "init.c"
#include "logic.c"

void main()
{
    struct chunk mainChunk = initChunk();
    mainChunk.ptr[0][0][1] = 4;
    printf("%x\n",mainChunk.ptr[0][0][1]);
    InitWindow(640,480, "window");

    while(!WindowShouldClose())
    {
        update();
        BeginDrawing();

        EndDrawing();
    }
}