#include "../include/machines.hpp"
// Initialize static ID counter
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

