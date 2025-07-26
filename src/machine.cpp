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
    DrawModel(resourceManager::getMachineModel(type), position, 1.0f, WHITE);
}

void conveyorMk1::update() {

}

void conveyorMk1::render() {
    DrawModel(resourceManager::getMachineModel(type), position, 1.0f, WHITE);
}

conveyorMk1::conveyorMk1(Vector3 position) : machine(CONVEYORMK1, position) {}


