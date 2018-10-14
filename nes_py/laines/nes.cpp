#include "gamestate.hpp"

NES::NES(const char* file_name){
    cartridge = new Cartridge(const char* file_name);
    joypad = new Joypad();
    gui = new GUI();
    ppu = new PPU();
    cpu = new CPU();
    // setup the CPU up
    cpu->set_cartridge(cartridge);
    cpu->set_joypad(joypad);
    // set the PPU up
    ppu->set_gui(gui);
    ppu->set_cartridge(cartridge);
}

NES::NES(NES* nes) {
    cartridge = new Cartridge(nes->cartridge);
    joypad = new Joypad(nes->joypad);
    gui = new GUI(nes->gui);
    ppu = new PPU(nes->ppu);
    cpu = new CPU(nes->cpu);
    // setup the CPU up
    cpu->set_cartridge(cartridge);
    cpu->set_joypad(joypad);
    // set the PPU up
    ppu->set_gui(gui);
    ppu->set_cartridge(cartridge);
}

NES::~NES() {
    delete cartridge;
    delete joypad;
    delete gui;
    delete ppu;
    delete cpu;
}
