#include "path-trace.h"
#include <SDL.h>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace PathTrace;

int main()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        cerr << "\nUnable to initialize SDL:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);
    SDL_Surface * screen = SDL_SetVideoMode(1024, 768, 32, SDL_DOUBLEBUF);
    {
        cerr << "\nUnable to set video mode:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    bool done = false;
    while(!done)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
        }
    }
    return EXIT_SUCCESS;
}

