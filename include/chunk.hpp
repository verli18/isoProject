#include "tileGrid.hpp"

#define CHUNKSIZE 32

class Chunk {
    public:
        Chunk(int x, int y);
        ~Chunk();
        void generateMesh();
        void updateMesh();

        void render();
        void renderWires();
        void renderDataPoint();

        tileGrid tiles;
        Texture2D textureAtlas;
        Model model;
        Mesh mesh;
        private:
        int chunkX, chunkY;
        bool meshGenerated;
};