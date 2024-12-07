void setCoord(struct chunk *chunk, int z,int y, int x, char value)
{
    chunk->ptr[z*1024+y*32+x]= value;
}

char getCoord(struct chunk *chunk, int x,int y, int z)
{
    return chunk->ptr[z*1024+y*32+x];
}

void update()
{

}