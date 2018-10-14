#include "cartridge.hpp"
#include "gui.hpp"
#include "joypad.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

/// an instance of NES hardware
class NES {
public:
    /// the current game cartridge
    Cartridge* cartridge;
    /// the joy-pad for the NES
    Joypad* joypad;
    /// the GUI for the NES
    GUI* gui;
    /// the PPU for the NES
    PPU* ppu;
    /// the CPU for the NES
    CPU* cpu;

    /// Initialize a new NES
    NES();
    /// create a new NES as a copy of another
    NES(NES* state);
    /// Delete an instance of NES
    ~NES();

};
