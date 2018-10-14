#pragma once
class Cartridge;
#include "cartridge.hpp"
#include "gui.hpp"
#include "joypad.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

/// an instance of NES hardware
class NES {
private:
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

public:

    /// Initialize a new NES
    NES() { };

    /// Initialize a new NES with a cartridge
    NES(const char* file_name);

    /// create a new NES as a copy of another
    NES(NES* state);

    /// Delete an instance of NES
    ~NES();

    /// Return the game cartridge in the NES
    Cartridge* get_cartridge() { return cartridge; };

    // Set the Cartridge for the NES
    void set_cartridge(Cartridge* new_cartridge) { cartridge = new_cartridge; };

    /// Return the joy-pad for the NES
    Joypad* get_joypad() { return joypad; };

    // Set the Joy-pad for the NES
    void set_joypad(Joypad* new_joypad) { joypad = new_joypad; };

    /// Return the GUI for the NES
    GUI* get_gui() { return gui; };

    /// Set the GUI for the NES
    void set_gui(GUI* new_gui) { gui = new_gui; };

    /// Return the PPU for the NES
    PPU* get_ppu() { return ppu; };

    // Set the PPU for the NES
    void set_ppu(PPU* new_ppu) { ppu = new_ppu; };

    /// Return the CPU for the NES
    CPU* get_cpu() { return cpu; };

    /// Set the CPU for the NES
    void set_cpu(CPU* new_cpu) { cpu = new_cpu; };

    /// Reset the machine
    void power() { cpu->power(); ppu->reset(); };

    /// Run a frame on the machine
    void run_frame() { cpu->run_frame(); };

};
