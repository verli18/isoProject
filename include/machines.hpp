#include <cstdint>
#include <memory>
#include <raylib.h>
#include <vector>

enum machineType {
    CONVEYORMK1,
    DRILLMK1
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

class machine {
    public:
        machine(machineType type, Vector3 position);
        virtual ~machine();
    
        machineType type;
        state currentState;

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

class machineManager {
    public:
        machineManager();
        ~machineManager();
    
    void update();
    void render();

    void addMachine(std::unique_ptr<machine> machine);
    void removeMachine(int ID);

    private:
    std::vector<std::unique_ptr<machine>> machines;
};
    