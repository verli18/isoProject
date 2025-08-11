#include "../include/machineManager.hpp"
#include "../include/machines.hpp"
#include "../include/resourceManager.hpp"
#include <raymath.h> // For Vector3Lerp

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

// --- DrillMk1 --- //

drillMk1::drillMk1(Vector3 position) 
    : machine(DRILLMK1, position, {{0, 0}}),
      // Use standard brace-initialization for the inventory slot
      inventory({ {OUTPUT, IRON_ORE, 64, {IRON_ORE, 0}} })
{}

drillMk1::~drillMk1() {}

void drillMk1::update(machineManager& manager) {
    const float PRODUCTION_TIME = 2.0f; // seconds to produce 1 ore
    productionProgress += GetFrameTime();

    if (productionProgress >= PRODUCTION_TIME) {
        productionProgress -= PRODUCTION_TIME;
        
        // This initialization now works because item is an aggregate type
        item newOre = {IRON_ORE, 1};
        inventory.tryAddItem(newOre);
    }
}

void drillMk1::render() {
    DrawModel(resourceManager::getMachineModel(type), {position.x+0.5f, position.y, position.z}, 0.5f, WHITE);
}

// --- ConveyorMk1 --- //

conveyorMk1::conveyorMk1(Vector3 position) 
    : machine(CONVEYORMK1, position, {{0, 0}}),
      // Use standard brace-initialization for the inventory slot
      inventory({ {STORAGE, std::nullopt, 1, {IRON_ORE, 0}} }) // one storage slot, capacity 1
{}

bool conveyorMk1::giveItem(item anItem, machineManager& manager) {

}

void conveyorMk1::update(machineManager& manager) {
}

void conveyorMk1::render() {
    DrawModel(resourceManager::getMachineModel(type), {position.x+0.5f, position.y, position.z+0.5f}, 0.5f, WHITE);

    // Render item on conveyor if present
    Inventory* inv = getInventory();
    if (inv && inv->getSlots()[0].currentItem.quantity > 0) {
        item currentItem = inv->getSlots()[0].currentItem;
        Texture2D texture = resourceManager::getItemTexture(static_cast<itemType>(currentItem.type));
        Rectangle sourceRect = resourceManager::getItemTextureUV(static_cast<itemType>(currentItem.type));
        DrawBillboardRec(resourceManager::camera, texture, sourceRect, position, Vector2{0.5f, 0.5f}, WHITE);
    }
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
}
