#include "../include/machineManager.hpp"
#include "../include/machines.hpp"
#include "../include/resourceManager.hpp"
#include <cstdint>

int machine::nextID = 0;

machine::machine(machineType type, Vector3 position) : type(type), position(position), ID(nextID++) {
}

machine::~machine() {}

void machine::updateAnimation() {
    // Placeholder for animation logic
}

machine::machine(machineType type, Vector3 position, std::initializer_list<machineTileOffset> offsets)
    : machine(type, position) {
    tileOffsets.assign(offsets.begin(), offsets.end());
}

globalMachinePos machine::getSlotGlobalPosition(const machineTileOffset& slotOffset) const {
    machineTileOffset rotatedOffset = slotOffset.getRotatedOffset(dir);
    return globalMachinePos{globalPos.x + rotatedOffset.x, globalPos.y + rotatedOffset.y};
}

void machine::renderSlots() {
    // Render slots if the machine has an inventory
    Inventory* inv = getInventory();
    if (!inv) return;
    
    const std::vector<InventorySlot>& slots = inv->getSlots();
    
    // For each slot, render lines to show its position and type
    for (size_t i = 0; i < slots.size(); ++i) {
        const InventorySlot& slot = slots[i];
        
        // Determine slot color based on type
        Color slotColor;
        switch (slot.type) {
            case INPUT:
                slotColor = Color{212, 127, 44, 255}; // Orange
                break;
            case OUTPUT:
                slotColor = Color{66, 32, 135, 255}; // Purple
                break;
            case STORAGE:
                slotColor = Color{93, 165, 162, 255}; // Teal
                break;
            default:
                slotColor = Color{255, 255, 255, 255}; // White
                break;
        }
        
        // If the slot has an interface tile, render it at that position
        if (slot.interfaceTile.has_value()) {
            slotInterfaceTile interfaceTile = slot.interfaceTile.value();
            
            // Transform the interface tile position based on machine direction
            machineTileOffset offset = {interfaceTile.y, interfaceTile.x};
            machineTileOffset rotatedOffset = offset.getRotatedOffset(dir);
            
            // Calculate the world position of the slot
            Vector3 slotPos = {position.x + rotatedOffset.x + 0.5f, position.y, position.z + rotatedOffset.y + 0.5f};
            
            // Draw a vertical line from the ground up to show the slot
            DrawLine3D(slotPos, Vector3{slotPos.x, slotPos.y + 1.0f, slotPos.z}, slotColor);
        }
    }
}

// --- DrillMk1 --- //

drillMk1::drillMk1(Vector3 position) 
    : machine(DRILLMK1, position, {{0, 0}, {0,1}}),
      // Use standard brace-initialization for the inventory slot
      inventory({ InventorySlot{slotInterfaceTile{-1,0}, OUTPUT, IRON_ORE, 64, {IRON_ORE, 0}} })
{}

drillMk1::~drillMk1() {}

void drillMk1::update(machineManager& manager) {
    const float PRODUCTION_TIME = 2.0f; // seconds to produce 1 ore
    productionProgress += GetFrameTime();

    if (productionProgress >= PRODUCTION_TIME) {
        productionProgress -= PRODUCTION_TIME;
        
        // This initialization now works because item is an aggregate type
        item newOre = {IRON_ORE, 1};
        
        // Try to give the item to the machine in front of the drill
        // Use the standard forward direction (0, -1) and rotate it based on drill direction
        machineTileOffset forwardOffset = {0, -1};
        machineTileOffset rotatedOffset = forwardOffset.getRotatedOffset(dir);
        globalMachinePos nextPos = {globalPos.x + rotatedOffset.x, globalPos.y + rotatedOffset.y};
        
        machine* nextMachine = manager.getMachineAt(nextPos);
        if (nextMachine != nullptr) {
            // Try to give the item to the next machine
            if (!nextMachine->giveItem(newOre, manager)) {
                // If the next machine couldn't accept the item, add it to our own inventory
                inventory.tryAddItem(newOre);
            }
        } else {
            // No machine in front, add to our own inventory
            inventory.tryAddItem(newOre);
        }
    }
}

void drillMk1::render() {
    // Calculate rotation angle based on direction
    float rotationAngle = 0.0f;
    switch (dir) {
        case NORTH: rotationAngle = 0.0f; break;
        case EAST:  rotationAngle = 90.0f; break;
        case SOUTH: rotationAngle = 180.0f; break;
        case WEST:  rotationAngle = 270.0f; break;
    }
    
    // Draw the model with rotation
    DrawModelEx(resourceManager::getMachineModel(type), {position.x+0.5f, position.y, position.z}, 
                {0, 1, 0}, rotationAngle, Vector3{0.5f, 0.5f, 0.5f}, WHITE);
                
    // Render slots for debugging
    renderSlots();
}

// --- ConveyorMk1 --- //

conveyorMk1::conveyorMk1(Vector3 position) 
    : machine(CONVEYORMK1, position, {{0, 0}}),
      // Use standard brace-initialization for the inventory slot
      inventory({ InventorySlot{slotInterfaceTile{-1,0}, STORAGE, std::nullopt, 1, {IRON_ORE, 0}} }) // one storage slot, capacity 1
{}

bool conveyorMk1::giveItem(item anItem, machineManager& manager) {
    // First check if we can accept the item (have space)
    // We need to check if our inventory slot is empty since we only have one slot with capacity 1
    const std::vector<InventorySlot>& slots = inventory.getSlots();
    const InventorySlot& slot = slots[0];
    if (slot.currentItem.quantity == 0) {
        // Reset processing time to create cooldown
        processingProgress = 0.0f;
        movementProgress = 0.0f; // Reset movement progress
        // Store the item temporarily
        heldItem = anItem;
        hasHeldItem = true;
        hasItemToRender = true; // Mark that we should render an item
        return true;
    }
    return false; // Could not accept item (slot is occupied)
}

