#include "cartridge.hpp"
#include "gui.hpp"
#include "joypad.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

/// a save state of the NES machine
class GameState {
public:
    /// the current game cartridge
    Cartridge* cartridge;
    /// the joy-pad for the game-state
    Joypad* joypad;
    /// the GUI for the game-state
    GUI* gui;
    /// the PPU for the game-state
    PPU* ppu;
    /// the CPU for the game-state
    CPU* cpu;

    /// Initialize a new game-state
    GameState() { };
    /// create a new game-state as a copy of another
    GameState(GameState* state);
    /// Delete a game-state
    ~GameState();
    /// Load the game-state's data into the machine
    void load();
};
