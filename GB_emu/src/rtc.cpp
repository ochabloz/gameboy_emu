//
//  rtc.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 02.08.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include "rtc.hpp"
#include <string>
#include <fstream>
#include <vector>
#include <time.h>

Rtc::Rtc(const char * basepath){
    std::string path_rom = std::string(basepath);
    rtc_filename = path_rom + ".rtc";
    std::ifstream rtc_file (rtc_filename, std::ios::binary);
    std::vector<char> rtc_tmp = std::vector<char>(std::istreambuf_iterator<char>(rtc_file),
                                                  std::istreambuf_iterator<char>());
    if(rtc_tmp.size() == 4){
        for (int i = 0; i < 4; i++) {
            ref_time |= rtc_tmp[i] << (i * 8);
        }
    }
    else{
        ref_time = time(NULL) % (512 * 24 * 60 * 60);
    }
    rtc_file.close();
    active = true;
}

Rtc::~Rtc(){
    std::ofstream sav_file (rtc_filename, std::ios::binary);
    char rtc_data[4];
    for (int i = 0; i < 4; i++) {
         rtc_data[i] = ref_time >> (i * 8);
    }
    sav_file.write(reinterpret_cast<char*>(&rtc_data[0]), 4);
    sav_file.close();
}
void Rtc::latch(bool on_off){
    if(on_off){
        if (active){
            latched_time = (uint32_t)time(NULL) % (512 * 24 * 60 * 60);
        }
        else{
            latched_time = stopped_time;
        }
        latched_time = latched_time - ref_time;
    }
}

uint8_t Rtc::convert_timestamp(time_rtc type, uint32_t timestamp){
    switch (type) {
        case seconds:
            return timestamp % 60;
            break;
        case minutes:
            return (timestamp / 60) % 60;
            break;
            
        case hours:
            return (timestamp / 3600) % 24;
            
        case days_low:
            return (uint8_t)(timestamp / (24 * 60 * 60));
            break;
        case days_high:
            /* Bit 0  Most significant bit of Day Counter (Bit 8)
             Bit 6  Halt (0=Active, 1=Stop Timer)
             Bit 7  Day Counter Carry Bit (1=Counter Overflow)
             */
            return ((timestamp / (24 * 60 * 60) > 255) ? 1 : 0) | (active) ? 0 : 1 << 6;
            break;
        default:
            return 0xFF;
            break;
    }
}
// TODO : make sure reg doesnt overflow
uint8_t Rtc::read(time_rtc reg){
    return convert_timestamp(reg, latched_time);
}

void Rtc::write(time_rtc reg, uint8_t val){
    const static uint32_t multiplier[] = {1, 60, 3600, (24 * 60 * 60)};
    
    uint32_t unix_time = (uint32_t)time(NULL) % (512 * 24 * 60 * 60);
    unix_time -= ref_time;
    if (reg == days_high) {
        ref_time += ((convert_timestamp(reg, unix_time) & 0x1) << 8) * (24 * 60 * 60);
        ref_time -= ((val & 0x1) << 8) * (24 * 60 * 60);
        
        active = (val & (0x1 << 6)) ? false : true;
        
        return;
    }
    ref_time += convert_timestamp(reg, unix_time) * multiplier[reg];
    ref_time -= val * multiplier[reg];
}

/*
****
*/
