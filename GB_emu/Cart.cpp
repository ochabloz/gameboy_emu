//
//  Cart.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 20.03.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include "Cart.hpp"
#include <string>

Cart::Cart(const char * file_path){
    std::ifstream file(file_path, std::ios::binary);
    rom = std::vector<char>(std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>());
    file.close();
    
    cart_ram_enable = false;
    
    for (int i = 0; i < 0x10; i++) {
        TITLE[i] = rom[0x134 + i];
    }
    cart_type = rom[0x147];
    
    switch (rom[0x148]) { // record the total number of rom banks
        case 0x00: rom_size = 2; break;
        case 0x01: rom_size = 4; break;
        case 0x02: rom_size = 8; break;
        case 0x03: rom_size = 16; break;
        case 0x04: rom_size = 32; break;
        case 0x05: rom_size = (cart_type < 5) ?  63 :  64; break; // MBC1 Chip only supports 63 banks
        case 0x06: rom_size = (cart_type < 5) ? 125 : 128; break; // MBC1 Chip only supports 125 banks
        case 0x07: rom_size = 255; break;
        case 0x52: rom_size = 72; break;
        case 0x53: rom_size = 80; break;
        case 0x54: rom_size = 96; break;
        default: break;
    }
    
    switch (rom[0x149]) { // record the total number of ram banks
        case 0x00: ram_size = 0; break;
        case 0x01: ram_size = 1; break;
        case 0x02: ram_size = 2; break;
        case 0x03: ram_size = 4; break;
        case 0x04: ram_size = 16; break;
        default:   ram_size = 0; break;
    }
    if (ram_size > 0) {
        file_path_rom = std::string(file_path);
        std::string sav_name = file_path_rom + ".sav";
        std::ifstream sav_file (sav_name, std::ios::binary);
        std::vector<char> ram_tmp = std::vector<char>(std::istreambuf_iterator<char>(sav_file),
                          std::istreambuf_iterator<char>());
        
        if (ram_tmp.size() == (ram_size * 0x2000)) {
            // size is ok.
            ram = ram_tmp;
        }
        else{
            // size not ok. reseting...
            ram = std::vector<char>(ram_size * 0x2000, 0);
        }
        
    }
    
    ram_bank  = 0;
    rom_bank1 = 1;
    rom_ram_mode = ROM_MODE;
}

Cart::~Cart(){
    if (ram_size > 0) {
        std::string sav_name = file_path_rom + ".sav";
        std::ofstream sav_file (sav_name, std::ios::binary);
        sav_file.write(reinterpret_cast<char*>(&ram[0]), ram.size());
        sav_file.close();
    }
    
}

uint8_t Cart::read(uint16_t addr){
    if (addr < 0x4000) {
        return rom[addr];
    }
    else if (addr < 0x8000){
        return rom[(rom_bank1 * 0x4000) + (addr - 0x4000)];
    }
    else if (addr < 0xC000){
        uint32_t ram_addr = (ram_bank * 0x2000) + (addr - 0xA000);
        return ram[ram_addr];
    }
    return 0x00;
}


void Cart::mbc1_write(uint16_t addr, uint8_t data){
    if (addr < 0x2000) {
        cart_ram_enable = (data == 0x0A);
    }
    else if(addr >= 0x2000 && addr < 0x4000){
        rom_bank1 = ((data & 0x1F) == 0) ? 0x1 : (rom_bank1 & ~(0x1F)) | (data & 0x1F);
        if (rom_bank1 > (rom_size - 1)) {
            rom_bank1 = rom_size - 1;
            printf("Warning! rom bank 1 is greater than size !\n");
        }
    }
    else if(addr >= 0x4000 && addr < 0x6000){
        switch (rom_ram_mode) {
            case ROM_MODE:
            {
                rom_bank1 &= 0x1F;
                rom_bank1 |= (data & 0x3) << 5;
                if (rom_bank1 > (rom_size - 1)) {
                    rom_bank1 = rom_size - 1;
                    printf("Warning! rom bank 1 is greater than size !\n");
                }
                printf("HI bank number changed : [%4X] = %X\n", addr, rom_bank1);
                break;
            }
            case RAM_MODE:
            {
                if (ram_size == 0) {
                    ram_bank = 0;
                    break;
                }
                ram_bank = ((data & 0x3) <= (ram_size - 1)) ? data & 3 : ram_size - 1;
                break;
            }
        } 
    }
    else if(addr >= 0x6000 && addr < 0x8000){
        if ((data & 0x01) == RAM_MODE) {
            rom_bank1 &= 0x1F;
            rom_ram_mode = RAM_MODE;
        }
        else{
            ram_bank = 0;
            rom_ram_mode = ROM_MODE;
        }
    }
    else if(addr >= 0xA000 && addr < 0xC000 && ram_size > 0){
        ram[addr - 0xA000] = data;
    }
    else{
        printf("Unknown cart write : [%4X] = %X\n", addr, data);
        
    }
}


void Cart::mbc5_write(uint16_t addr, uint8_t data){
    if (addr < 0x2000) {
        cart_ram_enable = (data == 0x0A);
    }
    else if(addr >= 0x2000 && addr < 0x4000){
        rom_bank1 = (data > (rom_size-1)) ? rom_size - 1 : data;
    }
    else if(addr >= 0x3000 && addr < 0x4000){
    
        rom_bank1 = (data & 0x1) ? rom_bank1 | 0x100 : rom_bank1 & 0xFF;
        rom_bank1 |= (data & 0x3) << 5;
    }
    
    else if(addr >= 0x4000 && addr < 0x6000){
        ram_bank = data;
    }
    else if(addr >= 0xA000 && addr < 0xC000){
        ram[(ram_bank * 0x2000) + (addr - 0xA000)] = data;
    }
    else{
        printf("Unknown cart write : [%4X] = %X\n", addr, data);
        
    }
}

void Cart::write(uint16_t addr, uint8_t data){
    switch (cart_type) {
        case ROM_ONLY: break;
            
        case MBC1:
        case MBC1_RAM:
        case MBC1_RAM_BAT:
            mbc1_write(addr, data); break;
            
        case MBC5:
        case MBC5_RAM:
        case MBC5_RAM_BAT:
            mbc5_write(addr, data); break;
            
        default:
            break;
    }
}

void Cart::get_title(char *title){
    for (int i = 0; i < 16; i++) {
        title[i] = this->TITLE[i];
    }
}
