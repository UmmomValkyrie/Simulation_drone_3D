#include "drone.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>


void seekEvent() {
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT: active = 0; break;
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) rightPressed = 1;
                if(event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)  leftPressed  = 1;
                if(event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)    upPressed    = 1;
                if(event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)  downPressed  = 1;
                if(event.key.keysym.sym == SDLK_ESCAPE) active = 0;
                break;
            case SDL_KEYUP:
                if(event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) rightPressed = 0;
                if(event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)  leftPressed  = 0;
                if(event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)    upPressed    = 0;
                if(event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)  downPressed  = 0;
                break;
        }
    } 
}
