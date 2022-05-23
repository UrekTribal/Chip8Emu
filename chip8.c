/*
 * This is a Chip 8 Emulator
 *
 * Copyright (C) 2014 Urek Tribal.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip8.h"

#define FONTNUM 80
#define SCREEN_WIDTH 640                                // Width of the window
#define SCREEN_HEIGHT 320                               // Height of the window
#define SCREEN_BPP 32                                   // Bits per pixel
#define W 64                                            // Width of the emulator screen
#define H 32                                            // Height of the emulator screen
#define MEMORYBEGIN 0x200                               // Location to being the counter

SDL_Window *window = NULL;
SDL_Surface *surface = NULL;
int color = 1;                                          // Used as a flag to see if the emulator shall or not draw on the screen
int scale = 10;                                         // The size of each rectangle

unsigned char ch_font[FONTNUM] =                        // Font set
{
  0xF0, 0x90, 0x90, 0x90, 0xF0,                         // 0
  0x20, 0x60, 0x20, 0x20, 0x70,                         // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0,                         // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0,                         // 3
  0x90, 0x90, 0xF0, 0x10, 0x10,                         // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0,                         // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0,                         // 6
  0xF0, 0x10, 0x20, 0x40, 0x40,                         // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0,                         // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0,                         // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90,                         // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0,                         // B
  0xF0, 0x80, 0x80, 0x80, 0xF0,                         // C
  0xE0, 0x90, 0x90, 0x90, 0xE0,                         // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0,                         // E
  0xF0, 0x80, 0xF0, 0x80, 0x80                          // F
};

void prepare_emulator(CH *C8, char *name)
{
    int i;
    C8->pc = MEMORYBEGIN;                                 // The system expects the application to load at memory location 0x200
    C8->ps = 0;                                           // Reset pointer to the stack
    C8->opcode = 0;                                       // Reset opcode
    C8->I = 0;                                            // Reset index register
    C8->delay_timer = 0;
    C8->sound_timer = 0;

    for(i = 0; i < MEMOSZ; i++)                           // Reset memory
        C8->memory[i] = 0;

    for(i = 0; i < STACKS; i++)                           // Reset stack
        C8->stack[i] = 0;

    for(i = 0; i < KEYNUM; i++)                           // Reset button
        C8->key[i] = 0;

    for(i = 0; i < REGISTER; i++)                         // Reset register
        C8->V[i] = 0;

    for(i = 0; i < GRAPHICS; i++)                         // Reset graphics
        C8->graphics[i] = 0;

    for(i = 0; i < FONTNUM; i++)                          // Load the font set into the memory of the Chip-8
        C8->memory[i] = ch_font[i];

    C8->rom = fopen(name, "rb");                          // Open ROM

    if(C8->rom == NULL){
        printf("Error. Game not found!");
        fclose(C8->rom);
        return;
    }

    fseek(C8->rom, 0L, SEEK_END);                           // Get size of ROM
    int romSize = ftell(C8->rom);                           // Set romSize to the size of ROM
    rewind(C8->rom);

    unsigned char *buffer = malloc(romSize);                // Create a pointer to a memory with the size of the ROM
    fread(buffer, romSize, 1, C8->rom);                     // Read the ROM into the pointer
    fclose(C8->rom);

    for(i = 0; i <= romSize; i++)                           // Load the ROM into the memory starting at 0x200 + i
        C8->memory[i+MEMORYBEGIN] = buffer[i];

    free(buffer);
    srand(time(NULL));                                      // Srand necessary for a single instruction
}

void cycles(CH *C8)
{
    int i, pressed;
    unsigned short int vx, vy;
    C8->opcode = C8->memory[C8->pc] << 8 | C8->memory[C8->pc + 1]; // Must merge both bytes as they are 1 byte long and must be 2 bytes long
    switch(C8->opcode & 0xF000){
        case 0x0000:
            switch(C8->opcode & 0x00FF)
            {
            case 0x00E0:                                     // 00E0: Clears the screen
                    for(i = 0; i < GRAPHICS; i++)
                        C8->graphics[i] = 0x0;
                    color = 1;
                    C8->pc += 2;
                break;
                case 0x00EE:                                     // 00EE: Returns from a subroutine
                    C8->ps--;                                 // Decrease stack pointer to avoid overwriting
                    C8->pc = C8->stack[C8->ps];                  // Put the address into the program counter
                    C8->pc += 2;
                break;
                default:
                    printf("Unknow opcode: 0x%x\n", C8->opcode);
            }
        break;
        case 0x1000:                                              // 1NNN: Jumps to address NNN
            C8->pc = C8->opcode & 0x0FFF;
        break;
        case 0x2000:                                              // 2NNN: Calls subroutine at NNN
            C8->stack[C8->ps] = C8->pc;                           // Store address on pointer stack
            C8->ps++;                                             // Increase stack pointer to avoid overwriting
            C8->pc = C8->opcode & 0x0FFF;                         // Set program counter to the address at NNN
        break;
        case 0x3000:                                              // 3XNN: Skips the next instruction if VX equals NN
            if((C8->V[(C8->opcode & 0x0F00) >> 8]) == (C8->opcode & 0x00FF))
                C8->pc += 4;
            else
                C8->pc += 2;
        break;
        case 0x4000:                                              // 4XNN: Skips the next instruction if VX doesn't equal NN
            if((C8->V[(C8->opcode & 0x0F00) >> 8]) != (C8->opcode & 0x00FF))
                C8->pc += 4;
            else
                C8->pc += 2;
        break;
        case 0x5000:                                              // 5XYO: Skips the next instruction if VX equals VY
            if((C8->V[(C8->opcode & 0x0F00) >> 8]) == (C8->V[(C8->opcode & 0x00F0) >> 4]))
                C8->pc += 4;
            else
                C8->pc += 2;
        break;
        case 0x6000:                                              // 6XNN: Sets VX to NN
            C8->V[(C8->opcode & 0x0F00) >> 8] = C8->opcode & 0x00FF;
            C8->pc += 2;
        break;
        case 0x7000:                                              // 7XNN: Adds NN to VX
            C8->V[(C8->opcode & 0x0F00) >> 8] += C8->opcode & 0x00FF;
            C8->pc += 2;
        break;
        case 0x8000:
            switch(C8->opcode & 0x000F)
            {
                case 0x0000:                                      // 8XY0: Sets VX to the value of VY
                    C8->V[(C8->opcode & 0x0F00) >> 8] = C8->V[(C8->opcode & 0x00F0) >> 4];
                    C8->pc += 2;
                break;
                case 0x0001:                                      // 8XY1: Sets VX to VX or VY
                    C8->V[(C8->opcode & 0x0F00) >> 8] |= C8->V[(C8->opcode & 0x00F0) >> 4];
                    C8->pc += 2;
                break;
                case 0x0002:                                      // 8XY2: Sets VX to VX and VY
                    C8->V[(C8->opcode & 0x0F00) >> 8] &= C8->V[(C8->opcode & 0x00F0) >> 4];
                    C8->pc += 2;
                break;
                case 0x0003:                                      // 8XY3: Sets VX to VX xor VY
                    C8->V[(C8->opcode & 0x0F00) >> 8] ^= C8->V[(C8->opcode & 0x00F0) >> 4];
                    C8->pc += 2;
                break;
                case 0x0004:                                       // 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't
                    if(C8->V[(C8->opcode & 0x00F0) >> 4] > (0xFF - C8->V[(C8->opcode & 0x0F00) >> 8]))
                        C8->V[0xF] = 1;
                    else
                        C8->V[0xF] = 0;
                    C8->V[(C8->opcode & 0x0F00) >> 8] += C8->V[(C8->opcode & 0x00F0) >> 4];
                    C8->pc += 2;
                break;
                case 0x0005:                                      // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                    if(C8->V[(C8->opcode & 0x00F0) >> 4] > C8->V[(C8->opcode & 0x0F00) >> 8]) // If VY > VX, there is a borrow
                        C8->V[0xF] = 0;
                    else
                        C8->V[0xF] = 1;
                    C8->V[(C8->opcode & 0x0F00) >> 8] -= C8->V[(C8->opcode & 0x00F0) >> 4];
                    C8->pc += 2;
                break;
                case 0x0006:                                      // 8XY6: Shifts VX right by one: VF is set to the value of the least significant bit of VX before the shift
                    C8->V[0xF] = C8->V[(C8->opcode & 0x0F00) >> 8] & 0x1;
                    C8->V[(C8->opcode & 0x0F00) >> 8] >>=  1;
                    C8->pc += 2;
                break;
                case 0x0007:                                      // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                    if(C8->V[(C8->opcode & 0x0F00) >> 8] > C8->V[(C8->opcode & 0x00F0) >> 4]) // If VX > VY , there is a borrow
                        C8->V[0xF] = 0;
                    else
                        C8->V[0xF] = 1;
                    C8->V[(C8->opcode & 0x0F00) >> 8] = C8->V[(C8->opcode & 0x00F0) >> 4] - C8->V[(C8->opcode & 0x0F00) >> 8];
                    C8->pc += 2;
                break;
                case 0x000E:                                      // 8XYE: Shifts VX left by one. VG is set to the value of the most significant bit of VX before the shift
                    C8->V[0xF] = C8->V[(C8->opcode & 0x0F00) >> 8] >> 7;
                    C8->V[(C8->opcode & 0x0F00) >> 8] <<= 1;
                    C8->pc += 2;
                break;
                default:
                    printf("Unknow opcode: 0x%x\n", C8->opcode);
            }
        break;
        case 0x9000:                                              // 9XYO: Skips the next instruction if VX doesn't equal VY
            if(C8->V[(C8->opcode & 0x0F00) >> 8] != C8->V[(C8->opcode & 0x00F0) >> 4])
                C8->pc += 4;
            else
                C8->pc += 2;
        break;
        case 0xA000:                                              // ANNN: Sets I to the address NNN
            C8->I = C8->opcode & 0x0FFF;
            C8->pc += 2;
        break;
        case 0xB000:                                              // BNNN: Jumps to the address NNN plus V0
            C8->pc = (C8->opcode & 0x0FFF) + C8->V[0x0];
        break;
        case 0xC000:                                              // CXNN: Sets VX to a random number and NN
            C8->V[(C8->opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (C8->opcode & 0x00FF);
            C8->pc += 2;
        break;
        case 0xD000:                                              // DXYN: Sprites stored in memory at location in index register, maximum 8bits wide. Wraps around the screen.
            vx = C8->V[(C8->opcode & 0x0F00) >> 8];
            vy = C8->V[(C8->opcode & 0x00F0) >> 4];
            unsigned short int height = C8->opcode & 0x000F;
            unsigned short int pixel;
            int yline;
            int xline;
            C8->V[0xF] = 0;

            for(yline = 0; yline < height; yline++){              // If when drawn, clears a pixel, register VF is set to 1 otherwise it is zero. All drawing is XOR drawing.
                pixel = C8->memory[C8->I + yline];
                for(xline = 0; xline < 8; xline++){
                    if((pixel & (0x80 >> xline)) != 0){
                        if(C8->graphics[vx + xline + ((vy + yline) * 64)] == 1)
                            C8->V[0xF] = 1;
                        C8->graphics[vx + xline + ((vy + yline) * 64)] ^= 1;
                    }
                }
            }
            color = 1;
            C8->pc += 2;
        break;
        case 0xE000:
            switch(C8->opcode & 0x00FF)
            {
                case 0x009E:                                // EX9E: Skips the next instruction if the key stored in VX is pressed
                    if(C8->key[C8->V[(C8->opcode & 0x0F00) >> 8]] == 1)
                    {
                        C8->key[C8->V[(C8->opcode & 0x0F00) >> 8]] = 0;
                        C8->pc += 4;
                    }
                    else
                        C8->pc += 2;
                break;
                case 0x00A1:                                 // EXA1: Skips the next instruction if the key stored in VX isn't pressed
                    if(C8->key[C8->V[(C8->opcode & 0x0F00) >> 8]] != 1)
                        C8->pc += 4;
                    else
                        C8->pc += 2;
                break;
                default:
                    printf("Unknow opcode: 0x%x\n", C8->opcode);
            }
        break;
        case 0xF000:
            switch(C8->opcode & 0x00FF)
            {
                case 0x0007:                                      // FX07: Sets VX to the value of the delay timer
                    C8->V[(C8->opcode & 0x0F00) >> 8] = C8->delay_timer;
                    C8->pc += 2;
                break;
                case 0x000A:                                      // FX0A: A key press is awaited, and then stored in VX
                    pressed = 0;
                    for(i = 0; i < KEYNUM; i++){
                        if(C8->key[i] != 0)
                        {
                            C8->key[i] = 0;
                            C8->V[(C8->opcode & 0x0F00) >> 8] = i;
                            pressed = 1;
                        }
                    }
                    if(pressed == 0)
                        return;
                    C8->pc += 2;
                break;
                case 0x0015:                                      // FX15: Sets the delay timer to VX
                    C8->delay_timer = C8->V[(C8->opcode & 0x0F00) >> 8];
                    C8->pc += 2;
                break;
                case 0x0018:                                      // FX18: Sets the sound timer to VX
                    C8->sound_timer = C8->V[(C8->opcode & 0x0F00) >> 8];
                    C8->pc += 2;
                break;
                case 0x001E:                                      // FX1E: Adds VX to I, VF is set to 1 when range overflow, and 0 when there isn't.
                    if(C8->I + C8->V[(C8->opcode & 0x0F00) >> 8] > 0xFFF)
                        C8->V[0xF] = 1;
                    else
                        C8->V[0xF] = 0;
                    C8->I += C8->V[(C8->opcode & 0x0F00) >> 8];
                    C8->pc += 2;
                break;
                case 0x0029:                                      // FX29: Sets I to the location of the sprite for the character in VX. Characters 0-F are represented by a 4X5 font
                    C8->I = C8->V[(C8->opcode & 0x0F00) >> 8] * 5;
                    C8->pc += 2;
                break;
                case 0x0033:                                      // FX33: Stores the Binary-coded decimal representation of VX, with the most significant of three digits at the address I
                    C8->memory[C8->I]   = C8->V[(C8->opcode & 0x0F00) >> 8] / 100;
                    C8->memory[C8->I+1] = (C8->V[(C8->opcode & 0x0F00) >> 8] /10) % 10;
                    C8->memory[C8->I+2] = (C8->V[(C8->opcode & 0x0F00) >> 8] %100) % 10;
                    C8->pc += 2;
                break;
                case 0x0055:                                      // FX55: Stores V0 to VX in memory starting at address I
                    for(i = 0; i <= C8->V[(C8->opcode & 0x0F00) >> 8]; i++)
                        C8->memory[C8->I+i] = C8->V[i];
                    C8->pc += 2;
                break;
                case 0x0065:                                      // FX65: Fills V0 to VX with values from memory starting at address I
                    for(i = 0; i <= C8->V[(C8->opcode & 0x0F00) >> 8]; i++)
                        C8->V[i] = C8->memory[C8->I + i];
                    C8->pc += 2;
                break;
                default:
                    printf("Unknow opcode: 0x%x\n", C8->opcode);
            }
        break;
        default:
                    printf("Unknow opcode: 0x%x\n", C8->opcode);
    }
    if(C8->delay_timer > 0)
        --C8->delay_timer;
    if(C8->sound_timer > 0)
        --C8->sound_timer;

    SDL_Delay(2);
}

void render(CH *C8)
{
        int y, x;
        for (y = 0; y < H; y++)                         // Render the emulator on screen using SDL
        {
            for (x = 0; x < W; x++)
            {
                SDL_Rect pixel;                         // To creates a rectangular area on screen
                pixel.x = x * scale;                    // Set the x, y, height and width location on screen
                pixel.y = y * scale;
                pixel.w = scale;
                pixel.h = scale;

                uint32_t pixel_color;

                if (C8->graphics[(y * W) + x])          // If this is number on the graphics is 1, set it white, else it's black
                {                                       // If in doubt, refer to instruction set DXYN
                    pixel_color = 0xFFFFFF;
                }
                else
                {
                    pixel_color = 0x0;
                }
                SDL_FillRect(surface, &pixel, pixel_color); // Fill the rectangle with black or white
                }
            }
        SDL_UpdateWindowSurface(window);                // Update the screen
    color = 0;                                          // Set the color 0 to not keep drawing over and over
}

void start()
{
    char game[50];
    printf("Write the name of the game: ");
    scanf("%s", game);
    getchar();

    initalize(game);
}

void initalize(char *game)
{
    CH C8;
    prepare_emulator(&C8, game);                        // Resets everything

    if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
        return;

    window = SDL_CreateWindow("CHIP-8 EMULATOR BY VIATA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if(window == NULL)
        return;

    surface = SDL_GetWindowSurface(window);

    SDL_Event event;                                   // To get inputs
    for(;;)
    {
        SDL_PollEvent(&event);

            if(event.type == SDL_KEYDOWN)              // Set inputs
            {
                switch(event.key.keysym.sym)
                {
                    case SDLK_1: C8.key[0x1] = 1; break;
                    case SDLK_2: C8.key[0x2] = 1; break;
                    case SDLK_3: C8.key[0x3] = 1; break;
                    case SDLK_4: C8.key[0xC] = 1; break;
                    case SDLK_q: C8.key[0x4] = 1; break;
                    case SDLK_w: C8.key[0x5] = 1; break;
                    case SDLK_e: C8.key[0x6] = 1; break;
                    case SDLK_r: C8.key[0xD] = 1; break;
                    case SDLK_a: C8.key[0x7] = 1; break;
                    case SDLK_s: C8.key[0x8] = 1; break;
                    case SDLK_d: C8.key[0x9] = 1; break;
                    case SDLK_f: C8.key[0xE] = 1; break;
                    case SDLK_z: C8.key[0xA] = 1; break;
                    case SDLK_x: C8.key[0x0] = 1; break;
                    case SDLK_c: C8.key[0xB] = 1; break;
                    case SDLK_v: C8.key[0xF] = 1; break;
                    case SDLK_ESCAPE: exit(1); break;
                }
            }
            else if (event.type == SDL_KEYUP)
            {
                switch(event.key.keysym.sym)
                {
                    case SDLK_1: C8.key[0x1] = 0; break;
                    case SDLK_2: C8.key[0x2] = 0; break;
                    case SDLK_3: C8.key[0x3] = 0; break;
                    case SDLK_4: C8.key[0xC] = 0; break;
                    case SDLK_q: C8.key[0x4] = 0; break;
                    case SDLK_w: C8.key[0x5] = 0; break;
                    case SDLK_e: C8.key[0x6] = 0; break;
                    case SDLK_r: C8.key[0xD] = 0; break;
                    case SDLK_a: C8.key[0x7] = 0; break;
                    case SDLK_s: C8.key[0x8] = 0; break;
                    case SDLK_d: C8.key[0x9] = 0; break;
                    case SDLK_f: C8.key[0xE] = 0; break;
                    case SDLK_z: C8.key[0xA] = 0; break;
                    case SDLK_x: C8.key[0x0] = 0; break;
                    case SDLK_c: C8.key[0xB] = 0; break;
                    case SDLK_v: C8.key[0xF] = 0; break;
                    }
                }


        if(color == 1)                                 // Draws only when required
            render(&C8);
        cycles(&C8);                                   // To keep the cycles running all the time
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}
