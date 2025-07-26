#include "../include/gamestate.hpp"
#include <raylib.h>

int main() {
    InitWindow(GAMEWIDTH * GAMESCALE, GAMEHEIGHT * GAMESCALE, "Isometric Game");//we're running this in the ABSOLUTE beginning of the program, otherwise we get a coredump that is very annoying to deal with
    gameState gameState;

    while (!WindowShouldClose()) {

        gameState.update();
        gameState.render();

    }
    
}