void conveyorMk1::update(machineManager& manager) {
    // If we have a held item, process it
    if (hasHeldItem) {
        // If this is the first update cycle for this item, add it to our inventory
        const std::vector<InventorySlot>& slots = inventory.getSlots();
        if (slots[0].currentItem.quantity == 0) {
            inventory.tryAddItem(heldItem);
        }
        
        processingProgress += GetFrameTime();
        movementProgress += GetFrameTime() / PROCESSING_TIME; // Update movement progress
        
        // Check if processing is complete
        if (processingProgress >= PROCESSING_TIME) {
            // Try to pass the item to the next machine in the chain
            // Use the standard forward direction (0, -1) and rotate it based on conveyor direction
            machineTileOffset forwardOffset = {0, -1};
            machineTileOffset rotatedOffset = forwardOffset.getRotatedOffset(dir);
            globalMachinePos nextPos = {globalPos.x + rotatedOffset.x, globalPos.y + rotatedOffset.y};
            
            machine* nextMachine = manager.getMachineAt(nextPos);
            if (nextMachine != nullptr) {
                // Try to give the item to the next machine
                if (nextMachine->giveItem(heldItem, manager)) {
                    // Successfully passed the item, clear our held item and remove from inventory
                    hasHeldItem = false;
                    processingProgress = 0.0f;
                    movementProgress = 0.0f; // Reset movement progress
                    hasItemToRender = false; // No longer need to render the item
                    // Remove the item from our inventory
                    item takenItem;
                    inventory.tryTakeItem(takenItem, static_cast<itemType>(heldItem.type));
                }
                // If next machine couldn't accept, we keep the item and will try again next update
            } else {
                // No machine in front, keep the item in our inventory
                // It will be rendered but not moved until a machine is placed
            }
        }
    }
}

void conveyorMk1::render() {
    // Calculate rotation angle based on direction
    float rotationAngle = 0.0f;
    switch (dir) {
        case NORTH: rotationAngle = 0.0f; break;
        case EAST:  rotationAngle = 90.0f; break;
        case SOUTH: rotationAngle = 180.0f; break;
        case WEST:  rotationAngle = 270.0f; break;
    }
    
    // Draw the model with rotation
    DrawModelEx(resourceManager::getMachineModel(type), {position.x+0.5f, position.y, position.z+0.5f}, 
                {0, 1, 0}, rotationAngle, Vector3{0.5f, 0.5f, 0.5f}, WHITE);

    // Render item on conveyor if present
    Inventory* inv = getInventory();
    if ((inv && inv->getSlots()[0].currentItem.quantity > 0) || hasItemToRender) {
        item currentItem;
        if (inv && inv->getSlots()[0].currentItem.quantity > 0) {
            currentItem = inv->getSlots()[0].currentItem;
        } else {
            currentItem = heldItem;
        }
        
        Texture2D texture = resourceManager::getItemTexture(static_cast<itemType>(currentItem.type));
        Rectangle sourceRect = resourceManager::getItemTextureUV(static_cast<itemType>(currentItem.type));
        
        // Calculate start and end positions based on conveyor direction
        Vector3 startPos = {0, 0, 0}, endPos = {0, 0, 0};
        switch (dir) {
            case NORTH:
                startPos = {position.x + 0.5f, position.y + 0.2f, position.z + 1.0f};
                endPos = {position.x + 0.5f, position.y + 0.2f, position.z};
                break;
            case EAST:
                startPos = {position.x, position.y + 0.2f, position.z + 0.5f};
                endPos = {position.x + 1.0f, position.y + 0.2f, position.z + 0.5f};
                break;
            case SOUTH:
                startPos = {position.x + 0.5f, position.y + 0.2f, position.z};
                endPos = {position.x + 0.5f, position.y + 0.2f, position.z + 1.0f};
                break;
            case WEST:
                startPos = {position.x + 1.0f, position.y + 0.2f, position.z + 0.5f};
                endPos = {position.x, position.y + 0.2f, position.z + 0.5f};
                break;
        }
        
        // Lerp between start and end positions based on movement progress
        float t = fminf(movementProgress, 1.0f); // Clamp to 1.0
        Vector3 itemPos = {
            startPos.x + t * (endPos.x - startPos.x),
            startPos.y + t * (endPos.y - startPos.y),
            startPos.z + t * (endPos.z - startPos.z)
        };
        
        // Position slightly above the conveyor
        DrawBillboardRec(resourceManager::camera, texture, sourceRect, itemPos, Vector2{0.5f, 0.5f}, WHITE);
    }
    
    // Render slots for debugging
    renderSlots();
}

// --- DroppedItem --- //

droppedItem::droppedItem(Vector3 position, itemType type) : machine(ITEM, position, {{0, 0}}) {
    itemInstance.type = type;
    itemInstance.quantity = 1;
}
droppedItem::~droppedItem() {}
void droppedItem::update(machineManager& manager) {}
void droppedItem::render() {
    Texture2D texture = resourceManager::getItemTexture(static_cast<itemType>(itemInstance.type));
    Rectangle sourceRect = resourceManager::getItemTextureUV(static_cast<itemType>(itemInstance.type));
    DrawBillboardRec(resourceManager::camera, texture, sourceRect, position, Vector2{0.5f, 0.5f}, WHITE);
    
    // Render slots for debugging (won't show anything since dropped items don't have slots)
    renderSlots();
}
