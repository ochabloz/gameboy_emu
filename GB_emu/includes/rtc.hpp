//
//  rtc.hpp
//  GB_emu
//
//  Created by Olivier Chabloz on 02.08.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#ifndef rtc_hpp
#define rtc_hpp

#include <stdio.h>
#include <stdint.h>
#include <string>

typedef enum {
    seconds = 0,
    minutes,
    hours,
    days_low,
    days_high
    
} time_rtc;

class Rtc {
private:
    uint32_t ref_time;
    uint32_t latched_time;
    uint32_t stopped_time;
    bool active;
    std::string rtc_filename;
    uint8_t convert_timestamp(time_rtc type, uint32_t timestamp);
    
public:
    Rtc(const char * basepath);
    void latch(bool on_off);
    uint8_t read(time_rtc reg);
    void write(time_rtc reg, uint8_t val);
    ~Rtc();
};

#endif /* rtc_hpp */
