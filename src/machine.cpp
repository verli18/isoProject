#include "../include/resourceManager.hpp"

int machine::nextID = 0;

machine::machine(machineType type, Vector3 position) {
    ID = nextID++;
    this->type = type;
    this->position = position;
}

machine::~machine() {
}

void machine::update() {
}

void machine::render() {
}

void machineManager::update() {
    for (std::unique_ptr<machine>& machine : machines) {
        machine->update();
    }
}

void machineManager::render() {
    //TODO: render solely what's visible
    for (std::unique_ptr<machine>& machine : machines) {
        machine->render();
    }
}

void machineManager::addMachine(std::unique_ptr<machine> machine) {
    previous = machine.get(); // Store pointer before moving
    machines.push_back(std::move(machine));
}

machineManager::machineManager() {
    machines = std::vector<std::unique_ptr<machine>>();
}

machineManager::~machineManager() {
}

drillMk1::drillMk1(Vector3 position) : machine(DRILLMK1, position) {}
drillMk1::~drillMk1() {}

void drillMk1::update() {

}

void drillMk1::render() {
    DrawModel(resourceManager::getMachineModel(type), {position.x+0.5f, position.y, position.z}, 0.5f, WHITE);
}

conveyorMk1::conveyorMk1(Vector3 position) : machine(CONVEYORMK1, position) {}

void conveyorMk1::update() {

}

void conveyorMk1::render() {
    DrawModel(resourceManager::getMachineModel(type), {position.x+0.5f, position.y, position.z+0.5f}, 0.5f, WHITE);
}

droppedItem::droppedItem(Vector3 position, itemType type) : machine(ITEM, position) {
    itemInstance.type = type;
    itemInstance.quantity = 1; // Assuming default quantity of 1 for dropped items
}
droppedItem::~droppedItem() {}

void droppedItem::update() {
}

void droppedItem::render() {
    Texture2D texture = resourceManager::getItemTexture(static_cast<itemType>(itemInstance.type));
    Rectangle sourceRect = resourceManager::getItemTextureUV(static_cast<itemType>(itemInstance.type));
    DrawBillboardRec(resourceManager::camera, texture, sourceRect, position, Vector2{0.5f, 0.5f}, WHITE);
}


