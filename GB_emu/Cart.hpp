//
//  Cart.hpp
//  GB_emu
//
//  Created by Olivier Chabloz on 20.03.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#ifndef Cart_hpp
#define Cart_hpp

#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <vector>

#define RAM_MODE 1
#define ROM_MODE 0


#define ROM_ONLY 0x00
#define MBC1 0x01
#define MBC1_RAM 0x02
#define MBC1_RAM_BAT 0x03
#define MBC2 0x05

#define MBC5 0x19
#define MBC5_RAM 0x1A
#define MBC5_RAM_BAT 0x1B


class Cart {
    int rom_bank1;
    int ram_bank;
    std::vector<char> rom;
    
    char TITLE[0x10];
    uint32_t rom_size;
    uint8_t ram_size;
    uint8_t header_checksum;
    uint8_t cart_type;
    
    bool cart_ram_enable;
    uint8_t rom_ram_mode;
    uint8_t cart_ram[0x8000];
    
    void mbc1_write(uint16_t addr, uint8_t data);
    void mbc5_write(uint16_t addr, uint8_t data);
    
public:
    Cart(char * file_path);
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    
    
    void get_title(char * title);
};

#endif /* Cart_hpp */
