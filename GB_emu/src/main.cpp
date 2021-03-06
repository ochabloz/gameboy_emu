//
//  main.cpp
//  GB_emu
//
//  Created by Olivier Chabloz on 28.02.17.
//  Copyright © 2017 Olivier Chabloz. All rights reserved.
//

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <chrono>

#include "Cart.hpp"
#include "Memory_map.hpp"
#include "Cpu.hpp"
#include "PPU.hpp"
#include "APU.hpp"

extern "C"{
  #include "argparse.h"
  #include "iniparse.h"
}

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#define DEFAULT_SCREEN_SCALE 4

// BGB palette (greenish tint)
#define DEFAULT_COLOR0 0xe0f8d0
#define DEFAULT_COLOR1 0x88c070
#define DEFAULT_COLOR2 0x346856
#define DEFAULT_COLOR3 0x081820

void audio_callback(void * data, uint8_t * stream, int len);

#define MAP_COLOR(X) SDL_MapRGBA(screen->format,(X & 0xff0000)>>16, (X & 0xff00)>>8,X & 0xff, 0xff)


int main(int argc, char * argv[]) {
    bool disable_audio = true;

    SDL_Surface * screen;
    // parse arguments :
    argparse_t parser = argparse_init(argc, (const char **)argv);
    const char * boot_rom  = argparse_get_opt(parser, 'b');
	const char * save_path = argparse_get_opt(parser, 'r');
    const char * cart_name = argparse_get_positional(parser, 0);
    bool fullscreen = (bool)argparse_get_long_opt(parser, "fullscreen");
	bool disable_screen = (bool)argparse_get_long_opt(parser, "disable_screen");
	bool help_text = (bool)argparse_get_long_opt(parser, "help");
    const char * scale = argparse_get_opt(parser, 's');
    const char * cfg_file = argparse_get_opt(parser, 'c');

	if (help_text) {
		puts("By no means feature complete GAME BOY emulator.");
		puts("Options:");
		puts("\t[-b bootrom.bin]\t Will start the emulation by executing the given bootrom.");
		puts("\t[-r path]\t Directory where to read and write saved game");
		puts("\t[-s SCALE]\t\t A scale factor for the screen (float accepted).");
        puts("\t[-c cfg_file]\t\t Configuration file to use");
		puts("\t[--fullscreen]\t\t Open the emulator in fullscreen mode.");
		puts("\t[--disable_screen]\t No window will be opened. Useful when running test rom.");
		puts("\t[--help]\t\t Print this message and exit.");
		puts("\nTypical Usage:");
		printf("\t%s romfile.gb\n", argv[0]);
		return EXIT_SUCCESS;
	}

	double screen_scale = DEFAULT_SCREEN_SCALE;
    if(scale != nullptr){
        if (atof(scale) > 0.1) {
            screen_scale = atof(scale);
        }
        else printf("SCALE %s is not a valid argument\n", scale);
    }
	int screen_width = ceilf(160.0 * screen_scale);
	int screen_height = ceilf(144.0 * screen_scale);

    iniparse_t ini = nullptr;
    if(cfg_file == nullptr){
        ini = iniparse_init("GB.cfg");
    }
    else{
        ini = iniparse_init(cfg_file);
    }

    if (cart_name == nullptr) {
        printf("Usage : %s romfile.gb\n", argv[0]);
        return EXIT_SUCCESS;
    }

    Cart cart(cart_name, save_path);
    if(!cart.status()){
        printf("the cartridge header doesn't match the checksum\n");
        return EXIT_SUCCESS;
    }
    if(cart.status() == 0xFF){
        printf("The file provided as rom is not readable by the program. Exitting.\n");
        return EXIT_FAILURE;
    }

    if(!fullscreen) fullscreen = iniparse_get_bool(ini, "GB:fullscreen") > 0;

    screen = SDL_CreateRGBSurface(0, 160, 144, 32, 0, 0, 0, 0);
    PPU ppu(cart.status(), (uint32_t *)screen->pixels, 0, 0, 0, 0);

    const char * current_palette = iniparse_get_key(ini, "GB:palette");
    uint32_t c[4] = {DEFAULT_COLOR0, DEFAULT_COLOR1, DEFAULT_COLOR2, DEFAULT_COLOR3};
    char palette_setting[50] = {0};
    for(int i = 0; i < 4; i++) {
        sprintf(palette_setting, "%s:c%d", current_palette, i);
        if(iniparse_get_key(ini, palette_setting) != NULL) c[i] = iniparse_get_int(ini, palette_setting);
        ppu.set_palette( MAP_COLOR(c[0]), MAP_COLOR(c[1]), MAP_COLOR(c[2]), MAP_COLOR(c[3]));
    }

    APU apu;
    Memory_map *memory;
    Cpu * processor;
    if(boot_rom == nullptr) boot_rom = iniparse_get_key(ini, "GB:bios");
    memory = new Memory_map(&cart, &ppu, &apu, boot_rom);
    processor = new Cpu(memory, memory->has_bootrom());

    // initialise audio
    static SDL_AudioSpec audio_spec;
    audio_spec.format = AUDIO_S16;
    audio_spec.callback = audio_callback;
    audio_spec.userdata = &apu;
    audio_spec.freq = 44100;
    audio_spec.samples = 2048;
    if(!disable_audio){
        if (SDL_OpenAudio(&audio_spec, NULL) < 0) {
            fprintf(stderr, "Cannot open audio: %s\n", SDL_GetError());
            return EXIT_FAILURE;
        }
    }
    // Initialize Screen

    SDL_Window* window = NULL;
    SDL_Renderer * renderer = NULL;
    SDL_Texture* screen_texture = NULL;

    //Initialize SDL
    if(!disable_screen){
        if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO)< 0 ){
    		fprintf(stderr, "Cannot init SDL: %s\n", SDL_GetError());
            return EXIT_FAILURE;
        }
        char title[16];
        cart.get_title(title);
        window = SDL_CreateWindow(title,
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  screen_width,
                                  screen_height,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
                                  ((fullscreen) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
                                  );

        renderer = SDL_CreateRenderer(window, -1, 0);
        SDL_PauseAudio(0);
    }

    bool quit = false;
    uint64_t cycles = 0;

    float drift = 0;

    while(!quit){
        //uint32_t time = SDL_GetTicks();
        auto time = std::chrono::high_resolution_clock::now();
        while (!ppu.screen_complete) {
            cycles += processor->run();
        }
        SDL_Event evt;
        while (SDL_PollEvent(&evt) && !disable_screen) {
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
                case SDL_QUIT: 
					quit = true;
					break;

                default: break;
            }
        }

        cycles = 0;


        //Apply the image stretched
		if (!disable_screen) {
			screen_texture = SDL_CreateTextureFromSurface(renderer, screen);
			SDL_Rect stretchRect;
			stretchRect.y = 0;
			SDL_GetWindowSize(window, &stretchRect.w, &stretchRect.h);
			float scale = ((stretchRect.h / 144.0));
			float scale_w = ((stretchRect.w / 160.0));
			if (scale_w < scale) {
				stretchRect.h = 144 * scale_w;
				scale = scale_w;
			}
			stretchRect.x = stretchRect.w;
			stretchRect.w = scale * 160.0;
			stretchRect.x = (stretchRect.x - stretchRect.w) / 2;
			stretchRect.x = (stretchRect.x < 0) ? 0 : stretchRect.x;

			SDL_RenderCopy(renderer, screen_texture, NULL, &stretchRect);
			SDL_RenderPresent(renderer);
			SDL_DestroyTexture(screen_texture);

			auto time_after = std::chrono::high_resolution_clock::now();
			uint32_t delta = std::chrono::duration_cast<std::chrono::milliseconds>(time_after - time).count();
			delta -= floor(drift);
			drift -= floor(drift);
			if (delta < 16) {
				SDL_Delay(16 - delta);

				time_after = std::chrono::high_resolution_clock::now();
				delta = std::chrono::duration_cast<std::chrono::microseconds>(time_after - time).count();
				float deltaf = delta / 1000.0;
				drift += (16.57 - deltaf);
			}
		}
		ppu.get_screen();
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
