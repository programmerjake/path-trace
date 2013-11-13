#include "path-trace.h"
#include <SDL.h>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace PathTrace;

Object * makeWorld()
{
    static Material matEmitR = Material(Color(0, 0, 0), 0, Color(2, 0, 0));
    static Material matEmitG = Material(Color(0, 0, 0), 0, Color(0, 2, 0));
    static Material matEmitB = Material(Color(0, 0, 0), 0, Color(0, 0, 2));
    static Material matDiffuseWhite = Material(Color(1, 1, 1), 1);
    Object * spheres[] =
    {
        new Sphere(Vector3D(1, 25, -3), 20, &matEmitR),
        new Sphere(Vector3D(0, 25, -2), 20, &matEmitG),
        new Sphere(Vector3D(-1, 25, -3), 20, &matEmitB),
        new Sphere(Vector3D(0, -6, -3), 5, &matDiffuseWhite),
        new Sphere(Vector3D(0, -0.5, -3), 0.5, &matDiffuseWhite),
    };
    Object * retval = spheres[0];
    for(int i=1;i<sizeof(spheres)/sizeof(spheres[0]);i++)
    {
        retval = new Union(retval, spheres[i]);
    }
    return retval;
}

int main()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        cerr << "\nUnable to initialize SDL:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);
    SDL_Surface * screen = SDL_SetVideoMode(320, 240, 32, SDL_DOUBLEBUF);
    if(screen == NULL)
    {
        cerr << "\nUnable to set video mode:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    while(SDL_LockSurface(screen) != 0)
        ;
    for(int y = 0; y < screen->h; y++)
    {
        for(int x = 0; x < screen->w; x++)
        {
            int r = ((x + y) % 3 == 0) ? 0xFF : 0;
            int g = ((x + y) % 3 == 0) ? 0xFF : 0;
            int b = ((x + y) % 3 == 0) ? 0xFF : 0;
            *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
        }
    }
    SDL_UnlockSurface(screen);
    SDL_Flip(screen);
    bool done = false;
    bool rendered = false;
    int y = 0;
    Object * const world = makeWorld();
    AutoDestruct<Object> autoDestruct1(world);
    while(!done)
    {
        SDL_Event event;
        while(!done)
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
                Color rayColor = tracePixel(world, x, y, screen->w, screen->h, 100);
                int r = max(0, min(0xFF, (int)floor(0x100 * rayColor.x)));
                int g = max(0, min(0xFF, (int)floor(0x100 * rayColor.y)));
                int b = max(0, min(0xFF, (int)floor(0x100 * rayColor.z)));
                *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
            }
            SDL_UnlockSurface(screen);
            SDL_Flip(screen);
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

