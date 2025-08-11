#pragma once

#include "item.hpp"
#include <vector>
#include <optional>

// Defines the purpose of an inventory slot
enum SlotType {
    INPUT,  // Can only have items put into it from the outside
    OUTPUT, // Can only have items taken from it from the outside
    STORAGE // Can be used for both (e.g., a chest or internal buffer)
};

struct InventorySlot {
    // --- Configuration (set on creation)
    SlotType type = STORAGE;
    std::optional<itemType> filter; // Optional: if set, only allows this item type
    uint16_t capacity = 64; // Max stack size for discrete items

    // --- State
    item currentItem = {IRON_ORE, 0}; // Default to empty

    // Note: Removed constructor to make this an aggregate type
};

class Inventory {
public:
    // Constructor can take a pre-configured list of slots
    Inventory(std::vector<InventorySlot> initialSlots);
    ~Inventory();

    /**
     * @brief Tries to add an item to any suitable INPUT or STORAGE slot.
     * Handles stacking with existing items.
     * @param anItem The item to add.
     * @return True if the entire item stack was successfully added, false otherwise.
    */
    bool tryAddItem(item anItem);

    /**
     * @brief Tries to take an item from any suitable OUTPUT or STORAGE slot.
     * @param anItem (output) The item that was taken.
     * @param desiredType (optional) The specific type of item to take.
     * @return True if an item was successfully taken, false otherwise.
    */
    bool tryTakeItem(item& anItem, std::optional<itemType> desiredType);

    const std::vector<InventorySlot>& getSlots() const { return slots; }

private:
    std::vector<InventorySlot> slots;
};