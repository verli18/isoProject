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
    Texture2D noise = LoadTextureFromImage(GenImagePerlinNoise(32, 32, 0, 0, 1.0));
}
void init()
{
    InitWindow(480,270, "window");
}