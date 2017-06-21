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

//Screen dimension constants
const int SCREEN_WIDTH = 160 * 4;
const int SCREEN_HEIGHT = 144 * 4;


void audio_callback(void * data, uint8_t * stream, int len);

int main(int argc, const char * argv[]) {
    if (argc <= 1) {
        printf("Usage : gb_emu romfile.gb");
        return EXIT_SUCCESS;
    }
    Cart cart(argv[1]);
    //The window we'll be rendering to
    SDL_Window* window = NULL;
    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;
    
    //Initialize SDL
    if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO)< 0 ){
        return 1;
    }
    char title[16];
    cart.get_title(title);
    window = SDL_CreateWindow( title, SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
    
    //Get window surface
    screenSurface = SDL_GetWindowSurface(window);
    
    
    // initialise audio
    static SDL_AudioSpec audio_spec;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = NULL;
    audio_spec.freq = 44100;
    if (SDL_OpenAudio(&audio_spec, NULL) < 0) {
        fprintf(stderr, "Cannot open audio: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    uint32_t time = SDL_GetTicks();
    bool quit = false;
    PPU ppu;
    APU apu;
    Memory_map memory(&cart, &ppu, &apu);
    Cpu processor(&memory);
    uint64_t cycles = 0;
    SDL_PauseAudio(0);
    while(!quit){
        time = SDL_GetTicks();
        while (!ppu.screen_complete) {
            cycles += processor.run();
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
                        memory.joypad |= bit_pos;
                    else
                        memory.joypad &= ~(bit_pos);
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
            stretchRect.x = 0;
            stretchRect.y = 0;
            stretchRect.w = SCREEN_WIDTH;
            stretchRect.h = SCREEN_HEIGHT;
            SDL_BlitScaled(ppu.get_screen(), NULL, screenSurface, &stretchRect );
            SDL_UpdateWindowSurface(window);
        }
    }
    
    
    return EXIT_SUCCESS;
}


void audio_callback(void * data, uint8_t * stream, int len){
    uint8_t usr_data[512];
    SDL_MixAudio(stream, usr_data, 512, SDL_MIX_MAXVOLUME);
}
