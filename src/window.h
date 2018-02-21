#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#include <cstdint>
#include <string>
#include <utility>

#include <SDL2/SDL.h>
#include <GLFL/glfl.h>

#include "exceptions.h"
#include "mat.h"
#include "reflection.h"
#include "template_utils.h"
#include "utils.h"

class Window
  : TemplateUtils::MoveFunc<Window>
{
    // Window data named "*" (`SDL_SetWindowData(window, "*", value)`) is reserved to hold a pointer to the associated window object.
    friend TemplateUtils::MoveFunc<Window>;
  public:
    DefineExceptionBase(exception)
    DefineExceptionStatic(cant_init_sdl, :exception, "Can't initialize SDL video subsystem.",)
    DefineExceptionStatic(cant_create_window, :exception, "Can't create a window.",
        ( std::string , name     , "Name"     )
        ( std::string , settings , "Settings" )
    )
    DefineExceptionStatic(cant_create_context, :exception, "Can't create an OpenGL context.",
        ( std::string , name     , "Name"     )
        ( std::string , settings , "Settings" )
    )

    ReflectMemberEnum(SwapModes, (no_vsync)(vsync)(late_swap_tearing)(any_mode))
    ReflectMemberEnum(Profiles, (core)(compatibility)(es)(any_profile))
    ReflectMemberEnum(YesOrNo, (yes)(no)(dont_care))
    ReflectMemberEnum(Position, (centered)(undefined)(custom))

    struct Settings
    {
        ivec2 coords = {};
        ivec2 min_size = {};
        bool resizable = 0;
        bool share_context = 0;

        Reflect(Settings)
        ( (bool)(fullscreen)(=0),
          (int)(display)(=0),
          (Position)(position)(=undefined),
          (int)(gl_major)(=3),
          (int)(gl_minor)(=3),
          (Profiles)(gl_profile)(=compatibility),
          (bool)(gl_debug)(=0),
          (SwapModes)(swap_mode)(=late_swap_tearing),
          (YesOrNo)(hardware_acceleration)(=dont_care),
          (bool)(forward_compatibility)(=0),
          (int)(msaa)(=0),
          (ivec4)(color_bits)(={}),
          (int)(depth_bits,stencil_bits)(=0), )

        Settings() {}
        using ref = Settings &;
        ref MinSize(ivec2 sz)
        {
            min_size = sz;
            return *this;
        }
        ref Resizable(bool r = 1)
        {
            resizable = r;
            return *this;
        }
        ref Fullscreen(bool f = 1)
        {
            fullscreen = f;
            return *this;
        }
        ref Display(int n)
        {
            display = n;
            return *this;
        }
        ref Position(ivec2 p)
        {
            position = custom;
            coords = p;
            return *this;
        }
        ref Position_Centered()
        {
            position = centered;
            return *this;
        }
        ref Position_Undefined()
        {
            position = undefined;
            return *this;
        }
        ref GlVersion(int maj, int min)
        {
            gl_major = maj;
            gl_minor = min;
            return *this;
        }
        ref GlProfile(Profiles p)
        {
            gl_profile = p;
            return *this;
        }
        ref GlDebug(bool d = 1)
        {
            gl_debug = d;
            return *this;
        }
        ref ShareContext(bool s = 1)
        {
            share_context = s;
            return *this;
        }
        ref SwapMode(SwapModes s)
        {
            swap_mode = s;
            return *this;
        }
        ref HardwareAcceleration(YesOrNo s)
        {
            hardware_acceleration = s;
            return *this;
        }
        ref ForwardCompatibility(YesOrNo f)
        {
            forward_compatibility = f;
            return *this;
        }
        ref MSAA(int m)
        {
            msaa = m;
            return *this;
        }
        ref ColorBits(ivec4 b)
        {
            color_bits = b;
            return *this;
        }
        ref DepthBits(int b)
        {
            depth_bits = b;
            return *this;
        }
        ref StencilBits(int b)
        {
            stencil_bits = b;
            return *this;
        }
    };

  private:
    static void OnMove(Window &&from, Window &to) noexcept;

    class WindowHandleFuncs
    {
        template <typename> friend class ::Utils::Handle;
        static SDL_Window *Create(std::string name, ivec2 size, Settings &settings);
        static void Destroy(SDL_Window *window);
        static void Error(std::string name, ivec2 size, const Settings &settings);
    };

    using Handle_t = Utils::Handle<WindowHandleFuncs>;

    class ContextHandleFuncs
    {
        template <typename> friend class ::Utils::Handle;
        static SDL_GLContext Create(SDL_Window *window, std::string name, const Settings &settings);
        static void Destroy(SDL_GLContext context);
        static void Error(SDL_Window *window, std::string name, const Settings &settings);
    };

    using ContextHandle_t = Utils::Handle<ContextHandleFuncs>;


    Handle_t window;
    ContextHandle_t context;
    Utils::AutoPtr<glfl::context> func_context;

    bool fullscreen = 0,
         resizable = 0;

  public:
    Window() {}
    Window(Window &&) = default;
    Window &operator=(Window &&) = default;
    Window(std::string name, ivec2 size, Settings settings = {});
    ~Window();
    void Create(std::string new_name, ivec2 new_size, Settings new_settings = {});
    void Destroy();
    bool Exists() const;
    void Activate() const; // This is necessary when you have multiple windows and you need to switch between them.
    SDL_Window *Handle() const;
    SDL_GLContext ContextHandle() const;
    const glfl::context &FuncContext() const;

    static const Window *FromHandle(SDL_Window *); // Returns a Window associated with a handle       (using `SDL_GetWindowData(handle, "*")`) or 0 if not found.
    static const Window *FromID(uint32_t);         // Returns a Window associated with an internal ID (using `FromHandle(SDL_GetWindowFromID(id))`) or 0 if not found.

    void Swap() const;
    void Fullscreen(bool);
    void ToggleFullscreen();
    ivec2 Size() const;

    // Create() resets those variables:
    mutable bool closure_requested = 0,
                 size_changed      = 0;
};

#endif
