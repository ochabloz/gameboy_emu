//
//  PPU.hpp
//  GB_emu
//
//  Created by Olivier Chabloz on 24.03.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#ifndef PPU_hpp
#define PPU_hpp

#include <stdio.h>
#include <stdint.h>



struct OAM { // Sprite attributes
    uint8_t Y;
    uint8_t X;
    uint8_t tile_nb;
    uint8_t attribute;
};

/* Pixel Processing Unit */
class PPU {
    uint32_t * screen;
    // Exposed registers
    uint8_t LCDC; /* LCD Control (R/W) */
    uint8_t STAT; /* LCDC Status (R/W) */
    uint8_t SCY;  /* Scroll Y (R/W) */
    uint8_t SCX;  /* Scroll X (R/W) */
    uint32_t LY;   /* LCDC Y-Coordinate (R) */
    uint8_t LYC;  /* LY Compare (R/W) */
    //uint8_t DMA;  /* DMA Transfert and Start Address (W) */ // memory map is taking care of DMA
    uint8_t BGP;  /* BG Palette Data (R/W) */
    uint8_t OBP0; /* Object Palette 0 Data (R/W) */
    uint8_t OBP1; /* Object Palette 1 Data (R/W) */
    uint8_t WY;   /* Window Y Position (R/W)     */
    uint8_t WX;   /* Window X Position (R/W)     */

    uint32_t global_palette[4]; // actual RGB values of the 4 DMG colors
    uint32_t bg_palette[4];     // RGB representation of BGP register
    uint32_t ob0_palette[4];    // RGB representation of BGP register
    uint32_t ob1_palette[4];

    // private variables
    uint32_t mode;              // mode in which the ppu is currently in
    uint32_t next_mode;         // next ppu mode
    int cycles_until_next_mode; // number of cycles until ppu will change mode
    int line_type;
    uint32_t current_line;

    uint32_t vblank;
    uint32_t hblank;
    uint32_t lyc_int;

    uint32_t y_refresh;

    uint8_t gb_mode;
    uint8_t vram[0x4000];
    uint8_t vram_bank;


    void do_line(uint8_t line_num);
    void update_palette(uint8_t P_REG, uint32_t * palette);

public:
    PPU(uint8_t gb_mode, uint32_t * frame_buffer, uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3);
    uint8_t run(uint32_t cycles);
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    uint8_t read_vram(uint16_t addr);
    void write_vram(uint16_t addr, uint8_t data);
    bool get_screen();
    void set_palette(uint32_t col0, uint32_t col1, uint32_t col2, uint32_t col3);

	uint8_t * serialize(uint32_t * size);
	int unserialize(uint8_t * data);

    bool screen_complete;
    struct OAM oam[40];
};

#endif /* PPU_hpp */
