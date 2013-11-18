#include "path-trace.h"
#include <SDL.h>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace PathTrace;
const bool multiThreaded = true;
const int rendererCount = 2;
const int rayCount = 2000;
const int rayDepth = 4;
const int ScreenWidth = 320, ScreenHeight = 240;
const char * const ProgramName = "Path Trace Test";
const float minimumColorDelta = 0.01; // if the color change is less than this then we don't need to check inside this box
const int blockSize = 64, maximumSampleSize = 16;

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

Object * makeLens(Vector3D position, Vector3D orientation, float radius, float sphereRadius, const Material * material)
{
    assert(radius <= sphereRadius);
    float dist = sqrt(sphereRadius * sphereRadius - radius * radius);
    orientation = normalize(orientation);
    return new Intersection(new Sphere(position + orientation * dist, sphereRadius, material), new Sphere(position - orientation * dist, sphereRadius, material));
}

Object * makeLens(Vector3D position, Vector3D focus, float radius, const Material * material)
{
    float ior = material->ior;
    assert(ior > 1 + eps);
    float distance = abs(focus - position);
    assert(distance > eps);
    return makeLens(position, focus - position, radius, 2 * distance * (ior - 1), material);
}

Object * makeWorld()
{
    static Material matEmitR = Material(Color(0, 0, 0), 0, Color(6, 0, 0));
    static Material matEmitG = Material(Color(0, 0, 0), 0, Color(0, 6, 0));
    static Material matEmitB = Material(Color(0, 0, 0), 0, Color(0, 0, 6));
    static Material matEmitW = Material(Color(0, 0, 0), 0, Color(2, 2, 2));
    static Material matEmitBrightW = Material(Color(0, 0, 0), 0, Color(40, 40, 40));
    static Material matDiffuseWhite = Material(Color(0.8, 0.8, 0.8), 1);
    static Material matGlass = Material(Color(0.7, 0.7, 0.7), 0, Color(0, 0, 0), Color(0.9, 0.9, 0.9), 1.3, 1);
    static Material matDiamond = Material(Color(0.7, 0.7, 0.7), 0, Color(0, 0, 0), Color(0.95, 0.95, 0.95), 2.419, 1);
    static Material matMirror = Material(Color(0.99, 0.99, 0.99), 0);
    Object * objects[] =
    {
        new Sphere(Vector3D(1, 16, -3), 6, &matEmitR),
        new Sphere(Vector3D(0, 16, -2), 6, &matEmitG),
        new Sphere(Vector3D(-1, 16, -3), 6, &matEmitB),
        new Sphere(Vector3D(0, 14, -3), 4, &matEmitBrightW),
        new Sphere(Vector3D(1, 0, -4), 0.2, &matDiffuseWhite),
        new Sphere(Vector3D(-1, 0, -4), 0.2, &matDiffuseWhite),
        new Plane(Vector3D(0, 0, -1), 20, &matDiffuseWhite),
        new Plane(Vector3D(0, 0, 1), 20, &matDiffuseWhite),
        new Plane(Vector3D(0, -1, 0), 20, &matEmitW),
        new Plane(Vector3D(0, 1, 0), 1, &matDiffuseWhite),
        new Plane(Vector3D(-1, 0, 0), 20, &matDiffuseWhite),
        new Plane(Vector3D(1, 0, 0), 20, &matDiffuseWhite),
        makeLens(Vector3D(-3.0/4, 0, -3), Vector3D(-1, 0, -3), 0.5, 1, &matGlass),
        makeLens(Vector3D(0, 1.5, -5), Vector3D(0, -1, -5), 0.8, &matDiamond),
    };
    return unionArray(objects, 0, sizeof(objects) / sizeof(objects[0]));
}

