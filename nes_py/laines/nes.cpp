#include "nes.hpp"

NES::NES(const char* file_name){
    joypad = new Joypad();
    gui = new GUI();
    cpu = new CPU();
    cpu->set_nes(this);
    ppu = new PPU();
    ppu->set_nes(this);
    cartridge = new Cartridge(file_name, this);
    cpu->power();
}

NES::NES(NES* nes) {
    // TODO: set the mapper's reference to NES
    cartridge = new Cartridge(nes->cartridge);
    joypad = new Joypad(nes->joypad);
    gui = new GUI(nes->gui);
    ppu = new PPU(nes->ppu);
    ppu->set_nes(this);
    cpu = new CPU(nes->cpu);
    cpu->set_nes(this);
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
