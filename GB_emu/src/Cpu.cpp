//
//  Cpu.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 02.03.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include "Cpu.hpp"
#include <iostream>
#include  <iomanip>
using namespace std;

//#define TRACE


#define writeHL(data) write_mem(REG(H, L), data)
#define readHL() read_mem(REG(H, L))
#define pushPC() push((uint8_t)(PC >> 8)); push((uint8_t)(PC & 0xFF))

#define read_PC_mem() read_mem(PC++)

// misc instruction implemented by macros
#define check_bit(x, reg) do{SF_Z(((reg) & (0x1 << (x))) == 0); SF_H(1); SF_N(0);}while(0)

Cpu::Cpu(Memory_map *m, bool dirty_boot):
    SP(0xfffe), A(0x11), B(0x00), C(0x13), D(0x00), E(0xd8), H(0x01), L(0x4d),
    timer_DIV(0xABCC), timer_TIMA(0x00), timer_TMA(0x00), timer_TCRL(0xF8),
    halted(false),
    IME(0), IF(0x01), IE(0x00){
    PC = (dirty_boot) ? 0x00 : 0x100;
    writeF(0xB0);

    mem = m;
#ifdef TRACE
    myfile.open ("/Users/Olivier/Dropbox/Coding/gameboy/emu/gb_emu.log", ios::out);
#endif
};

Cpu::~Cpu(){
#ifdef TRACE
    myfile.close();
#endif
}

inline void Cpu::writeF(uint8_t val){
    flag_c = (val & 0x10) > 0;
    flag_h = (val & 0x20) > 0;
    flag_n = (val & 0x40) > 0;
    flag_z = (val & 0x80) > 0;
}

inline uint8_t Cpu::readF(){
    uint8_t ret = (flag_c) ? 0x10 : 0x00;
    ret |= (flag_h) ? 0x20 : 0x00;
    ret |= (flag_n) ? 0x40 : 0x00;
    ret |= (flag_z) ? 0x80 : 0x00;
    return ret;
}

void Cpu::write_HL_pointer(uint8_t data){
    uint16_t pointer = (H << 8) | L;
    write_mem(pointer, data);
}



inline uint8_t Cpu::read_mem(uint16_t addr){
    cycle += 4;

    // catches io ctrls for timer and interrupt
    request_interrupt(mem->sync_cycle(cycle - mem_cycle));
    mem_cycle = cycle;
    switch (addr) {
        case 0xFF04: return (uint8_t)(timer_DIV >> 8); // DIV - Divider Register
        case 0xFF05:
        {
            switch (timer_TCRL & 0x3) {
                case 0b00: return (uint8_t)(timer_TIMA >> 10); break; // 18 bits timer
                case 0b01: return (uint8_t)(timer_TIMA >> 4); break; // 12 bits timer
                case 0b10: return (uint8_t)(timer_TIMA >> 6); break; // 14 bits timer
                case 0b11: return (uint8_t)(timer_TIMA >> 8); break; // 16 bits timer
            }
        }
        case 0xFF06: return timer_TMA;
        case 0xFF07: return timer_TCRL;
        case 0xFF0f: return IF | 0xe0;
        case 0xFFFF: return IE & 0x1F;
        default:     return mem->read(addr);
    }
}

inline void Cpu::write_mem(uint16_t addr, uint8_t data){
    cycle += 4;

    switch (addr) {
        case 0xFF04:
            timer_DIV = 0;
            break;
        case 0xFF05:
        {
            switch (timer_TCRL & 0x3) {
                case 0b00: timer_TIMA = ((uint32_t)data << 10) | (timer_TIMA & 0x3FF); break; // 18 bits timer
                case 0b01: timer_TIMA = ((uint32_t)data << 4) | (timer_TIMA & 0xF); break; // 12 bits timer
                case 0b10: timer_TIMA = ((uint32_t)data << 6) | (timer_TIMA & 0x3F); break; // 14 bits timer
                case 0b11: timer_TIMA = ((uint32_t)data << 8) | (timer_TIMA & 0xFF) ; break; // 16 bits timer
            }
            break;
        }
        case 0xFF06: timer_TMA = data; break;
        case 0xFF07: timer_TCRL = 0xF8 | (data & 0x07); break;
        case 0xFF0F: IF = data; break;
        case 0xFFFF: IE = data; break;

        default:
            mem->write(addr, data);
            break;
    }
}

void Cpu::push(uint8_t val){
    write_mem(--SP, val);
}
uint8_t Cpu::pull(){
    return read_mem(SP++);
}

