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
    static Material matGlass = Material(Color(0.1, 0.1, 0.1), 0, Color(0, 0, 0), Color(0.9, 0.9, 0.9), 1.3, 1);
    static Material matMirror = Material(Color(0.99, 0.99, 0.99), 0);
    Object * spheres[] =
    {
        new Sphere(Vector3D(1, 20, -3), 15, &matEmitR),
        new Sphere(Vector3D(0, 20, -2), 15, &matEmitG),
        new Sphere(Vector3D(-1, 20, -3), 15, &matEmitB),
        new Sphere(Vector3D(0, -6, -3), 5, &matDiffuseWhite),
        new Sphere(Vector3D(1, -0.5, -3), 0.5, &matDiffuseWhite),
        new Sphere(Vector3D(-1, -0.5, -3), 0.5, &matMirror),
    };
    Object * retval = spheres[0];
    for(unsigned i = 1; i < sizeof(spheres) / sizeof(spheres[0]); i++)
    {
        retval = new Union(retval, spheres[i]);
    }
    return retval;
}

const int ScreenWidth = 320, ScreenHeight = 240;
const char * const ProgramName = "Path Trace Test";

int main()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        cerr << "\nUnable to initialize SDL:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);
    SDL_Surface * screen = SDL_SetVideoMode(ScreenWidth, ScreenHeight, 32, SDL_SWSURFACE);
    if(screen == NULL)
    {
        cerr << "\nUnable to set video mode:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    while(SDL_LockSurface(screen) != 0)
        ;
    for(int y = 0; y < ScreenHeight; y++)
    {
        for(int x = 0; x < ScreenWidth; x++)
        {
            int r = ((x + y) % 20 == 0) ? 0xFF : 0;
            int g = ((x + y) % 20 == 0) ? 0xFF : 0;
            int b = ((x + y) % 20 == 0) ? 0xFF : 0;
            *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
        }
    }
    SDL_UnlockSurface(screen);
    SDL_Flip(screen);
    bool done = false;
    bool rendered = false;
    int y = 0;
    int count = 1;
    const int maxCount = 50000;
    const int rayCount = 100;
    Color * screenBuffer = new Color[ScreenWidth * ScreenHeight];
    Object * const world = makeWorld();
    AutoDestruct<Object> autoDestruct1(world);
    SDL_WM_SetCaption(ProgramName, NULL);
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
            bool updatedFrame = false;
            while(SDL_LockSurface(screen) != 0)
                ;
            for(int x = 0; x < ScreenWidth; x++)
            {
                Color & c = screenBuffer[x + ScreenWidth * y];
                if(count <= 1)
                {
                    c = Color(0, 0, 0);
                }
                Color rayColor = tracePixel(world, x, y, ScreenWidth, ScreenHeight, rayCount);
                c += rayColor;
                int r = max(0, min(0xFF, (int)floor(0x100 * c.x / count)));
                int g = max(0, min(0xFF, (int)floor(0x100 * c.y / count)));
                int b = max(0, min(0xFF, (int)floor(0x100 * c.z / count)));
                *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
            }
            y++;
            if(y >= ScreenHeight)
            {
                y = 0;
                count++;
                updatedFrame = true;
                if(count > maxCount / rayCount)
                {
                    rendered = true;
                    count = 1;
                }
            }
            if(!rendered)
            {
                for(int x = 0; x < ScreenWidth; x++)
                {
                    int r = ((x + y) % 20 == 0) ? 0xFF : 0;
                    int g = ((x + y) % 20 == 0) ? 0xFF : 0;
                    int b = ((x + y) % 20 == 0) ? 0xFF : 0;
                    *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
                }
            }
            SDL_UnlockSurface(screen);
            SDL_Flip(screen);
            if(updatedFrame)
            {
                char str[512];
                sprintf(str, "%s : rays/pixel = %i", ProgramName, (rendered ? maxCount / rayCount : count - 1) * rayCount);
                SDL_WM_SetCaption(str, NULL);
            }
        }
    }
    return EXIT_SUCCESS;
}

