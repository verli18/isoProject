#pragma once
#include <cstdint>
#include <memory>
#include <initializer_list>
#include <raylib.h>
#include <vector>
#include <optional>
#include "inventory.hpp"

// Forward-declare the manager to break circular dependency
class machineManager;

enum machineType {
    CONVEYORMK1,
    DRILLMK1,
    ITEM
};

enum direction {
    NORTH,
    EAST,
    SOUTH,
    WEST
};

enum animFrames {
    IDLE_START,
    IDLE_END,
    WORKING_START,
    WORKING_END
};

enum state{
    IDLE,
    WORKING
};

struct machineTileOffset { // this is for defining slots occupied by a machine
    int x;
    int y;
    
    // Transform coordinates based on machine direction
    machineTileOffset getRotatedOffset(direction dir) const {
        switch (dir) {
            case NORTH: return {x, y};           // 0° rotation
            case EAST:  return {-y, x};          // 90° clockwise
            case SOUTH: return {-x, -y};         // 180° rotation
            case WEST:  return {y, -x};          // 270° clockwise
            default:    return {x, y};
        }
    }
};

struct globalMachinePos {
    int x;
    int y;
};

// Comparator for using globalMachinePos in std::map
struct CompareGlobalMachinePos {
    bool operator()(const globalMachinePos& a, const globalMachinePos& b) const {
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        return a.y < b.y;
    }
};

class machine {
    public:
        machine(machineType type, Vector3 position);
        machine(machineType type, Vector3 position, std::initializer_list<machineTileOffset> offsets);
        virtual ~machine();
    
        machineType type;
        state currentState;
        std::vector<machineTileOffset> tileOffsets;

        uint16_t animFrame; //current animation frame

        int ID;
        Vector3 position; //for renndering
        globalMachinePos globalPos; //for machine interactions
        direction dir = NORTH;

    // Update method now takes the manager to allow for interactions
    virtual void update(machineManager& manager) = 0;
    virtual void render() = 0;

    // Inventory system access
    virtual Inventory* getInventory() { return nullptr; }

    // Item giving method
    virtual bool giveItem(item anItem, machineManager& manager) { return false; }
    
    // Get the global position of a slot based on machine direction
    globalMachinePos getSlotGlobalPosition(const machineTileOffset& slotOffset) const;
    
    // Render slots for debugging
    void renderSlots();

    protected:
        void updateAnimation();
    
    private:
        static int nextID; 
};

class drillMk1 : public machine {
    public:
        drillMk1(Vector3 position);
        ~drillMk1();
    
    void update(machineManager& manager) override;
    void render() override;
    Inventory* getInventory() override { return &inventory; }
    
    private:
        float productionProgress = 0.0f;
        Inventory inventory;
};

class conveyorMk1 : public machine {
    public:
        conveyorMk1(Vector3 position);
        void update(machineManager& manager) override;
        void render() override;

        Inventory* getInventory() override { return &inventory; }
        bool giveItem(item anItem, machineManager& manager) override;
    private:
        Inventory inventory;
        item heldItem;
        bool hasHeldItem = false;
        float processingProgress = 0.0f;
        const float PROCESSING_TIME = 0.5f; // 0.5 seconds processing time
};

class droppedItem : public machine {
    public:
        droppedItem(Vector3 position, itemType type);
        ~droppedItem();

        item itemInstance;
    void update(machineManager& manager) override;
    void render() override;
    private:
};
