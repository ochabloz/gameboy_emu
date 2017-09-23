//
//  APU.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 06.04.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include "APU.hpp"
APU::APU(): sample_num(0){};

uint8_t APU::read(uint16_t addr){
    switch (addr) {
        case 0xFF10: return chan1_sweep;        
        case 0xFF11: return chan1_pattern_duty; 
        case 0xFF12: return chan1_vol_envelop;  
        case 0xFF13: return chan1_freq_lo;      
        case 0xFF14: return chan1_freq_hi;      
    
        case 0xFF16: return chan2_pattern_duty; 
        case 0xFF17: return chan2_vol_envelop;  
        case 0xFF18: return chan2_freq_lo;      
        case 0xFF19: return chan2_freq_hi;      
        
        case 0xFF1A: return chan3_enable;       
        case 0xFF1B: return chan3_length;       
        case 0xFF1C: return chan3_out_level;    
        case 0xFF1D: return chan3_freq_lo_data; 
        case 0xFF1E: return chan3_freq_hi_data; 
            
        case 0xFF20: return chan4_length;       
        case 0xFF21: return chan4_vol_envelop;  
        case 0xFF22: return chan4_polynomial;   
        case 0xFF23: return chan4_counter_init; 
            
        case 0xFF24: return chan_controls;      
        case 0xFF25: return sound_sel_output;   
        case 0xFF26: return sound_enable;       
    }
    return 0xFF;
}

void APU::write(uint16_t addr, uint8_t data){
    switch (addr) {
        case 0xFF10: chan1_sweep = data;        break;
        case 0xFF11: chan1_pattern_duty = data; break;
        case 0xFF12: chan1_vol_envelop = data;  break;
        case 0xFF13: chan1_freq_lo = data;      break;
        case 0xFF14: chan1_freq_hi = data;      break;
            
        case 0xFF16: chan2_pattern_duty = data; break;
        case 0xFF17: chan2_vol_envelop = data;  break;
        case 0xFF18: chan2_freq_lo = data;      break;
        case 0xFF19: chan2_freq_hi = data;      break;
            
        case 0xFF1A: chan3_enable = data;       break;
        case 0xFF1B: chan3_length = data;       break;
        case 0xFF1C: chan3_out_level = data;    break;
        case 0xFF1D: chan3_freq_lo_data = data; break;
        case 0xFF1E: chan3_freq_hi_data = data; break;
            
        case 0xFF20: chan4_length = data;       break;
        case 0xFF21: chan4_vol_envelop = data;  break;
        case 0xFF22: chan4_polynomial = data;   break;
        case 0xFF23: chan4_counter_init = data; break;
            
        case 0xFF24: chan_controls = data;      break;
        case 0xFF25: sound_sel_output = data;   break;
        case 0xFF26: sound_enable = data;       break;
    }
}

void APU::generate_channel_1(uint16_t * buffer, uint16_t buffer_len){
    // Generate square wave
    #define FREQ 44100 // number of samples / second
    #define SQ_FREQ 100  // the frequency of the square wave

    /* So the period of the sq wave will contain FREQ / SQ_FREQ samples */
    uint32_t samples_per_period = FREQ / SQ_FREQ;
    
    /* Half of the period the signal is up, half down */
    
    /* the signal is generated with a for loop*/
    for (uint32_t i = 0; i < buffer_len; i++) {
        if ((i + sample_num) % samples_per_period < (samples_per_period / 2)) {
            buffer[i] = 0;//28000;
        }
        else{
            buffer[i] = 0;
        }
        sample_num = ((sample_num + 1) > FREQ) ? 0 : sample_num + 1;
    }
}
