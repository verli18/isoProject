
struct textureAtlas {
    int width;
    int height;
    int uOffset;
    int vOffset;    
};

enum {
  AIR = 0,
  DIRT,
  GRASS,
  STONE,
  SNOW,
};

extern struct textureAtlas textures[];
extern int textureCount;
