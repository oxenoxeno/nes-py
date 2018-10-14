#include "cpu.hpp"

CPU::CPU() {
    remainingCycles = 0;
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

void CPU::power() {
    INT<RESET>();
}

void CPU::tick() { nes->get_ppu()->step(); nes->get_ppu()->step(); nes->get_ppu()->step(); remainingCycles--; }

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
        return nes->get_ppu()->access<is_write>(addr % 8, v);
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
            return nes->get_joypad()->read_state(1);
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
            nes->get_joypad()->write_strobe(v & 1);
        // Joypad 0
        else
            return nes->get_joypad()->read_state(0);
    }
    // Cartridge
    else if (0x4018 <= addr && addr <= 0xFFFF) {
        return nes->get_cartridge()->access<is_write>(addr, v);
    }

    return 0;
}

void CPU::exec() {
    // Fetch the opcode and switch over it
    switch (rd(PC++)) {
        // Select the right function to emulate the instruction:
        case 0x00: return INT<BRK>()        ;  case 0x01: return ORA<&CPU::izx>()  ;
        case 0x05: return ORA<&CPU::zp>()   ;  case 0x06: return ASL<&CPU::zp>()   ;
        case 0x08: return PHP()             ;  case 0x09: return ORA<&CPU::imm>()  ;
        case 0x0A: return ASL_A()           ;  case 0x0D: return ORA<&CPU::abs>()  ;
        case 0x0E: return ASL<&CPU::abs>()  ;  case 0x10: return br<N,0>()         ;
        case 0x11: return ORA<&CPU::izy>()  ;  case 0x15: return ORA<&CPU::zpx>()  ;
        case 0x16: return ASL<&CPU::zpx>()  ;  case 0x18: return flag<C,0>()       ;
        case 0x19: return ORA<&CPU::aby>()  ;  case 0x1D: return ORA<&CPU::abx>()  ;
        case 0x1E: return ASL<&CPU::_abx>() ;  case 0x20: return JSR()             ;
        case 0x21: return AND<&CPU::izx>()  ;  case 0x24: return BIT<&CPU::zp>()   ;
        case 0x25: return AND<&CPU::zp>()   ;  case 0x26: return ROL<&CPU::zp>()   ;
        case 0x28: return PLP()             ;  case 0x29: return AND<&CPU::imm>()  ;
        case 0x2A: return ROL_A()           ;  case 0x2C: return BIT<&CPU::abs>()  ;
        case 0x2D: return AND<&CPU::abs>()  ;  case 0x2E: return ROL<&CPU::abs>()  ;
        case 0x30: return br<N,1>()         ;  case 0x31: return AND<&CPU::izy>()  ;
        case 0x35: return AND<&CPU::zpx>()  ;  case 0x36: return ROL<&CPU::zpx>()  ;
        case 0x38: return flag<C,1>()       ;  case 0x39: return AND<&CPU::aby>()  ;
        case 0x3D: return AND<&CPU::abx>()  ;  case 0x3E: return ROL<&CPU::_abx>() ;
        case 0x40: return RTI()             ;  case 0x41: return EOR<&CPU::izx>()  ;
        case 0x45: return EOR<&CPU::zp>()   ;  case 0x46: return LSR<&CPU::zp>()   ;
        case 0x48: return PHA()             ;  case 0x49: return EOR<&CPU::imm>()  ;
        case 0x4A: return LSR_A()           ;  case 0x4C: return JMP()             ;
        case 0x4D: return EOR<&CPU::abs>()  ;  case 0x4E: return LSR<&CPU::abs>()  ;
        case 0x50: return br<V,0>()         ;  case 0x51: return EOR<&CPU::izy>()  ;
        case 0x55: return EOR<&CPU::zpx>()  ;  case 0x56: return LSR<&CPU::zpx>()  ;
        case 0x58: return flag<I,0>()       ;  case 0x59: return EOR<&CPU::aby>()  ;
        case 0x5D: return EOR<&CPU::abx>()  ;  case 0x5E: return LSR<&CPU::_abx>() ;
        case 0x60: return RTS()             ;  case 0x61: return ADC<&CPU::izx>()  ;
        case 0x65: return ADC<&CPU::zp>()   ;  case 0x66: return ROR<&CPU::zp>()   ;
        case 0x68: return PLA()             ;  case 0x69: return ADC<&CPU::imm>()  ;
        case 0x6A: return ROR_A()           ;  case 0x6C: return JMP_IND()         ;
        case 0x6D: return ADC<&CPU::abs>()  ;  case 0x6E: return ROR<&CPU::abs>()  ;
        case 0x70: return br<V,1>()         ;  case 0x71: return ADC<&CPU::izy>()  ;
        case 0x75: return ADC<&CPU::zpx>()  ;  case 0x76: return ROR<&CPU::zpx>()  ;
        case 0x78: return flag<I,1>()       ;  case 0x79: return ADC<&CPU::aby>()  ;
        case 0x7D: return ADC<&CPU::abx>()  ;  case 0x7E: return ROR<&CPU::_abx>() ;
        case 0x81: return st<&CPU::izx>(A)  ;  case 0x84: return st<&CPU::zp>(Y)   ;
        case 0x85: return st<&CPU::zp>(A)   ;  case 0x86: return st<&CPU::zp>(X)   ;
        case 0x88: return dec(Y)            ;  case 0x8A: return tr(X, A)          ;
        case 0x8C: return st<&CPU::abs>(Y)  ;  case 0x8D: return st<&CPU::abs>(A)  ;
        case 0x8E: return st<&CPU::abs>(X)  ;  case 0x90: return br<C,0>()         ;
        case 0x91: return st<&CPU::izy>(A)  ;  case 0x94: return st<&CPU::zpx>(Y)  ;
        case 0x95: return st<&CPU::zpx>(A)  ;  case 0x96: return st<&CPU::zpy>(X)  ;
        case 0x98: return tr(Y, A)          ;  case 0x99: return st<&CPU::aby>(A)  ;
        case 0x9A: return tr(X, S)          ;  case 0x9D: return st<&CPU::abx>(A)  ;
        case 0xA0: return ld<&CPU::imm>(Y)  ;  case 0xA1: return ld<&CPU::izx>(A)  ;
        case 0xA2: return ld<&CPU::imm>(X)  ;  case 0xA4: return ld<&CPU::zp>(Y)   ;
        case 0xA5: return ld<&CPU::zp>(A)   ;  case 0xA6: return ld<&CPU::zp>(X)   ;
        case 0xA8: return tr(A, Y)          ;  case 0xA9: return ld<&CPU::imm>(A)  ;
        case 0xAA: return tr(A, X)          ;  case 0xAC: return ld<&CPU::abs>(Y)  ;
        case 0xAD: return ld<&CPU::abs>(A)  ;  case 0xAE: return ld<&CPU::abs>(X)  ;
        case 0xB0: return br<C,1>()         ;  case 0xB1: return ld<&CPU::izy>(A)  ;
        case 0xB4: return ld<&CPU::zpx>(Y)  ;  case 0xB5: return ld<&CPU::zpx>(A)  ;
        case 0xB6: return ld<&CPU::zpy>(X)  ;  case 0xB8: return flag<V,0>()       ;
        case 0xB9: return ld<&CPU::aby>(A)  ;  case 0xBA: return tr(S, X)          ;
        case 0xBC: return ld<&CPU::abx>(Y)  ;  case 0xBD: return ld<&CPU::abx>(A)  ;
        case 0xBE: return ld<&CPU::aby>(X)  ;  case 0xC0: return cmp<&CPU::imm>(Y) ;
        case 0xC1: return cmp<&CPU::izx>(A) ;  case 0xC4: return cmp<&CPU::zp>(Y)  ;
        case 0xC5: return cmp<&CPU::zp>(A)  ;  case 0xC6: return DEC<&CPU::zp>()   ;
        case 0xC8: return inc(Y)            ;  case 0xC9: return cmp<&CPU::imm>(A) ;
        case 0xCA: return dec(X)            ;  case 0xCC: return cmp<&CPU::abs>(Y) ;
        case 0xCD: return cmp<&CPU::abs>(A) ;  case 0xCE: return DEC<&CPU::abs>()  ;
        case 0xD0: return br<Z,0>()         ;  case 0xD1: return cmp<&CPU::izy>(A) ;
        case 0xD5: return cmp<&CPU::zpx>(A) ;  case 0xD6: return DEC<&CPU::zpx>()  ;
        case 0xD8: return flag<D,0>()       ;  case 0xD9: return cmp<&CPU::aby>(A) ;
        case 0xDD: return cmp<&CPU::abx>(A) ;  case 0xDE: return DEC<&CPU::_abx>() ;
        case 0xE0: return cmp<&CPU::imm>(X) ;  case 0xE1: return SBC<&CPU::izx>()  ;
        case 0xE4: return cmp<&CPU::zp>(X)  ;  case 0xE5: return SBC<&CPU::zp>()   ;
        case 0xE6: return INC<&CPU::zp>()   ;  case 0xE8: return inc(X)            ;
        case 0xE9: return SBC<&CPU::imm>()  ;  case 0xEA: return NOP()             ;
        case 0xEC: return cmp<&CPU::abs>(X) ;  case 0xED: return SBC<&CPU::abs>()  ;
        case 0xEE: return INC<&CPU::abs>()  ;  case 0xF0: return br<Z,1>()         ;
        case 0xF1: return SBC<&CPU::izy>()  ;  case 0xF5: return SBC<&CPU::zpx>()  ;
        case 0xF6: return INC<&CPU::zpx>()  ;  case 0xF8: return flag<D,1>()       ;
        case 0xF9: return SBC<&CPU::aby>()  ;  case 0xFD: return SBC<&CPU::abx>()  ;
        case 0xFE: return INC<&CPU::_abx>() ;
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
