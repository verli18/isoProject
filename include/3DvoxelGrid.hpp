#include <vector>
#include <raylib.h>
#include <cmath>


struct tile{
    //alright we're scrapping the marching cubes shit, that is way too annoying to deal with and honestly we won't even need layers of terrain so no use for that, let's use collumns now
    char type; //0 for air
    int tileHeight[4]; //one for each corner
    Color lighting;
    
};

class VoxelGrid {
    public:
        VoxelGrid(int width, int height);
        ~VoxelGrid();
        void setVoxel(int x, int y, tile voxel);
        void generatePerlinTerrain(float scale, int offsetX, int offsetY, int heightCo);
        tile getVoxel(int x, int y);
        void renderSurface();
        void renderWires();

        void generateMesh();

        Mesh mesh;

        unsigned int getWidth();
        unsigned int getHeight();
        unsigned int getDepth();

        Vector3 getVoxelIndexDDA(Ray ray);

        Model model;
    private:
        Image perlinNoise;
        int width;
        int height;
        int depth;
        std::vector<std::vector<tile>> grid;
};