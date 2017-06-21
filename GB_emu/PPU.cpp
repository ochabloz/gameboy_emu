//
//  PPU.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 24.03.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include "PPU.hpp"

#define STAT_LYC_INT()  ((STAT>>6) & 0x01)
#define STAT_OAM_INT()  ((STAT>>5) & 0x01)
#define STAT_VBL_INT()  ((STAT>>4) & 0x01)
#define STAT_HBL_INT()  ((STAT>>3) & 0x01)


#define DISPLAY_ENABLE()   ((LCDC>>7) & 0x01) // (0=Off, 1=On)
#define WIN_TILE_SEL()     ((LCDC>>6) & 0x01) // (0=9800-9BFF, 1=9C00-9FFF)
#define WIN_DISP_ENABLE()  ((LCDC>>5) & 0x01) // (0=Off, 1=On)
#define BG_WIN_DATA_SEL()  ((LCDC>>4) & 0x01) // (0=8800-97FF, 1=8000-8FFF)
#define BG_TILE_DISP_SEL() ((LCDC>>3) & 0x01) // (0=9800-9BFF, 1=9C00-9FFF)
#define SPRITE_SIZE()      ((LCDC>>2) & 0x01) // (0=8x8, 1=8x16)
#define SPRITE_ENABLE()    ((LCDC>>1) & 0x01) // (0=Off, 1=On)
#define BG_DISPLAY()       ((LCDC)    & 0x01) // (0=Off, 1=On) NOTE: GBC differs

#define SPRITE_PRIORITY(S) ((S >> 7) & 0x01)
#define SPRITE_Y_FLIP(S)   ((S >> 6) & 0x01)
#define SPRITE_X_FLIP(S)   ((S >> 5) & 0x01)
#define SPRITE_PALETTE(S)  ((S >> 4) & 0x01)
#define SPRITE_COL_BANK(S) ((S >> 3) & 0x01)
#define SPRITE_COL_PAL(S)  ((S >> 0) & 0x07)


#define LINE_T_FIRST  0
#define LINE_T_NORMAL 1
#define LINE_T_VBLANK 2
#define LINE_T_LAST   3

#define MAP_COLOR(X) SDL_MapRGBA(screen->format,(X & 0xff0000)>>16, (X & 0xff00)>>8,X & 0xff, 0xff)

PPU::PPU():
    screen_complete(false), LCDC(0x91), STAT(0x81),
    screen(SDL_CreateRGBSurface(0, 160, 144, 32, 0, 0, 0, 0)),
    global_palette{MAP_COLOR(COLOR0), MAP_COLOR(COLOR1), MAP_COLOR(COLOR2), MAP_COLOR(COLOR3)}{
    write(0xFF47, 0xE4); // BGP default value
    
    LY = 0x00;
    current_line = 154;
    line_type = LINE_T_LAST;
    cycles_until_next_mode = 80;
    next_mode = 0;
    vblank = 0;
    hblank = 0;
    lyc_int = 0;
};

uint8_t PPU::read(uint16_t addr){
    switch (addr) {
        case 0xFF40: return LCDC; break;
        case 0xFF41: return (STAT & 0x7C) | ((uint8_t)lyc_int <<2) | (mode & 0x3) | 0x80; break;
        case 0xFF42: return SCY; break;
        case 0xFF43: return SCX; break;
        case 0xFF44: return (DISPLAY_ENABLE()) ? (uint8_t)LY : 0x00; break;
        case 0xFF45: return LYC; break;
        /* case 0xFF46: return DMA; break; WRITE ONLY */
        case 0xFF47: return BGP; break;
        case 0xFF48: return OBP0; break;
        case 0xFF49: return OBP1; break;
        case 0xFF4A: return WY; break;
        case 0xFF4B: return WX; break;
        default: return 0xFF; break;
    }
}

