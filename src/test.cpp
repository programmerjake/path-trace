#include "path-trace.h"
#include <SDL.h>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <cstdio>
#include <ctime>
#include <vector>
#include <mutex>
#include <condition_variable>

using namespace std;
using namespace PathTrace;
const bool multiThreaded = true;
const int rendererCount = 4000;
const int rayCount = 100000;
const int rayDepth = 4;//64;
const int ScreenWidth = 1680, ScreenHeight = 1050;
const char *const ProgramName = "Path Trace Test";
const float minimumColorDelta = 1 / pow((float)rayCount / 2, 0.4); // if the color change is less than this then we don't need to check inside this box
const int blockSize = [](int count)
{
    int retval = 1024;
    while(retval > count && retval > 4)
    {
        retval /= 2;
    }
    return retval / 4;
}(ScreenWidth / 4), maximumSampleSize = ScreenHeight / (480 / 16);

Object *unionArray(Object *array[], int start, int end)
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

Object *makeLens(Vector3D position, Vector3D orientation, float radius, float sphereRadius, const Material *material)
{
    assert(radius <= sphereRadius);
    float dist = sqrt(sphereRadius * sphereRadius - radius * radius);
    orientation = normalize(orientation);
    return new Intersection(new Sphere(position + orientation * dist, sphereRadius, material), new Sphere(position - orientation * dist, sphereRadius, material));
}

Object *makeLensPointedAt(Vector3D position, Vector3D focus, float focusFactor, float radius, const Material *material)
{
    float ior = material->ior;
    assert(ior > 1 + eps);
    float distance = abs(focus - position) * focusFactor;
    assert(distance > eps);
    return makeLens(position, focus - position, radius, 2 * distance * (ior - 1), material);
}

Vector3D interpolate(float t, Vector3D a, Vector3D b)
{
    return a + t * (b - a);
}

Object *makeWorld()
{
    static Material matEmitR = Material(Color(0, 0, 0), 0, Color(12, 0, 0));
    static Material matEmitG = Material(Color(0, 0, 0), 0, Color(0, 12, 0));
    static Material matEmitB = Material(Color(0, 0, 0), 0, Color(0, 0, 12));
    static Material matEmitW = Material(Color(0, 0, 0), 0, Color(2, 2, 2));
    static Material matEmitBrightW = Material(Color(0, 0, 0), 0, Color(80, 80, 80));
    static Material matDiffuseWhite = Material(Color(0.8, 0.8, 0.8), 1);
    static Material matGlass = Material(Color(0.7, 0.7, 0.7), 0, Color(0, 0, 0), Color(0.9, 0.9, 0.9), 1.3, 1);
    static Material matDiamond = Material(Color(0.9, 0.9, 0.9), 0, Color(0, 0, 0), Color(0.95, 0.95, 0.95), 2.419, 1);
    static Material matMirror = Material(Color(0.99, 0.99, 0.99), 0);
    Object *objects[] =
    {
        new Sphere(Vector3D(-1 + sin(M_PI * 2 / 3) * 3, 6 + cos(M_PI * 2 / 3) * 3, 14), 6, &matEmitR),
        new Sphere(Vector3D(-1 + sin(M_PI * 4 / 3) * 3, 6 + cos(M_PI * 2 / 3) * 3, 14), 6, &matEmitB),
        new Sphere(Vector3D(-1, 6 + 3, 14), 6, &matEmitG),
        new Sphere(Vector3D(-1, 6, 14), 6, &matEmitBrightW),
        new Sphere(Vector3D(-1, 6, 16), 7.5, &matMirror),
        new Sphere(Vector3D(1, 0, -4), 0.2, &matDiffuseWhite),
        new Sphere(Vector3D(-1, 0, -4), 0.2, &matDiffuseWhite),
        new Plane(Vector3D(0, 0, -1), 10, &matDiffuseWhite),
        new Plane(Vector3D(0, 0, 1), 20, &matDiffuseWhite),
        new Plane(Vector3D(0, -1, 0), 20, &matEmitW),
        new Plane(Vector3D(0, 1, 0), 5, &matDiffuseWhite),
        new Plane(Vector3D(-1, 0, 0), 20, &matDiffuseWhite),
        new Plane(Vector3D(1, 0, 0), 20, &matDiffuseWhite),
        //makeLens(Vector3D(-2.5 / 4, 0, -2.5), Vector3D(-1, 0, -4), 0.5, 1, &matGlass),
        makeLensPointedAt(interpolate(0.9, Vector3D(-1, 10, 14), Vector3D(0, 0, -10)), Vector3D(0, -1, -20), 1.2, 2.5, &matDiamond),
    };
    return unionArray(objects, 0, sizeof(objects) / sizeof(objects[0]));
}

