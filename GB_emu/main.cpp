//
//  main.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 28.02.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include <iostream>

#include <vector>

#include "Cart.hpp"
#include "Memory_map.hpp"
#include "Cpu.hpp"
#include "PPU.hpp"
#include "APU.hpp"
#include <SDL2/SDL.h>

#include <stdlib.h>
#include <unistd.h>

#include <getopt.h>




void audio_callback(void * data, uint8_t * stream, int len);

int main(int argc, char * const argv[]) {
    double screen_scale = 4;
    int screen_width = ceilf(160.0 * screen_scale);
    int screen_height = ceilf(144.0 * screen_scale);
    bool fullscreen = false;
    int c;
    char * cart_name = nullptr;
    char * boot_rom  = nullptr;
    // parse arguments :
    while ((c = getopt(argc, argv, "fr:b:s:")) != -1) {
        switch (c) {
            case 'f':
                fullscreen = true;
                break;
            case 's':
                if (atof(optarg)) {
                    screen_scale = atof(optarg);
                    screen_width = 160.0 * screen_scale;
                    screen_height = 144.0 * screen_scale;
                }
                break;
            case 'r':
                cart_name = optarg;
                break;
            case 'b':
                boot_rom = optarg;
            default:
                break;
        }
    }
    
    if (cart_name == nullptr) {
        printf("Usage : gb_emu -r romfile.gb\n");
        return EXIT_SUCCESS;
    }
    
    Cart cart(cart_name);
    if(!cart.status()){
        printf("the cartridge header doesn't match the checksum\n");
        return EXIT_SUCCESS;
    }
    //The window we'll be rendering to
    SDL_Window* window = NULL;
    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;
    
    //Initialize SDL
    if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO)< 0 ){
        return EXIT_FAILURE;
    }
    char title[16];
    cart.get_title(title);
    window = SDL_CreateWindow( title, SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                              screen_width, screen_height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
                              ((fullscreen) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
                              );
    
    //Get window surface
    screenSurface = SDL_GetWindowSurface(window);
    
    
    uint32_t time = SDL_GetTicks();
    bool quit = false;
    PPU ppu;
    APU apu;
    Memory_map *memory;
    Cpu * processor;
    if(boot_rom == nullptr){
        memory = new Memory_map(&cart, &ppu, &apu);
        processor = new Cpu(memory, false);
    }
    else{
        memory = new Memory_map(&cart, &ppu, &apu, boot_rom);
        processor = new Cpu(memory, true);
    }
    
    // initialise audio
    static SDL_AudioSpec audio_spec;
    audio_spec.format = AUDIO_S16;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = &apu;
    audio_spec.freq = 44100;
    audio_spec.samples = 2048;
    if (SDL_OpenAudio(&audio_spec, NULL) < 0) {
        fprintf(stderr, "Cannot open audio: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    
    uint64_t cycles = 0;
    SDL_PauseAudio(0);
    while(!quit){
        time = SDL_GetTicks();
        while (!ppu.screen_complete) {
            cycles += processor->run();
        }
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                {
                    uint8_t bit_pos = 0x00;
                    switch(evt.key.keysym.sym){
                        case SDLK_RIGHT:    bit_pos = 0x01 << 0x00; break;
                        case SDLK_LEFT:     bit_pos = 0x01 << 0x01; break;
                        case SDLK_UP:       bit_pos = 0x01 << 0x02; break;
                        case SDLK_DOWN:     bit_pos = 0x01 << 0x03; break;

                        case SDLK_s:        bit_pos = 0x10 << 0x00; break;
                        case SDLK_a:        bit_pos = 0x10 << 0x01; break;
                        case SDLK_BACKSPACE:bit_pos = 0x10 << 0x02; break;
                        case SDLK_RETURN:   bit_pos = 0x10 << 0x03; break;

                        case SDLK_ESCAPE:   quit = true;
                        default:            bit_pos = 0x00; break;
                    }
                    if(evt.type == SDL_KEYUP)
                        memory->joypad |= bit_pos;
                    else
                        memory->joypad &= ~(bit_pos);
                    break;
                }
                case SDL_QUIT: quit = true; break;
                
                default: break;
            }
        }
        
        cycles = 0;
        uint32_t delta = (SDL_GetTicks() - time);
        if(delta < 13){
            SDL_Delay(13 - delta);
        }
        {
            //Apply the image stretched
            SDL_Rect stretchRect;
            
            stretchRect.y = 0;
            SDL_GetWindowSize(window, &stretchRect.w, &stretchRect.h);
            screenSurface = SDL_GetWindowSurface(window);
            float scale = floor(stretchRect.h / 140.0 * 10.0) / 10.0;
            stretchRect.x = stretchRect.w;
            stretchRect.w = scale * 160.0;
            stretchRect.x = (stretchRect.x - stretchRect.w) / 2;
            stretchRect.x = (stretchRect.x < 0) ? 0 : stretchRect.x;
            SDL_BlitScaled(ppu.get_screen(), NULL, screenSurface, &stretchRect );
            SDL_UpdateWindowSurface(window);
        }
    }
    return EXIT_SUCCESS;
}


void audio_callback(void * data, uint8_t * stream, int len){
    APU apu_data = *(APU*)data;
    uint16_t usr_data[4096 * 2];
    memset(stream, 0, len);
    apu_data.generate_channel_1(usr_data, len);
    SDL_MixAudio(stream, (uint8_t*)usr_data, len, SDL_MIX_MAXVOLUME);
}
