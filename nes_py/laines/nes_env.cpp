#include "nes_env.hpp"

NESEnv::NESEnv(wchar_t* path) {
    // convert the wchar_t type to a string
    std::wstring ws_rom_path(path);
    this->rom_path = std::string(ws_rom_path.begin(), ws_rom_path.end());
    // set the backup state to NULL
    backup_nes = nullptr;
    current_nes = nullptr;
}

void NESEnv::reset() {
    // delete the old NES if there was one
    delete current_nes;
    // setup the NES emulator for the game
    current_nes = new NES(this->rom_path.c_str());
}

void NESEnv::step(unsigned char action) {
    // write the action to the player's joy-pad
    current_nes->get_joypad()->write_buttons(0, action);
    // run a frame on the CPU
    current_nes->get_cpu()->run_frame();
}

void NESEnv::backup() {
    // delete any current backup
    delete backup_nes;
    // copy the current state as the backup state
    backup_nes = new NES(current_nes);
}

void NESEnv::restore() {
    // delete the current state in progress
    delete current_nes;
    // copy the backup state into the current state
    current_nes = new NES(backup_nes);
}
