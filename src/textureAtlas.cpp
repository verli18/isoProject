#include "../include/textureAtlas.hpp"

struct textureAtlas textures[] = {
    {16, 16, 0, 0},
    {16, 16, 16, 0},
    {16, 16, 32, 0},
    {16, 16, 48, 0},
    {16, 16, 64, 0},
};

int textureCount = sizeof(textures) / sizeof(textures[0]);
