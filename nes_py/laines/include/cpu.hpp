#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "common.hpp"
#include "flags.hpp"
#include "joypad.hpp"
#include "cartridge.hpp"
class PPU;
#include "ppu.hpp"

/// A structure to contain all local variables of a CPU for state backup
struct CPUState {
    /// the CPU RAM
    u8 ram[0x800];
    /// the CPU registers
    u8 A, X, Y, S;
    /// the program counter
    u16 PC;
    /// the CPU flags register
    Flags P;
    /// non-mask-able interrupt and interrupt request flag
    bool nmi, irq;

    /// Initialize a new CPU State
    CPUState() {
        P.set(0x04);
        A = X = Y = S = 0x00;
        memset(ram, 0xFF, sizeof(ram));
        nmi = irq = false;
    }

    /// Initialize a new CPU State as a copy of another
    CPUState(CPUState* state) {
        // copy the RAM array into this CPU state
        std::copy(std::begin(state->ram), std::end(state->ram), std::begin(ram));
        // copy the registers
        A = state->A;
        X = state->X;
        Y = state->Y;
        S = state->S;
        // copy the program counter
        PC = state->PC;
        // copy the flags
        P = state->P;
        // copy the interrupt flags
        nmi = state->nmi;
        irq = state->irq;
    }
};

/// The CPU (MOS6502) for the NES
namespace CPU {

    // Interrupt type
    enum IntType { NMI, RESET, IRQ, BRK };
    // Addressing mode
    typedef u16 (*Mode)(void);

    /**
        Set the local PPU to a new value

        @param new_ppu the PPU pointer to replace the existing pointer
    */
    void set_ppu(PPU* new_ppu);

    /**
        Return a pointer to this CPU's PPU object.

        @returns a pointer to the CPU's PPU
    */
    PPU* get_ppu();

    /**
        Set the local joy-pad to a new value

        @param new_joypad the joy-pad pointer to replace the existing pointer
    */
    void set_joypad(Joypad* new_joypad);

    /**
        Return a pointer to this CPU's joy-pad object.

        @returns a pointer to the CPU's joy-pad
    */
    Joypad* get_joypad();

    /// Set the Cartridge instance pointer to a new value.
    void set_cartridge(Cartridge* new_cartridge);

    /// Return the pointer to this PPU's Cartridge instance
    Cartridge* get_cartridge();

    /**
        Return the value of the given memory address.
        This is meant as a public getter to the memory of the machine for RAM hacks.

        @param address the 16-bit address to read from memory
        @returns the byte located at the given address

    */
    u8 read_mem(u16 address);

    /**
        Return the value of the given memory address.
        This is meant as a public getter to the memory of the machine for RAM hacks.

        @param address the 16-bit address to read from memory
        @param value the 8-bit value to write to the given memory address

    */
    void write_mem(u16 address, u8 value);

    /**
        Set the non-maskable interrupt flag.

        @param v the value to set the flag to
    */
    void set_nmi(bool v = true);

    /**
        Set the interrupt request flag.

        @param v the value to set the flag to
    */
    void set_irq(bool v = true);

    /// Turn on the CPU
    void power();

    /// Run the CPU for roughly a frame
    void run_frame();

    /// Return a new CPU state of the CPU variables
    CPUState* get_state();

    /// Restore the CPU variables from a saved state
    void set_state(CPUState* state);
}
