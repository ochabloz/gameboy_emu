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
#include <SDL2/SDL.h>

#define BGB

#ifdef BGB // BGB palette (greenish tint)
#define COLOR0 0xe0f8d0
#define COLOR1 0x88c070
#define COLOR2 0x346856
#define COLOR3 0x081820
#else      // B/W palette
#define COLOR0 0xEEEEEE
#define COLOR1 0xCCCCCC
#define COLOR2 0x888888
#define COLOR3 0x000000
#endif


struct OAM { // Sprite attributes
    uint8_t Y;
    uint8_t X;
    uint8_t tile_nb;
    uint8_t attribute;
};

/* Pixel Processing Unit */
class PPU {
    SDL_Surface *screen;
    
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
    uint32_t mode;                   // mode in which the ppu is currently in
    uint32_t next_mode;              // next ppu mode
    int cycles_until_next_mode; // number of cycles until ppu will change mode
    int line_type;
    int current_line;
    
    uint32_t vblank;
    uint32_t hblank;
    uint32_t lyc_int;
    
    uint32_t y_refresh;
    
    uint8_t gb_mode;
    uint8_t vram[0x4000];
    uint8_t vram_bank;
    
    
    void do_line(void);
    void update_palette(uint8_t P_REG, uint32_t * palette);
    uint8_t read_vram(uint16_t addr);
    void write_vram(uint16_t addr, uint8_t data);
public:
    PPU(uint8_t gb_mode);
    uint8_t run(uint32_t cycles);
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    SDL_Surface * get_screen();
    void set_palette(uint32_t col0, uint32_t col1, uint32_t col2, uint32_t col3);
    bool screen_complete;
    
    struct OAM oam[40];
};

#endif /* PPU_hpp */