class Runnable
{
private:
    static thread **threads;
    static atomic_bool *threadDone;
    static int threadCount;
    static Runnable *head, *tail;
    static bool didInit;
    static mutex listMutex;
    Runnable *next, *prev;
    atomic_bool started, finished;
    condition_variable_any finished_cond;

    static void threadFn(atomic_bool * done)
    {
        listMutex.lock();
        while(head)
        {
            Runnable *theRunner = head;
            head = head->next;
            if(head)
                head->prev = NULL;
            else
                tail = NULL;
            theRunner->prev = NULL;
            theRunner->next = NULL;
            theRunner->started = true;
            listMutex.unlock();
            try
            {
                theRunner->run();
            }
            catch(...)
            {
                exit(1);
            }
            listMutex.lock();
            theRunner->finished = true;
            theRunner->finished_cond.notify_all();
        }
        *done = true;
        listMutex.unlock();
    }

    static void doInit()
    {
        if(didInit)
        {
            return;
        }
        didInit = true;
        threadCount = thread::hardware_concurrency();
        if(threadCount == 0)
        {
            threadCount = 4;
        }
        threads = new thread *[threadCount];
        threadDone = new atomic_bool[threadCount];
        for(int i = 0; i < threadCount; i++)
        {
            threads[i] = NULL;
            threadDone[i] = true;
        }
    }
    Runnable(const Runnable &) = delete;
    const Runnable &operator =(const Runnable &) = delete;
public:
    Runnable()
    {
        doInit();
    }
    void start()
    {
        listMutex.lock();
        if(head)
            tail->next = this;
        else
            head = this;
        next = NULL;
        prev = tail;
        tail = this;
        listMutex.unlock();
        for(int i = 0; i < threadCount; i++)
        {
            if(threadDone[i])
            {
                if(threads[i])
                {
                    threads[i]->join();
                }
                delete threads[i];
                threadDone[i] = false;
                threads[i] = new thread(threadFn, &threadDone[i]);
                break;
            }
        }
    }
    virtual void run() = 0;
    void join()
    {
        listMutex.lock();
        while(!finished)
            finished_cond.wait(listMutex);
        listMutex.unlock();
    }
    virtual ~Runnable()
    {
        listMutex.lock();
        if(started)
        {
            while(!finished)
                finished_cond.wait(listMutex);
        }
        else
        {
            if(next == NULL)
                tail = prev;
            else
                next->prev = prev;
            if(prev == NULL)
                head = next;
            else
                prev->next = next;
        }
        listMutex.unlock();
    }
};

thread **Runnable::threads = NULL;
atomic_bool *Runnable::threadDone = NULL;
int Runnable::threadCount = 0;
bool Runnable::didInit = false;
Runnable *Runnable::head = NULL, *Runnable::tail = NULL;
mutex Runnable::listMutex;

