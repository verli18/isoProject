#include <vector>
#include <raylib.h>
#include <cstdint> // for uint64_t
#include "FastNoise/FastNoise.h"
#include "FastNoise/Generators/DomainWarp.h"

// Timing data for terrain pipeline phases
struct PhaseAverages {
    double noiseGenMs = 0.0;
    double indexingMs = 0.0;
    double meshGenMs = 0.0;
    double waterGenMs = 0.0;          // water mesh gen
    double tempMoistSampleMs = 0.0;   // sampling of temp/moist
    uint64_t samplesNoise = 0;
    uint64_t samplesIndex = 0;
    uint64_t samplesMesh = 0;
    uint64_t samplesWater = 0;
    uint64_t samplesTempMoist = 0;
};

class machine;

struct tile{
    char type; //0 for air
    float tileHeight[4]; //TODO: make this int with half-unit increments
    Color lighting[4];
    int moisture = 0;
    int temperature = 0;

    // Interpreted as absolute Y level in half-units: waterY = 0.5f * waterLevel
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

        // Retrieve running average timings for phases
        PhaseAverages getPhaseAverages();

        // Expose last measurement snapshot (ms) for UI
        PhaseAverages lastTimings;

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
};