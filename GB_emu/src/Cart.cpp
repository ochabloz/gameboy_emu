//
//  Cart.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 20.03.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include "Cart.hpp"
extern "C"{
    #include "utils.h"
}
#include <string.h>

Cart::Cart(const char * file_path, const char * save_path):  ram_size(0), rtc(nullptr){
    std::ifstream file(file_path, std::ios::binary);
    rom = std::vector<char>(std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>());
    file.close();
    if(rom.size() < 0x150){
        gb_mode = 0xFF;
        return;
    }
	this->save_path = save_path;
    cart_ram_enable = false;

    for (int i = 0; i < 0x10; i++) {
        TITLE[i] = rom[0x134 + i];
    }
    cart_type = rom[0x147];
    gb_mode = rom[0x143];
    super_gb = rom[0x146];

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
	if (cart_type == MBC2_BAT) {
		ram_size = 1;
	}

    // When a cartridge contains ram, create or open a file with name "<rom_file>.sav"
    if (ram_size > 0) {
        if(save_path != NULL){
            const char * filename = path_get_filename(file_path);
            save_file = path_concatenate(save_path, filename);
            save_file = filename_replace_ext(save_file, "sav");
        }
        else{
            save_file = (char*)malloc(strlen(file_path) + 1);
            memcpy(save_file, file_path, strlen(file_path) + 1);
            save_file = filename_replace_ext(save_file, "sav");
        }

        std::ifstream sav_file (save_file, std::ios::binary);
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

        if(cart_type == MBC3_RAM_TIM_BAT){
            rtc = new Rtc(file_path);
        }
    }

    ram_bank  = 0;
    rom_bank1 = 1;
    rom_ram_mode = ROM_MODE;
}

Cart::~Cart(){
    if (rtc != nullptr){
        delete rtc;
    }
    if (ram_size > 0) {
        std::ofstream sav_file (save_file, std::ios::binary);
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
        if(rtc != nullptr && ram_bank > 0x7){
            return rtc->read((time_rtc)(ram_bank - 8));
        }
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
                //    printf("Warning! rom bank 1 is greater than size !\n");
                }
                //printf("HI bank number changed : [%4X] = %X\n", addr, rom_bank1);
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
    // there is no effect when there is a write anywhere else
}


void Cart::mbc5_write(uint16_t addr, uint8_t data){
    if (addr < 0x2000) {
        cart_ram_enable = (data & 0x0F) == 0xA;
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
}

void Cart::mbc3_write(uint16_t addr, uint8_t data){
    switch (addr & 0xF000) {
        case 0x0000:
        case 0x1000:
        { //  RAM and RTC Registers Enable
            cart_ram_enable = (data & 0x0F) == 0xA;
            break;
        }
        case 0x2000:
        case 0x3000:
        { // ROM bank select
            rom_bank1 = (!data) ? 0x1 : 0x7f & data;
            break;
        }
        case 0x4000:
        case 0x5000:
        { // RAM Bank/RTC Register
            ram_bank = data;
            break;
        }
        case 0x6000:
        case 0x7000:
        { // Latch Clock Data
            rtc->latch(data);
            break;
        }
        case 0xA000:
        case 0xB000:
        {
            if (ram_bank > 7) {
                rtc->write((time_rtc)(ram_bank - 8), data);
                //printf("wrote 0x%02x in rtc[0x%02x]\n", data, ram_bank-8);
            }
            else{
                ram[(ram_bank * 0x2000) + (addr - 0xA000)] = data;
            }
            break;
        }

        default:
            break;
    }
}

void Cart::write(uint16_t addr, uint8_t data){
    switch (cart_type) {
        case ROM_ONLY: break;

        case MBC1:
        case MBC1_RAM:
        case MBC1_RAM_BAT:
		case MBC2_BAT:
            mbc1_write(addr, data); break;

        case MBC3_RAM_TIM_BAT:
            mbc3_write(addr, data);
            break;
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
    title[15] = '\0';
}

// check header checksum of the cartridge
// return FF when file empty or doesn't exists
// return 0 for wrong checksum
// return 1 for GB
// return 2 for GBC
uint8_t Cart::status(){
    if (gb_mode == 0xFF) {
        return 0xFF;
    }
    uint8_t checksum = 0x19;
    for (int i = 0x134; i <= 0x14d; i++) {
        checksum += rom[i];
    }
    if (!checksum) {
        return (gb_mode == 0x80 || gb_mode == 0xC0) ? 0x2 : 0x1;
    }
    return 0x00;
}
