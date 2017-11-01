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
#include "rtc.hpp"

#define RAM_MODE 1
#define ROM_MODE 0


#define ROM_ONLY 0x00
#define MBC1 0x01
#define MBC1_RAM 0x02
#define MBC1_RAM_BAT 0x03
#define MBC2 0x05
#define MBC2_BAT 0x06
#define ROM_RAM 0x08
#define ROM_RAM_BAT 0x09
#define MMM01 0x0b

#define MBC3_RAM_TIM_BAT 0x10

#define MBC5 0x19
#define MBC5_RAM 0x1A
#define MBC5_RAM_BAT 0x1B



class Cart {
    uint32_t rom_bank1;
    int ram_bank;
    std::vector<char> rom;
    std::vector<char> ram;
    char * file_path_rom;
    char TITLE[0x10];
    uint32_t rom_size;
    uint8_t ram_size;
    uint8_t header_checksum;
    uint8_t cart_type;
    uint8_t gb_mode;
    uint8_t super_gb;

    bool cart_ram_enable;
    uint8_t rom_ram_mode;
    //uint8_t rtc[5];
    uint32_t rtc_unix;
    Rtc * rtc;

    void mbc1_write(uint16_t addr, uint8_t data);
    void mbc3_write(uint16_t addr, uint8_t data);
    void mbc5_write(uint16_t addr, uint8_t data);

public:
    Cart(const char * file_path);
    ~Cart();
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);

    uint8_t status();
    void get_title(char * title);
};

#endif /* Cart_hpp */
