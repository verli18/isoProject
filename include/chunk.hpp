#include "tileGrid.hpp"

#define CHUNKSIZE 32

class Chunk {
    public:
        Chunk(int x, int y);
        ~Chunk();
        void generateMesh();
        void updateMesh();

        void render();
        // Draw only opaque terrain
        void renderTerrain();
        // Draw only transparent water layer
        void renderWater();
        void renderWires();
        // Draw only water wireframe
        void renderWaterWires();
        void renderDataPoint();

        tileGrid tiles;
        Texture2D textureAtlas;
        Model model;
        Mesh mesh;
        private:
        int chunkX, chunkY;
        bool meshGenerated;
};