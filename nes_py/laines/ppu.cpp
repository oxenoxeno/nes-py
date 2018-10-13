#include <cstring>
#include "cpu.hpp"
#include "ppu.hpp"

PPU::PPU() {
    frameOdd = false;
    scanline = dot = 0;
    ctrl.r = mask.r = status.r = 0;
    memset(pixels, 0x00, sizeof(pixels));
    memset(ciRam,  0xFF, sizeof(ciRam));
    memset(oamMem, 0x00, sizeof(oamMem));
}

PPU::PPU(PPU* ppu) {
    mirroring = ppu->mirroring;
    std::copy(std::begin(ppu->ciRam), std::end(ppu->ciRam), std::begin(ciRam));
    std::copy(std::begin(ppu->cgRam), std::end(ppu->cgRam), std::begin(cgRam));
    std::copy(std::begin(ppu->oamMem), std::end(ppu->oamMem), std::begin(oamMem));
    std::copy(std::begin(ppu->oam), std::end(ppu->oam), std::begin(oam));
    std::copy(std::begin(ppu->secOam), std::end(ppu->secOam), std::begin(secOam));
    std::copy(std::begin(ppu->pixels), std::end(ppu->pixels), std::begin(pixels));
    vAddr = ppu->vAddr;
    tAddr = ppu->tAddr;
    fX = ppu->fX;
    oamAddr = ppu->oamAddr;
    ctrl = ppu->ctrl;
    mask = ppu->mask;
    status = ppu->status;
    nt = ppu->nt;
    at = ppu->at;
    bgL = ppu->bgL;
    bgH = ppu->bgH;
    atShiftL = ppu->atShiftL;
    atShiftH = ppu->atShiftH;
    bgShiftL = ppu->bgShiftL;
    bgShiftH = ppu->bgShiftH;
    atLatchL = ppu->atLatchL;
    atLatchH = ppu->atLatchH;
    scanline = ppu->scanline;
    dot = ppu->dot;
    frameOdd = ppu->frameOdd;
}

u16 PPU::nt_mirror(u16 addr) {
    switch (mirroring) {
        case VERTICAL:    return addr % 0x800;
        case HORIZONTAL:  return ((addr / 2) & 0x400) + (addr % 0x400);
        default:          return addr - 0x2000;
    }
}

u8 PPU::rd(u16 addr) {
    // CHR-ROM/RAM
    if (0x0000 <= addr && addr <= 0x1FFF) {
        return cartridge->chr_access<0>(addr);
    }
    // Nametables
    else if (0x2000 <= addr && addr <= 0x3EFF) {
        return ciRam[nt_mirror(addr)];
    }
    // Palettes
    else if (0x3F00 <= addr && addr <= 0x3FFF) {
        if ((addr & 0x13) == 0x10)
            addr &= ~0x10;
        return cgRam[addr & 0x1F] & (mask.gray ? 0x30 : 0xFF);
    }

    return 0;
}

void PPU::wr(u16 addr, u8 v) {
    // CHR-ROM/RAM
    if (0x0000 <= addr && addr <= 0x1FFF) {
        cartridge->chr_access<1>(addr, v);
    }
    // Nametables
    else if (0x2000 <= addr && addr <= 0x3EFF) {
        ciRam[nt_mirror(addr)] = v;
    }
    // Palettes
    else if (0x3F00 <= addr && addr <= 0x3FFF) {
        if ((addr & 0x13) == 0x10)
            addr &= ~0x10;
        cgRam[addr & 0x1F] = v;
    }
}

template <bool write> u8 PPU::access(u16 index, u8 v) {
    static u8 res;      // Result of the operation.
    static u8 buffer;   // VRAM read buffer.
    static bool latch;  // Detect second reading.

    /* Write into register */
    if (write) {
        res = v;

        switch (index) {
            // PPUCTRL   ($2000).
            case 0:  ctrl.r = v; tAddr.nt = ctrl.nt; break;
            // PPUMASK   ($2001).
            case 1:  mask.r = v; break;
            // OAMADDR   ($2003).
            case 3:  oamAddr = v; break;
            // OAMDATA   ($2004).
            case 4:  oamMem[oamAddr++] = v; break;
            // PPUSCROLL ($2005).
            case 5:
                // First write.
                if (!latch) { fX = v & 7; tAddr.cX = v >> 3; }
                // Second write.
                else  { tAddr.fY = v & 7; tAddr.cY = v >> 3; }
                latch = !latch; break;
            // PPUADDR   ($2006).
            case 6:
                // First write.
                if (!latch) { tAddr.h = v & 0x3F; }
                // Second write.
                else        { tAddr.l = v; vAddr.r = tAddr.r; }
                latch = !latch; break;
            // PPUDATA ($2007).
            case 7:  wr(vAddr.addr, v); vAddr.addr += ctrl.incr ? 32 : 1;
        }
    }
    /* Read from register */
    else
        switch (index) {
            // PPUSTATUS ($2002):
            case 2:  res = (res & 0x1F) | status.r; status.vBlank = 0; latch = 0; break;
            // OAMDATA ($2004).
            case 4:  res = oamMem[oamAddr]; break;
            // PPUDATA ($2007).
            case 7:
                if (vAddr.addr <= 0x3EFF) {
                    res = buffer;
                    buffer = rd(vAddr.addr);
                }
                else
                    res = buffer = rd(vAddr.addr);
                vAddr.addr += ctrl.incr ? 32 : 1;
        }
    return res;
}
template u8 PPU::access<0>(u16, u8);
template u8 PPU::access<1>(u16, u8);

