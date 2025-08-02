#include <vector>
#include <raylib.h>

class machine;

struct tile{
    //alright we're scrapping the marching cubes shit, that is way too annoying to deal with and honestly we won't even need layers of terrain so no use for that, let's use collumns now
    char type; //0 for air
    float tileHeight[4]; //TODO: make this int with half-unit increments
    Color lighting[4];
    int moisture = 0;
    int temperature = 0;

    // Interpreted as absolute Y level in half-units: waterY = 0.5f * waterLevel
    // When 0, tile has no water and is ignored for meshing & logic.
    int waterLevel = 0;

    // Machine data
    machine* occupyingMachine = nullptr; // Pointer to machine on this tile

};

class tileGrid {
    public:
        tileGrid(int width, int height);
        ~tileGrid();
        void setTile(int x, int y, tile voxel);
        // Generate terrain with Perlin noise fractal (multiple octaves)
        // scale: base frequency, offsetX/Y: noise seed offsets, heightCo: height coefficient
        // octaves: number of noise layers, persistence: amplitude attenuation, lacunarity: frequency multiplier, exponent: curve shaping
        void generatePerlinTerrain(float scale, int heightCo,
                                   int octaves = 4, float persistence = 0.25f,
                                   float lacunarity = 2.0f, float exponent = 1.0f, int baseGenOffset[] = {});
        tile getTile(int x, int y);
        void renderWires();
        void renderDataPoint(Color a, Color b, int tile::*dataMember, int chunkX, int chunkY);

        void generateMesh();
        // Generate a simple water mesh comprised of flat quads at water level per tile
        void generateWaterMesh();
        void updateLighting(Vector3 sunDirection, Vector3 sunColor, float ambientStrength, Vector3 ambientColor, float shiftIntensity, float shiftDisplacement);

        Mesh mesh;
        Mesh waterMesh;

        unsigned int getWidth();
        unsigned int getHeight();
        unsigned int getDepth();

        Vector3 getTileIndexDDA(Ray ray);

        // Machine management
        bool placeMachine(int x, int y, machine* machinePtr);
        void removeMachine(int x, int y);
        machine* getMachineAt(int x, int y);
        bool isOccupied(int x, int y);

        Model model;
        Model waterModel;
    private:
        bool meshGenerated = false;
        Image perlinNoise;
        int width;
        int height;
        int depth;
        std::vector<std::vector<tile>> grid;
};