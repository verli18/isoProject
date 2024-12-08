struct vec2 cartToIso(int x, int y){
    struct vec2 isoVec;
    isoVec.x = (x-y) * tileW/2;
    isoVec.y = (x+y) * tileH/2;
    return isoVec;
}

void update()
{
    if(IsKeyDown(KEY_D)){cameraPos.x--;}
    if(IsKeyDown(KEY_A)){cameraPos.x++;}

    if(IsKeyDown(KEY_S)){cameraPos.y--;}
    if(IsKeyDown(KEY_W)){cameraPos.y++;}

}

void drawWorld(Texture2D cube, struct chunk *chunk)
{
    struct vec2 testVec;
    bool isVisible = false;

    for(int x = 0; x < 32; x++){
        for(int y = 0; y < 32; y++){
            for(int z = 0; z < 32; z++){
                    
                testVec = cartToIso(x,y);
                testVec.x += cameraPos.x;
                testVec.y += cameraPos.y-z*21;
                    
                if(testVec.x > -tileW && testVec.x < 480 && testVec.y > -tileH*2 && testVec.y < 270)
                {
                    isVisible = true;
                }
                else
                {
                    isVisible = false;
                }
                if(isVisible == true && chunk->ptr[x][y][z] != 0)
                {
                    DrawTexture(cube,testVec.x,testVec.y, (Color){x*8+5,y*8+5,z*8+5,255});
                }
            }
                

        }
    }
}