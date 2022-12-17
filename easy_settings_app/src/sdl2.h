//
// Created by baifeng on 2022/10/13.
//

#ifndef SDL2_TEST_SDL2_H
#define SDL2_TEST_SDL2_H

#include <memory>
#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

#if defined(__PSP__)
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>

namespace psp {
    static bool* quit = nullptr;

    /* Exit callback */
    int exit_callback(int arg1, int arg2, void *common) {
        *quit = true;
        return 0;
    }

    /* Callback thread */
    int CallbackThread(SceSize args, void *argp) {
        int cbid;

        cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
        sceKernelRegisterExitCallback(cbid);
        sceKernelSleepThreadCB();

        return 0;
    }

    /* Sets up the callback thread and returns its thread id */
    int SetupCallbacks(void) {
        int thid = 0;
        thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
        if(thid >= 0) {
            sceKernelStartThread(thid, 0, 0);
        }
        return thid;
    }
}
#endif

namespace sdl2 {

    template<typename T>
    struct Vector {
        T x;
        T y;
        Vector():x(0), y(0) {}
        Vector(T const& x, T const& y):x(x), y(y) {}
        Vector(Vector<T> const& v2):x(v2.x), y(v2.y) {}
        Vector<T>& operator+=(Vector<T> const& v2) {
            x += v2.x;
            y += v2.y;
            return *this;
        }
        Vector<T>& operator-=(Vector<T> const& v2) {
            x -= v2.x;
            y -= v2.y;
            return *this;
        }
        Vector<T>& operator*=(Vector<T> const& v2) {
            x *= v2.x;
            y *= v2.y;
            return *this;
        }
        Vector<T>& operator/=(Vector<T> const& v2) {
            x /= v2.x;
            y /= v2.y;
            return *this;
        }
        Vector<T>& operator=(Vector<T> const& v2) {
            x = v2.x;
            y = v2.y;
            return *this;
        }
        Vector<T> operator+(Vector<T> const& v2) {
            return {x+v2.x, y+v2.y};
        }
        Vector<T> operator-(Vector<T> const& v2) {
            return {x-v2.x, y-v2.y};
        }
        Vector<T> operator*(Vector<T> const& v2) {
            return {x*v2.x, y*v2.y};
        }
        Vector<T> operator/(Vector<T> const& v2) {
            return {x/v2.x, y/v2.y};
        }
        Vector<T> operator+(T v) {
            return {x+v, y+v};
        }
        Vector<T> operator-(T v) {
            return {x-v, y-v};
        }
        Vector<T> operator*(T v) {
            return {x*v, y*v};
        }
        Vector<T> operator/(T v) {
            return {x/v, y/v};
        }
    };

    typedef Vector<int> Point2i;
    typedef Vector<float> Point2f;
    typedef Vector<int> Size;
    typedef Vector<float> Sizef;

    struct transform {
        float angle;
        Point2f scale;
        Point2f position;
        Point2f anchor;
        Size size;
    };

    // 屏幕像素
    Size screen_size{1280, 720};
    // 窗口标题
    std::string window_title = "sdl2-demo";
    // 屏幕背景色
    SDL_Color background = {0x0, 0x0, 0x0, 0xff};

    namespace __ {
        // 委托函数
        typedef void (*InputProc)(SDL_Event const& e);
        typedef void (*VoidProc)();
        typedef void (*UpdateProc)(float delta);
        typedef void (*RenderProc)(SDL_Renderer* renderer);
    }

    // 委托函数实例
    static __::InputProc user_event = nullptr;
    static __::VoidProc user_init = nullptr;
    static __::VoidProc user_fini = nullptr;
    static __::UpdateProc user_update = nullptr;
    static __::RenderProc user_render = nullptr;

    // 私有空间
    namespace __ {

        // 退出标记
        static bool quit = false;

        // 窗口和渲染器
        static SDL_Window* sdl_window = nullptr;
        static SDL_Renderer* sdl_renderer = nullptr;
        // 游戏控制器
        static SDL_Joystick* _gGameController = nullptr;

        // 帧率
        class Fps {
        public:
            explicit Fps(int limit = 60):
                    _frameDelay(1000.0f/limit),
                    _frameStart(0),
                    _frameTime(0),
                    _delta(_frameDelay / 1000.0f) {}
            void start() {
                _frameStart = SDL_GetTicks();
            }
            void next() {
                int const _delay = _frameDelay;
                _frameTime = SDL_GetTicks() - _frameStart;
                if (_delay > _frameTime) {
                    SDL_Delay(_delay - _frameTime);
                }
            }
            float delta() const {
                return _frameDelay > _frameTime ? _delta : _frameTime / 1000.0f;
            }
        private:
            int _frameStart, _frameTime;
            float _frameDelay, _delta;
        };

        static Fps fps;

        static int initSDL() {

            char rendername[256] = {0};
            SDL_RendererInfo info;

            SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);   // Initialize SDL2
            //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            TTF_Init();

            //检查操纵杆
            if( SDL_NumJoysticks() < 1 ) {
                printf( "Warning: No joysticks connected!\n" );
            } else {
                //加载操纵杆
                _gGameController = SDL_JoystickOpen( 0 );
                if( _gGameController == nullptr ) {
                    printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
                }
            }

            SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            // Create an application window with the following settings:
            sdl_window = SDL_CreateWindow(
                    window_title.c_str(),         //    const char* title
                    SDL_WINDOWPOS_CENTERED,  //    int x: initial x position
                    SDL_WINDOWPOS_CENTERED,  //    int y: initial y position
                    screen_size.x,                      //    int w: width, in pixels
                    screen_size.y,                      //    int h: height, in pixels
                    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE        //    Uint32 flags: window options, see docs
            );

            // Check that the window was successfully made
            if(sdl_window==NULL){
                // In the event that the window could not be made...
                std::cout << "Could not create window: " << SDL_GetError() << '\n';
                SDL_Quit();
                return 1;
            }

            for (int it = 0; it < SDL_GetNumRenderDrivers(); it++) {
                SDL_GetRenderDriverInfo(it, &info);
                strcat(rendername, info.name);
                strcat(rendername, " ");
            }

            sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_TARGETTEXTURE);
            SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderSetLogicalSize(sdl_renderer, screen_size.x, screen_size.y);

            int result = Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 1024 );
            // Check load
            if ( result != 0 ) {
                std::cout << "Failed to open audio: " << Mix_GetError() << std::endl;
            }

            return 0;
        }

        static void finiSDL() {
            // Close game controller
            SDL_JoystickClose( _gGameController );
            _gGameController = nullptr;

            SDL_DestroyWindow(sdl_window);
            SDL_Quit();
        }

        static void input() {
            SDL_Event e;
            while( SDL_PollEvent( &e ) != 0 ) {
                if( e.type == SDL_QUIT ) {
                    quit = true;
                    continue;
                }
                if (user_event) {
                    user_event(e);
                }
            }
        }

        static int init() {
            auto ret = initSDL();
            if (ret != 0) {
                return ret;
            }
            if (user_init) {
                user_init();
            }
            return 0;
        }

        static void update() {
            if (user_update) {
                user_update(fps.delta());
            }
        }

        static void render() {
            SDL_SetRenderDrawColor(sdl_renderer, background.r, background.g, background.b, background.a);
            SDL_RenderClear(sdl_renderer);
            if (user_render) {
                user_render(sdl_renderer);
            }
            SDL_RenderPresent(sdl_renderer);
        }

        static void fini() {
            if (user_fini) {
                user_fini();
            }
            finiSDL();
        }
    }

    int run() {
        using namespace __;
#if defined(__PSP__)
        psp::quit = &quit;
        psp::SetupCallbacks();
        sceCtrlSetSamplingCycle(0);
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
#endif
        auto ret = init();
        if (ret != 0) {
            return ret;
        }
        while (!quit) {
            fps.start();
            input();
            update();
            render();
            fps.next();
        }
        fini();
        return 0;
    }

    void quit() {
        __::quit = true;
    }

    inline SDL_Window* window() {
        return __::sdl_window;
    }

    inline SDL_Renderer* renderer() {
        return __::sdl_renderer;
    }

    inline char* res_path(char const* path) {
        static char ret[256] = {0};
#if defined(__vita__)
        snprintf(ret, sizeof(ret), "app0:%s", path);
#else
        snprintf(ret, sizeof(ret), "%s", path);
#endif
        return ret;
    }

    void draw(SDL_Renderer* renderer, SDL_Texture* img, SDL_Rect* srcrect, transform const& trans) {
        if (img == nullptr) {
            return;
        }
        int width = trans.size.x * abs(trans.scale.x);
        int height = trans.size.y * abs(trans.scale.y);
        SDL_Point center = {
            int(width * trans.anchor.x),
            int(height * trans.anchor.y),
        };
        SDL_Rect dstrect = {
            int(trans.position.x - center.x),
            int(trans.position.y - center.y),
            width, height,
        };
        auto flip = SDL_RendererFlip::SDL_FLIP_NONE;
        if (trans.scale.x < 0) {
            flip = SDL_RendererFlip::SDL_FLIP_HORIZONTAL;
        } else if (trans.scale.y < 0) {
            flip = SDL_RendererFlip::SDL_FLIP_VERTICAL;
        }
        SDL_RenderCopyEx(renderer, img, srcrect, &dstrect, trans.angle, &center, flip);
    }

    void draw(SDL_Renderer* renderer, SDL_Texture* img, transform const& trans) {
        draw(renderer, img, nullptr, trans);
    }

    // 键值
    namespace KeyCode {
        typedef enum {
            UNKNOWN = 0,
            UP,
            DOWN,
            LEFT,
            RIGHT,
            L1,
            R1,
            SELECT,
            START,
            A,
            B,
            X,
            Y,
            MAX,
        } Type;
    }

    KeyCode::Type psp_key(int key) {
        static KeyCode::Type const key_map[KeyCode::MAX] = {
                KeyCode::X,
                KeyCode::A,
                KeyCode::B,
                KeyCode::Y,
                KeyCode::L1,
                KeyCode::R1,
                KeyCode::DOWN,
                KeyCode::LEFT,
                KeyCode::UP,
                KeyCode::RIGHT,
                KeyCode::SELECT,
                KeyCode::START,
                KeyCode::UNKNOWN,
        };
        return key_map[ key % KeyCode::MAX ];
    }
}

#ifdef sdl2_main
#undef sdl2_main
#endif

#if defined(__PSP__)
#include <SDL_main.h>
#define sdl2_main() SDL_main(int argc, char * argv[])
#else
#define sdl2_main() main(int argc, char * argv[])
#endif

#endif //SDL2_TEST_SDL2_H
