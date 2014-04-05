#include "path-trace.h"
#ifndef SERVER_ONLY
#include <SDL.h>
#endif
#include <iostream>
#include <cstdlib>
#include "thread.h"
#include "mutex.h"
#include "atomic.h"
#include <cstdio>
#include <ctime>
#include <vector>
#include <sstream>
#include "condition_variable.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>
#include <errno.h>
#include "image_texture.h"

using namespace std;
using namespace PathTrace;
const char * NET_PORT = "12346";
const bool multiThreaded = true;
const int rendererCount = 200;
const int rayCount = 1;
const int rayDepth = 16;
const int ScreenWidth = 192 * 3, ScreenHeight = 108 * 3;
const char *const ProgramName = "Path Trace Test";
const float minimumColorDelta = 0.003; // if the color change is less than this then we don't need to check inside this box

static int getBlockSize(int count)
{
    int retval = 1024;
    while(retval > count && retval > 4)
    {
        retval /= 2;
    }
    return retval / 4;
}

const int blockSize = getBlockSize(ScreenWidth / 8), maximumSampleSize = ScreenHeight / (480 / 4);

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

Material * makeSkyBox(string folderName)
{
    if(folderName == "")
        folderName = ".";
    else if(folderName[folderName.size() - 1] == '/')
        folderName.erase(folderName.size() - 1);
    return new Material(new ColorTexture(0), new ColorTexture(0), new ImageSkyboxTexture(Image(folderName + "/top.png"), Image(folderName + "/bottom.png"), Image(folderName + "/left.png"), Image(folderName + "/right.png"), Image(folderName + "/front.png"), Image(folderName + "/back.png")));
}

