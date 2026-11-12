// surprise.cpp
#include "surprise.h"
#include "surprise_image.h"
#include <SDL2/SDL.h>
#include <iostream>

void showSurpriseImage() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << "\n";
        return;
    }

    // Create a window (change 600, 600 to whatever size you want)
    SDL_Window* window = SDL_CreateWindow("SURPRISE!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600, SDL_WINDOW_SHOWN);
    if (!window) { SDL_Quit(); return; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // Load the image from the embedded binary array
    SDL_RWops* rw = SDL_RWFromConstMem(surprise_bmp, surprise_bmp_len);
    SDL_Surface* surface = SDL_LoadBMP_RW(rw, 1);
    
    if (!surface) {
        std::cerr << "Failed to load embedded BMP: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
        }
        SDL_RenderClear(renderer);
        if (texture) {
            SDL_Rect destRect = {0, 0, 600, 600}; // Match window size
            SDL_RenderCopy(renderer, texture, NULL, &destRect);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // Limit to ~60 FPS
    }

    if (texture) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}