u16 PPU::nt_addr() {
    return 0x2000 | (vAddr.r & 0xFFF);
}

u16 PPU::at_addr() {
    return 0x23C0 | (vAddr.nt << 10) | ((vAddr.cY / 4) << 3) | (vAddr.cX / 4);
}

u16 PPU::bg_addr() {
    return (ctrl.bgTbl * 0x1000) + (nt * 16) + vAddr.fY;
}

void PPU::h_scroll() {
    if (!rendering()) return;
    if (vAddr.cX == 31) vAddr.r ^= 0x41F;
    else vAddr.cX++;
}

void PPU::v_scroll() {
    if (!rendering()) return;
    if (vAddr.fY < 7) vAddr.fY++;
    else {
        vAddr.fY = 0;
        if      (vAddr.cY == 31)   vAddr.cY = 0;
        else if (vAddr.cY == 29) { vAddr.cY = 0; vAddr.nt ^= 0b10; }
        else                       vAddr.cY++;
    }
}

void PPU::h_update() {
    if (!rendering()) return;
    vAddr.r = (vAddr.r & ~0x041F) | (tAddr.r & 0x041F);
}

void PPU::v_update() {
    if (!rendering()) return;
    vAddr.r = (vAddr.r & ~0x7BE0) | (tAddr.r & 0x7BE0);
}

void PPU::reload_shift() {
    bgShiftL = (bgShiftL & 0xFF00) | bgL;
    bgShiftH = (bgShiftH & 0xFF00) | bgH;

    atLatchL = (at & 1);
    atLatchH = (at & 2);
}

void PPU::clear_oam() {
    for (int i = 0; i < 8; i++) {
        secOam[i].id    = 64;
        secOam[i].y     = 0xFF;
        secOam[i].tile  = 0xFF;
        secOam[i].attr  = 0xFF;
        secOam[i].x     = 0xFF;
        secOam[i].dataL = 0;
        secOam[i].dataH = 0;
    }
}

void PPU::eval_sprites() {
    int n = 0;
    for (int i = 0; i < 64; i++) {
        int line = (scanline == 261 ? -1 : scanline) - oamMem[i*4 + 0];
        // If the sprite is in the scanline, copy its properties
        // into secondary OAM:
        if (line >= 0 && line < spr_height()) {
            secOam[n].id   = i;
            secOam[n].y    = oamMem[i*4 + 0];
            secOam[n].tile = oamMem[i*4 + 1];
            secOam[n].attr = oamMem[i*4 + 2];
            secOam[n].x    = oamMem[i*4 + 3];

            if (++n > 8) {
                status.sprOvf = true;
                break;
            }
        }
    }
}

void PPU::load_sprites() {
    u16 addr;
    for (int i = 0; i < 8; i++) {
        // Copy secondary OAM into primary.
        oam[i] = secOam[i];

        // Different address modes depending on the sprite height:
        if (spr_height() == 16)
            addr = ((oam[i].tile & 1) * 0x1000) + ((oam[i].tile & ~1) * 16);
        else
            addr = ( ctrl.sprTbl      * 0x1000) + ( oam[i].tile       * 16);

        // Line inside the sprite.
        unsigned sprY = (scanline - oam[i].y) % spr_height();
        // Vertical flip.
        if (oam[i].attr & 0x80) sprY ^= spr_height() - 1;
        // Select the second tile if on 8x16.
        addr += sprY + (sprY & 8);

        oam[i].dataL = rd(addr + 0);
        oam[i].dataH = rd(addr + 8);
    }
}

