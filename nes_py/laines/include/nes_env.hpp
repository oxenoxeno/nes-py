#pragma once
#include <string>
#include "nes.hpp"

/// An abstraction of an NES environment for OpenAI Gym
class NESEnv {
private:
    /// the current NES being emulated
    NES* current;
    /// the backup NES to restore to
    NES* backup;

public:

    /**
        Initialize a new NESEnv.

        @param path the path to the ROM for the emulator to load
        @returns a new instance of NESEnv for a given ROM
    */
    NESEnv(wchar_t* path);

    /// Reset the emulator to its initial state.
    void reset() { current->power(); };

    /**
        Perform a discrete "step" of the NES by rendering 1 frame.

        @param action the controller bitmap of which buttons to press.
        The parameter uses 1 for "pressed" and 0 for "not pressed".
        It uses the following mapping of bits to buttons:
        7: RIGHT
        6: LEFT
        5: DOWN
        4: UP
        3: START
        2: SELECT
        1: B
        0: A
    */
    void step(unsigned char action);

    /// Backup the game state to the backup NES.
    void backup();

    /// Restore the game state from the backup NES.
    void restore();

};
