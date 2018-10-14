#include "gamestate.hpp"

NES::NES(){
    cartridge = new Cartridge();
    joypad = new Joypad();
    gui = new GUI();
    ppu = new PPU();
    cpu = new CPU();
}

NES::NES(NES* nes) {
    cartridge = new Cartridge(nes->cartridge);
    joypad = new Joypad(nes->joypad);
    gui = new GUI(nes->gui);
    ppu = new PPU(nes->ppu);
    cpu = new CPU(nes->cpu);
}

NES::~NES() {
    delete cartridge;
    delete joypad;
    delete gui;
    delete ppu;
    delete cpu;
}

// void NES::load() {
//     // setup the CPU up
//     cpu->set_cartridge(cartridge);
//     cpu->set_joypad(joypad);
//     // set the PPU up
//     ppu->set_gui(gui);
//     ppu->set_cartridge(cartridge);
// }
