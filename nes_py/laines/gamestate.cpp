#include "gamestate.hpp"

GameState::GameState() {
    cpu_state = new CPUState();
    ppu = new PPU::_PPU();
    PPU::set_ppu(ppu);
};

GameState::~GameState() {
    delete cartridge;
    delete joypad;
    delete gui;
    delete ppu;
    delete cpu_state;
}

GameState::GameState(GameState* state) {
    cartridge = new Cartridge(state->cartridge);
    joypad = new Joypad(state->joypad);
    gui = new GUI(state->gui);
    ppu = new PPU::_PPU(state->ppu);
    cpu_state = new CPUState(state->cpu_state);
}

GameState::GameState(GameState* state, CPUState* cpu) {
    cartridge = new Cartridge(state->cartridge);
    joypad = new Joypad(state->joypad);
    gui = new GUI(state->gui);
    ppu = new PPU::_PPU(state->ppu);
    cpu_state = cpu;
}

void GameState::load() {
    // setup the CPU up
    CPU::set_state(cpu_state);
    CPU::set_cartridge(cartridge);
    CPU::set_joypad(joypad);
    // set the PPU up
    PPU::set_ppu(ppu);
    PPU::set_gui(gui);
    PPU::set_cartridge(cartridge);
}
