#include "cpu.hpp"

CPU::CPU() {
    P.set(0x04);
    A = X = Y = S = 0x00;
    memset(ram, 0xFF, sizeof(ram));
    nmi = irq = false;
}

CPU::CPU(CPU* cpu) {
    // copy the RAM array into this CPU state
    std::copy(std::begin(cpu->ram), std::end(cpu->ram), std::begin(ram));
    // copy the registers
    A = cpu->A;
    X = cpu->X;
    Y = cpu->Y;
    S = cpu->S;
    // copy the program counter
    PC = cpu->PC;
    // copy the flags
    P = cpu->P;
    // copy the interrupt flags
    nmi = cpu->nmi;
    irq = cpu->irq;
}

// TODO: remove this and just use initializer?
void CPU::power() {
    remainingCycles = 0;

    P.set(0x04);
    A = X = Y = S = 0x00;
    memset(ram, 0xFF, sizeof(ram));

    nmi = irq = false;
    INT<RESET>();
}

void CPU::run_frame() {
    remainingCycles += TOTAL_CYCLES;

    while (remainingCycles > 0) {
        if (nmi) INT<NMI>();
        else if (irq && !P[I]) INT<IRQ>();

        exec();
    }
}

template<bool is_write> u8 CPU::access(u16 addr, u8 v) {
    u8* r;
    // RAM
    if (0x0000 <= addr && addr <= 0x1FFF) {
        r = &ram[addr % 0x800];
        if (is_write)
            *r = v;
        return *r;
    }
    // PPU
    else if (0x2000 <= addr && addr <= 0x3FFF) {
        return ppu->access<is_write>(addr % 8, v);
    }
    // APU (not implemented, NOP instead)
    else if ((0x4000 <= addr && addr <= 0x4013) || addr == 0x4015) {
        return 1;
    }
    // Joypad 1
    else if (addr == 0x4017) {
        if (is_write)
            return 1;
        else
            return joypad->read_state(1);
    }
    // OAM / DMA
    else if (addr == 0x4014) {
        if (is_write)
            dma_oam(v);
    }
    // Joypad Strobe and Joypad 0
    else if (addr == 0x4016) {
        // Joypad strobe
        if (is_write)
            joypad->write_strobe(v & 1);
        // Joypad 0
        else
            return joypad->read_state(0);
    }
    // Cartridge
    else if (0x4018 <= addr && addr <= 0xFFFF) {
        return cartridge->access<is_write>(addr, v);
    }

    return 0;
}

template<Mode m> void CPU::st(u8& r) {
    if (r == A && m == izy) {
        T; wr(_izy()    , A);
    }
    if (r == A && m == abx) {
        T; wr( abs() + X, A);
    }
    if (r == A && m == aby) {
        T; wr( abs() + Y, A);
    }
    else {
        wr(   m()    , r);
    }
}

template<u8& s, u8& d> void CPU::tr()      { upd_nz(d = s); T; };
template<>             void CPU::tr<X,S>() { S = X;         T; };  // TSX, exception.

