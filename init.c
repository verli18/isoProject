struct chunk{
    char *ptr;
};

struct chunk initChunk()
{
    struct chunk chunk;
    chunk.ptr = calloc(32*32*32,1);
    return chunk;
}