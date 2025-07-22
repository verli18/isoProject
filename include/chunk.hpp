#include "marchingCubes.hpp"

#define CHUNKSIZE 16

class Chunk {
    public:
        Chunk(int x, int y, int z);
        ~Chunk();
        void generateMesh();
        void updateMesh();

        void render();
        void renderWires();

        VoxelGrid voxelGrid;
        MarchingCubes marchingCubes;
        Texture2D textureAtlas;
        Model model;
        Mesh mesh;
        private:
        int chunkX, chunkY, chunkZ;
        bool meshGenerated;
};