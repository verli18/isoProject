#include "../include/inventory.hpp"

Inventory::Inventory(std::vector<InventorySlot> initialSlots) {
    this->slots = std::move(initialSlots);
}

Inventory::~Inventory() {}

bool Inventory::tryAddItem(item anItem) {
    if (anItem.quantity <= 0) {
        return true; // Nothing to add
    }

    // First pass: Try to stack with existing items in suitable slots
    // An item can be placed in any slot that is not strictly for INPUT from an external source.
    // The machine's own logic can place items into its OUTPUT buffer.
    for (auto& slot : slots) {
        // Allow adding to OUTPUT slots internally.
        if (slot.type == INPUT || slot.type == STORAGE || slot.type == OUTPUT) {
            if (slot.currentItem.type == anItem.type && slot.currentItem.quantity > 0) {
                uint16_t space = slot.capacity - slot.currentItem.quantity;
                if (space > 0) {
                    uint16_t toAdd = std::min((uint16_t)anItem.quantity, space);
                    slot.currentItem.quantity += toAdd;
                    anItem.quantity -= toAdd;
                    if (anItem.quantity == 0) {
                        return true; // Entire item stack was added
                    }
                }
            }
        }
    }

    // Second pass: Try to place remaining items in empty slots
    for (auto& slot : slots) {
        if (slot.type == INPUT || slot.type == STORAGE || slot.type == OUTPUT) {
            if (slot.currentItem.quantity == 0) {
                // Check filter
                if (!slot.filter.has_value() || slot.filter.value() == anItem.type) {
                    uint16_t toAdd = std::min((uint16_t)anItem.quantity, (uint16_t)slot.capacity);
                    slot.currentItem.type = anItem.type;
                    slot.currentItem.quantity = toAdd;
                    anItem.quantity -= toAdd;
                    if (anItem.quantity == 0) {
                        return true; // Entire item stack was added
                    }
                }
            }
        }
    }

    return false;
}

bool Inventory::tryTakeItem(item& anItem, std::optional<itemType> desiredType) {
    // Iterate through slots to find a suitable item
    for (auto& slot : slots) {
        if (slot.type == OUTPUT || slot.type == STORAGE) {
            if (slot.currentItem.quantity > 0) {
                bool typeMatch = !desiredType.has_value() || slot.currentItem.type == desiredType.value();
                if (typeMatch) {
                    // Take the entire stack from the slot
                    anItem = slot.currentItem;
                    slot.currentItem.quantity = 0;
                    return true;
                }
            }
        }
    }

    return false; // No suitable item found
}
