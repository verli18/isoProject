#include "../include/machineManager.hpp"
#include <algorithm>

machineManager::machineManager() {}

machineManager::~machineManager() {}

void machineManager::addMachine(std::unique_ptr<machine> machine) {
    // Note: Assumes machine->globalPos has been set before adding.
    // The chunkManager should be responsible for setting this.
    machineGrid[machine->globalPos] = machine.get();
    machines.push_back(std::move(machine));
}

machine* machineManager::getMachineAt(globalMachinePos pos) {
    auto it = machineGrid.find(pos);
    if (it != machineGrid.end()) {
        return it->second;
    }
    return nullptr;
}

void machineManager::removeMachineAt(globalMachinePos pos) {
    if (!world) return;

    auto it = machineGrid.find(pos);
    if (it == machineGrid.end()) {
        return; // No machine at this position
    }

    machine* machineToRemove = it->second;

    // --- Remove from tileGrid ---
    // Assuming chunk size is 32x32. This should probably be a shared constant.
    const int chunkSize = 32;
    int chunkX = pos.x / chunkSize;
    int chunkY = pos.y / chunkSize;
    int localX = pos.x % chunkSize;
    int localY = pos.y % chunkSize;

    // Handle negative coordinates correctly
    if (pos.x < 0) { chunkX--; localX += chunkSize; }
    if (pos.y < 0) { chunkY--; localY += chunkSize; }

    Chunk* chunk = world->getChunk(chunkX, chunkY);
    if (chunk) {
        chunk->tiles.removeMachine(localX, localY);
    }

    // --- Remove from manager ---
    machineGrid.erase(it);

    machines.erase(
        std::remove_if(machines.begin(), machines.end(),
                       [&](const std::unique_ptr<machine>& m) {
                           return m.get() == machineToRemove;
                       }),
        machines.end());
}

void machineManager::update() {
    // By passing "*this", each machine's update method gets a reference
    // to the manager, allowing it to query for other machines.
    for (auto& machine : machines) {
        machine->update(*this);
    }
}

void machineManager::render() {
    for (auto& machine : machines) {
        machine->render();
    }
}
