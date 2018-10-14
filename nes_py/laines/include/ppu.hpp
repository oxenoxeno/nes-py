#pragma once
#include <iostream>
#include <cstring>
#include "common.hpp"
class NES;
#include "nes.hpp"

/// Scanline configuration options
enum Scanline  { VISIBLE, POST, NMI, PRE };
/// Mirroring configuration options
enum Mirroring { VERTICAL, HORIZONTAL };

/// Sprite buffer
struct Sprite {
    /// Index in OAM
    u8 id;
    /// X position
    u8 x;
    /// Y position
    u8 y;
    /// Tile index
    u8 tile;
    /// Attributes
    u8 attr;
    /// Tile data (low)
    u8 dataL;
    /// Tile data (high)
    u8 dataH;
};

/// PPUCTRL ($2000) register
union Ctrl {
    struct {
        /// Nametable ($2000 / $2400 / $2800 / $2C00)
        unsigned nt     : 2;
        /// Address increment (1 / 32)
        unsigned incr   : 1;
        /// Sprite pattern table ($0000 / $1000)
        unsigned sprTbl : 1;
        /// BG pattern table ($0000 / $1000)
        unsigned bgTbl  : 1;
        /// Sprite size (8x8 / 8x16)
        unsigned sprSz  : 1;
        /// PPU master/slave
        unsigned slave  : 1;
        /// Enable NMI
        unsigned nmi    : 1;
    };
    u8 r;
};

/// PPUMASK ($2001) register
union Mask {
    struct {
        /// Grayscale
        unsigned gray    : 1;
        /// Show background in leftmost 8 pixels
        unsigned bgLeft  : 1;
        /// Show sprite in leftmost 8 pixels
        unsigned sprLeft : 1;
        /// Show background
        unsigned bg      : 1;
        /// Show sprites
        unsigned spr     : 1;
        /// Intensify reds
        unsigned red     : 1;
        /// Intensify greens
        unsigned green   : 1;
        /// Intensify blues
        unsigned blue    : 1;
    };
    u8 r;
};

/// PPUSTATUS ($2002) register
union Status {
    struct {
        /// Not significant
        unsigned bus    : 5;
        /// Sprite overflow
        unsigned sprOvf : 1;
        /// Sprite 0 Hit
        unsigned sprHit : 1;
        /// In VBlank?
        unsigned vBlank : 1;
    };
    u8 r;
};

/// Loopy's VRAM address
union Addr {
    struct {
        // Coarse X
        unsigned cX : 5;
        // Coarse Y
        unsigned cY : 5;
        // Nametable
        unsigned nt : 2;
        // Fine Y
        unsigned fY : 3;
    };
    struct {
        unsigned l : 8;
        unsigned h : 7;
    };
    unsigned addr : 14;
    unsigned r : 15;
};

/// an instance of the Picture Processing Unit (PPU)
class PPU {
private:
    /// the RGB palette
    u32 nesRgb[64] = {
        0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
        0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
        0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
        0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
        0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
        0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
        0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
        0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
    };
    /// Mirroring mode
    Mirroring mirroring;
    /// VRAM for name-tables
    u8 ciRam[0x800];
    /// VRAM for palettes
    u8 cgRam[0x20];
    /// VRAM for sprite properties
    u8 oamMem[0x100];
    /// Sprite buffers
    Sprite oam[8], secOam[8];
    /// Video buffer
    u32 pixels[256 * 240];

    /// Loopy V, T
    Addr vAddr, tAddr;
    /// Fine X
    u8 fX;
    /// OAM address
    u8 oamAddr;

    /// PPUCTRL   ($2000) register
    Ctrl ctrl;
    /// PPUMASK   ($2001) register
    Mask mask;
    /// PPUSTATUS ($2002) register
    Status status;

    /// Background latches:
    u8 nt, at, bgL, bgH;
    /// Background shift registers:
    u8 atShiftL, atShiftH; u16 bgShiftL, bgShiftH;
    bool atLatchL, atLatchH;

    /// Rendering counters:
    int scanline, dot;
    bool frameOdd;

    /// the NES this CPU belongs to
    NES* nes;

public:
    /// Initialize a new PPU.
    PPU();

    /// Initialize a new PPU as a copy of another PPU.
    PPU(PPU* ppu);

    /// Set the NES for this PPU
    void set_nes(NES* new_nes) { nes = new_nes; };

    /// Get the NES for this PPU
    NES* get_nes() { return nes; };

    /// Set the PPU to the given mirroring mode.
    void set_mirroring(Mirroring mode) { mirroring = mode; };

    /// Get CIRAM address according to mirroring.
    u16 nt_mirror(u16 addr);

    bool rendering() { return mask.bg || mask.spr; };
    int spr_height() { return ctrl.sprSz ? 16 : 8; };

    /// Read an address from PPU memory.
    u8 rd(u16 addr);

    /// Write a byte to PPU memory.
    void wr(u16 addr, u8 v);

    /// Access PPU through registers.
    template <bool write> u8 access(u16 index, u8 v = 0);

    /// Calculate graphics addresses
    u16 nt_addr();
    /// Calculate graphics addresses
    u16 at_addr();
    /// Calculate graphics addresses
    u16 bg_addr();

    /// Increment the horizontal scroll by one pixel
    void h_scroll();

    /// Increment the vertical scroll by one pixel
    void v_scroll();

    /// Copy horizontal scrolling data from loopy T to loopy V
    void h_update();

    /// Copy vertical scrolling data from loopy T to loopy V
    void v_update();

    /// Put new data into the shift registers
    void reload_shift();

    /// Clear secondary OAM
    void clear_oam();

    /// Fill secondary OAM with the sprite infos for the next scanline
    void eval_sprites();

    /// Load the sprite info into primary OAM and fetch their tile data.
    void load_sprites();

    /// Process a pixel, draw it if it's on screen
    void pixel();

    /// Execute a cycle of a scanline
    template<Scanline s> void scanline_cycle();

    /// Execute a PPU cycle.
    void step();

    /// Reset the PPU to a blank state.
    void reset();

};
