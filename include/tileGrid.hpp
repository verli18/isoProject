#include <vector>
#include <raylib.h>
#include <cstdint> // for uint64_t
#include "FastNoise/FastNoise.h"
#include "FastNoise/Generators/DomainWarp.h"


// Tunable parameters for water and hydrology generation
struct WaterParams {
    // Climate -> water presence thresholding
    float minDepthBase = 0.05f;       // baseline minimum depth to keep water
    float drynessCoeff = 0.20f;       // additional min depth per unit dryness (1 - moisture)
    float evapCoeff    = 0.20f;       // additional min depth per unit evaporation
    float evapStart    = 0.70f;       // normalized temperature where evaporation starts
    float evapRange    = 0.30f;       // range to reach full evaporation effect
    float waterAmountScale = 1.0f;    // scales raw lake depth before thresholding

    // Water mesh skirt to hide terrain seams
    int   waterPad       = 2;         // tiles to dilate water outward in mesh
    float waterSkirt     = 0.5f;      // epsilon overlap above terrain when deciding coverage

    // Hydrological potential smoothing/weighting
    int   hydroBlurIters = 1;         // 0..3 recommended
    float hydroSlopeIntercept = 0.4f; // factor in (intercept + weight*slope)
    float hydroSlopeWeight    = 0.6f; // weight for local slope contribution
};

class machine;

struct tile{
    char type; //type is basically the biome, too lazy to rewrite all the instances of type as biome
    float tileHeight[4]; //TODO: make this int with half-unit increments
    uint16_t lighting; //let's use RGB15 because why not lmao 
    
    //Biome data
    uint8_t moisture = 0;
    uint8_t temperature = 0;

    //geological data
    uint8_t magmaticPotential = 0;
    uint8_t sulfidePotential = 0;
    uint8_t hydrologicalPotential = 0;
    uint8_t biologicalPotential = 0;
    // Interpreted as absolute Y level in half-units: waterY = 0.5f * waterLevel
    uint8_t waterLevel = 0;

    // Machine data
    machine* occupyingMachine = nullptr; // Pointer to machine on this tile
};

class tileGrid {
    public:
        tileGrid(int width, int height);
        ~tileGrid();
        void setTile(int x, int y, tile voxel);
        // Generate terrain with Perlin noise fractal (multiple octaves)
        void generatePerlinTerrain(float scale, int heightCo,
                                   int octaves = 4, float persistence = 0.25f,
                                   float lacunarity = 2.0f, float exponent = 1.0f, int baseGenOffset[] = {});
        tile getTile(int x, int y);
        void renderWires();
        void renderDataPoint(Color a, Color b, uint8_t tile::*dataMember, int chunkX, int chunkY);

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

    // Parameters you can tweak at runtime before generation
    WaterParams waterParams;

    // Optional helper to set parameters in one call
    void setWaterParams(const WaterParams& params) { waterParams = params; }

    private:
        bool meshGenerated = false;
        Image perlinNoise;
        int width;
        int height;
        int depth;
        std::vector<std::vector<tile>> grid;
        FastNoise::SmartNode<FastNoise::FractalFBm> fnFractal;
        FastNoise::SmartNode<FastNoise::DomainWarpGradient> fnWarp;
        FastNoise::SmartNode<FastNoise::FractalFBm> fnRegion;
        FastNoise::SmartNode<FastNoise::FractalFBm> fnMoisture;
        FastNoise::SmartNode<FastNoise::FractalFBm> fnTemperature;
        FastNoise::SmartNode<FastNoise::FractalFBm> fnMagmatic;
        FastNoise::SmartNode<FastNoise::FractalFBm> fnBiological;
};