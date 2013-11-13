#include "path-trace.h"
#include <SDL.h>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace PathTrace;

Object * makeWorld()
{
    static Material matEmitGreen = Material(Color(0, 0, 0), 0, Color(0, 1, 0));
    return new Sphere(Vector3D(0, 0, -3), 1, &matEmitGreen);
}

int main()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        cerr << "\nUnable to initialize SDL:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);
    SDL_Surface * screen = SDL_SetVideoMode(1024, 768, 32, SDL_DOUBLEBUF);
    if(screen == NULL)
    {
        cerr << "\nUnable to set video mode:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    bool done = false;
    bool rendered = false;
    int y = 0;
    Object * const world = makeWorld();
    AutoDestruct<Object> autoDestruct1(world);
    while(!done)
    {
        SDL_Event event;
        while(true)
        {
            if(rendered)
            {
                if(!SDL_WaitEvent(&event))
                {
                    break;
                }
            }
            else
            {
                if(!SDL_PollEvent(&event))
                {
                    break;
                }
            }
            switch(event.type)
            {
            case SDL_QUIT:
                done = true;
                break;
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE)
                {
                    done = true;
                }
                break;
            default:
                break;
            }
        }
        if(!rendered)
        {
            while(SDL_LockSurface(screen) != 0)
                ;
            for(int x = 0; x < screen->w; x++)
            {
                Color rayColor = tracePixel(world, x, y, screen->w, screen->h);
                int r = max(0, min(0xFF, (int)floor(0x100 * rayColor.x)));
                int g = max(0, min(0xFF, (int)floor(0x100 * rayColor.y)));
                int b = max(0, min(0xFF, (int)floor(0x100 * rayColor.z)));
                *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
            }
            SDL_UnlockSurface(screen);
            y++;
            if(y >= screen->h)
            {
                y = 0;
                rendered = true;
            }
        }
    }
    return EXIT_SUCCESS;
}