namespace
{
class RenderBlock
{
public:
    RenderBlock(const int x, const int y, int size, const Object * world)
        : xOrigin(x), yOrigin(y), buffer(new Color[(size + 1) * (size + 1)]), validBuffer(new bool[(size + 1) * (size + 1)]), size(size), world(world), ran_finish(false)
    {
        for(int i=0;i<(size+1)*(size+1);i++)
            validBuffer[i] = false;
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
    void copyToBuffer(Color * screenBuffer, int w, int h)
    {
        finish();
        for(int y=yOrigin; y<yOrigin+size&&y<h; y++)
        {
            for(int x=xOrigin; x<xOrigin+size&&x<w; x++)
            {
                screenBuffer[x + w * y] = pixel(x, y);
            }
        }
    }
    ~RenderBlock()
    {
        if(multiThreaded)
            finish();
        delete []buffer;
        delete []validBuffer;
    }
private:
    Color & pixel(int x, int y)
    {
        return buffer[x - xOrigin + (y - yOrigin) * (size + 1)];
    }
    bool & validPixel(int x, int y)
    {
        return validBuffer[x - xOrigin + (y - yOrigin) * (size + 1)];
    }
    void setPixel(int x, int y, Color color)
    {
        if(x < xOrigin || x - xOrigin > size)
            return;
        if(y < yOrigin || y - yOrigin > size)
            return;
        pixel(x, y) = color;
        validPixel(x, y) = true;
    }
    void interpolateSquare(int xOrigin, int yOrigin, int size, Color tl, Color tr, Color bl, Color br)
    {
        for(int y=0; y<size; y++)
        {
            float fy = (float)y / size;
            Color l = tl + fy * (bl - tl);
            Color r = tr + fy * (br - tr);
            for(int x=0; x<size; x++)
            {
                float fx = (float)x / size;
                setPixel(x + xOrigin, y + yOrigin, l + fx * (r - l));
            }
        }
    }
    bool colorCloseEnough(Color a, Color b)
    {
        return abs_squared(a - b) <= minimumColorDelta;
    }
    Color calcPixelColor(int x, int y)
    {
        if(x >= xOrigin && x <= size && y >= yOrigin && y <= size)
        {
            if(validPixel(x, y))
                return pixel(x, y);
        }
        Color retval = tracePixel(*spanIterator, x, y, ScreenWidth, ScreenHeight, rayCount, rayDepth);
        setPixel(x, y, retval);
        return retval;
    }
    void renderSquare(int x, int y, int size, Color tl, Color tr, Color bl, Color br)
    {
        if(size <= 1)
        {
            setPixel(x, y, tl);
            return;
        }
        if(colorCloseEnough(tl, tr) &&
                colorCloseEnough(tl, bl) &&
                colorCloseEnough(tl, br) &&
                colorCloseEnough(tr, bl) &&
                colorCloseEnough(tr, br) &&
                colorCloseEnough(bl, br) && size <= maximumSampleSize)
        {
            interpolateSquare(x, y, size, tl, tr, bl, br);
            return;
        }
        int halfSize = size / 2;
        int cx = x + halfSize;
        int cy = y + halfSize;
        Color tc = calcPixelColor(cx, y);
        Color cl = calcPixelColor(x, cy);
        Color cc = calcPixelColor(cx, cy);
        Color cr = calcPixelColor(x + size, cy);
        Color bc = calcPixelColor(cx, y + size);
        renderSquare(x, y, halfSize, tl, tc, cl, cc);
        renderSquare(cx, y, halfSize, tc, tr, cc, cr);
        renderSquare(x, cy, halfSize, cl, cc, bl, bc);
        renderSquare(cx, cy, halfSize, cc, cr, bc, br);
    }
    void run() throw()
    {
        spanIterator = world->makeSpanIterator();
        renderSquare(xOrigin, yOrigin, size, calcPixelColor(xOrigin, yOrigin), calcPixelColor(xOrigin + size, yOrigin), calcPixelColor(xOrigin, yOrigin + size), calcPixelColor(xOrigin + size, yOrigin + size));
        delete spanIterator;
    }
    static SDLCALL int runFn(void * data)
    {
        ((RenderBlock *)data)->run();
        return 0;
    }
    const int xOrigin, yOrigin;
    SDL_Thread * thread;
    Color * const buffer;
    bool * const validBuffer;
    const int size;
    const Object * world;
    bool ran_finish;
    SpanIterator * spanIterator;
};
}

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
    int bx = 0, by = 0;
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
        if(!rendered && !done)
        {
            static RenderBlock * renderers[rendererCount];
            int tempX = bx, tempY = by;
            bool anyLeft = true;
            for(int i = 0; i < rendererCount; i++)
            {
                if(anyLeft)
                {
                    renderers[i] = new RenderBlock(tempX, tempY, blockSize, world);
                    tempX += blockSize;
                    if(tempX >= ScreenWidth)
                    {
                        tempX = 0;
                        tempY += blockSize;
                        if(tempY >= ScreenHeight)
                        {
                            anyLeft = false;
                        }
                    }
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
                }
            }
            while(SDL_LockSurface(screen) != 0)
                ;
            for(int i = 0; i < rendererCount; i++)
            {
                if(renderers[i] != nullptr)
                {
                    if(rendered)
                    {
                        delete renderers[i];
                        continue;
                    }
                    renderers[i]->copyToBuffer(screenBuffer, ScreenWidth, ScreenHeight);
                    for(int y=by; y<by+blockSize&&y<ScreenHeight; y++)
                    {
                        for(int x=bx; x<bx+blockSize&&x<ScreenWidth; x++)
                        {
                            Color & c = screenBuffer[x + ScreenWidth * y];
                            int r = max(0, min(0xFF, (int)floor(0x100 * c.x / count)));
                            int g = max(0, min(0xFF, (int)floor(0x100 * c.y / count)));
                            int b = max(0, min(0xFF, (int)floor(0x100 * c.z / count)));
                            *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
                        }
                    }
                    delete renderers[i];
                    bx += blockSize;
                    if(bx >= ScreenWidth)
                    {
                        bx = 0;
                        by += blockSize;
                        if(by >= ScreenHeight)
                        {
                            rendered = true;
                        }
                    }
                }
            }
            SDL_UnlockSurface(screen);
            SDL_Flip(screen);
        }
    }
    return EXIT_SUCCESS;
}

