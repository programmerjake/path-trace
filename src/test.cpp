#include "path-trace.h"
#include <SDL.h>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace PathTrace;
const bool multiThreaded = true;
const int rendererCount = 4;
const int maxCount = 50000;
const int rayCount = 200;
const int ScreenWidth = 320, ScreenHeight = 240;
const char * const ProgramName = "Path Trace Test";

Object * unionArray(Object * array[], int start, int end)
{
    if(end - start == 1)
    {
        return array[start];
    }
    if(end - start == 2)
    {
        return new Union(array[start], array[start + 1]);
    }
    int split = (end - start) / 2 + start;
    return new Union(unionArray(array, start, split), unionArray(array, split, end));
}

Object * makeWorld()
{
    static Material matEmitR = Material(Color(0, 0, 0), 0, Color(6, 0, 0));
    static Material matEmitG = Material(Color(0, 0, 0), 0, Color(0, 6, 0));
    static Material matEmitB = Material(Color(0, 0, 0), 0, Color(0, 0, 6));
    static Material matEmitW = Material(Color(0, 0, 0), 0, Color(2, 2, 2));
    static Material matEmitBrightW = Material(Color(0, 0, 0), 0, Color(20, 20, 20));
    static Material matDiffuseWhite = Material(Color(0.8, 0.8, 0.8), 1);
    static Material matGlass = Material(Color(0.5, 0.5, 0.5), 0, Color(0, 0, 0), Color(0.9, 0.9, 0.9), 1.3, 1);
    static Material matMirror = Material(Color(0.99, 0.99, 0.99), 0);
    Object * objects[] =
    {
        /*new Sphere(Vector3D(1, 8, -3), 3, &matEmitR),
        new Sphere(Vector3D(0, 8, -2), 3, &matEmitG),
        new Sphere(Vector3D(-1, 8, -3), 3, &matEmitB),*/
        new Sphere(Vector3D(0, 8, 0), 2, &matEmitBrightW),
        new Sphere(Vector3D(0, -6, -3), 5, &matDiffuseWhite),
        new Sphere(Vector3D(1, -0.5, -3), 0.5, &matDiffuseWhite),
        new Plane(Vector3D(0, 0, -1), 20, &matDiffuseWhite),
        new Plane(Vector3D(0, 0, 1), 20, &matDiffuseWhite),
        new Plane(Vector3D(0, -1, 0), 20, &matDiffuseWhite),
        new Plane(Vector3D(0, 1, 0), 20, &matDiffuseWhite),
        new Plane(Vector3D(-1, 0, 0), 20, &matDiffuseWhite),
        new Plane(Vector3D(1, 0, 0), 20, &matDiffuseWhite),
        new Sphere(Vector3D(-1, 0, -3), 0.5, &matGlass),
    };
    return unionArray(objects, 0, sizeof(objects) / sizeof(objects[0]));
}

class RenderLine
{
public:
    RenderLine(const int x, const int y, const int rayCount, Color * buffer, int bufferSize, const Object * world)
        : rayCount(rayCount), x(x), y(y), buffer(buffer), bufferSize(bufferSize), world(world), ran_finish(false)
    {
        if(multiThreaded)
        {
            thread = SDL_CreateThread(runFn, (void *)this);
        }
    }
    void finish()
    {
        if(ran_finish)
        {
            return;
        }
        ran_finish = true;
        if(multiThreaded)
        {
            SDL_WaitThread(thread, NULL);
        }
        else
        {
            run();
        }
    }
    ~RenderLine()
    {
        finish();
    }
private:
    void run() throw()
    {
        SpanIterator & spanIterator = *world->makeSpanIterator();
        Color * temp = new Color[bufferSize];
        for(int i = 0; i < bufferSize; i++)
        {
            temp[i] = tracePixel(spanIterator, x + i, y, ScreenWidth, ScreenHeight, rayCount);
        }
        for(int i = 0; i < bufferSize; i++)
        {
            buffer[i] += temp[i];
        }
        delete []temp;
        delete &spanIterator;
    }
    static SDLCALL int runFn(void * data)
    {
        ((RenderLine *)data)->run();
        return 0;
    }
    const int rayCount;
    const int x, y;
    SDL_Thread * thread;
    Color * buffer;
    const int bufferSize;
    const Object * world;
    bool ran_finish;
};

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
            static RenderLine * renderers[rendererCount];
            for(int i = 0; i < rendererCount; i++)
            {
                if(count <= 1)
                {
                    for(int x = 0; x < ScreenWidth; x++)
                    {
                        Color & c = screenBuffer[x + ScreenWidth * (y + i)];
                        c = Color(0, 0, 0);
                    }
                }
                if(y + i < ScreenHeight)
                {
                    renderers[i] = new RenderLine(0, i + y, rayCount, &screenBuffer[ScreenWidth * (y + i)], ScreenWidth, world);
                }
                else
                {
                    renderers[i] = nullptr;
                }
            }
            for(int i = 0; i < rendererCount; i++)
            {
                if(renderers[i] != nullptr)
                {
                    renderers[i]->finish();
                    delete renderers[i];
                }
            }
            while(SDL_LockSurface(screen) != 0)
                ;
            for(int i = 0; i < rendererCount; i++)
            {
                for(int x = 0; x < ScreenWidth; x++)
                {
                    Color & c = screenBuffer[x + ScreenWidth * y];
                    int r = max(0, min(0xFF, (int)floor(0x100 * c.x / count)));
                    int g = max(0, min(0xFF, (int)floor(0x100 * c.y / count)));
                    int b = max(0, min(0xFF, (int)floor(0x100 * c.z / count)));
                    *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
                }
                y++;
                if(y >= ScreenHeight)
                {
                    break;
                }
            }
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

