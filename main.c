#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "init.c"
#include "logic.c"

void main()
{
    struct chunk mainChunk = initChunk();
    setCoord(&mainChunk, 0, 0, 1,1);
    printf("%x\n",getCoord(&mainChunk, 0, 0, 0));
    InitWindow(640,480, "window");

    while(!WindowShouldClose())
    {
        update();
        BeginDrawing();

        EndDrawing();
    }
}