void PPU::write(uint16_t addr, uint8_t data){
    switch (addr) {
        case 0xFF40:{
            if (!DISPLAY_ENABLE() && (data & 0x80) ) {
                // reset PPU
                LY = 0x00;
                current_line = 0;
                line_type = LINE_T_FIRST;
                next_mode = 2;
                mode = 0;
            }
            LCDC = data;
            break;
        }
        case 0xFF41: STAT = data; break;
        case 0xFF42: SCY = data; break;
        case 0xFF43: SCX = data; break;
        /* case 0xFF44: LY = data; break; READ ONLY */
        case 0xFF45: LYC = data; break;
        /* case 0xFF46: DMA = data; break; DMA is taken care by memory mapper */
        case 0xFF47:{
            BGP = data;
            update_palette(BGP, bg_palette);
            break;
        }
        case 0xFF48:{
            OBP0 = data;
            update_palette(OBP0, ob0_palette);
            break;
        }

        case 0xFF49:{
            OBP1 = data;
            update_palette(OBP1, ob1_palette);
            break;
        }
        case 0xFF4A: WY = data; break;
        case 0xFF4B: WX = data; break;
        default: break;
    }
}

void PPU::update_palette(uint8_t P_REG, uint32_t * palette){
    for (int i = 0; i < 4; i++) {
        uint8_t pal = (P_REG >> (i*2)) & 0x3;
        palette[i] = global_palette[pal];
    }
}

#define MODE0 0 /* During H-Blank */
#define MODE1 1 /* During V-Blank */
#define MODE2 2 /* Searching OAM-VRAM */            /* OAM-VRAM not available to CPU */
#define MODE3 3 /* Transfer Data to LCD driver */   /* OAM-VRAM not available to CPU */

int test_cycles;
int last_ly;
// 70224 cycles in total
uint8_t PPU::run(uint32_t cycles){
    uint8_t ret = 0;
    if (cycles) {
        lyc_int = 0;
        /* if (LY != last_ly) {
            printf("LY = %d, test_cycles = %d\n",last_ly, test_cycles);
            last_ly = LY;
            test_cycles = 0;
        }
        test_cycles += cycles; */
    }
    
    cycles_until_next_mode -= cycles;
    if (cycles_until_next_mode <= 0) {
        
        if (mode != 1 && next_mode == 1 && STAT_VBL_INT()) {
            ret |= 0x1 << 1;   // request LCD STAT interrupt
        }
        if (mode != 0 && next_mode == 0 && STAT_HBL_INT()) {
            ret |= 0x1 << 1;   // request LCD STAT interrupt
        }
        if (mode != 2 && next_mode == 2 && STAT_OAM_INT()) {
            ret |= 0x1 << 1;   // request LCD STAT interrupt
        }
        
        
        mode = next_mode;
        switch (line_type) {
            case LINE_T_LAST:
            {
                if (next_mode == 1) {
                    LY = 0;
                    
                    cycles_until_next_mode += 452;
                    next_mode = 0;
                }
                else if (next_mode == 0) {
                    cycles_until_next_mode += 4;
                    next_mode = 2;
                    current_line = 0;
                    line_type = LINE_T_FIRST;
                }
                break;
            }
            case LINE_T_FIRST:
            {
                if (next_mode == 2){
                    do_line();
                    cycles_until_next_mode += 80;
                    next_mode = 3;
                }
                else if (next_mode == 3){ // HBLANK at 420 cycles
                    cycles_until_next_mode += 336;
                    next_mode = 0;
                }
                else if (next_mode == 0)
                {
                    cycles_until_next_mode += 32;
                    current_line++;
                    line_type = LINE_T_NORMAL;
                }
                break;
            }
            case LINE_T_NORMAL:
            {
                if (next_mode == 2){
                    lyc_int = (LY == LYC);
                    cycles_until_next_mode += 80;
                    next_mode = 3;
                }
                else if (next_mode == 3){ // HBLANK at 420 cycles
                    cycles_until_next_mode += 336;
                    next_mode = 0;
                }
                else if (next_mode == 0 && (current_line == LY))
                {
                    cycles_until_next_mode += 32;
                    current_line++;
                    line_type = (current_line < 144) ? LINE_T_NORMAL : LINE_T_VBLANK;
                }
                else if (next_mode == 0){
                    LY++;
                    do_line(); // do line should return the number of sprites rendered
                    cycles_until_next_mode += 4;
                    next_mode = 2;
                }
                break;
            }
            case LINE_T_VBLANK:
            {
                if (next_mode == 0){
                    LY++;
                    cycles_until_next_mode += 4;
                    next_mode = 1;
                    
                    if(LY == 0x90){
                        ret = DISPLAY_ENABLE(); // request V-BLANK interrupt if the display is enabled
                        screen_complete = true;
                    }
                }
                else if (next_mode == 1 && (current_line == LY)){
                    cycles_until_next_mode += 452;
                    current_line++;
                }
                else if (next_mode == 1){
                    LY++;
                    cycles_until_next_mode += 4;
                    next_mode = 1;
                    line_type = (current_line < 153) ? LINE_T_VBLANK : LINE_T_LAST;
                }
                break;
            }
                
            default:
                break;
        }
    }
    if (lyc_int && STAT_LYC_INT()) {
        ret |= 0x1 << 1;   // request LCD STAT interrupt
    }
    return ret;
}
/*
uint8_t PPU::run(uint32_t cycles){
    y_refresh += cycles;
    uint8_t ret = 0;
    if(y_refresh >= 452){
        LY++;
        y_refresh -= 452;
        if(LY < 0x90){
            do_line();
        }
        if(LY == 0x90){
            ret = DISPLAY_ENABLE(); // request V-BLANK interrupt if the display is enabled
            screen_complete = true;
        }
        if(LY == LYC && DISPLAY_ENABLE()){
            STAT |= 1 << 2;
            if (STAT_LYC_INT()) {
                ret |= 0x1 << 1;   // request LCD STAT interrupt
            }
        }
        else{
            STAT &= ~(1 << 2);
        }
    }
    if(LY > 153){
        LY = 0;
    }
    return ret;
}
*/