void PPU::pixel() {
    u8 palette = 0, objPalette = 0;
    bool objPriority = 0;
    int x = dot - 2;

    if (scanline < 240 && x >= 0 && x < 256) {
        if (mask.bg && !(!mask.bgLeft && x < 8)) {
            // Background:
            palette = (NTH_BIT(bgShiftH, 15 - fX) << 1) |
                       NTH_BIT(bgShiftL, 15 - fX);
            if (palette)
                palette |= ((NTH_BIT(atShiftH,  7 - fX) << 1) |
                             NTH_BIT(atShiftL,  7 - fX))      << 2;
        }
        // Sprites:
        if (mask.spr && !(!mask.sprLeft && x < 8))
            for (int i = 7; i >= 0; i--) {
                // Void entry.
                if (oam[i].id == 64) continue;
                unsigned sprX = x - oam[i].x;
                // Not in range.
                if (sprX >= 8) continue;
                // Horizontal flip.
                if (oam[i].attr & 0x40) sprX ^= 7;

                u8 sprPalette = (NTH_BIT(oam[i].dataH, 7 - sprX) << 1) |
                                 NTH_BIT(oam[i].dataL, 7 - sprX);
                // Transparent pixel.
                if (sprPalette == 0) continue;

                if (oam[i].id == 0 && palette && x != 255)
                    status.sprHit = true;
                sprPalette |= (oam[i].attr & 3) << 2;
                objPalette  = sprPalette + 16;
                objPriority = oam[i].attr & 0x20;
            }
        // Evaluate priority:
        if (objPalette && (palette == 0 || objPriority == 0))
            palette = objPalette;

        pixels[scanline*256 + x] = nesRgb[rd(0x3F00 + (rendering() ? palette : 0))];
    }
    // Perform background shifts:
    bgShiftL <<= 1; bgShiftH <<= 1;
    atShiftL = (atShiftL << 1) | atLatchL;
    atShiftH = (atShiftH << 1) | atLatchH;
}

template<Scanline s> void PPU::scanline_cycle() {
    static u16 addr;

    if (s == NMI && dot == 1) { status.vBlank = true; if (ctrl.nmi) CPU::set_nmi(); }
    else if (s == POST && dot == 0) gui->new_frame(pixels);
    else if (s == VISIBLE || s == PRE) {
        // Sprites:
        switch (dot) {
            case   1: clear_oam(); if (s == PRE) { status.sprOvf = status.sprHit = false; } break;
            case 257: eval_sprites(); break;
            case 321: load_sprites(); break;
        }
        // Background
        if ((2 <= dot && dot <= 255) || (322 <= dot && dot <= 337)) {
            pixel();
            switch (dot % 8) {
                // Nametable:
                case 1:  addr  = nt_addr(); reload_shift(); break;
                case 2:  nt    = rd(addr);  break;
                // Attribute:
                case 3:  addr  = at_addr(); break;
                case 4:  at    = rd(addr);  if (vAddr.cY & 2) at >>= 4;
                                            if (vAddr.cX & 2) at >>= 2; break;
                // Background (low bits):
                case 5:  addr  = bg_addr(); break;
                case 6:  bgL   = rd(addr);  break;
                // Background (high bits):
                case 7:  addr += 8;         break;
                case 0:  bgH   = rd(addr); h_scroll(); break;
            }
        }
        // Vertical bump
        else if (dot == 256) {
            pixel();
            bgH = rd(addr);
            v_scroll();
        }
        // Update horizontal position
        else if (dot == 257) {
            pixel();
            reload_shift();
            h_update();
        }
        // Update vertical position
        else if (280 <= dot && dot <= 304) {
            if (s == PRE)
                v_update();
        }
        // No shift reloading
        else if (dot == 1) {
            addr = nt_addr();
            if (s == PRE)
                status.vBlank = false;
        }
        else if (dot == 321 || dot == 339) {
            addr = nt_addr();
        }
        // Nametable fetch instead of attribute
        else if (dot == 338) {
            nt = rd(addr);
        }
        else if (dot == 340) {
            nt = rd(addr);
            if (s == PRE && rendering() && frameOdd)
                dot++;
        }

        // Signal scanline to mapper:
        if (dot == 260 && rendering()) cartridge->signal_scanline();
    }
}

void PPU::step() {
    if (0 <= scanline && scanline <= 239)
        scanline_cycle<VISIBLE>();
    else if (scanline == 240)
        scanline_cycle<POST>();
    else if (scanline == 241)
        scanline_cycle<NMI>();
    else if (scanline == 261)
        scanline_cycle<PRE>();
    // Update dot and scanline counters:
    if (++dot > 340) {
        dot %= 341;
        if (++scanline > 261) {
            scanline = 0;
            frameOdd ^= 1;
        }
    }
}

void PPU::reset() {
    frameOdd = false;
    scanline = dot = 0;
    ctrl.r = mask.r = status.r = 0;

    memset(pixels, 0x00, sizeof(pixels));
    memset(ciRam,  0xFF, sizeof(ciRam));
    memset(oamMem, 0x00, sizeof(oamMem));
}
