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
    /// the state for the PPU
    PPU::_PPU* ppu;
    /// the state for the CPU
    CPUState* cpu_state;

    /// Initialize a new game-state
    GameState();
    /// Delete a game-state
    ~GameState();
    /// create a new game-state as a copy of another
    GameState(GameState* state);
    /// create a new game-state as a copy of another with different states
    GameState(GameState* state, CPUState* new_cpu_state);
    /// Load the game-state's data into the machine
    void load();
};
