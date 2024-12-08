#define tileH 24
#define tileW 48

struct chunk{
    char *ptr[32][32];
};

struct vec2{
    int x;
    int y;
};
struct vec2 cameraPos;


struct chunk initChunk()
{
    struct chunk chunk;

    for(int i = 0; i<32; i++){
        for(int j = 0; j<32; j++){
            chunk.ptr[i][j] = calloc(32,1);
        }
    }
    return chunk;
}

void genTerrain(struct chunk *chunk)
{
    Image noise = GenImagePerlinNoise(32, 32, 0, 0, 0.5);
    Color noiseSample;
    for(int x = 0; x < 32; x++){
        for(int y = 0; y < 32; y++){
            for(int z = 0; z < 32; z++){

            noiseSample = GetImageColor(noise, x,y);

            if(z < (noiseSample.r/16))
                chunk->ptr[x][y][z] = 1;
            }
        }
    }
}

void init()
{
    InitWindow(480,270, "window");
}