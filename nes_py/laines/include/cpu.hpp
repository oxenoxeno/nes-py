#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "common.hpp"
#include "cpu_flags.hpp"
class NES;
#include "nes.hpp"

class CPU;
/// the addressing mode as a callable function
typedef u16 (CPU::*Mode)(void);

/// The CPU (MOS6502) for the NES
class CPU {
private:

    // Interrupt type
    enum IntType { NMI, RESET, IRQ, BRK };

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

    /// the NES this CPU belongs to
    NES* nes;

    /**
        The total number of CPU cycle per emulated frame.
        Original value is 29781. New value over-clocks the CPU (500000 is fast)
    */
    const int TOTAL_CYCLES = 29781;
    /// Remaining clocks to end frame
    int remainingCycles;

    /**
        Set the non-maskable interrupt flag.

        @param v the value to set the flag to
    */
    void set_nmi(bool v = true) { nmi = v; };

    /**
        Set the interrupt request flag.

        @param v the value to set the flag to
    */
    void set_irq(bool v = true) { irq = v; };

    /// Perform a single CPU cycle, i.e., one clock tick
    void tick();

    /* Flags updating */
    void upd_cv(u8 x, u8 y, s16 r) { P[C] = (r>0xFF); P[V] = ~(x^y) & (x^r) & 0x80; };
    void upd_nz(u8 x)              { P[N] = x & 0x80; P[Z] = (x == 0);              };
    // Does adding I to A cross a page?
    bool cross(u16 a, u8 i) { return ((a+i) & 0xFF00) != ((a & 0xFF00)); };

    /* MARK: Memory access */

    void dma_oam(u8 bank) { for (int i = 0; i < 256; i++)  wr(0x2014, rd(bank*0x100 + i)); };

    template<bool is_write> u8 access(u16 addr, u8 v = 0);

    u8  wr(u16 a, u8 v)      { tick(); return access<1>(a, v);   };
    u8  rd(u16 a)            { tick(); return access<0>(a);      }
    u16 rd16_d(u16 a, u16 b) { return rd(a) | (rd(b) << 8); };
    u16 rd16(u16 a)          { return rd16_d(a, a+1);       };
    u8  push(u8 v)           { return wr(0x100 + (S--), v); };
    u8  pop()                { return rd(0x100 + (++S));    };

    /* MARK: Addressing modes */

    u16 imm()   { return PC++;                                       };
    u16 imm16() { PC += 2; return PC - 2;                            };
    u16 abs()   { return rd16(imm16());                              };
    u16 _abx()  { tick(); return abs() + X;                               };  // Exception.
    u16 abx()   { u16 a = abs(); if (cross(a, X)) tick(); return a + X;   };
    u16 aby()   { u16 a = abs(); if (cross(a, Y)) tick(); return a + Y;   };
    u16 zp()    { return rd(imm());                                  };
    u16 zpx()   { tick(); return (zp() + X) % 0x100;                      };
    u16 zpy()   { tick(); return (zp() + Y) % 0x100;                      };
    u16 izx()   { u8 i = zpx(); return rd16_d(i, (i+1) % 0x100);     };
    u16 _izy()  { u8 i = zp();  return rd16_d(i, (i+1) % 0x100) + Y; };  // Exception.
    u16 izy()   { u16 a = _izy(); if (cross(a-Y, Y)) tick(); return a;    };

    /* MARK: STx */

    template<Mode m> void st(u8& r);
    template<Mode m> void ld(u8& r)  { u16 a = (this->*m)(); u8 p = rd(a); upd_nz(r = p);                  };  // LDx
    template<Mode m> void cmp(u8& r) { u16 a = (this->*m)(); u8 p = rd(a); upd_nz(r - p); P[C] = (r >= p); };  // CMP, CPx

    /* MARK: Arithmetic and bitwise */

    template<Mode m> void ADC() { u16 a = (this->*m)(); u8 p = rd(a); s16 r = A + p + P[C]; upd_cv(A, p, r); upd_nz(A = r); };
    template<Mode m> void SBC() { u16 a = (this->*m)(); u8 p = rd(a) ^ 0xFF; s16 r = A + p + P[C]; upd_cv(A, p, r); upd_nz(A = r); };
    template<Mode m> void BIT() { u16 a = (this->*m)(); u8 p = rd(a); P[Z] = !(A & p); P[N] = p & 0x80; P[V] = p & 0x40; };
    template<Mode m> void AND() { u16 a = (this->*m)(); u8 p = rd(a); upd_nz(A &= p); };
    template<Mode m> void EOR() { u16 a = (this->*m)(); u8 p = rd(a); upd_nz(A ^= p); };
    template<Mode m> void ORA() { u16 a = (this->*m)(); u8 p = rd(a); upd_nz(A |= p); };

    /* MARK: Read-Modify-Write */

