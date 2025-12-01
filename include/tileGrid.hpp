#ifndef TILEGRID_HPP
#define TILEGRID_HPP

#include <vector>
#include <raylib.h>
#include <cstdint> // for uint64_t

#include "resourceManager.hpp"
#include "biome.hpp"


// Tunable parameters for water and hydrology generation
struct WaterParams {
    float waterAmountScale = 1.0f;
    float minDepthBase = 0.0f;
    float drynessCoeff = 0.0f;
    float evapCoeff = 0.0f;
    float evapStart = 0.0f;
    float evapRange = 1.0f;
    int waterPad = 0;
    float waterSkirt = 0.0f;
    float seaLevelThreshold = 30.0f; // Default to a very low value so it doesn't interfere by default
};

class machine;

struct tile{
    // Biome data
    BiomeType biome;              // Primary biome
    BiomeType secondaryBiome;     // Secondary biome for blending
    
    char type; // Texture ID (kept for compatibility, but should be derived from biome)
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
    uint8_t crystalinePotential = 0;
    
    // Biome blending data (for smooth transitions)
    uint8_t secondaryType = 0;    // Secondary texture to blend towards
    uint8_t blendStrength = 0;    // 0=100% primary, 255=100% secondary
    
    // Erosion data (computed from WorldMap erosion simulation)
    // 0 = flat/depositional area (full grass/snow coverage)
    // 255 = heavily eroded (exposed dirt/rock, minimal vegetation)
    uint8_t erosionFactor = 0;
    
    // Water data
    // Interpreted as absolute Y level in half-units: waterY = 0.5f * waterLevel
    uint8_t waterLevel = 0;
    // River data: D8 flow direction (0-7 = E,SE,S,SW,W,NW,N,NE; 255 = no flow/lake)
    uint8_t flowDir = 255;
    // River width at this tile (0 = no river, 1-255 = river width category)
    uint8_t riverWidth = 0;
    // Marching squares case for river shape (0-15, based on neighbor connectivity)
    uint8_t riverCase = 0;

    // Machine data
    machine* occupyingMachine = nullptr; // Pointer to machine on this tile
    machineTileOffset tileOffset;
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

        tileGrid* neighborChunks[8];
        
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
        
};

#endif // TILEGRID_HPP