struct chunk{
    char *ptr[32][32];
};

struct chunk initChunk()
{
    struct chunk chunk;
    // chunk.ptr = calloc(32*32*32,1);
    for(int i = 0; i<32; i++){
        for(int j = 0; j<32; j++){
            chunk.ptr[i][j] = calloc(32,1);
        }
    }
    return chunk;
}