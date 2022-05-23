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

#ifndef CHIP8_H_INCLUDED
#define CHIP8_H_INCLUDED

#include <stdio.h>
#include <SDL.h>

#define MEMOSZ 4096 //0xFFF
#define GRAPHICS 64*32
#define REGISTER 0x10 // 16
#define STACKS 0x10 // 16
#define KEYNUM 0x10 // 16


typedef struct Chip8                    // begin emulator struct
{

    unsigned short int opcode;          // Operator code for the CPU
    unsigned char memory[MEMOSZ];       // Memory of the Chip 8. 4k is the total
    unsigned char V[REGISTER];          // CPU register
    unsigned short int I;               // Index register
    unsigned short int pc;              // Program counter
    unsigned char graphics[GRAPHICS];        // Graphics of Chip 8. 2048 pixels(64x32) black and white
    unsigned char delay_timer;          // Count at 60Hz
    unsigned char sound_timer;          // When the time gets to 0, it buzzer a sound
    unsigned short int stack[STACKS];   // To remember the current location before a jump, has 16 levels
    unsigned short int ps;              // A pointer to the current location at the stack
    unsigned char key[KEYNUM];          // CHIP 8 has 16 commands
    FILE *rom;                          // To open the game

}CH;                                    // end emulator struct


/*
 * Memory Map
 * 0x000 - 0x1FF - Chip 8 interpreter
 * 0x050 - 0x0A0 - Used for the built in 4x5 pixel font set (0-F)
 * 0x200 - 0xFFF - Program ROM and work RAM
 */


void prepare_emulator(CH *, char *);  // To reset everything, pass the font and game to the memory of the emulator
void cycles(CH *);                    // The cycles of the CPU
void render(CH *);                    // To render the graphics
void start();                         // To input the game
void initalize(char *);               // To initialize the game


#endif // CHIP8_H_INCLUDED
