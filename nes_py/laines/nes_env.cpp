#include "nes_env.hpp"

NESEnv::NESEnv(wchar_t* path) {
    // convert the wchar_t type to a string
    std::wstring ws_rom_path(path);
    std::string rom_path(ws_rom_path.begin(), ws_rom_path.end());
    // setup the NES emulator for the game
    current = new NES(rom_path.c_str());
    // set the backup state to NULL
    backup_state = nullptr;
}

void NESEnv::step(unsigned char action) {
    // write the action to the player's joy-pad
    current->get_joypad()->write_buttons(0, action);
    // run a frame on the CPU
    current->run_frame();
}

void NESEnv::backup() {
    // delete any current backup
    delete backup_state;
    // copy the current state as the backup state
    backup_state = new NES(current);
}

void NESEnv::restore() {
    // delete the current state in progress
    delete current;
    // copy the backup state into the current state
    current = new NES(backup);
}
