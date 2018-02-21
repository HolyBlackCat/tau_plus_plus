#include "window.h"

#include "program.h"

#include <utility>

static constexpr char window_data_name_this_ptr[] = "*";

static SDL_Window *active_window = 0;

void Window::OnMove(Window &&/*from*/, Window &to) noexcept
{
    SDL_SetWindowData(*to.window, window_data_name_this_ptr, (void *)&to);
}

SDL_Window *Window::WindowHandleFuncs::Create(std::string name, ivec2 size, Settings &settings)
{
    uint32_t flags = SDL_WINDOW_OPENGL;
    uint32_t context_flags = 0;

    if (settings.resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    switch (settings.position)
    {
      case centered:
        settings.coords = ivec2(SDL_WINDOWPOS_CENTERED_DISPLAY(settings.display));
        break;
      case undefined:
        settings.coords = ivec2(SDL_WINDOWPOS_UNDEFINED_DISPLAY(settings.display));
        break;
      case custom:
        // Nothing.
        break;
    }
    if (settings.gl_major || settings.gl_minor)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, settings.gl_major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, settings.gl_minor);
    }
    switch (settings.gl_profile)
    {
      case core:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        break;
      case compatibility:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        break;
      case es:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        break;
      default:
        break;
    }
    if (settings.gl_debug)
        context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    if (settings.share_context)
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    switch (settings.hardware_acceleration)
    {
      case yes:
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        break;
      case no:
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 0);
        break;
      default:
        break;
    }
    if (settings.forward_compatibility)
        context_flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
    if (settings.msaa == 1) settings.msaa = 0;
    if (settings.msaa)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, settings.msaa);
    }
    if (settings.color_bits.r) SDL_GL_SetAttribute(SDL_GL_RED_SIZE    , settings.color_bits.r);
    if (settings.color_bits.g) SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE  , settings.color_bits.g);
    if (settings.color_bits.b) SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE   , settings.color_bits.b);
    if (settings.color_bits.a) SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE  , settings.color_bits.a);
    if (settings.depth_bits  ) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE  , settings.depth_bits  );
    if (settings.stencil_bits) SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, settings.stencil_bits);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);

    SDL_Window *ret = SDL_CreateWindow(name.c_str(), settings.coords.x, settings.coords.y, size.x, size.y, flags);

    if (!settings.resizable)
    {
        SDL_DisplayMode mode{};
        mode.w = size.x;
        mode.h = size.y;
        SDL_GetClosestDisplayMode(settings.display, &mode, &mode);
        SDL_SetWindowDisplayMode(ret, &mode);
    }
    return ret;
}
void Window::WindowHandleFuncs::Destroy(SDL_Window *window)
{
    SDL_DestroyWindow(window);
}
void Window::WindowHandleFuncs::Error(std::string name, ivec2 /*size*/, const Settings &settings)
{
    throw cant_create_window(name, Reflection::to_string_tree(settings));
}

SDL_GLContext Window::ContextHandleFuncs::Create(SDL_Window *window, std::string /*name*/, const Settings &/*settings*/)
{
    return SDL_GL_CreateContext(window);
}
void Window::ContextHandleFuncs::Destroy(SDL_GLContext context)
{
    SDL_GL_DeleteContext(context);
}
void Window::ContextHandleFuncs::Error(SDL_Window */*window*/, std::string name, const Settings &settings)
{
    throw cant_create_context(name, Reflection::to_string_tree(settings));
}

Window::Window(std::string name, ivec2 size, Settings settings)
{
    Create(name, size, settings);
}

Window::~Window()
{
    if (active_window == *window)
        active_window = 0;
}

void Window::Create(std::string name, ivec2 size, Settings settings)
{
    static bool need_sdl_init = 1;
    if (need_sdl_init)
    {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO))
            throw cant_init_sdl();
        need_sdl_init = 0;
    }

    Handle_t new_window = {{name, size, settings}};
    ContextHandle_t new_context = {{*new_window, name, settings}};
    window  = new_window.move();
    context = new_context.move();

    if (settings.min_size)
        SDL_SetWindowMinimumSize(*window, settings.min_size.x, settings.min_size.y);

    switch (settings.swap_mode)
    {
      case no_vsync:
        if (SDL_GL_SetSwapInterval(0))
        {
            settings.swap_mode = vsync;
            SDL_GL_SetSwapInterval(1);
        }
        break;
      case vsync:
        if (SDL_GL_SetSwapInterval(1))
        {
            settings.swap_mode = no_vsync;
            SDL_GL_SetSwapInterval(0);
        }
        break;
      case late_swap_tearing:
        if (SDL_GL_SetSwapInterval(-1))
        {
            settings.swap_mode = vsync;
            if (SDL_GL_SetSwapInterval(1))
            {
                settings.swap_mode = no_vsync;
                SDL_GL_SetSwapInterval(0);
            }
        }
        break;
      default:
        break;
    }
    if (settings.fullscreen)
        Fullscreen(1);

    func_context.alloc();
    glfl::set_active_context(func_context);
    glfl::set_function_loader(SDL_GL_GetProcAddress);
    if (settings.gl_profile != es)
        glfl::load_gl(settings.gl_major, settings.gl_minor);
    else
        glfl::load_gles(settings.gl_major, settings.gl_minor);

    SDL_SetWindowData(*window, window_data_name_this_ptr, this);

    active_window = *window;

    fullscreen = settings.fullscreen;
    resizable = settings.resizable;

    closure_requested = size_changed = 0;
}
void Window::Destroy()
{
    context.destroy();
    window.destroy();
}
bool Window::Exists() const
{
    return bool(window);
}

void Window::Activate() const
{
    DebugAssert("Attempt to activate a null window.", *window);
    if (active_window == *window)
        return;

    active_window = *window;

    SDL_GL_MakeCurrent(*window, *context);
    glfl::set_active_context(func_context);
}

SDL_Window *Window::Handle() const
{
    return *window;
}
SDL_GLContext Window::ContextHandle() const
{
    return *context;
}
const glfl::context &Window::FuncContext() const
{
    return *func_context;
}

const Window *Window::FromHandle(SDL_Window *handle)
{
    return (Window *)SDL_GetWindowData(handle, window_data_name_this_ptr);
}
const Window *Window::FromID(uint32_t id)
{
    SDL_Window *win = SDL_GetWindowFromID(id);
    if (!win)
        return 0;
    return FromHandle(win);
}

void Window::Swap() const
{
    Activate();
    SDL_GL_SwapWindow(*window);
}

void Window::Fullscreen(bool f)
{
    fullscreen = f;
    if (f)
        SDL_SetWindowFullscreen(*window, resizable ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN);
    else
        SDL_SetWindowFullscreen(*window, 0);
}

void Window::ToggleFullscreen()
{
    Fullscreen(!fullscreen);
}

ivec2 Window::Size() const
{
    ivec2 ret;
    SDL_GetWindowSize(*window, &ret.x, &ret.y);
    return ret;
}
