#include "gamestate.hpp"

GameState::GameState(GameState* state) {
    cartridge = new Cartridge(state->cartridge);
    joypad = new Joypad(state->joypad);
    gui = new GUI(state->gui);
    ppu = new PPU(state->ppu);
    cpu = new CPU(state->cpu);
}

GameState::~GameState() {
    delete cartridge;
    delete joypad;
    delete gui;
    delete ppu;
    delete cpu;
}

void GameState::load() {
    // setup the CPU up
    cpu->set_cartridge(cartridge);
    cpu->set_joypad(joypad);
    // set the PPU up
    ppu->set_gui(gui);
    ppu->set_cartridge(cartridge);
}
