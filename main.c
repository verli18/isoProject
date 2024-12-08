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
   
    while(!WindowShouldClose())
    {
        update();

        BeginDrawing();

        ClearBackground(BLACK);
        struct vec2 testVec;
        for(int x = 0; x < 32; x++){
            for(int y = 0; y < 32; y++){
                for(int z = 0; z < 32; z++){
                    
                    testVec = cartToIso(x,y);
                    testVec.x += cameraPos.x;
                    testVec.y += cameraPos.y-z*21;
                    if(testVec.x > -tileW && testVec.x < 480 && testVec.y > -tileH*2 && testVec.y < 270){
                        DrawTexture(cube,testVec.x,testVec.y, (Color){x*8,y*8,z*8,255});
                    }
                }
                

            }
        }
        DrawRectangle(0,0,100,24,BLACK);
        DrawFPS(0,0);
       EndDrawing();
    }
}