void PPU::do_line(){
    if (!DISPLAY_ENABLE()) {
        for (int i = 0; i < (160); i++) {
            ((uint32_t *)screen->pixels)[LY * 160 + i] = global_palette[0];
        }
        return;
    }
    int32_t x = 0;
    
    for (int i = 0; i < 21; i++) { // 20 Tiles per line + 1 if there is an offset
        uint16_t tile_number = (((LY+ SCY) / 8) & 0x1F) * 0x20 + ((SCX / 8 + i) & 0x1F);
        
        uint16_t Tile_addr;
        if (BG_TILE_DISP_SEL()) {
             Tile_addr = vram[0x1C00 + tile_number];
        }
        else{
            Tile_addr = vram[0x1800 + tile_number];
        }
        
        if (BG_WIN_DATA_SEL() == 0) {
            Tile_addr =  (Tile_addr < 0x80) ? (Tile_addr << 4) + 0x1000 : Tile_addr << 4;
        }
        else{
            Tile_addr <<= 4;
        }
        uint8_t data  = vram[Tile_addr + ((LY+SCY) % 8) *2];
        uint8_t data2 = vram[Tile_addr + (((LY+SCY) % 8)* 2)+ 1];
        
        uint8_t pix_offset = SCX % 8;
        
        for(int k = 0; k< 8; k++){
            uint8_t x_pos = x + k - pix_offset;
            if(x_pos < 160){
                uint8_t color = ((data2 >> (7-k) & 1) << 1)| (data >> (7 - k) & 1);
                ((uint32_t *)screen->pixels)[LY * 160 + x_pos] = bg_palette[color];
            }
        }
        x += 8;
    }

    
    x = 0;
    if (WIN_DISP_ENABLE() && LY >= WY) {
        for (int pos_x = 0; pos_x < 160; pos_x +=8) {
            if (pos_x+7 >= (WX)) {
                uint16_t tile_number = (((LY- WY) / 8) & 0x1F) * 0x20 + (((pos_x - (WX-7)) / 8 ) & 0x1F);
                
                uint16_t Tile_addr;
                if (WIN_TILE_SEL()) {
                    Tile_addr = vram[0x1C00 + tile_number];
                }
                else{
                    Tile_addr = vram[0x1800 + tile_number];
                }
                
                if (BG_WIN_DATA_SEL() == 0) {
                    Tile_addr =  (Tile_addr < 0x80) ? (Tile_addr << 4) + 0x1000 : Tile_addr << 4;
                }
                else{
                    Tile_addr <<= 4;
                }
                uint8_t data  = vram[Tile_addr + ((LY+WY) % 8) *2];
                uint8_t data2 = vram[Tile_addr + (((LY+WY) % 8)* 2)+ 1];
                
                uint8_t pix_offset = (WX-7) % 8;
                
                for(int k = 0; k< 8; k++){
                    uint8_t x_pos = pos_x + k - pix_offset;
                    if(x_pos < 160){
                        uint8_t color = ((data2 >> (7-k) & 1) << 1)| (data >> (7 - k) & 1);
                        ((uint32_t *)screen->pixels)[LY * 160 + x_pos] = bg_palette[color];
                    }
                }
            }
        }
        
        for (int i = 0; i < 21; i++) { // 20 Tiles per line + 1 if there is an offset
            
            x += 8;
        }
    }
    
    
    
    
    
    int nb_sprites = 0; // Keep track of how many sprites are rendered on the line since a maximum of 10 is allowed per line.
    int8_t priority[160] = {0};
    if(SPRITE_ENABLE()){
        for (int i = 0; i < 40; i++) {
            // TODO : lots of things work only for 8x8 sprites
            if (oam[i].X < (160 + 8) && oam[i].X > 0 && oam[i].Y < (144 + 16) && oam[i].Y > 0){
                uint8_t res = 8 - (oam[i].Y - 8 - LY);
                if (res < 8) {
                    nb_sprites++;
                    
                    // Load pixels from VRAM
                    uint8_t j;
                    if (SPRITE_Y_FLIP(oam[i].attribute)) {
                        j = 7 - (LY - (oam[i].Y - 16));
                    }
                    else{
                        j = 8 - ((oam[i].Y) - (LY + 8));
                    }
                    uint8_t sp_line0 = vram[(oam[i].tile_nb << 4) + j*2];
                    uint8_t sp_line1 = vram[(oam[i].tile_nb << 4) + (j * 2)+ 1];
                    
                    // Load corresponding palette
                    uint32_t * SPAL = (SPRITE_PALETTE(oam[i].attribute)) ? ob1_palette : ob0_palette;
                    
                    
                    for (int pix_x = 0; pix_x < 8; pix_x++) {
                        uint8_t x = oam[i].X + pix_x - 8;
                        if (x < 160) {
                            uint8_t order = (SPRITE_X_FLIP(oam[i].attribute)) ? 0 + pix_x : 7 - pix_x;
                            uint8_t color = ((sp_line1 >> order & 1) << 1) | (sp_line0 >> order & 1);
                            // color 0 is discarded
                            if(color && (priority[x] == 0 || priority[x] > oam[i].X)){
                                if(SPRITE_PRIORITY(oam[i].attribute)){ // BG priority
                                    uint32_t pix = ((uint32_t *)screen->pixels)[LY * 160 + x];
                                    if(pix == bg_palette[0]){
                                        ((uint32_t *)screen->pixels)[LY * 160 + x] = SPAL[color];
                                        priority[x] = oam[i].X;
                                    }
                                }
                                else{ // sprite priority over BG
                                    ((uint32_t *)screen->pixels)[LY * 160 + x] = SPAL[color];
                                    priority[x] = oam[i].X;
                                }
                            }
                        }
                    }
                }
            }
            if(nb_sprites == 10){
                break;
            }
        }
    }
}

SDL_Surface * PPU::get_screen(){
    if (screen_complete) {
        screen_complete = false;
        return screen;
    }
    return (SDL_Surface*)nullptr;
}
