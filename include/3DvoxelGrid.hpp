#include <vector>
#include <raylib.h>


struct tile{
    //alright we're scrapping the marching cubes shit, that is way too annoying to deal with and honestly we won't even need layers of terrain so no use for that, let's use collumns now
    char type; //0 for air
    float tileHeight[4]; //one for each corner
    Color lighting[4];

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
        void updateLighting(Vector3 sunDirection, Vector3 sunColor, float ambientStrength, Vector3 ambientColor, float shiftIntensity, float shiftDisplacement);

        Mesh mesh;

        unsigned int getWidth();
        unsigned int getHeight();
        unsigned int getDepth();

        Vector3 getVoxelIndexDDA(Ray ray);

        Model model;
        Shader terrainShader;
        
        // Lighting uniforms
        int sunDirectionLoc;
        int sunColorLoc;
        int ambientStrengthLoc;
        int ambientColorLoc;
        float shiftIntensityLoc;
        float shiftDisplacementLoc;
    private:
        Image perlinNoise;
        int width;
        int height;
        int depth;
        std::vector<std::vector<tile>> grid;
};