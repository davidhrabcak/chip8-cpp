#include <stdio.h>
#include "chip8.h"
#include <string.h>
#include <cstdlib>

// followed tutorial on https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
// used Documentation on http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.4


unsigned short opcode;
unsigned short stack[16]; // stack for jumps
unsigned short sp; // stack pointer
unsigned int I; // index register
bool sound_played;

unsigned char memory[4096];


unsigned char chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void chip8::clear_sound_played() {
    sound_played = false;
}

bool chip8::get_sound_played() {
    return sound_played;
}

bool chip8::get_df() {
    return df;
}

void chip8::set_df(bool val) {
    df = val;
}

int chip8::get_display(int pos) {
    return graphics[pos];
}

int chip8::get_display_pos(int x, int y) {
    return graphics[y + (64*x)];
}

void chip8::set_sp(int val) {
    sp = val;
}

int chip8::get_sp() {
    return sp;
}

void chip8::initialize() {
    this->reg.pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;

    // clear display
    memset(graphics, 0, 2048*sizeof(unsigned char));

    //clear stack
    memset(stack, 0, 16*sizeof(unsigned short));

    // clear V registers
    memset(this->reg.V, 0, 16*sizeof(unsigned char));

    // clear memory
    memset(memory, 0, 4096*sizeof(unsigned char));

    //clear sound flag
    sound_played = false;

    // load fontset
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }

    // reset timers
    sound_timer = 0;
    delay_timer = 0;

}


// by JamesGriffin on github
void chip8::load(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f == NULL) printf("Invalid ROM filename!\n");
    
    // file size
    fseek(f, 0, SEEK_END);
    long rom_size = ftell(f);
    rewind(f);

     // Allocate memory to store rom
    char* rom_buffer = (char*) malloc(sizeof(char) * rom_size);
    if (rom_buffer == NULL) {
        printf("Failed to allocate memory for ROM\n");
    }

    // Copy ROM into buffer
    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, f);
    if (result != (long unsigned int)rom_size) {
        printf("Failed to read ROM\n");
    }

    // Copy buffer to memory
    if ((4096-512) > rom_size){
        for (int i = 0; i < rom_size; ++i) {
            memory[i + 0x200] = rom_buffer[i];   // Load into memory starting
                                               // at 0x200 ()
        }
    }
    else {
        printf("ROM too large to fit in memory\n");
    }
    free(rom_buffer);
    fclose(f);
}