uint8_t Cpu::run(){
    cycle = 0;
    mem_cycle = 0;
    // Check if an interrupt has occured.
    if (IME != 0 && (IF & IE & 0x1F)){
        halted = false;
        for(uint8_t mask = 0x1; mask <= 0x10; mask = mask << 1){
            if((IE & mask) && (IF & mask)){
                switch (mask) {
                    case 0x01: pushPC(); PC=0x0040; IME = 0; IF &= 0xFE; cycle +=0xC; break; // V-Blank interrupt
                    case 0x02: pushPC(); PC=0x0048; IME = 0; IF &= 0xFC; cycle +=0xC; break; // LCD STAT interrput
                    case 0x04: pushPC(); PC=0x0050; IME = 0; IF &= 0xFB; cycle +=0xC; break; // Timer
                    case 0x08: pushPC(); PC=0x0058; IME = 0; IF &= 0xF7; cycle +=0xC; break; // Serial
                    case 0x10: pushPC(); PC=0x0060; IME = 0; IF &= 0xEF; cycle +=0xC; break; // Joypad
                    default:
                        break;
                }
            }
        }
    }



    if (IME_delay) { // IE instruction is taken into account after the next instruction
        IME = 1;
        IME_delay = 0;
    }
    uint8_t opcode;
    if(halted){
        opcode = 0x76;
        cycle += 4;
    }
    else{
#ifdef TRACE
        myfile << "PC=0x"<< setfill('0') << setw(4) << std::hex << PC <<";";
        myfile <<" SP=0x"<< setfill('0') << setw(4) << std::hex << SP <<";";
        myfile << " A=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)A <<";";
        myfile << " B=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)B <<";";
        myfile << " C=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)C <<";";
        myfile << " D=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)D <<";";
        myfile << " E=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)E <<";";
        myfile << " H=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)H <<";";
        myfile << " L=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)L <<";";
        myfile << " F=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)F <<";";
#endif
        opcode = read_PC_mem();
#ifdef TRACE
        myfile << " OP=0x"<< setfill('0') << setw(2) << std::hex << (uint16_t)opcode <<";";
#endif
    }

    switch (opcode) {
        case 0x00: break; // NOP

        case 0x07: RLC_Handler(&A); break;

        case 0x08:
        {   // LD [##], SP
            uint16_t addr = read_PC_mem();
            addr |= (read_PC_mem() << 8);
            write_mem(addr, (uint8_t)SP);
            write_mem(addr + 1, (uint8_t)(SP>>8));
            break;
        }
        case 0x0F: RRC_Handler(&A); break;

        case 0x17: RL_handler(&A); break;
        case 0x1F: RR_handler(&A); SF_Z(0); break;  // RRA
        case 0x27: daa_handler(); break;
        case 0x2F: A = ~A; SF_N(1); SF_H(1); break; // CPL

        case 0x01: ld_nn_Handler(&B, &C); break;

        case 0x10: /* TODO: STOP HANDLING */ PC++; break;

        case 0x11: ld_nn_Handler(&D, &E); break;
        case 0x21: ld_nn_Handler(&H, &L); break;
        case 0x31: ld_nn_Handler((uint8_t*)&SP + 1 , (uint8_t*)&SP); break;
        case 0x32: ldd_hla_Handler(); break;

        case 0x34:
        {   // HL++
            uint8_t tmp = readHL() + 1;
            writeHL(tmp);
            SF_Z(tmp == 0); SF_N(0); SF_H(!(tmp & 0xF));
            break;
        }

        case 0x3A: ldd_ahl_Handler(); break;

        case 0x3F: flag_c ^= true; SF_N(0); SF_H(0); break;

        case 0x06: ld_n_Handler(&B, read_PC_mem()); break; // ld B, #
        case 0x0E: ld_n_Handler(&C, read_PC_mem()); break;
        case 0x16: ld_n_Handler(&D, read_PC_mem()); break;
        case 0x1E: ld_n_Handler(&E, read_PC_mem()); break;
        case 0x26: ld_n_Handler(&H, read_PC_mem()); break;
        case 0x2E: ld_n_Handler(&L, read_PC_mem()); break;
        case 0x3E: ld_n_Handler(&A, read_PC_mem()); break;
        case 0x47: ld_n_Handler(&B, A); break;			   // ld B, A
        case 0x4F: ld_n_Handler(&C, A); break;
        case 0x57: ld_n_Handler(&D, A); break;
        case 0x5F: ld_n_Handler(&E, A); break;
        case 0x67: ld_n_Handler(&H, A); break;
        case 0x6F: ld_n_Handler(&L, A); break;
        case 0x7F: ld_n_Handler(&A, A); break;
        case 0x78: ld_n_Handler(&A, B); break;
        case 0x79: ld_n_Handler(&A, C); break;
        case 0x7A: ld_n_Handler(&A, D); break;
        case 0x7B: ld_n_Handler(&A, E); break;
        case 0x7C: ld_n_Handler(&A, H); break;
        case 0x7D: ld_n_Handler(&A, L); break;
        case 0x0A: ld_n_Handler(&A, read_mem(REG(B,C))); break;
        case 0x1A: ld_n_Handler(&A, read_mem(REG(D,E))); break;
        case 0x7E: ld_n_Handler(&A, read_mem(REG(H,L))); break;
        case 0xFA:{
            uint16_t addr = read_PC_mem();				 // ld A, ##[]
            addr |= read_PC_mem() << 8;
            ld_n_Handler(&A, read_mem(addr));
        } break;
        case 0x40: ld_n_Handler(&B, B); break;
        case 0x41: ld_n_Handler(&B, C); break;
        case 0x42: ld_n_Handler(&B, D); break;
        case 0x43: ld_n_Handler(&B, E); break;
        case 0x44: ld_n_Handler(&B, H); break;
        case 0x45: ld_n_Handler(&B, L); break;
        case 0x46: ld_n_Handler(&B, read_mem(REG(H,L))); break;
        case 0x48: ld_n_Handler(&C, B); break;
        case 0x49: ld_n_Handler(&C, C); break;
        case 0x4A: ld_n_Handler(&C, D); break;
        case 0x4B: ld_n_Handler(&C, E); break;
        case 0x4C: ld_n_Handler(&C, H); break;
        case 0x4D: ld_n_Handler(&C, L); break;
        case 0x4E: ld_n_Handler(&C, read_mem(REG(H,L))); break;
        case 0x50: ld_n_Handler(&D, B); break;
        case 0x51: ld_n_Handler(&D, C); break;
        case 0x52: ld_n_Handler(&D, D); break;
        case 0x53: ld_n_Handler(&D, E); break;
        case 0x54: ld_n_Handler(&D, H); break;
        case 0x55: ld_n_Handler(&D, L); break;
        case 0x56: ld_n_Handler(&D, read_mem(REG(H,L))); break;
        case 0x58: ld_n_Handler(&E, B); break;
        case 0x59: ld_n_Handler(&E, C); break;
        case 0x5A: ld_n_Handler(&E, D); break;
        case 0x5B: ld_n_Handler(&E, E); break;
        case 0x5C: ld_n_Handler(&E, H); break;
        case 0x5D: ld_n_Handler(&E, L); break;
        case 0x5E: ld_n_Handler(&E, read_mem(REG(H,L))); break;
        case 0x60: ld_n_Handler(&H, B); break;
        case 0x61: ld_n_Handler(&H, C); break;
        case 0x62: ld_n_Handler(&H, D); break;
        case 0x63: ld_n_Handler(&H, E); break;
        case 0x64: ld_n_Handler(&H, H); break;
        case 0x65: ld_n_Handler(&H, L); break;
        case 0x66: ld_n_Handler(&H, read_mem(REG(H,L))); break;
        case 0x68: ld_n_Handler(&L, B); break;
        case 0x69: ld_n_Handler(&L, C); break;
        case 0x6A: ld_n_Handler(&L, D); break;
        case 0x6B: ld_n_Handler(&L, E); break;
        case 0x6C: ld_n_Handler(&L, H); break;
        case 0x6D: ld_n_Handler(&L, L); break;
        case 0x6E: ld_n_Handler(&L, read_mem(REG(H,L))); break;

        case 0x76: halted = (IF & IE & 0x1F) == 0; break;

        case 0x80: add(B); break;
        case 0x81: add(C); break;
        case 0x82: add(D); break;
        case 0x83: add(E); break;
        case 0x84: add(H); break;
        case 0x85: add(L); break;
        case 0x86: add(readHL()); break;
        case 0x87: add(A); break;
        case 0xC6: add(read_PC_mem()); break;

        case 0x88: adc(B); break;
        case 0x89: adc(C); break;
        case 0x8A: adc(D); break;
        case 0x8B: adc(E); break;
        case 0x8C: adc(H); break;
        case 0x8D: adc(L); break;
        case 0x8E: adc(readHL()); break;
        case 0x8F: adc(A); break;
        case 0xCE: adc(read_PC_mem()); break;


        case 0x09: add16bits(&H, &L, &B, &C); break;
        case 0x19: add16bits(&H, &L, &D, &E); break;
        case 0x29: add16bits(&H, &L, &H, &L); break;
        case 0x39: add16bits(&H, &L, ((uint8_t*)&SP) + 1, ((uint8_t*)&SP)); break;

        case 0x2A: ld_inc(); break;
        case 0x22: ld_hlinc(); break;

        case 0x3C: A++; SF_Z(A== 0); SF_N(0); SF_H(!(A & 0xF)); break;
        case 0x04: B++; SF_Z(B== 0); SF_N(0); SF_H(!(B & 0xF)); break;
        case 0x0C: C++; SF_Z(C== 0); SF_N(0); SF_H(!(C & 0xF)); break;
        case 0x14: D++; SF_Z(D== 0); SF_N(0); SF_H(!(D & 0xF)); break;
        case 0x1C: E++; SF_Z(E== 0); SF_N(0); SF_H(!(E & 0xF)); break;
        case 0x24: H++; SF_Z(H== 0); SF_N(0); SF_H(!(H & 0xF)); break;
        case 0x2C: L++; SF_Z(L== 0); SF_N(0); SF_H(!(L & 0xF)); break;


        case 0x77: write_mem(REG(H,L), A); break;
        case 0x70: write_mem(REG(H,L), B); break;
        case 0x71: write_mem(REG(H,L), C); break;
        case 0x72: write_mem(REG(H,L), D); break;
        case 0x73: write_mem(REG(H,L), E); break;
        case 0x74: write_mem(REG(H,L), H); break;
        case 0x75: write_mem(REG(H,L), L); break;
        case 0x36: write_mem(REG(H,L), read_PC_mem()); break;

        case 0x02: write_mem(REG(B,C), A); break;
        case 0x12: write_mem(REG(D,E), A); break;

        case 0xEA:{
            uint16_t addr = read_PC_mem();
            addr |= read_PC_mem() << 8;
             write_mem(addr, A);
        } break;

        case 0xE0: write_mem(0xFF00 + read_PC_mem(), A); break;
        case 0xE2: write_mem(0xFF00 + C, A); break;
        case 0xF0: A = read_mem(0xFF00 + read_PC_mem()); break;
        case 0xF2: A = read_mem(0xFF00 + C); break;

        case 0x3D: dec(&A); break;
        case 0x05: dec(&B); break;
        case 0x0D: dec(&C); break;
        case 0x15: dec(&D); break;
        case 0x1D: dec(&E); break;
        case 0x25: dec(&H); break;
        case 0x2D: dec(&L); break;
        case 0x35: dec_pointer(); break;

        case 0x03: inc_nn(&B, &C); break;
        case 0x13: inc_nn(&D, &E); break;
        case 0x23: inc_nn(&H, &L); break;
        case 0x33: SP++; cycle += 4; break;

        case 0x0B: dec_nn(&B, &C); break;
        case 0x1B: dec_nn(&D, &E); break;
        case 0x2B: dec_nn(&H, &L); break;
        case 0x3B: SP--; cycle += 4; break;

        case 0x90: sub(B); break;
        case 0x91: sub(C); break;
        case 0x92: sub(D); break;
        case 0x93: sub(E); break;
        case 0x94: sub(H); break;
        case 0x95: sub(L); break;
        case 0x96: sub(readHL()); break;
        case 0x97: sub(A); break;
        case 0xD6: sub(read_PC_mem()); break;

        case 0x98: sbc(B); break;
        case 0x99: sbc(C); break;
        case 0x9A: sbc(D); break;
        case 0x9B: sbc(E); break;
        case 0x9C: sbc(H); break;
        case 0x9D: sbc(L); break;
        case 0x9E: sbc(readHL()); break;
        case 0x9F: sbc(A); break;
        case 0xDE: sbc(read_PC_mem()); break;

        case 0xA0: A &= B; FLAGS(A==0, 0, 1, 0); break;
        case 0xA1: A &= C; FLAGS(A==0, 0, 1, 0); break;
        case 0xA2: A &= D; FLAGS(A==0, 0, 1, 0); break;
        case 0xA3: A &= E; FLAGS(A==0, 0, 1, 0); break;
        case 0xA4: A &= H; FLAGS(A==0, 0, 1, 0); break;
        case 0xA5: A &= L; FLAGS(A==0, 0, 1, 0); break;
        case 0xA6: A &= readHL(); FLAGS(A==0, 0, 1, 0); break;
        case 0xA7: A &= A; FLAGS(A==0, 0, 1, 0); break;
        case 0xE6: A &= read_PC_mem(); FLAGS(A==0, 0, 1, 0); break;

        case 0xB7: or_Handler(A); break;
        case 0xB0: or_Handler(B); break;
        case 0xB1: or_Handler(C); break;
        case 0xB2: or_Handler(D); break;
        case 0xB3: or_Handler(E); break;
        case 0xB4: or_Handler(H); break;
        case 0xB5: or_Handler(L); break;
        case 0xB6: or_Handler(read_mem(REG(H,L))); break;
        case 0xF6: or_Handler(read_PC_mem()); break;

        case 0xA8: xor_Handler(B); break;
        case 0xA9: xor_Handler(C); break;
        case 0xAA: xor_Handler(D); break;
        case 0xAB: xor_Handler(E); break;
        case 0xAC: xor_Handler(H); break;
        case 0xAD: xor_Handler(L); break;
        case 0xAE: xor_Handler(read_mem(REG(H,L))); break;
        case 0xAF: xor_Handler(A); break;
        case 0xEE: xor_Handler(read_PC_mem()); break;

        case 0xB8: compareA(B); break;
        case 0xB9: compareA(C); break;
        case 0xBA: compareA(D); break;
        case 0xBB: compareA(E); break;
        case 0xBC: compareA(H); break;
        case 0xBD: compareA(L); break;
        case 0xBE: compareA(readHL()); break;
        case 0xBF: compareA(A); break;
        case 0xFE: compareA(read_PC_mem()); break;

        case 0x18:
        {
            int8_t f_pc = (int8_t)read_PC_mem();
            PC += f_pc;
            cycle += 4;
            break;
        }
        case 0xC3: JP_nn_Handler(); break;
        case 0xE9: PC = REG(H, L); break; // Jump to address contained in HL.

        case 0x20: jump_cond(!RF_Z()); break; // NZ, Jump if Z flag is reset.
        case 0x28: jump_cond(RF_Z()); break; // Z, Jump if Z flag is set.
        case 0x30: jump_cond(!RF_C()); break; // NC, Jump if C flag is reset.
        case 0x38: jump_cond(RF_C()); break; // C, Jump if C flag is set.

        case 0xC2: JP_nn_cond_Handler(!RF_Z()); break; // NZ, Jump if Z flag is reset.
        case 0xCA: JP_nn_cond_Handler(RF_Z()); break; // Z, Jump if Z flag is set.
        case 0xD2: JP_nn_cond_Handler(!RF_C()); break; // NC, Jump if C flag is reset.
        case 0xDA: JP_nn_cond_Handler(RF_C()); break; // C, Jump if C flag is set.

        case 0xC4: call_cond_handler(!RF_Z()); break; // NZ, Jump if Z flag is reset.
        case 0xCC: call_cond_handler(RF_Z()); break; // Z, Jump if Z flag is set.
        case 0xD4: call_cond_handler(!RF_C()); break; // NC, Jump if C flag is reset.
        case 0xDC: call_cond_handler(RF_C()); break; // C, Jump if C flag is set.

        case 0xC0: if(!RF_Z()) ret_handler(); cycle += 4; break;
        case 0xC8: if(RF_Z()) ret_handler(); cycle += 4; break;
        case 0xD0: if(!RF_C()) ret_handler(); cycle += 4; break;
        case 0xD8: if(RF_C()) ret_handler(); cycle += 4; break;

        case 0x37: SF_N(0); SF_H(0); SF_C(1); break; // SCF Set carry Flag

        case 0xE8:
        {   // ADD SP, #
            int8_t ad = read_PC_mem();
            FLAGS(0, 0, ((SP & 0xF) + (ad & 0xF)) > 0xF, (uint8_t)SP + (uint8_t)ad > 0xFF);
            SP += ad;
            cycle +=8;
            break;
        }
        case 0xC7: resets_handler(0x00); break;
        case 0xCF: resets_handler(0x08); break;
        case 0xD7: resets_handler(0x10); break;
        case 0xDF: resets_handler(0x18); break;
        case 0xE7: resets_handler(0x20); break;
        case 0xEF: resets_handler(0x28); break;
        case 0xF7: resets_handler(0x30); break;
        case 0xFF: resets_handler(0x38); break;

        case 0xC9: ret_handler(); break;
        case 0xCD: call_handler(); break;
        case 0xf3: IME = 0; break;
        case 0xfB: IME_delay = 1; break;
        case 0xD9: IME = 1; ret_handler(); break;

        case 0xF5: push_instruction(A, readF()); break;
        case 0xC5: push_instruction(B, C); break;
        case 0xD5: push_instruction(D, E); break;
        case 0xE5: push_instruction(H, L); break;

        case 0xF1: {uint8_t tmpF; pop(&A, &tmpF); writeF(tmpF); break;}
        case 0xC1: pop(&B, &C); break;
        case 0xD1: pop(&D, &E); break;
        case 0xE1: pop(&H, &L); break;

        case 0xF8: ldhlsp_plus_n_handler(); break;
        case 0xF9: SP = (H << 8) | L; cycle += 4; break;

        case 0xCB:
            {
                uint8_t opcode_misc = read_PC_mem();
                switch (opcode_misc) {
                    case 0x00: RLC_Handler(&B); SF_Z(B == 0); break;
                    case 0x01: RLC_Handler(&C); SF_Z(C == 0); break;
                    case 0x02: RLC_Handler(&D); SF_Z(D == 0); break;
                    case 0x03: RLC_Handler(&E); SF_Z(E == 0); break;
                    case 0x04: RLC_Handler(&H); SF_Z(H == 0); break;
                    case 0x05: RLC_Handler(&L); SF_Z(L == 0); break;
                    case 0x06:{ uint8_t tmp = readHL(); RLC_Handler(&tmp); writeHL(tmp);SF_Z(tmp == 0);  break;}
                    case 0x07: RLC_Handler(&A); SF_Z(A == 0); break;

                    case 0x08: RRC_Handler(&B); SF_Z(B == 0); break;
                    case 0x09: RRC_Handler(&C); SF_Z(C == 0); break;
                    case 0x0A: RRC_Handler(&D); SF_Z(D == 0); break;
                    case 0x0B: RRC_Handler(&E); SF_Z(E == 0); break;
                    case 0x0C: RRC_Handler(&H); SF_Z(H == 0); break;
                    case 0x0D: RRC_Handler(&L); SF_Z(L == 0); break;
                    case 0x0E:{ uint8_t tmp = readHL(); RRC_Handler(&tmp); writeHL(tmp); SF_Z(tmp == 0); break;}
                    case 0x0F: RRC_Handler(&A); SF_Z(A == 0); break;

                    case 0x10: RL_handler(&B); SF_Z(B == 0); break;
                    case 0x11: RL_handler(&C); SF_Z(C == 0); break;
                    case 0x12: RL_handler(&D); SF_Z(D == 0); break;
                    case 0x13: RL_handler(&E); SF_Z(E == 0); break;
                    case 0x14: RL_handler(&H); SF_Z(H == 0); break;
                    case 0x15: RL_handler(&L); SF_Z(L == 0); break;
                    case 0x16:{ uint8_t tmp = readHL(); RL_handler(&tmp); writeHL(tmp); SF_Z(tmp == 0); break;}
                    case 0x17: RL_handler(&A); SF_Z(A == 0); break;

                    case 0x18: RR_handler(&B); SF_Z(B == 0); break;
                    case 0x19: RR_handler(&C); SF_Z(C == 0); break;
                    case 0x1A: RR_handler(&D); SF_Z(D == 0); break;
                    case 0x1B: RR_handler(&E); SF_Z(E == 0); break;
                    case 0x1C: RR_handler(&H); SF_Z(H == 0); break;
                    case 0x1D: RR_handler(&L); SF_Z(L == 0); break;
                    case 0x1E:{ uint8_t tmp = readHL(); RR_handler(&tmp); writeHL(tmp); SF_Z(tmp == 0); break;}
                    case 0x1F: RR_handler(&A); SF_Z(A == 0);  break;

                    case 0x37: swap(&A); break;
                    case 0x30: swap(&B); break;
                    case 0x31: swap(&C); break;
                    case 0x32: swap(&D); break;
                    case 0x33: swap(&E); break;
                    case 0x34: swap(&H); break;
                    case 0x35: swap(&L); break;
                    case 0x36: {uint8_t tmp = readHL(); swap(&tmp); writeHL(tmp); break;}

                    case 0x40: check_bit(0, B); break;
                    case 0x41: check_bit(0, C); break;
                    case 0x42: check_bit(0, D); break;
                    case 0x43: check_bit(0, E); break;
                    case 0x44: check_bit(0, H); break;
                    case 0x45: check_bit(0, L); break;
                    case 0x46: check_bit(0, readHL()); break;
                    case 0x47: check_bit(0, A); break;

                    case 0x48: check_bit(1, B); break;
                    case 0x49: check_bit(1, C); break;
                    case 0x4A: check_bit(1, D); break;
                    case 0x4B: check_bit(1, E); break;
                    case 0x4C: check_bit(1, H); break;
                    case 0x4D: check_bit(1, L); break;
                    case 0x4E: check_bit(1, readHL()); break;
                    case 0x4F: check_bit(1, A); break;

                    case 0x50: check_bit(2, B); break;
                    case 0x51: check_bit(2, C); break;
                    case 0x52: check_bit(2, D); break;
                    case 0x53: check_bit(2, E); break;
                    case 0x54: check_bit(2, H); break;
                    case 0x55: check_bit(2, L); break;
                    case 0x56: check_bit(2, readHL()); break;
                    case 0x57: check_bit(2, A); break;

                    case 0x58: check_bit(3, B); break;
                    case 0x59: check_bit(3, C); break;
                    case 0x5A: check_bit(3, D); break;
                    case 0x5B: check_bit(3, E); break;
                    case 0x5C: check_bit(3, H); break;
                    case 0x5D: check_bit(3, L); break;
                    case 0x5E: check_bit(3, readHL()); break;
                    case 0x5F: check_bit(3, A); break;

                    case 0x60: check_bit(4, B); break;
                    case 0x61: check_bit(4, C); break;
                    case 0x62: check_bit(4, D); break;
                    case 0x63: check_bit(4, E); break;
                    case 0x64: check_bit(4, H); break;
                    case 0x65: check_bit(4, L); break;
                    case 0x66: check_bit(4, readHL()); break;
                    case 0x67: check_bit(4, A); break;

                    case 0x68: check_bit(5, B); break;
                    case 0x69: check_bit(5, C); break;
                    case 0x6A: check_bit(5, D); break;
                    case 0x6B: check_bit(5, E); break;
                    case 0x6C: check_bit(5, H); break;
                    case 0x6D: check_bit(5, L); break;
                    case 0x6E: check_bit(5, readHL()); break;
                    case 0x6F: check_bit(5, A); break;

                    case 0x70: check_bit(6, B); break;
                    case 0x71: check_bit(6, C); break;
                    case 0x72: check_bit(6, D); break;
                    case 0x73: check_bit(6, E); break;
                    case 0x74: check_bit(6, H); break;
                    case 0x75: check_bit(6, L); break;
                    case 0x76: check_bit(6, readHL()); break;
                    case 0x77: check_bit(6, A); break;

                    case 0x78: check_bit(7, B); break;
                    case 0x79: check_bit(7, C); break;
                    case 0x7A: check_bit(7, D); break;
                    case 0x7B: check_bit(7, E); break;
                    case 0x7C: check_bit(7, H); break;
                    case 0x7D: check_bit(7, L); break;
                    case 0x7E: check_bit(7, readHL()); break;
                    case 0x7F: check_bit(7, A); break;

                    // RESET BIT POS : 10PP PRRR
                    // Where P is bit position and R is register
                    # define reset_bit(r, x)  r & ~(0x1 << x)
                    case 0x87: A = reset_bit(A, 0); break;
                    case 0x80: B = reset_bit(B, 0); break;
                    case 0x81: C = reset_bit(C, 0); break;
                    case 0x82: D = reset_bit(D, 0); break;
                    case 0x83: E = reset_bit(E, 0); break;
                    case 0x84: H = reset_bit(H, 0); break;
                    case 0x85: L = reset_bit(L, 0); break;
                    case 0x86: {uint8_t tmp = reset_bit(readHL(), 0); writeHL(tmp); break;}

                    case 0x8F: A = reset_bit(A, 1); break;
                    case 0x88: B = reset_bit(B, 1); break;
                    case 0x89: C = reset_bit(C, 1); break;
                    case 0x8A: D = reset_bit(D, 1); break;
                    case 0x8B: E = reset_bit(E, 1); break;
                    case 0x8C: H = reset_bit(H, 1); break;
                    case 0x8D: L = reset_bit(L, 1); break;
                    case 0x8E: {uint8_t tmp = reset_bit(readHL(), 1); writeHL(tmp); break;}

                    case 0x97: A = reset_bit(A, 2); break;
                    case 0x90: B = reset_bit(B, 2); break;
                    case 0x91: C = reset_bit(C, 2); break;
                    case 0x92: D = reset_bit(D, 2); break;
                    case 0x93: E = reset_bit(E, 2); break;
                    case 0x94: H = reset_bit(H, 2); break;
                    case 0x95: L = reset_bit(L, 2); break;
                    case 0x96: {uint8_t tmp = reset_bit(readHL(), 2); writeHL(tmp); break;}

                    case 0x9F: A = reset_bit(A, 3); break;
                    case 0x98: B = reset_bit(B, 3); break;
                    case 0x99: C = reset_bit(C, 3); break;
                    case 0x9A: D = reset_bit(D, 3); break;
                    case 0x9B: E = reset_bit(E, 3); break;
                    case 0x9C: H = reset_bit(H, 3); break;
                    case 0x9D: L = reset_bit(L, 3); break;
                    case 0x9E: {uint8_t tmp = reset_bit(readHL(), 3); writeHL(tmp); break;}

                    case 0xA7: A = reset_bit(A, 4); break;
                    case 0xA0: B = reset_bit(B, 4); break;
                    case 0xA1: C = reset_bit(C, 4); break;
                    case 0xA2: D = reset_bit(D, 4); break;
                    case 0xA3: E = reset_bit(E, 4); break;
                    case 0xA4: H = reset_bit(H, 4); break;
                    case 0xA5: L = reset_bit(L, 4); break;
                    case 0xA6: {uint8_t tmp = reset_bit(readHL(), 4); writeHL(tmp); break;}

                    case 0xAF: A = reset_bit(A, 5); break;
                    case 0xA8: B = reset_bit(B, 5); break;
                    case 0xA9: C = reset_bit(C, 5); break;
                    case 0xAA: D = reset_bit(D, 5); break;
                    case 0xAB: E = reset_bit(E, 5); break;
                    case 0xAC: H = reset_bit(H, 5); break;
                    case 0xAD: L = reset_bit(L, 5); break;
                    case 0xAE: {uint8_t tmp = reset_bit(readHL(), 5); writeHL(tmp); break;}

                    case 0xB7: A = reset_bit(A, 6); break;
                    case 0xB0: B = reset_bit(B, 6); break;
                    case 0xB1: C = reset_bit(C, 6); break;
                    case 0xB2: D = reset_bit(D, 6); break;
                    case 0xB3: E = reset_bit(E, 6); break;
                    case 0xB4: H = reset_bit(H, 6); break;
                    case 0xB5: L = reset_bit(L, 6); break;
                    case 0xB6: {uint8_t tmp = reset_bit(readHL(), 6); writeHL(tmp); break;}

                    case 0xBF: A = reset_bit(A, 7); break;
                    case 0xB8: B = reset_bit(B, 7); break;
                    case 0xB9: C = reset_bit(C, 7); break;
                    case 0xBA: D = reset_bit(D, 7); break;
                    case 0xBB: E = reset_bit(E, 7); break;
                    case 0xBC: H = reset_bit(H, 7); break;
                    case 0xBD: L = reset_bit(L, 7); break;
                    case 0xBE: {uint8_t tmp = reset_bit(readHL(), 7); writeHL(tmp); break;}

                        // SET BIT POS : 10PP PRRR
                        // Where P is bit position and R is register
# define set_bit(r, x)  r | (0x1 << x)
                    case 0xC7: A = set_bit(A, 0); break;
                    case 0xC0: B = set_bit(B, 0); break;
                    case 0xC1: C = set_bit(C, 0); break;
                    case 0xC2: D = set_bit(D, 0); break;
                    case 0xC3: E = set_bit(E, 0); break;
                    case 0xC4: H = set_bit(H, 0); break;
                    case 0xC5: L = set_bit(L, 0); break;
                    case 0xC6: {uint8_t tmp = set_bit(readHL(), 0); writeHL(tmp); break;}

                    case 0xCF: A = set_bit(A, 1); break;
                    case 0xC8: B = set_bit(B, 1); break;
                    case 0xC9: C = set_bit(C, 1); break;
                    case 0xCA: D = set_bit(D, 1); break;
                    case 0xCB: E = set_bit(E, 1); break;
                    case 0xCC: H = set_bit(H, 1); break;
                    case 0xCD: L = set_bit(L, 1); break;
                    case 0xCE: {uint8_t tmp = set_bit(readHL(), 1); writeHL(tmp); break;}

                    case 0xD7: A = set_bit(A, 2); break;
                    case 0xD0: B = set_bit(B, 2); break;
                    case 0xD1: C = set_bit(C, 2); break;
                    case 0xD2: D = set_bit(D, 2); break;
                    case 0xD3: E = set_bit(E, 2); break;
                    case 0xD4: H = set_bit(H, 2); break;
                    case 0xD5: L = set_bit(L, 2); break;
                    case 0xD6: {uint8_t tmp = set_bit(readHL(), 2); writeHL(tmp); break;}

                    case 0xDF: A = set_bit(A, 3); break;
                    case 0xD8: B = set_bit(B, 3); break;
                    case 0xD9: C = set_bit(C, 3); break;
                    case 0xDA: D = set_bit(D, 3); break;
                    case 0xDB: E = set_bit(E, 3); break;
                    case 0xDC: H = set_bit(H, 3); break;
                    case 0xDD: L = set_bit(L, 3); break;
                    case 0xDE: {uint8_t tmp = set_bit(readHL(), 3); writeHL(tmp); break;}

                    case 0xE7: A = set_bit(A, 4); break;
                    case 0xE0: B = set_bit(B, 4); break;
                    case 0xE1: C = set_bit(C, 4); break;
                    case 0xE2: D = set_bit(D, 4); break;
                    case 0xE3: E = set_bit(E, 4); break;
                    case 0xE4: H = set_bit(H, 4); break;
                    case 0xE5: L = set_bit(L, 4); break;
                    case 0xE6: {uint8_t tmp = set_bit(readHL(), 4); writeHL(tmp); break;}

                    case 0xEF: A = set_bit(A, 5); break;
                    case 0xE8: B = set_bit(B, 5); break;
                    case 0xE9: C = set_bit(C, 5); break;
                    case 0xEA: D = set_bit(D, 5); break;
                    case 0xEB: E = set_bit(E, 5); break;
                    case 0xEC: H = set_bit(H, 5); break;
                    case 0xED: L = set_bit(L, 5); break;
                    case 0xEE: {uint8_t tmp = set_bit(readHL(), 5); writeHL(tmp); break;}

                    case 0xF7: A = set_bit(A, 6); break;
                    case 0xF0: B = set_bit(B, 6); break;
                    case 0xF1: C = set_bit(C, 6); break;
                    case 0xF2: D = set_bit(D, 6); break;
                    case 0xF3: E = set_bit(E, 6); break;
                    case 0xF4: H = set_bit(H, 6); break;
                    case 0xF5: L = set_bit(L, 6); break;
                    case 0xF6: {uint8_t tmp = set_bit(readHL(), 6); writeHL(tmp); break;}

                    case 0xFF: A = set_bit(A, 7); break;
                    case 0xF8: B = set_bit(B, 7); break;
                    case 0xF9: C = set_bit(C, 7); break;
                    case 0xFA: D = set_bit(D, 7); break;
                    case 0xFB: E = set_bit(E, 7); break;
                    case 0xFC: H = set_bit(H, 7); break;
                    case 0xFD: L = set_bit(L, 7); break;
                    case 0xFE: {uint8_t tmp = set_bit(readHL(), 7); writeHL(tmp); break;}

                    case 0x20: FLAGS(0, 0, 0, (B & 0x80) > 1); B <<= 1; SF_Z(B == 0); break;
                    case 0x21: FLAGS(0, 0, 0, (C & 0x80) > 1); C <<= 1; SF_Z(C == 0); break;
                    case 0x22: FLAGS(0, 0, 0, (D & 0x80) > 1); D <<= 1; SF_Z(D == 0); break;
                    case 0x23: FLAGS(0, 0, 0, (E & 0x80) > 1); E <<= 1; SF_Z(E == 0); break;
                    case 0x24: FLAGS(0, 0, 0, (H & 0x80) > 1); H <<= 1; SF_Z(H == 0); break;
                    case 0x25: FLAGS(0, 0, 0, (L & 0x80) > 1); L <<= 1; SF_Z(L == 0); break;
                    case 0x26:{
                        uint8_t tmp = readHL();
                        FLAGS(0, 0, 0, (tmp & 0x80) > 1);
                        tmp <<= 1;
                        SF_Z(tmp == 0);
                        writeHL(tmp);
                        break;
                    }
                    case 0x27: FLAGS(0, 0, 0, (A & 0x80) > 1); A <<= 1; SF_Z(A == 0); break;

                    // Arithmetic shift right (SRA)
                    case 0x28: SRA_handler(&B); break;
                    case 0x29: SRA_handler(&C); break;
                    case 0x2A: SRA_handler(&D); break;
                    case 0x2B: SRA_handler(&E); break;
                    case 0x2C: SRA_handler(&H); break;
                    case 0x2D: SRA_handler(&L); break;
                    case 0x2E:{uint8_t tmp = readHL(); SRA_handler(&tmp); writeHL(tmp); break;}
                    case 0x2F: SRA_handler(&A); break;

                    // SRL
                    case 0x38: SRL_handler(&B); break;
                    case 0x39: SRL_handler(&C); break;
                    case 0x3A: SRL_handler(&D); break;
                    case 0x3B: SRL_handler(&E); break;
                    case 0x3C: SRL_handler(&H); break;
                    case 0x3D: SRL_handler(&L); break;
                    case 0x3E:{uint8_t tmp = readHL(); SRL_handler(&tmp); writeHL(tmp); break;}
                    case 0x3F: SRL_handler(&A); break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }

#ifdef TRACE
    if (!halted) {
        myfile << " cycles :" << (uint16_t)cycle<< ";";
        myfile << std::endl;
    }
#endif
    // sync memory with cpu
    request_interrupt(mem->sync_cycle(cycle - mem_cycle));

    run_timer();
    return cycle;
}


// Instructions


inline void Cpu::JP_nn_Handler(){
    uint16_t jump_to = read_PC_mem();
    jump_to = jump_to | (read_PC_mem() << 8);
    PC = jump_to;
    cycle += 4;
}

inline void Cpu::JP_nn_cond_Handler(uint8_t cond){
    if (cond) {
        JP_nn_Handler();
    }
    else{
        read_PC_mem(); read_PC_mem();
    }
}

inline void Cpu::xor_Handler(uint8_t reg){
    A ^= reg;
    FLAGS(A == 0, 0, 0, 0);
}
inline void Cpu::or_Handler(uint8_t reg){
    A |= reg;
    FLAGS(A==0, 0, 0, 0);
}

inline void Cpu::ldd_ahl_Handler(){
    A = readHL();
    uint16_t HL_tmp = (H << 8| L) - 1;
    H = HL_tmp >> 8;
    L = (uint8_t)HL_tmp;
}

inline void Cpu::ld_nn_Handler(uint8_t * r1, uint8_t * r2){
    *r2 = read_PC_mem();
    *r1 = read_PC_mem();
}

inline void Cpu::ld_n_Handler(uint8_t * reg, uint8_t value){
    *reg = value;
}

inline void Cpu::ldd_hla_Handler(){
    write_HL_pointer(A);
    uint16_t hl = (H << 8) | L;
    hl--;
    H = hl >> 8;
    L = hl & 0xFF;
}

inline void Cpu::dec(uint8_t * reg){
    SF_H((*reg & 0xF) == 0);
    *reg = *reg - 1;
    SF_Z(*reg == 0);
    SF_N(1);
}

inline void Cpu::add(uint8_t value){
    A += value;
    FLAGS(A == 0, 0, (A & 0xF) < (value & 0xF) , (A < value));
}

inline void Cpu::adc(uint8_t value){
    uint8_t cflag = RF_C();
    uint8_t hflag = ((A & 0xF) + (value & 0xF) + cflag) > 0xF;
    A += value + cflag;
    FLAGS(A == 0, 0, hflag, (A < (value + cflag)));
}

inline void Cpu::sub(uint8_t value){
    SF_H((value & 0xF) > (A & 0xF));
    SF_C(A < value);
    A = A - value;
    SF_Z(A == 0);
    SF_N(1);
}

inline void Cpu::sbc(uint8_t value){
    int8_t cflag = RF_C();
    SF_H(((int8_t)(A & 0xF) - (int8_t)(value & 0xF) - cflag) < 0);
    SF_C(((int)(A) - (int)(value) - (int)(cflag) < 0));
    A = A - (value + cflag);
    SF_Z(A == 0);
    SF_N(1);
}

inline void Cpu::dec_pointer(){
    uint8_t val = read_mem(REG(H,L));
    SF_H((val & 0xF) == 0);
    val--;
    SF_Z(val == 0);
    SF_N(1);
    write_HL_pointer(val);
}

inline void Cpu::jump_cond(uint8_t cond){
    if(cond){
        int8_t jump = read_PC_mem();
        PC += jump;
    }
    else{
        PC++;
    }
    cycle += 4;
}

inline void Cpu::ld_inc(){ // A = [HL++]
    uint16_t addr = REG(H,L);
    A = read_mem(addr);
    addr++;
    L = addr & 0xFF;
    H = (addr >> 8) & 0xFF;

}

inline void Cpu::ld_hlinc(){ //[HL++] = A
    uint16_t addr = REG(H,L);
    write_mem(addr, A);
    addr++;
    L = addr & 0xFF;
    H = (addr >> 8) & 0xFF;

}

inline void Cpu::compareA(uint8_t reg){
    SF_H((reg & 0xF) > (A & 0xF));
    uint8_t val = A - reg;
    SF_Z(val == 0);
    SF_N(1);
    SF_C(A < reg);
}

inline void Cpu::call_handler(){
    uint16_t addr = read_PC_mem();
    addr |= (read_PC_mem() << 8);
    push((uint8_t)(PC >> 8));
    push((uint8_t)(PC & 0xFF));
    PC = addr;
    cycle += 4;
}

inline void Cpu::call_cond_handler(uint8_t cond){
    if(cond){
        call_handler();
    }
    else{
        PC += 2;
        cycle += 8;
    }
}

inline void Cpu::ret_handler(){
    uint16_t addr = pull();
    addr |= pull() << 8;
    PC = addr;
    cycle += 4;
}

inline void Cpu::dec_nn(uint8_t * r1, uint8_t * r2){
    uint16_t data = ((*r1 << 8) | *r2) -1;
    *r1 = (uint8_t)(data >> 8);
    *r2 = (uint8_t)(data);
    cycle += 4;
}

inline void Cpu::inc_nn(uint8_t * r1, uint8_t * r2){
    uint16_t data = ((*r1 << 8) | *r2) +1;
    *r1 = (uint8_t)(data >> 8);
    *r2 = (uint8_t)(data);
    cycle += 4;
}

inline void Cpu::swap(uint8_t *reg){
    *reg = ((*reg << 4) & 0xF0) | ((*reg >> 4) & 0x0F);
    FLAGS(*reg == 0, 0, 0, 0);
}

inline void Cpu::resets_handler(uint16_t addr){
    push((uint8_t)(PC >> 8));
    push((uint8_t)(PC & 0xFF));
    PC = addr;
    cycle += 4;
}

inline void Cpu::pop(uint8_t * H, uint8_t * L){
    *L = pull();
    *H = pull();
}

inline void Cpu::push_instruction(uint8_t H, uint8_t L){
    push(H);
    push(L);
    cycle += 4;
}

inline void Cpu::add16bits(uint8_t * HD, uint8_t * LD, uint8_t * HS, uint8_t * LS){
    uint16_t src = (*HS << 8) | *LS;
    uint16_t dst = (*HD << 8) | *LD;
    dst = dst + src;
    SF_N(0);
    SF_H((dst & 0xFFF) < (src & 0xFFF));
    SF_C(dst < src);
    *HD = (dst >>8) & 0xFF;
    *LD = dst & 0xFF;
    cycle += 4;
}

inline void Cpu::daa_handler(){
    /** decimal adjust accumulator (A)
     * Exemple =>A = 0x11 + 0x6 = 0x17; daa convert 0x17 to 0x23 => base10(0x17) = 23
     */
    int a = A;
    if (!flag_n){
        if (flag_h || (a & 0xF) > 9)
            a += 0x06;
        if (flag_c|| a > 0x9F)
            a += 0x60;
    }
    else{
        if (flag_h)
            a = (a - 6) & 0xFF;
        if (flag_c)
            a -= 0x60;
    }
    if(!flag_c){
        SF_C((a & 0x100) == 0x100);
    }
    A = (uint8_t)a;
    SF_Z(A == 0);
    SF_H(0);
}

inline void Cpu::RR_handler(uint8_t * reg){
    uint8_t fl = *reg & 1;
    *reg = (RF_C()) ? (*reg >> 1) | 0x80 : *reg >> 1;
    SF_Z(*reg == 0);
    SF_C(fl);
    SF_H(0);
    SF_N(0);
}

inline void Cpu::RL_handler(uint8_t * reg){
    uint8_t cflag = (*reg >> 7) & 0x01;
    *reg = *reg << 1;
    *reg = RF_C() ? *reg | 1 : *reg;
    FLAGS(0, 0, 0, cflag);
}

inline void Cpu::RRC_Handler(uint8_t * reg){
    uint8_t cflag =  *reg & 0x01;
    *reg = (cflag << 7) | (*reg >> 1);
    FLAGS(0, 0, 0, cflag);
}

inline void Cpu::SRA_handler(uint8_t * reg){
    SF_C(*reg & 0x1);
    *reg = (*reg >> 1) | (*reg & 0x80);
    SF_Z(*reg==0); SF_H(0); SF_N(0);
}

inline void Cpu::SRL_handler(uint8_t *reg){
    SF_C(*reg & 0x1);
    SF_H(0); SF_N(0);
    *reg >>= 1;
    SF_Z(*reg==0);
}

inline void Cpu::RLC_Handler(uint8_t * reg){
    FLAGS(0, 0, 0, (*reg & 0x80) > 0);
    *reg = (*reg << 1) | ((*reg >> 7) & 0x01);
}

inline void Cpu::ldhlsp_plus_n_handler(){
    int8_t ad = read_PC_mem();

    FLAGS(0, 0, ((SP & 0xF) + (ad & 0xF)) > 0xF, (uint8_t)SP + (uint8_t)ad > 0xFF);
    int16_t reg = SP + ad;
    H = (uint8_t)(reg >> 8);
    L = (uint8_t)(reg & 0xFF);
    cycle += 4;
}

inline void Cpu::run_timer(){
    timer_DIV += cycle;
    if(timer_TCRL & 0x04){ // TIMA Enable
        timer_TIMA += cycle;

        bool overflow = false;
        // check if overflow
        switch (timer_TCRL & 0x3) {
            case 0b00: overflow = (timer_TIMA & 0xFFFC0000) > 0; break; // 18 bits timer
            case 0b01: overflow = (timer_TIMA & 0xFFFFF000) > 0; break; // 12 bits timer
            case 0b10: overflow = (timer_TIMA & 0xFFFFC000) > 0; break; // 14 bits timer
            case 0b11: overflow = (timer_TIMA & 0xFFFF0000) > 0; break; // 16 bits timer
        }
        if (overflow) {
            request_interrupt(INT_TIMER);
            switch (timer_TCRL & 0x3) {
                case 0b00: timer_TIMA = ((uint32_t)timer_TMA << 10) | (timer_TIMA & 0x3FF); break; // 18 bits timer
                case 0b01: timer_TIMA = ((uint32_t)timer_TMA << 4)  | (timer_TIMA & 0xF); break; // 12 bits timer
                case 0b10: timer_TIMA = ((uint32_t)timer_TMA << 6)  | (timer_TIMA & 0x3F); break; // 14 bits timer
                case 0b11: timer_TIMA = ((uint32_t)timer_TMA << 8)  | (timer_TIMA & 0xFF); break; // 16 bits timer
            }
        }
    }
}

inline void Cpu::request_interrupt(uint8_t INT){
    IF |= INT;
}


uint8_t * Cpu::serialize(uint32_t * size){
    // first allocate the correct amount of Bytes necessary
    *size = 23;
    uint8_t * serialized_cpu = (uint8_t *)malloc(*size);
    serialized_cpu[0] = (uint8_t)SP;
    serialized_cpu[1] = (uint8_t)(SP >> 8);
    serialized_cpu[2] = A;
    serialized_cpu[3] = B;
    serialized_cpu[4] = C;
    serialized_cpu[5] = D;
    serialized_cpu[6] = E;
    serialized_cpu[7] = H;
    serialized_cpu[8] = L;
    serialized_cpu[9] = readF();
    serialized_cpu[10] = (uint8_t)PC;
    serialized_cpu[11] = (uint8_t)(PC >> 8);
    serialized_cpu[12] = (uint8_t)timer_DIV;
    serialized_cpu[13] = (uint8_t)(timer_DIV >> 8);
    serialized_cpu[14] = timer_TIMA;
    serialized_cpu[15] = timer_TCRL;
    serialized_cpu[16] = halted;
    serialized_cpu[17] = IME;
    serialized_cpu[18] = IME_delay;
    serialized_cpu[19] = IF;
    serialized_cpu[20] = IE;
    serialized_cpu[21] = cycle;
    serialized_cpu[22] = mem_cycle;
    return serialized_cpu;
}

int Cpu::unserialize(uint8_t * serialized_cpu){
    SP = *((uint16_t*)serialized_cpu);
    A = serialized_cpu[2];
    B = serialized_cpu[3];
    C = serialized_cpu[4];
    D = serialized_cpu[5];
    E = serialized_cpu[6];
    H = serialized_cpu[7];
    L = serialized_cpu[8];
    writeF(serialized_cpu[9]);
    PC =  *((uint16_t*)(serialized_cpu + 10));
    timer_DIV = *((uint16_t*)(serialized_cpu + 12));
    timer_TIMA = serialized_cpu[14];
    timer_TCRL = serialized_cpu[15];
    halted = serialized_cpu[16];
    IME = serialized_cpu[17];
    IME_delay = serialized_cpu[18];
    IF = serialized_cpu[19];
    IE = serialized_cpu[20];
    cycle = serialized_cpu[21];
    mem_cycle = serialized_cpu[22];
    return 0;
}