    template<Mode m> void ASL() { u16 a = (this->*m)(); u8 p = rd(a); P[C] = p & 0x80; tick(); upd_nz(wr(a, p << 1)); };
    template<Mode m> void LSR() { u16 a = (this->*m)(); u8 p = rd(a); P[C] = p & 0x01; tick(); upd_nz(wr(a, p >> 1)); };
    template<Mode m> void ROL() { u16 a = (this->*m)(); u8 p = rd(a); u8 c = P[C]     ; P[C] = p & 0x80; tick(); upd_nz(wr(a, (p << 1) | c) ); };
    template<Mode m> void ROR() { u16 a = (this->*m)(); u8 p = rd(a); u8 c = P[C] << 7; P[C] = p & 0x01; tick(); upd_nz(wr(a, c | (p >> 1)) ); };
    template<Mode m> void DEC() { u16 a = (this->*m)(); u8 p = rd(a); tick(); upd_nz(wr(a, --p)); };
    template<Mode m> void INC() { u16 a = (this->*m)(); u8 p = rd(a); tick(); upd_nz(wr(a, ++p)); };

    /* MARK: DEx, INx */

    void dec(u8& r) { upd_nz(--r); tick(); };
    void inc(u8& r) { upd_nz(++r); tick(); };

    /* MARK: Bit shifting on the accumulator */

    void ASL_A() { P[C] = A & 0x80; upd_nz(A <<= 1); tick(); };
    void LSR_A() { P[C] = A & 0x01; upd_nz(A >>= 1); tick(); };
    void ROL_A() { u8 c = P[C]     ; P[C] = A & 0x80; upd_nz(A = ((A << 1) | c) ); tick(); };
    void ROR_A() { u8 c = P[C] << 7; P[C] = A & 0x01; upd_nz(A = (c | (A >> 1)) ); tick(); };

    /* MARK: Txx (move values between registers) */

    void tr(u8& s, u8& d);

    /* MARK: Stack operations */

    void PLP() { tick(); tick(); P.set(pop()); };
    void PHP() { tick(); push(P.get() | (1 << 4)); };  // B flag set.
    void PLA() { tick(); tick(); A = pop(); upd_nz(A);  };
    void PHA() { tick(); push(A); };

    /* MARK: Flow control (branches, jumps) */

    template<Flag f, bool v> void br() { s8 j = rd(imm()); if (P[f] == v) { tick(); PC += j; } };
    void JMP_IND() { u16 i = rd16(imm16()); PC = rd16_d(i, (i&0xFF00) | ((i+1) % 0x100)); };
    void JMP()     { PC = rd16(imm16()); };
    void JSR()     { u16 t = PC+1; tick(); push(t >> 8); push(t); PC = rd16(imm16()); };

    /* MARK: Return instructions */

    void RTS() { tick(); tick();  PC = (pop() | (pop() << 8)) + 1; tick(); };
    void RTI() { PLP(); PC =  pop() | (pop() << 8);         };

    /* MARK: Interrupts, Flags */

    template<Flag f, bool v> void flag() { P[f] = v; tick(); };  // Clear and set flags.
    template<IntType t> void INT() {
        // BRK already performed the fetch.
        tick(); if (t != BRK) tick();
        // Writes on stack are inhibited on RESET.
        if (t != RESET) {
            push(PC >> 8); push(PC & 0xFF);
            push(P.get() | ((t == BRK) << 4));  // Set B if BRK.
        }
        else { S -= 3; tick(); tick(); tick(); }
        P[I] = true;
                              /*   NMI    Reset    IRQ     BRK  */
        constexpr u16 vect[] = { 0xFFFA, 0xFFFC, 0xFFFE, 0xFFFE };
        PC = rd16(vect[t]);
        if (t == NMI) nmi = false;
    };
    void NOP() { tick(); };

public:
    /// Initialize a new CPU State
    CPU();

    /// Initialize a new CPU State as a copy of another
    CPU(CPU* cpu);

    /// Set the NES for this PPU
    void set_nes(NES* new_nes) { nes = new_nes; };

    /// Get the NES for this PPU
    NES* get_nes() { return nes; };

    /**
        Return the value of the given memory address.
        This is meant as a public getter to the memory of the machine for RAM hacks.

        @param address the 16-bit address to read from memory
        @returns the byte located at the given address

    */
    u8 read_mem(u16 address) { return ram[address % 0x800]; };

    /**
        Return the value of the given memory address.
        This is meant as a public getter to the memory of the machine for RAM hacks.

        @param address the 16-bit address to read from memory
        @param value the 8-bit value to write to the given memory address

    */
    void write_mem(u16 address, u8 value) { ram[address % 0x800] = value; };

    /// Execute a CPU instruction.
    void exec();

    /// Turn on the CPU
    void power();

    /// Run the CPU for roughly a frame
    void run_frame();

};
