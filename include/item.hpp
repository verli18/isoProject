#include <unordered_map>
class item{
    public:
        item();
        ~item();
    int type;
    int quantity;
    
    private:
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
