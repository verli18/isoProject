#include <vector>
#include <raylib.h>
#include <cmath>


struct Voxels{
    char type; //0 for air
    Color lighting;
    bool isTerrain;
    bool isMachine;
};

class VoxelGrid {
    public:
        VoxelGrid(int width, int height, int depth);
        ~VoxelGrid();
        void setVoxel(int x, int y, int z, Voxels voxel);
        void generatePerlinTerrain(float scale, int offsetX, int offsetY);
        Voxels getVoxel(int x, int y, int z);
        void render();

        unsigned int getWidth();
        unsigned int getHeight();
        unsigned int getDepth();

        Vector3 getVoxelIndexDDA(Ray ray);

    private:
        Image perlinNoise;
        int width;
        int height;
        int depth;
        std::vector<std::vector<std::vector<Voxels>>> grid;
};