class RenderBlock final : public Runnable
{
public:
    RenderBlock(const int x, const int y, int size, const Object *world)
        : Runnable(), xOrigin(x), yOrigin(y), buffer(new Color[(size + 1) * (size + 1)]), validBuffer(new bool[(size + 1) * (size + 1)]), size(size), world(world), ran_finish(false), finished(false)
    {
        for(int i = 0; i < (size + 1) * (size + 1); i++)
        {
            validBuffer[i] = false;
        }
        start();
    }
    void finish()
    {
        if(ran_finish)
        {
            return;
        }
        join();
        ran_finish = true;
    }
    bool done()
    {
        if(ran_finish)
        {
            return true;
        }
        if(multiThreaded)
        {
            return finished;
        }
        return false;
    }
    void copyToBuffer(Color *screenBuffer, int w, int h)
    {
        for(int y = yOrigin; y < yOrigin + size && y < h; y++)
        {
            for(int x = xOrigin; x < xOrigin + size && x < w; x++)
            {
                if(validPixel(x, y))
                {
                    screenBuffer[x + w * y] = pixel(x, y);
                }
            }
        }
    }
    ~RenderBlock()
    {
        delete []buffer;
        delete []validBuffer;
    }
private:
    Color &pixel(int x, int y)
    {
        return buffer[x - xOrigin + (y - yOrigin) * (size + 1)];
    }
    bool &validPixel(int x, int y)
    {
        return validBuffer[x - xOrigin + (y - yOrigin) * (size + 1)];
    }
    void setPixel(int x, int y, Color color)
    {
        if(x < xOrigin || x - xOrigin > size)
        {
            return;
        }
        if(y < yOrigin || y - yOrigin > size)
        {
            return;
        }
        pixel(x, y) = color;
        validPixel(x, y) = true;
    }
    void interpolateSquare(int xOrigin, int yOrigin, int size, Color tl, Color tr, Color bl, Color br)
    {
        for(int y = 0; y < size; y++)
        {
            float fy = (float)y / size;
            Color l = tl + fy * (bl - tl);
            Color r = tr + fy * (br - tr);
            for(int x = 0; x < size; x++)
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
        if(x >= xOrigin && x <= xOrigin + size && y >= yOrigin && y <= yOrigin + size)
        {
            if(validPixel(x, y))
            {
                return pixel(x, y);
            }
        }
        Color retval = tracePixel(*spanIterator, x, y, ScreenWidth, ScreenHeight, rayCount, rayDepth, ScreenWidth, ScreenHeight, min(ScreenWidth, ScreenHeight) * 2);
        float r = max(retval.x, max(retval.y, retval.z));
        if(r > 1)
        {
            retval.x += (r - 1) / 2;
            retval.y += (r - 1) / 2;
            retval.z += (r - 1) / 2;
            retval.x = min(1.0f, retval.x);
            retval.y = min(1.0f, retval.y);
            retval.z = min(1.0f, retval.z);
        }
        setPixel(x, y, retval);
        return retval;
    }
    void renderSquare(int x, int y, int size, Color tl, Color tr, Color bl, Color br)
    {
        if(x > ScreenWidth || y > ScreenHeight)
        {
            return;
        }
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
protected:
    virtual void run() override
    {
        spanIterator = world->makeSpanIterator();
        renderSquare(xOrigin, yOrigin, size, calcPixelColor(xOrigin, yOrigin), calcPixelColor(xOrigin + size, yOrigin), calcPixelColor(xOrigin, yOrigin + size), calcPixelColor(xOrigin + size, yOrigin + size));
        delete spanIterator;
        finished = true;
    }
private:
    const int xOrigin, yOrigin;
    Color *const buffer;
    bool *const validBuffer;
    const int size;
    const Object *world;
    bool ran_finish;
    SpanIterator *spanIterator;
    atomic_bool finished;
};

int main(int argc, char **argv)
{
    if(argc == 2)
    {
        if(argv[1] == string("-h") || argv[1] == string("--help"))
        {
            cout << "usage: path-trace [--novideo]\n";
            return EXIT_SUCCESS;
        }
    }
    bool useVideo = true;
    if(argc >= 2 && string(argv[1]) == "--novideo")
    {
        useVideo = false;
    }
    if(SDL_Init(useVideo ? SDL_INIT_VIDEO : 0) != 0)
    {
        cerr << "\nUnable to initialize SDL:  " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);
    SDL_Surface *screen = NULL;
    if(useVideo)
    {
        screen = SDL_SetVideoMode(ScreenWidth, ScreenHeight, 32, SDL_SWSURFACE);
    }
    if(screen == NULL)
    {
        if(useVideo)
        {
            cerr << "\nUnable to set video mode:  " << SDL_GetError() << endl;
        }
        useVideo = false;
        screen = SDL_CreateRGBSurface(SDL_SWSURFACE, ScreenWidth, ScreenHeight, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
        if(screen == NULL)
        {
            cerr << "\nUnable to create surface:  " << SDL_GetError() << endl;
            return EXIT_FAILURE;
        }
    }

    bool done = false;
    bool rendered = false;
    int bx = 0, by = 0;
    int count = 1;
    Color *screenBuffer = new Color[ScreenWidth * ScreenHeight];
    while(SDL_LockSurface(screen) != 0)
        ;
    for(int y = 0; y < ScreenHeight; y++)
    {
        for(int x = 0; x < ScreenWidth; x++)
        {
            int r = 0x80;
            int g = 0x0;
            int b = 0x80;
            screenBuffer[x + y * ScreenWidth] = Color(r / (float)0xFF, g / (float)0xFF, b / (float)0xFF);
            *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
        }
    }
    SDL_UnlockSurface(screen);
    if(useVideo)
    {
        SDL_Flip(screen);
        SDL_WM_SetCaption(ProgramName, NULL);
    }
    Object *const world = makeWorld();
    AutoDestruct<Object> autoDestruct1(world);
    RenderBlock *renderers[rendererCount];
    bool renderedAllBlocks = true;
    while(!done)
    {
        SDL_Event event;
        while(!done)
        {
            if(!useVideo)
            {
                if(rendered)
                {
                    done = true;
                }
                break;
            }
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
            if(renderedAllBlocks)
            {
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
            }
            if(multiThreaded)
            {
                renderedAllBlocks = true;
                for(int i = 0; i < rendererCount; i++)
                {
                    if(renderers[i] != nullptr)
                    {
                        if(!renderers[i]->done())
                        {
                            renderedAllBlocks = false;
                            break;
                        }
                    }
                }
                if(!renderedAllBlocks)
                {
                    while(SDL_LockSurface(screen) != 0)
                        ;
                    int tempX = bx, tempY = by;
                    for(int i = 0; i < rendererCount; i++)
                    {
                        if(renderers[i] != nullptr)
                        {
                            renderers[i]->copyToBuffer(screenBuffer, ScreenWidth, ScreenHeight);
                            for(int y = tempY; y < tempY + blockSize && y < ScreenHeight; y++)
                            {
                                for(int x = tempX; x < tempX + blockSize && x < ScreenWidth; x++)
                                {
                                    Color &c = screenBuffer[x + ScreenWidth * y];
                                    int r = max(0, min(0xFF, (int)floor(0x100 * c.x / count)));
                                    int g = max(0, min(0xFF, (int)floor(0x100 * c.y / count)));
                                    int b = max(0, min(0xFF, (int)floor(0x100 * c.z / count)));
                                    *(Uint32 *)((Uint8 *)screen->pixels + y * screen->pitch + x * screen->format->BytesPerPixel) = SDL_MapRGB(screen->format, r, g, b);
                                }
                            }
                        }
                        tempX += blockSize;
                        if(tempX >= ScreenWidth)
                        {
                            tempX = 0;
                            tempY += blockSize;
                            if(tempY >= ScreenHeight)
                            {
                                break;
                            }
                        }
                    }
                    SDL_UnlockSurface(screen);
                    if(useVideo)
                    {
                        SDL_Flip(screen);
                    }
                    SDL_Delay(100);
                    continue;
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
                    for(int y = by; y < by + blockSize && y < ScreenHeight; y++)
                    {
                        for(int x = bx; x < bx + blockSize && x < ScreenWidth; x++)
                        {
                            Color &c = screenBuffer[x + ScreenWidth * y];
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
                            char fname[100];
                            sprintf(fname, "image%08X.bmp", (unsigned)time(NULL));
                            SDL_SaveBMP(screen, fname);
                        }
                    }
                }
            }
            SDL_UnlockSurface(screen);
            if(useVideo)
            {
                SDL_Flip(screen);
            }
            else
            {
                static int dotCount = 0;
                cout << "rendering.";
                for(int i = dotCount++; i >= 0; i++)
                {
                    cout << ".";
                }
                cout << "\x1B[K\r";
                dotCount %= 15;
            }
        }
    }
    return EXIT_SUCCESS;
}

