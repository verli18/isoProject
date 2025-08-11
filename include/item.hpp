#pragma once
#include <cstdint>

// Note: Removed constructor/destructor to make this an aggregate type
// This allows for brace initialization, e.g. item myItem = {IRON_ORE, 1};
class item{
    public:
    uint16_t type;
    uint16_t quantity;
}; 

struct itemTextureUVs{
    int Uoffset;
    int Voffset;
    int Usize;
    int Vsize;
};

enum itemType{
    IRON_ORE,
    COPPER_ORE,    
};