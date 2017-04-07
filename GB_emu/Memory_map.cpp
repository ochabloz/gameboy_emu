//
//  Memory_map.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 28.02.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include "Memory_map.hpp"
#include <string.h>

Memory_map::Memory_map(Cart *cart, PPU * ppu, APU * apu){
    this->cart = cart;
    this->ppu = ppu;
    this->apu = apu;
    d_pad = 0x0F;
    btns  = 0x0F;
    joypad_reg = 0x00;
    DMA_src = 0;
    DMA_dst = 0;
}

uint8_t Memory_map::sync_cycle(uint8_t cycle){
    // Here      : DMA takes 640 cycles to complete
    uint8_t ret = 0;
    if(DMA_src != 0){
        for (int i = 0; i < (cycle / 4); i++) {
            write(DMA_dst, read(DMA_src));
            DMA_dst = (DMA_dst < 0xFE9F) ? DMA_dst + 1 : 0;
            DMA_src = ((DMA_src & 0xFF) < 0x9F) ? DMA_src+1 : 0;
        }
    }
    if (serial_cycles_until_shift > 0) {
        serial_cycles_until_shift -= cycle;
        if (serial_cycles_until_shift <= 0) {
            ret |= 0x08;
            serial_recieved = 0xFF;
            serial_cycles_until_shift = 0;
            serial_clock &= 0x7F;
        }
    }
    return ppu->run((uint32_t)cycle) | ret;
}

uint8_t Memory_map::read(uint16_t addr){
    if(addr < 0x8000) { // ROM address space
        return cart->read(addr);
    }
    else if(addr <= 0x9FFF){ // Video RAM address space
        return ppu->vram[addr - 0x8000];
    }
    else if(addr >= 0xA000 && addr <= 0xBFFF){ // External RAM address space
        return cart->read(addr);
    }
    else if(addr >= 0xC000 && addr <= 0xDFFF){ // Work RAM address space
        return this->wram[addr - 0xC000];
    }
    else if(addr >= 0xE000 && addr <= 0xFDFF){ // Work RAM (Duplicate) address space
        return this->wram[addr - 0xE000];
    }
    else if(addr >= 0xFE00 && addr <= 0xFE9F){ // OAM address space
        return ((uint8_t*)ppu->oam)[addr - 0xFE00];
    }
    else if(addr >= 0xFEA0 && addr <= 0xFEFF){ // OAM address space
        // not usable
    }
    else if(addr >= 0xFF00 && addr <= 0xFF7F){ // IO controls address space
        if(addr == 0xFF00){
            if(joypad_reg == 0x1){
                return  (joypad_reg << 4) | btns | 0xC0;
            }
            else if (joypad_reg == 0x2){
                return (joypad_reg << 4) | d_pad | 0xC0;
            }
            else{
                return 0xFF;
            }
        }
        else if (addr == 0xFF01){
            return serial_recieved;
        }
        else if (addr == 0xFF02){
            return serial_clock | 0x7E;
        }
        switch (addr & 0xFFF0) {
            case 0xFF10:
            case 0xFF20: return apu->read(addr);
            case 0xFF40: return ppu->read(addr);
        }
    }
    else if(addr >= 0xFF80 && addr <= 0xFFFE){ // IO controls address space
        return hram[addr - 0xFF80];
    }
    else{
        printf("oops");
    }
    return 0x00;
}

void Memory_map::write(uint16_t addr, uint8_t data){
    if(addr >= 0x0000 && addr < 0x8000){ // ROM address space
        cart->write(addr, data);
    }
    else if(addr >= 0x8000 && addr <= 0x9FFF){ // Video RAM address space
        ppu->vram[addr - 0x8000] = data;
    }
    else if(addr >= 0xA000 && addr < 0xC000){ // External RAM address space
        cart->write(addr, data);
    }
    else if(addr >= 0xC000 && addr <= 0xDFFF){ // Work RAM address space
        this->wram[addr - 0xC000] = data;
    }
    else if(addr >= 0xE000 && addr <= 0xFDFF){ // Work RAM (Duplicate) address space
        this->wram[addr - 0xE000] = data;
    }
    else if(addr >= 0xFE00 && addr <= 0xFE9F){ // OAM address space
        ((uint8_t*)ppu->oam)[addr - 0xFE00] = data;
    }
    else if(addr >= 0xFEA0 && addr <= 0xFEFF){
        // not usable
    }
    else if(addr >= 0xFF00 && addr <= 0xFF7F){ // IO controls address space
        if(addr == 0xFF00){
            joypad_reg = (data >> 4) & 0x3;
        }
        else if (addr == 0xFF01){
            serial_data = data;
        }
        else if (addr == 0xFF02){
            serial_clock = data;
            if ((serial_clock & 0x81) == 0x81) {
                // Bit 0 = 1 Internal clock. Data is sent
                serial_cycles_until_shift = 4096;
            }
        }
        else if (addr == 0xFF46){
            //DMA request
            DMA_src = data << 8;
            DMA_dst = 0xFE00;
        }
        else {
            switch (addr & 0xFFF0){
                case 0xFF10:
                case 0xFF20: apu->write(addr, data); break;
                case 0xFF40: ppu->write(addr, data); break;
            }
        }
    }
    else if(addr >= 0xFF80 && addr <= 0xFFFE){ // IO controls address space
        hram[addr - 0xFF80] = data;
    }
    else{
        printf("oops");
    }
}
