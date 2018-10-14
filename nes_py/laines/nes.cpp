#include "nes.hpp"

NES::NES(const char* file_name){
    joypad = new Joypad();
    gui = new GUI();
    ppu = new PPU();
    cpu = new CPU();
    cartridge = new Cartridge(file_name, this);
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

/// Run a frame on the machine
void NES::run_frame() { cpu->run_frame(); };
