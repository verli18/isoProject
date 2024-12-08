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