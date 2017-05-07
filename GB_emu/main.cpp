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


int main(int argc, const char * argv[]) {
    if (argc <= 1) {
        printf("Usage : gb_emu romfile.gb");
        return 0;
    }
    Cart cart(argv[1]);
    //The window we'll be rendering to
    SDL_Window* window = NULL;
    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;
    
    //Initialize SDL
    if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)< 0 ){
        return 1;
    }
    char title[16];
    cart.get_title(title);
    window = SDL_CreateWindow( title, SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
    
    //Get window surface
    screenSurface = SDL_GetWindowSurface(window);
    
    uint32_t time = SDL_GetTicks();
    bool quit = false;
    PPU ppu;
    APU apu;
    Memory_map memory(&cart, &ppu, &apu);
    Cpu processor(&memory);
    uint64_t cycles = 0;
    while(!quit){
        time = SDL_GetTicks();
        while (!ppu.screen_complete) {
            cycles += processor.run();
        }
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_KEYUP:
                {
                    if(evt.key.keysym.sym == SDLK_LEFT){
                        memory.d_pad |= 0x01 << 0x1;
                    }
                    else if(evt.key.keysym.sym == SDLK_RIGHT){
                        memory.d_pad |= 0x01 << 0x0;
                    }
                    else if(evt.key.keysym.sym == SDLK_UP){
                        memory.d_pad |= 0x01 << 0x2;
                    }
                    else if(evt.key.keysym.sym == SDLK_DOWN){
                        memory.d_pad |= 0x01 << 0x3;
                    }
                    
                    else if(evt.key.keysym.sym == SDLK_s){
                        memory.btns |= 0x01 << 0x0;
                    }
                    else if(evt.key.keysym.sym == SDLK_a){
                        memory.btns |= 0x01 << 0x1;
                    }
                    else if(evt.key.keysym.sym == SDLK_RETURN){
                        memory.btns |= 0x01 << 0x3;
                    }
                    else if(evt.key.keysym.sym == SDLK_BACKSPACE){
                        memory.btns |= 0x01 << 0x2;
                    }
                    else if (evt.key.keysym.sym == SDLK_ESCAPE){
                        quit = true;
                    }
                    break;
                }
                case SDL_KEYDOWN:
                {
                    if(evt.key.keysym.sym == SDLK_LEFT){
                        memory.d_pad &= ~(0x01 << 0x1);
                    }
                    else if(evt.key.keysym.sym == SDLK_RIGHT){
                        memory.d_pad &= ~(0x01 << 0x0);
                    }
                    else if(evt.key.keysym.sym == SDLK_UP){
                        memory.d_pad &= ~(0x01 << 0x2);
                    }
                    else if(evt.key.keysym.sym == SDLK_DOWN){
                        memory.d_pad &= ~(0x01 << 0x3);
                    }
                    
                    else if(evt.key.keysym.sym == SDLK_s){
                        memory.btns &= ~(0x01 << 0x0);
                    }
                    else if(evt.key.keysym.sym == SDLK_a){
                        memory.btns &= ~(0x01 << 0x1);
                    }
                    else if(evt.key.keysym.sym == SDLK_RETURN){
                        memory.btns &= ~(0x01 << 0x3);
                    }
                    else if(evt.key.keysym.sym == SDLK_BACKSPACE){
                        memory.btns &= ~(0x01 << 0x2);
                    }
                    break;
                }
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
    
    
    return 0;
}

