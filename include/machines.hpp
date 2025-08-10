#include <cstdint>
#include <memory>
#include <initializer_list>
#include <raylib.h>
#include <vector>
#include "item.hpp"

enum machineType {
    CONVEYORMK1,
    DRILLMK1,
    ITEM
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
        Vector3 position;
        
    virtual void update() = 0;
    virtual void render() = 0;

    protected:
        void updateAnimation();
    
    private:
        static int nextID; 
};

class drillMk1 : public machine {
    public:
        drillMk1(Vector3 position);
        ~drillMk1();
    
    void update() override;
    void render() override;
    
    private:
};

class conveyorMk1 : public machine {
    public:
        conveyorMk1(Vector3 position);
        void update() override;
        void render() override;
    private:
};

class droppedItem : public machine {
    public:
        droppedItem(Vector3 position, itemType type);
        ~droppedItem();

        item itemInstance;
    void update() override;
    void render() override;
    private:
};

class machineManager {
    public:
        machineManager();
        ~machineManager();

        // Non-copyable
        machineManager(const machineManager&) = delete;
        machineManager& operator=(const machineManager&) = delete;
    
    void update();
    void render();

    void addMachine(std::unique_ptr<machine> machine);
    void removeMachine(int ID);

    machine* previous;
    private:
    std::vector<std::unique_ptr<machine>> machines;
};
    