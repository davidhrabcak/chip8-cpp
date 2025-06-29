#include "chip8.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdio.h>
using namespace std;


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 640;
const int PIXEL_SIZE = 20;
const int DISPLAY_WIDTH = 64;
const int DISPLAY_HEIGHT = 32;

const SDL_Keycode keymap[16] = {
    SDLK_x, // 0
    SDLK_1, // 1
    SDLK_2, // 2
    SDLK_3, // 3
    SDLK_q, // 4
    SDLK_w, // 5
    SDLK_e, // 6
    SDLK_a, // 7
    SDLK_s, // 8
    SDLK_d, // 9
    SDLK_z, // A
    SDLK_c, // B
    SDLK_4, // C
    SDLK_r, // D
    SDLK_f, // E
    SDLK_v  // F
};

// main chip8 emulation
chip8 my_chip;

// SDL objects
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Initialize SDL and create a window
bool init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("64x32 Display",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

// clean up SDL
void clean_up() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void handle_key_presses(bool key[]) {
    SDL_Event event;

    // poll pending events
    
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) exit(0);
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            bool isDown = (event.type == SDL_KEYDOWN);

            for (int i = 0; i < 16; ++i) {
                if (event.key.keysym.sym == keymap[i]) {
                    key[i] = isDown;
                    break;
                }
            }
        }
    }
}

void draw_frame(const unsigned char graphics[64 * 32]) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //  black
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // white

    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            if (graphics[y * DISPLAY_WIDTH + x]) {
                SDL_Rect pixel = {
                    x * PIXEL_SIZE,
                    y * PIXEL_SIZE,
                    PIXEL_SIZE,
                    PIXEL_SIZE
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }

    SDL_RenderPresent(renderer);
}
void error_handler(int errnum) {
    if ((errnum == -1) || (errnum == -4)) { // unknown opcode
                printf("Stopping simulation, error unrecoverable.\n");
                exit(0);
            }
            else if (errnum == -3) { // sp < 0
                printf("Restoring stack pointer\n");
                my_chip.set_sp(my_chip.get_sp() + 1);
            }
            else if (errnum == -4) {
                printf("Restoring stack pointer\n");
                my_chip.set_sp(my_chip.get_sp() - 1);
            }
}

int main(int argc, const char* argv[]) {
    if (!init_SDL()) return 1;
    bool running = true;
    my_chip.initialize();
    if (argc > 1) {
        printf("Loading \"%s\"...\n", argv[1]);
        my_chip.load(argv[1]);
    } else {
        my_chip.load("Ibm_logo.ch8");
    }

    while (running) {
        int run = my_chip.emulate_cycle();
        if (run < 0) error_handler(run);
        if (my_chip.get_df()) {
            draw_frame(my_chip.graphics);
        }
        handle_key_presses(my_chip.key);
        SDL_Delay(16); // 60 hz
        }

        


    clean_up();
    return 0;
}