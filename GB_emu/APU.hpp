//
//  APU.hpp
//  GB_emu
//
//  Created by Olivier Chabloz on 06.04.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#ifndef APU_hpp
#define APU_hpp

#include <stdio.h>
#include <stdint.h>

class APU {
    //
    uint8_t chan1_sweep;        // mapped on 0xFF10
    uint8_t chan1_pattern_duty; // mapped on 0xFF11
    uint8_t chan1_vol_envelop;  // mapped on 0xFF12
    uint8_t chan1_freq_lo;      // mapped on 0xFF13
    uint8_t chan1_freq_hi;      // mapped on 0xFF14
    
    uint8_t chan2_pattern_duty; // mapped on 0xFF16
    uint8_t chan2_vol_envelop;  // mapped on 0xFF17
    uint8_t chan2_freq_lo;      // mapped on 0xFF18
    uint8_t chan2_freq_hi;      // mapped on 0xFF19
    
    uint8_t chan3_enable;       // mapped on 0xFF1A
    uint8_t chan3_length;       // mapped on 0xFF1B
    uint8_t chan3_out_level;    // mapped on 0xFF1C
    uint8_t chan3_freq_lo_data; // mapped on 0xFF1D
    uint8_t chan3_freq_hi_data; // mapped on 0xFF1E
    
    uint8_t chan4_length;       // mapped on 0xFF20
    uint8_t chan4_vol_envelop;  // mapped on 0xFF21
    uint8_t chan4_polynomial;   // mapped on 0xFF22
    uint8_t chan4_counter_init; // mapped on 0xFF23
    
    uint8_t chan_controls;      // mapped on 0xFF24
    uint8_t sound_sel_output;   // mapped on 0xFF25
    uint8_t sound_enable;       // mapped on 0xFF26
    
public:
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
};

#endif /* APU_hpp */
