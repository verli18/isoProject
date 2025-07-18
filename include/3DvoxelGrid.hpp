#include <vector>
#include <raylib.h>

class VoxelGrid {
    public:
        VoxelGrid(int width, int height, int depth);
        ~VoxelGrid();
        void setVoxel(int x, int y, int z, bool value);
        void generatePerlinTerrain(float scale, int offsetX, int offsetY);
        bool getVoxel(int x, int y, int z);
        void render();

        unsigned int getWidth();
        unsigned int getHeight();
        unsigned int getDepth();

    private:
        Image perlinNoise;
        int width;
        int height;
        int depth;
        std::vector<std::vector<std::vector<bool>>> grid;
};