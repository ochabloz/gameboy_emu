//
//  Cpu.hpp
//  GB_emu
//
//  Created by Olivier Chabloz on 02.03.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#ifndef Cpu_hpp
#define Cpu_hpp
#include <fstream>
#include "Memory_map.hpp"

#define INT_VBLANK 0x01
#define INT_LCD_ST 0x02
#define INT_TIMER  0x04
#define INT_SERIAL 0x08
#define INT_JOYPAD 0x10

#define SF_Z(x) (F = (F & 0x7F) | (x) << 7)
#define SF_N(x) (F = (F & 0xBF) | (x) << 6)
#define SF_H(x) (F = (F & 0xDF) | (x) << 5)
#define SF_C(x) (F = (F & 0xEF) | (x) << 4)

#define RF_Z() ((F >> 7) & 0x1)
#define RF_N() ((F >> 6) & 0x1)
#define RF_H() ((F >> 5) & 0x1)
#define RF_C() ((F >> 4) & 0x1)

#define FLAGS(Z,N,H,C) SF_Z(Z); SF_N(N); SF_H(H); SF_C(C)

#define REG(H, L)  ((H<< 8) | L)

class Cpu {
    uint16_t PC;
    uint16_t SP;
    uint8_t A;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;
    
    uint8_t F;
    
    uint16_t timer_DIV;
    uint32_t timer_TIMA;
    uint8_t timer_TMA;
    uint8_t timer_TCRL;
    
    bool halted;
    
    uint8_t IME; /* Interrupt Master enable */
    uint8_t IF; /* Interrupt Request mapped to memory 0xFF0F */
    uint8_t IE; /* Interrupt Enable mapped to memory 0xFFFF  */
    uint8_t IME_delay;
    
    Memory_map *mem;
    inline uint8_t read_PC_mem(); // read from PC++
    inline uint8_t read_mem(uint16_t addr);
    inline void write_mem(uint16_t addr, uint8_t data);
    uint8_t get_nn_pointer();
    void write_HL_pointer(uint8_t data);
    void push(uint8_t val);
    uint8_t pull();
    
    // instruction handlers. all inline
    inline void call_handler();
    inline void call_cond_handler(uint8_t cond);
    inline void ret_handler();
    inline void resets_handler(uint16_t addr);
    inline void JP_nn_Handler();
    inline void JP_nn_cond_Handler(uint8_t cond);
    inline void xor_Handler(uint8_t reg);
    inline void or_Handler(uint8_t reg);
    inline void ld_n_Handler(uint8_t * reg, uint8_t value);
    inline void ld_nn_Handler(uint8_t * r1, uint8_t * r2);
    inline void ldd_hla_Handler();
    inline void ldd_ahl_Handler();
    inline void add(uint8_t value);
    inline void adc(uint8_t value);
    inline void add16bits(uint8_t * HD, uint8_t * LD, uint8_t * HS, uint8_t * LS);
    inline void dec(uint8_t * reg);
    inline void dec_pointer();
    inline void jump_cond(uint8_t cond);
    inline void sub(uint8_t value);
    inline void sbc(uint8_t value);
    inline void compareA(uint8_t reg);
    inline void ld_inc();
    inline void ld_hlinc();
    inline void dec_nn(uint8_t * r1, uint8_t * r2);
    inline void inc_nn(uint8_t * r1, uint8_t * r2);
    inline void pop(uint8_t * H, uint8_t * L);
    inline void push_instruction(uint8_t H, uint8_t L);
    inline void daa_handler();
    inline void RR_handler(uint8_t * reg);
    inline void RL_handler(uint8_t * reg);
    inline void RRC_Handler(uint8_t * reg);
    inline void SRA_handler(uint8_t * reg);
    inline void SRL_handler(uint8_t *reg);
    inline void RLC_Handler(uint8_t * reg);
    inline void ldhlsp_plus_n_handler();
    
    // misc
    std::ofstream myfile;
    inline void swap(uint8_t *reg);
    
    inline void run_timer();
public:
    
    Cpu(Memory_map *m);
    ~Cpu();
    uint8_t cycle;
    uint8_t mem_cycle;
    uint8_t run();
    inline void request_interrupt(uint8_t INT);
};




#endif /* Cpu_hpp */
