#ifndef TEXTURE_ATLAS_HPP
#define TEXTURE_ATLAS_HPP

struct textureAtlas {
    int width;
    int height;
    int uOffset;
    int vOffset;    
    int sideUOffset;
    int sideVOffset;
};

enum {
  AIR = 0,
  GRASS,
  SNOW,
  DIRT,
};

inline struct textureAtlas textures[] = {
    {0, 0, 0, 0, 0, 0},
    {16, 16, 0, 0, 16, 0},
    {16, 16, 32, 0, 48, 0},
    {16, 16, 48, 0, 48, 0},
};

inline int textureCount = sizeof(textures) / sizeof(textures[0]);

#endif // TEXTURE_ATLAS_HPP