void CPU::exec() {
    // Fetch the opcode and switch over it
    switch (rd(PC++)) {
        // Select the right function to emulate the instruction:
        case 0x00: return INT<BRK>()  ;  case 0x01: return ORA<izx>()  ;
        case 0x05: return ORA<zp>()   ;  case 0x06: return ASL<zp>()   ;
        case 0x08: return PHP()       ;  case 0x09: return ORA<imm>()  ;
        case 0x0A: return ASL_A()     ;  case 0x0D: return ORA<abs>()  ;
        case 0x0E: return ASL<abs>()  ;  case 0x10: return br<N,0>()   ;
        case 0x11: return ORA<izy>()  ;  case 0x15: return ORA<zpx>()  ;
        case 0x16: return ASL<zpx>()  ;  case 0x18: return flag<C,0>() ;
        case 0x19: return ORA<aby>()  ;  case 0x1D: return ORA<abx>()  ;
        case 0x1E: return ASL<_abx>() ;  case 0x20: return JSR()       ;
        case 0x21: return AND<izx>()  ;  case 0x24: return BIT<zp>()   ;
        case 0x25: return AND<zp>()   ;  case 0x26: return ROL<zp>()   ;
        case 0x28: return PLP()       ;  case 0x29: return AND<imm>()  ;
        case 0x2A: return ROL_A()     ;  case 0x2C: return BIT<abs>()  ;
        case 0x2D: return AND<abs>()  ;  case 0x2E: return ROL<abs>()  ;
        case 0x30: return br<N,1>()   ;  case 0x31: return AND<izy>()  ;
        case 0x35: return AND<zpx>()  ;  case 0x36: return ROL<zpx>()  ;
        case 0x38: return flag<C,1>() ;  case 0x39: return AND<aby>()  ;
        case 0x3D: return AND<abx>()  ;  case 0x3E: return ROL<_abx>() ;
        case 0x40: return RTI()       ;  case 0x41: return EOR<izx>()  ;
        case 0x45: return EOR<zp>()   ;  case 0x46: return LSR<zp>()   ;
        case 0x48: return PHA()       ;  case 0x49: return EOR<imm>()  ;
        case 0x4A: return LSR_A()     ;  case 0x4C: return JMP()       ;
        case 0x4D: return EOR<abs>()  ;  case 0x4E: return LSR<abs>()  ;
        case 0x50: return br<V,0>()   ;  case 0x51: return EOR<izy>()  ;
        case 0x55: return EOR<zpx>()  ;  case 0x56: return LSR<zpx>()  ;
        case 0x58: return flag<I,0>() ;  case 0x59: return EOR<aby>()  ;
        case 0x5D: return EOR<abx>()  ;  case 0x5E: return LSR<_abx>() ;
        case 0x60: return RTS()       ;  case 0x61: return ADC<izx>()  ;
        case 0x65: return ADC<zp>()   ;  case 0x66: return ROR<zp>()   ;
        case 0x68: return PLA()       ;  case 0x69: return ADC<imm>()  ;
        case 0x6A: return ROR_A()     ;  case 0x6C: return JMP_IND()   ;
        case 0x6D: return ADC<abs>()  ;  case 0x6E: return ROR<abs>()  ;
        case 0x70: return br<V,1>()   ;  case 0x71: return ADC<izy>()  ;
        case 0x75: return ADC<zpx>()  ;  case 0x76: return ROR<zpx>()  ;
        case 0x78: return flag<I,1>() ;  case 0x79: return ADC<aby>()  ;
        case 0x7D: return ADC<abx>()  ;  case 0x7E: return ROR<_abx>() ;
        case 0x81: return st<izx>(A) ;  case 0x84: return st<zp>(Y)  ;
        case 0x85: return st<zp>(A)  ;  case 0x86: return st<zp>(X)  ;
        case 0x88: return dec<Y>()    ;  case 0x8A: return tr<X,A>()   ;
        case 0x8C: return st<abs>(Y) ;  case 0x8D: return st<abs>(A) ;
        case 0x8E: return st<abs>(X) ;  case 0x90: return br<C,0>()   ;
        case 0x91: return st<izy>(A) ;  case 0x94: return st<zpx>(Y) ;
        case 0x95: return st<zpx>(A) ;  case 0x96: return st<zpy>(X) ;
        case 0x98: return tr<Y,A>()   ;  case 0x99: return st<aby>(A) ;
        case 0x9A: return tr<X,S>()   ;  case 0x9D: return st<abx>(A) ;
        case 0xA0: return ld<Y,imm>() ;  case 0xA1: return ld<A,izx>() ;
        case 0xA2: return ld<X,imm>() ;  case 0xA4: return ld<Y,zp>()  ;
        case 0xA5: return ld<A,zp>()  ;  case 0xA6: return ld<X,zp>()  ;
        case 0xA8: return tr<A,Y>()   ;  case 0xA9: return ld<A,imm>() ;
        case 0xAA: return tr<A,X>()   ;  case 0xAC: return ld<Y,abs>() ;
        case 0xAD: return ld<A,abs>() ;  case 0xAE: return ld<X,abs>() ;
        case 0xB0: return br<C,1>()   ;  case 0xB1: return ld<A,izy>() ;
        case 0xB4: return ld<Y,zpx>() ;  case 0xB5: return ld<A,zpx>() ;
        case 0xB6: return ld<X,zpy>() ;  case 0xB8: return flag<V,0>() ;
        case 0xB9: return ld<A,aby>() ;  case 0xBA: return tr<S,X>()   ;
        case 0xBC: return ld<Y,abx>() ;  case 0xBD: return ld<A,abx>() ;
        case 0xBE: return ld<X,aby>() ;  case 0xC0: return cmp<Y,imm>();
        case 0xC1: return cmp<A,izx>();  case 0xC4: return cmp<Y,zp>() ;
        case 0xC5: return cmp<A,zp>() ;  case 0xC6: return DEC<zp>()   ;
        case 0xC8: return inc<Y>()    ;  case 0xC9: return cmp<A,imm>();
        case 0xCA: return dec<X>()    ;  case 0xCC: return cmp<Y,abs>();
        case 0xCD: return cmp<A,abs>();  case 0xCE: return DEC<abs>()  ;
        case 0xD0: return br<Z,0>()   ;  case 0xD1: return cmp<A,izy>();
        case 0xD5: return cmp<A,zpx>();  case 0xD6: return DEC<zpx>()  ;
        case 0xD8: return flag<D,0>() ;  case 0xD9: return cmp<A,aby>();
        case 0xDD: return cmp<A,abx>();  case 0xDE: return DEC<_abx>() ;
        case 0xE0: return cmp<X,imm>();  case 0xE1: return SBC<izx>()  ;
        case 0xE4: return cmp<X,zp>() ;  case 0xE5: return SBC<zp>()   ;
        case 0xE6: return INC<zp>()   ;  case 0xE8: return inc<X>()    ;
        case 0xE9: return SBC<imm>()  ;  case 0xEA: return NOP()       ;
        case 0xEC: return cmp<X,abs>();  case 0xED: return SBC<abs>()  ;
        case 0xEE: return INC<abs>()  ;  case 0xF0: return br<Z,1>()   ;
        case 0xF1: return SBC<izy>()  ;  case 0xF5: return SBC<zpx>()  ;
        case 0xF6: return INC<zpx>()  ;  case 0xF8: return flag<D,1>() ;
        case 0xF9: return SBC<aby>()  ;  case 0xFD: return SBC<abx>()  ;
        case 0xFE: return INC<_abx>() ;
        default: {
            std::cout <<
                "Invalid OPcode! PC: " <<
                PC <<
                " OPcode: 0x" <<
                std::hex <<
                (int)rd(PC-1) <<
                std::endl;
            return NOP();
        }
    }
}
