#include "../include/item.hpp"

item::item() : type(IRON_ORE), quantity(0) {
    // Default constructor initializes with IRON_ORE and quantity 0
}

item::~item() {
    // Destructor - nothing to clean up as there's no dynamic memory
}
