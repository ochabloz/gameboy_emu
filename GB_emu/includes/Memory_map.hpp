//
//  Memory_map.hpp
//  GB_emu
//
//  Created by Olivier Chabloz on 28.02.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#ifndef Memory_map_hpp
#define Memory_map_hpp

#include <stdint.h>
#include "Cart.hpp"
#include "PPU.hpp"
#include "APU.hpp"
#include <vector>

class Memory_map{
    uint8_t wram[0x2000];
    uint8_t hram[0x007F];
    uint16_t DMA_src;
    uint16_t DMA_dst;
    Cart * cart;
    PPU * ppu;
    APU * apu;
    uint8_t joypad_reg;
    std::vector<char> boot_rom;
    bool boot_rom_activated;
    
    int serial_cycles_until_shift;
    int serial_data;
    uint8_t serial_recieved;
    uint8_t serial_clock;

    
public:
    uint8_t joypad;
    uint8_t sync_cycle(uint8_t cycle);
    Memory_map(Cart *cart, PPU * ppu, APU * apu, const char * boot_rom_file);
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    bool has_bootrom(){return this->boot_rom_activated;}
};


#endif /* Memory_map_hpp */