int chip8::emulate_cycle() {
    // fetch opcode
    opcode = memory[this->reg.pc] << 8 | memory[this->reg.pc + 1];
    // decode + execute opcode
    switch(opcode & 0xF000) {
        case 0x0000: {
            if ((opcode & 0x0F00) != 0) {
                printf("Warning: instruction %X is unsupported, skipping\n", opcode);
                this->reg.pc +=2;
                break; // compatibility instruction - no operation needed
            }
            int inst = opcode & 0x00FF;
            if (inst == 0xE0) { // 0x00E0 CLS
                memset(graphics, 0, 2048*sizeof(unsigned char));
                this->reg.pc += 2;
            } else if (inst == 0xEE) { //0x00EE RET
                sp--;
                if (sp < 0) {
                    printf("Invalid stack pointer");
                    return -3;
                }
                this->reg.pc = stack[sp];
                this->reg.pc +=2;
            } else if (inst == 0x00) {
                printf("Possible memory overwrite, instruction given was 0x0.\n");
                return -1;
            } else {
                printf("Unknown opcode: 0x%X\n", opcode);
                return -1;
            }
            break;
        }
        case 0x1000: { // 0x1nnn JP
            int set = opcode & 0x0FFF;
            this->reg.pc = set;
            break;
        }

        case 0x2000: { // 0x2nnn CALL
            stack[sp] = this->reg.pc;
            sp++;
            if (sp > 15) {
                printf("Stack overflow\n");
                return -4;
            }
            this->reg.pc = opcode & 0x0FFF;
            break;
        }

        case 0x3000: { // 0x3xkk SE
            int index = (opcode & 0x0F00) >> 8;
            if (this->reg.V[index] == (opcode & 0x00FF)) this->reg.pc += 2;
            this->reg.pc += 2;
            break;
        }
        
        case 0x4000: { // 0x4xkk SNE
            int index = (opcode & 0x0F00) >> 8;
            if (this->reg.V[index] != (opcode & 0x00FF)) this->reg.pc += 2;
            this->reg.pc += 2;
            break;
        }

        case 0x5000: { // 0x5xy0 SE
            int x = (opcode & 0x0F00) >> 8;
            int y = (opcode & 0x00F0) >> 4;
            if (this->reg.V[x] == this->reg.V[y]) this->reg.pc +=2;
            this->reg.pc += 2;
            break;
        }
        
        case 0x6000: { // 0x6xkk LD
            int x = (opcode & 0x0F00) >> 8;
            int val = opcode & 0x00FF;
            this->reg.V[x] = val;
            this->reg.pc += 2;
            break;
        }

        case 0x7000: { // 0x7xkk ADD
            int x = (opcode & 0x0F00) >> 8;
            int val = opcode & 0x00FF;
            this->reg.V[x] += val;
            this->reg.pc += 2;
            break;
        }

        case 0x8000: {
            int ins = opcode & 0x000F;
            int x = (opcode & 0x0F00) >> 8;
            int y = (opcode & 0x00F0) >> 4;
            switch(ins) {
                case 0x0000: { // 0x8xy0 SET
                    this->reg.V[x] = this->reg.V[y];
                    this->reg.pc +=2;
                    break;
                }

                case 0x0001: { // 0x8xy1 OR
                    this->reg.V[x] = this->reg.V[x] | this->reg.V[y];
                    this->reg.pc += 2;
                    break;
                }

                case 0x0002: { // 0x8xy2 AND
                    this->reg.V[x] = this->reg.V[x] & this->reg.V[y];
                    this->reg.pc += 2;
                    break;
                }

                case 0x0003: { // 0x8xy3 XOR
                    this->reg.V[x] = this->reg.V[x] ^ this->reg.V[y];
                    this->reg.pc += 2;
                    break;
                }

                case 0x0004: { // 0x8xy4 ADD
                    this->reg.V[x] = (this->reg.V[x] + this->reg.V[y]) & 0xFF;
                    this->reg.V[0xF] = ((this->reg.V[x] + this->reg.V[y]) > 255) ? 1 : 0;
                    this->reg.pc +=2;
                    break;
                }

                case 0x0005: { // 0x8xy5 SUB
                    this->reg.V[x] = this->reg.V[x] - this->reg.V[y];
                    this->reg.V[0xF] = (this->reg.V[y] > this->reg.V[x]) ? 1 : 0;
                    this->reg.pc +=2;
                    break;
                }

                case 0x0006: { // 0x8xy6 SHR
                    this->reg.V[x] = this->reg.V[x] >> 1;
                    this->reg.V[0xF] = this->reg.V[x] & 0x0001;
                    this->reg.pc +=2;
                    break;
                }

                case 0x0007: { // 0x8xy7 SUBM
                    this->reg.V[x] = this->reg.V[y] - this->reg.V[x];
                    this->reg.V[0xF] = (this->reg.V[x] > this->reg.V[y]) ? 1 : 0;
                    this->reg.pc += 2;
                    break;
                }

                case 0x000E: { // 0x8xyE SHL
                    this->reg.V[x] = this->reg.V[x] << 1;
                    this->reg.V[0xF] = this->reg.V[x] >> 7;
                    this->reg.pc +=2;
                    break;
                }
                default: {
                    printf("Unknown opcode: 0x%X\n", opcode);
                    this->reg.pc += 2;
                    return -1;
                }
            };
            break;
        }

        case 0x9000: { // 0x9xy0 SNE
            int x = (opcode & 0x0F00) >> 8;
            int y = (opcode & 0x00F0) >> 4;
            if (this->reg.V[x] != this->reg.V[y]) this->reg.pc += 2;
            this->reg.pc += 2;
            break;
        }
        
        case 0xA000: { // 0xAnnn LD
            int val = opcode & 0x0FFF;
            I = val;
            this->reg.pc += 2;
            break;
        }

        case 0xB000: { // 0xBnnn JP
            this->reg.pc = (opcode & 0x0FFF) + this->reg.V[0];
            break;
        }
        
        case 0xC000: { // 0xCxkk RND
            int x = (opcode & 0x0F00) >> 8;
            int random = rand() % 255;
            this->reg.V[x] = random & (opcode & 0x00FF);
            this->reg.pc += 2;
            break;
        }

        // from Triavanicus on github
        case 0xD000: { // 0xDxyn DRW
            unsigned short x = this->reg.V[(opcode & 0x0F00) >> 8];
            unsigned short y = this->reg.V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

            this->reg.V[0xF] = 0;
            for (int yline = 0; yline < height; yline++)
            {
                pixel = memory[I + yline];
                for(int xline = 0; xline < 8; xline++)
                {
                    if((pixel & (0x80 >> xline)) != 0) {
                        if(graphics[(x + xline + ((y + yline) * 64))] == 1) {
                            this->reg.V[0xF] = 1;
                        }
                        graphics[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }

            this->df = true;
            this->reg.pc += 2;
            break;
        }

        case 0xE000: {
            int ins = opcode & 0x00FF;
            if (ins == 0x009E) { // 0xEx9E SKP
                int x = (opcode & 0x0F00) >> 8;
                if (key[this->reg.V[x]] == true) this->reg.pc +=2;

            } else if (ins == 0x00A1) { // 0xExA1 SKNP
                int x = (opcode & 0x0F00) >> 8;
                if (key[this->reg.V[x]] == false) this->reg.pc +=2;

            } else {
                printf("Unknown opcode: 0x%X\n", opcode);
                return -1;
            }
            this->reg.pc += 2;
            break;
        }
        
        case 0xF000: {
            switch(opcode & 0x00FF) {
                case 0x0007: { // 0xFx07 LD
                    int x = (opcode & 0x0F00) >> 8;
                    this->reg.V[x] = delay_timer;
                    this->reg.pc += 2;
                    break;
                }

                case 0x000A: { // 0xFx0A LD
                bool key_press = false;
                int x = (opcode & 0x0F00) >> 8;
                for (int i = 0; i < 16; i++) {
                    if (key[i] == true) {
                        key_press = true;
                        this->reg.V[x] = i;
                    }
                }
                if (key_press) {
                    this->reg.pc += 2; // interrupt until key pressed
                }
                break;
                }

                case 0x0015: { // 0xFx15 LD
                    int x = (opcode & 0x0F00) >> 8;
                    delay_timer = this->reg.V[x];
                    this->reg.pc += 2;
                    break;
                }

                case 0x0018: { // 0xFx18 LD
                    int x = (opcode & 0x0F00) >> 8;
                    sound_timer = this->reg.V[x];
                    this->reg.pc += 2;
                    break;
                }

                case 0x001E: { // 0xFx1E ADD
                    int x = (opcode & 0x0F00) >> 8;
                    I += this->reg.V[x];
                    this->reg.pc += 2;
                    break;
                }

                case 0x0029: { // 0xFx29 LD
                    int x = (opcode & 0x0F00) >> 8;
                    I = x*5;
                    this->reg.pc += 2;
                    break;
                }

                case 0x0033: { // 0xFx33 LD
                    int x = (opcode & 0x0F00) >> 8;
                    int binary = this->reg.V[x];
                    memory[I] = binary / 100;
                    memory[I + 1] = (binary / 10) % 10;
                    memory[I + 2] = binary % 10;
                    this->reg.pc += 2;
                    break;
                }

                case 0x0055: { // 0xFx55 LD
                    int x = (opcode & 0x0F00) >> 8;
                    //printf("current x = %d\n", x);
                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = this->reg.V[i];
                        //printf("Loaded V%d into memory[%d]\n", i, I+i);
                    }
                    I += x + 1;
                    this->reg.pc += 2;
                    break;
                }

                case 0x0065: { // 0xFx65 LD
                    int x = (opcode & 0x0F00) >> 8;
                    //printf("current x = %d\n", x);
                    for (int i = 0; i <= x; i++) {
                        this->reg.V[i] = memory[I + i];
                        //printf("Set value %d into V%d\n", memory[I + i], i);
                    }
                    I += x + 1;
                    this->reg.pc += 2;
                    break;
                }

                default: {
                    printf("Unknown opcode: 0x%X\n", opcode);
                    return -1;
                }
            };
            break;
            }
        

        default: {
            printf("Unknown opcode: 0x%X\n", opcode);
            return -1;
        }
    };

    // update timers
    if (delay_timer > 0)  delay_timer--;
    if (sound_timer > 0) {
        if (sound_timer == 1) sound_played = true;
        sound_timer--;
    }
    //Debug prints
    //printf("PC: %d\n", this->reg.pc);
    //printf("ST: %d\n", sound_timer);
    //printf("V2: %d\n", this->reg.V[2]);
    
    return 1;
}

