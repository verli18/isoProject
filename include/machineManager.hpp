#pragma once

#include <vector>
#include <memory>
#include <map>
#include "machines.hpp"
#include "chunkManager.hpp"

class machineManager {
public:
    machineManager();
    ~machineManager();

    // Non-copyable
    machineManager(const machineManager&) = delete;
    machineManager& operator=(const machineManager&) = delete;

    void addMachine(std::unique_ptr<machine> machine);
    machine* getMachineAt(globalMachinePos pos);
    void removeMachineAt(globalMachinePos pos);

    void update();
    void render();

    // Pointer to the world to interact with tiles
    chunkManager* world = nullptr;

private:
    std::vector<std::unique_ptr<machine>> machines;
    std::map<globalMachinePos, machine*, CompareGlobalMachinePos> machineGrid;
};