Object *makeWorld()
{
    static Material matEmitR(new ColorTexture(0), new ColorTexture(0), new ColorTexture(24, 0, 0));
    static Material matEmitG(new ColorTexture(0), new ColorTexture(0), new ColorTexture(0, 24, 0));
    static Material matEmitB(new ColorTexture(0), new ColorTexture(0), new ColorTexture(0, 0, 24));
    static Material matEmitW(new ColorTexture(0), new ColorTexture(0), new ColorTexture(2));
    static Material matEmitBrightW(new ColorTexture(0), new ColorTexture(0), new ColorTexture(40));
    static Material matDiffuseWhite(new ColorTexture(0.8), new ColorTexture(1));
    static Material matBrightDiffuseWhite(new ColorTexture(8), new ColorTexture(1));
    static Material matGlass(new ColorTexture(0.7), new ColorTexture(0), new ColorTexture(0), new ColorTexture(0.9), 1.3, new ColorTexture(1));
    static Material matDiamond(new ColorTexture(0.2), new ColorTexture(0), new ColorTexture(0), new ColorTexture(0.9), 2.419, new ColorTexture(1));
    static Material matMirror(new ColorTexture(0.99), new ColorTexture(0));
    static Material matImageInternal(new ImageTexture(Image("image.png")));
    static Material & matImage = *transform(Matrix::scale(0.1), &matImageInternal);
    static Material matImageEmitInternal(new ColorTexture(0), new ColorTexture(0), new ImageTexture(Image("image.png")));
    static Material & matImageEmit = *transform(Matrix::translate(-1, 0, -4).inverse(), &matImageEmitInternal);
    static Material & matSkyBox = *transform(Matrix::rotateY(2 * M_PI / 4), makeSkyBox("sky01"));
    Object *objects[] =
    {
        /*new Sphere(Vector3D(-1 + sin(M_PI * 2 / 3) * 3, 6 + cos(M_PI * 2 / 3) * 3, 14), 6, &matEmitR),
        new Sphere(Vector3D(-1 + sin(M_PI * 4 / 3) * 3, 6 + cos(M_PI * 2 / 3) * 3, 14), 6, &matEmitB),
        new Sphere(Vector3D(-1, 6 + 3, 14), 6, &matEmitG),
        new Sphere(Vector3D(-1, 6, 14), 6, &matEmitBrightW),
        new Sphere(Vector3D(-1, 6, 16), 7.5, &matMirror),*/
        new Sphere(Vector3D(1, 0, -4), 0.2, transform(Matrix::translate(-1, 0, 4), &matSkyBox)),
        new Intersection(new Sphere(Vector3D(1, 0, -4), 0.2 * 5, &matGlass), new Union(new Plane(Vector3D(-1, 0, -0.7), Vector3D(1, 0, -4), &matGlass), new Sphere(Vector3D(1, 0, -4), 0.2, transform(Matrix::translate(-1, 0, 4), &matEmitW)))),
        new Sphere(Vector3D(-1, 0, -4), 0.2, &matDiffuseWhite),
        new Plane(Vector3D(0, 0, -1), 200, &matSkyBox),
        new Plane(Vector3D(0, 0, 1), 200, &matSkyBox),
        new Plane(Vector3D(0, -1, 0), 200, &matSkyBox),
        new Plane(Vector3D(0, 1, 0), 200, &matSkyBox),
        new Plane(Vector3D(1, 0, 0), 200, &matSkyBox),
        new Plane(Vector3D(-1, 0, 0), 200, &matSkyBox),
        makeLens(Vector3D(-2.5 / 4, 0, -2.5), Vector3D(-1, 0, -4), 0.5, 1, &matGlass),
        //makeLensPointedAt(interpolate(0.9, Vector3D(-1, 10, 14), Vector3D(0, 0, -10)), Vector3D(0, -1, -20), 1.2, 2.5, &matDiamond),
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
    static volatile bool didInit;
    static mutex listMutex, doingInit;
    Runnable *next, *prev;
    atomic_bool started, finished;
    condition_variable_any finished_cond;

    static void threadFn(atomic_bool *done)
    {
        listMutex.lock();
        while(head)
        {
            Runnable *theRunner = head;
            head = head->next;
            if(head)
            {
                head->prev = NULL;
            }
            else
            {
                tail = NULL;
            }
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
        doingInit.lock();
        if(didInit)
        {
            doingInit.unlock();
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
        doingInit.unlock();
    }
    Runnable(const Runnable &);
    const Runnable &operator =(const Runnable &);
public:
    Runnable()
    {
        doInit();
    }
    void start()
    {
        listMutex.lock();
        if(head)
        {
            tail->next = this;
        }
        else
        {
            head = this;
        }
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
        {
            finished_cond.wait(listMutex);
        }
        listMutex.unlock();
    }
    virtual ~Runnable()
    {
        listMutex.lock();
        if(started)
        {
            while(!finished)
            {
                finished_cond.wait(listMutex);
            }
        }
        else
        {
            if(next == NULL)
            {
                tail = prev;
            }
            else
            {
                next->prev = prev;
            }
            if(prev == NULL)
            {
                head = next;
            }
            else
            {
                prev->next = next;
            }
        }
        listMutex.unlock();
    }
    static int getThreadCount()
    {
        doInit();
        return threadCount;
    }
};

thread **Runnable::threads = NULL;
atomic_bool *Runnable::threadDone = NULL;
int Runnable::threadCount = 0;
volatile bool Runnable::didInit = false;
Runnable *Runnable::head = NULL, *Runnable::tail = NULL;
mutex Runnable::listMutex, Runnable::doingInit;

class BlockRenderer
{
    const BlockRenderer & operator =(const BlockRenderer &);
    BlockRenderer(const BlockRenderer &);
public:
    BlockRenderer()
    {
    }
    virtual bool done() = 0;
    virtual void copyToBuffer(Color *screenBuffer, int w, int h) = 0;
    virtual ~BlockRenderer()
    {
    }
};

class RenderBlock : public Runnable, public BlockRenderer
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
    void copyToSocket(FILE *f)
    {
        for(int y = yOrigin; y < yOrigin + size; y++)
        {
            for(int x = xOrigin; x < xOrigin + size; x++)
            {
                if(validPixel(x, y))
                {
                    Color v = pixel(x, y);
                    fprintf(f, "P%i,%i=%g,%g,%g\n", x, y, v.x, v.y, v.z);
                }
            }
        }
        fprintf(f, "E\n");
        fflush(f);
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
    virtual void run()
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

class NetRenderBlock : public BlockRenderer
{
private:
    const int x, y, size;
    Color * const cBuffer;
    bool * const vBuffer;
    volatile bool finished;
    mutex bufferMutex;
    thread * th;
    static vector<string> addresses;
    static void threadFn(NetRenderBlock * nrb)
    {
        while(!nrb->run())
        {
            this_thread::sleep_for(chrono::seconds(1));
        }
        nrb->finished = true;
    }
    bool run()
    {
        string address = addresses[rand() % addresses.size()];
        string port = NET_PORT;
        size_t pos = address.find_last_of(':');
        if(pos != string::npos)
        {
            port = address.substr(pos + 1);
            address = address.substr(0, pos);
        }
        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;
        addrinfo * addrList;
        int retval = getaddrinfo(address.c_str(), port.c_str(), &hints, &addrList);
        if (0 != retval)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
            return false;
        }
        int fd = -1;
        for(addrinfo * addr = addrList; addr; addr = addr->ai_next)
        {
            fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if(fd == -1)
                continue;
            if(-1 != connect(fd, addr->ai_addr, addr->ai_addrlen))
                break;
            close(fd);
            fd = -1;
        }
        if(fd == -1)
        {
            perror("can't connect");
            freeaddrinfo(addrList);
            return false;
        }
        freeaddrinfo(addrList);
        FILE * rdF = fdopen(dup(fd), "r");
        FILE * wrF = fdopen(fd, "w");
        int good = fgetc(rdF);
        if(good != '1')
        {
            fclose(rdF);
            fclose(wrF);
            return false;
        }
        fprintf(wrF, "%i %i %i\n", x, y, size);
        if(ferror(wrF))
        {
            fclose(rdF);
            fclose(wrF);
            return false;
        }
        fclose(wrF);
        while(true)
        {
            switch(fgetc(rdF))
            {
            case 'S':
            case 'E':
                break;
            case '\n':
                break;
            case 'P':
            {
                int px, py;
                Color c;
                if(5 != fscanf(rdF, "%i,%i=%f,%f,%f", &px, &py, &c.x, &c.y, &c.z) || px < x || py < y || px >= x + size || py >= y + size)
                {
                    fclose(rdF);
                    return false;
                }
                px -= x;
                py -= y;
                bufferMutex.lock();
                vBuffer[px + size * py] = true;
                cBuffer[px + size * py] = c;
                bufferMutex.unlock();
                break;
            }
            case 'F':
                fclose(rdF);
                return true;
            default:
                fclose(rdF);
                return false;
            }
        }
    }
public:
    NetRenderBlock(int x, int y, int size)
        : x(x), y(y), size(size), cBuffer(new Color[size * size]), vBuffer(new bool[size * size]), finished(false)
    {
        for(int i = 0; i < size * size; i++)
        {
            vBuffer[i] = false;
        }
        th = NULL;
#if 0
        threadFn(this);
#else
        th = new thread(threadFn, this);
#endif
    }
    bool done()
    {
        return finished;
    }
    void copyToBuffer(Color *screenBuffer, int w, int h)
    {
        bufferMutex.lock();
        for(int py = y; py < y + size && py < h; py++)
        {
            for(int px = x; px < x + size && px < w; px++)
            {
                int rx = px - x, ry = py - y;
                if(vBuffer[rx + size * ry])
                {
                    screenBuffer[px + w * py] = cBuffer[rx + size * ry];
                }
            }
        }
        bufferMutex.unlock();
    }
    ~NetRenderBlock()
    {
        th->join();
        delete th;
        delete []cBuffer;
        delete []vBuffer;
    }
    friend void client(vector<string>);
};

vector<string> NetRenderBlock::addresses;

Object *const world = makeWorld();
AutoDestruct<Object> autoDestruct1(world);

void serverThreadFn(int fd)
{
    static atomic_int running_count(0);
    FILE *f2 = fdopen(dup(fd), "r");
    FILE *f = fdopen(fd, "w");
    if(running_count++ >= Runnable::getThreadCount() * 2)
    {
        running_count--;
        fprintf(f, "0\n");
        fclose(f2);
        fclose(f);
        return;
    }
    fprintf(f, "1\n");
    fflush(f);
    int x, y, size;
    if(3 != fscanf(f2, " %i %i %i", &x, &y, &size) || x < 0 || y < 0 || size <= 0 || size > 1000000 || x > ScreenWidth || y > ScreenHeight || size > ScreenWidth || size > ScreenHeight)
    {
        running_count--;
        fclose(f2);
        fclose(f);
        return;
    }
    cout << "Started Block (" << x << ", " << y << ") " << size << endl;
    fclose(f2);
    fprintf(f, "S\n");
    fflush(f);
    RenderBlock *rb = new RenderBlock(x, y, size, world);
    while(!rb->done())
    {
        rb->copyToSocket(f);
        this_thread::sleep_for(chrono::seconds(1));
    }
    rb->copyToSocket(f);
    fprintf(f, "F\n");
    fclose(f);
    delete rb;
    cout << "Finished Block (" << x << ", " << y << ") " << size << endl;
    running_count--;
}

int server()
{
    addrinfo hints;
    memset((void *)&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    addrinfo *addrList;
    if(0 != getaddrinfo(NULL, NET_PORT, &hints, &addrList))
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }
    int fd = -1;
    const char *errorStr = "getaddrinfo";
    for(addrinfo *addr = addrList; addr != NULL; addr = addr->ai_next)
    {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        errorStr = "socket";
        if(fd < 0)
        {
            continue;
        }
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
        if(::bind(fd, addr->ai_addr, addr->ai_addrlen) == 0)
        {
            break;
        }
        int temp = errno;
        close(fd);
        errno = temp;
        errorStr = "bind";
        fd = -1;
    }
    if(fd < 0)
    {
        perror(errorStr);
        exit(EXIT_FAILURE);
    }

    if(listen(fd, 50) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for(int fd2 = accept(fd, NULL, NULL); fd2 >= 0; fd2 = accept(fd, NULL, NULL))
    {
        thread(serverThreadFn, fd2).detach();
    }

    perror("accept");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
}

bool isNetworkClient = false;

BlockRenderer * makeBlockRenderer(int x, int y, int size)
{
    if(isNetworkClient)
        return new NetRenderBlock(x, y, size);
    return new RenderBlock(x, y, size, world);
}

void client(vector<string> addresses)
{
    NetRenderBlock::addresses = addresses;
    isNetworkClient = true;
}
#ifdef SERVER_ONLY
int main()
{
    return server();
}
#else
int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    if(argc == 2)
    {
        if(argv[1] == string("-h") || argv[1] == string("--help"))
        {
            cout << "usage: path-trace [--novideo] [--server] [--client server.web.address]\n";
            return EXIT_SUCCESS;
        }
        else if(argv[1] == string("--server"))
        {
            return server();
        }
        else
        {
            cout << "usage: path-trace [--novideo] [--server] [--client server.web.address]\n";
            return EXIT_FAILURE;
        }
    }
    else if(argc >= 3)
    {
        if(argv[1] == string("--client"))
        {
            vector<string> addresses;
            for(int i = 2; i < argc; i++)
            {
                addresses.push_back(string(argv[i]));
            }
            client(addresses);
        }
        else
        {
            cout << "usage: path-trace [--novideo] [--server] [--client server.web.address]\n";
            return EXIT_FAILURE;
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
    BlockRenderer *renderers[rendererCount];
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
                        renderers[i] = makeBlockRenderer(tempX, tempY, blockSize);
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
                        renderers[i] = NULL;
                    }
                }
            }
            if(multiThreaded)
            {
                renderedAllBlocks = true;
                for(int i = 0; i < rendererCount; i++)
                {
                    if(renderers[i] != NULL)
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
                        if(renderers[i] != NULL)
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
            while(SDL_LockSurface(screen) != 0)
                ;
            for(int i = 0; i < rendererCount; i++)
            {
                if(renderers[i] != NULL)
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
#endif // SERVER_ONLY
