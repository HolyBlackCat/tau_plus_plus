#include "events.h"

#include <atomic>
#include <csignal>
#include <exception>
#include <vector>

#include <SDL2/SDL.h>

#include "program.h"
#include "strings.h"
#include "window.h"

namespace Events
{
    static Time_t tick_counter = 0;

    Time_t Time()
    {
        return tick_counter;
    }


    static std::atomic_bool exit_requested = 0;

    bool ExitRequested()
    {
        return exit_requested.load();
    }
    void IgnoreExitRequest()
    {
        exit_requested = 0;
    }
    void MakeExitRequest()
    {
        exit_requested = 1;
    }

    namespace Input
    {
        class Keys
        {
            template <typename T> struct Pack
            {
                T value_pressed = 0, value_released = 0, value_repeated = 0;

                void Set(Action a, const T &value)
                {
                    switch (a)
                    {
                      case pressed:
                        value_pressed = value;
                        break;
                      case released:
                        value_released = value;
                        break;
                      case repeated:
                        value_repeated = value;
                        break;
                      default:
                        break;
                    }
                }
                T Get(Action a) const
                {
                    switch (a)
                    {
                      case pressed:
                        return value_pressed;
                      case released:
                        return value_released;
                      case repeated:
                        return value_repeated;
                      default:
                        return 0;
                    }
                }
            };
            std::vector<Pack<Time_t>> times;
            Pack<Index> last;

          public:
            Keys() {}
            Keys(int size) : times(size) {}

            Time_t GetTime(Action action, Index index) const
            {
                if (index < 0 || index >= int(times.size()))
                    return 0;
                return times[index].Get(action);
            }
            Index GetLast(Action action) const
            {
                return last.Get(action);
            }

            void Set(Action action, Index index)
            {
                if (index <= 0 || index >= int(times.size())) // Sic! `index == 0` can't be set directly.
                    return;
                times[0].Set(action, Time());
                times[index].Set(action, Time());
                last.Set(action, index);
            }
        };

        class Vectors
        {
            struct Pack
            {
                ivec2 vec = {};
                Time_t time = 0;
            };
            std::vector<Pack> array;

          public:
            Vectors() {}
            Vectors(int size) : array(size) {}

            ivec2 GetVec(VecIndex index) const
            {
                if (index < 0 || index >= int(array.size()))
                    return {};
                return array[index].vec;
            }
            Time_t GetTime(VecIndex index) const
            {
                if (index < 0 || index >= int(array.size()))
                    return 0;
                return array[index].time;
            }

            void Set(VecIndex index, ivec2 value)
            {
                if (index < 0 || index >= int(array.size()))
                    return;
                array[index].vec = value;
                array[index].time = Time();
            }
            void Accumulate(VecIndex index, ivec2 value)
            {
                if (index < 0 || index >= int(array.size()))
                    return;
                if (array[index].time == Time())
                {
                    array[index].vec += value;
                }
                else
                {
                    array[index].vec = value;
                    array[index].time = Time();
                }
            }
        };

        static Keys dummy_keys, keyboard_keys(SDL_NUM_SCANCODES), mouse_keys(31);
        static Vectors dummy_vecs, mouse_vecs(vec_mouse_count);

        static Keys &GetKeys(DeviceType type, DeviceID id)
        {
            (void)id;

            switch (type)
            {
              case keyboard:
                return keyboard_keys;
              case mouse:
                return mouse_keys;
              default:
                return dummy_keys;
            }
        }
        static Vectors &GetVectors(DeviceType type, DeviceID id)
        {
            (void)id;

            switch (type)
            {
              case mouse:
                return mouse_vecs;
              default:
                return dummy_vecs;
            }
        }

        Time_t KeyTime(DeviceType device, DeviceID id, Action action, Index index)
        {
            return GetKeys(device, id).GetTime(action, index);
        }
        Index LastKey(DeviceType device, DeviceID id, Action action)
        {
            return GetKeys(device, id).GetLast(action);
        }
        ivec2 Vector(DeviceType device, DeviceID id, VecIndex v_index)
        {
            return GetVectors(device, id).GetVec(v_index);
        }
        Time_t VectorTime(DeviceType device, DeviceID id, VecIndex v_index)
        {
            return GetVectors(device, id).GetTime(v_index);
        }


        static std::string text_input;

        const std::string &Text()
        {
            return text_input;
        }
    }

    void Process()
    {
        if (exit_requested)
            Program::Exit();

        // Sic! This is incremented before processing the events because we reserve `time == 0` for things that never happened.
        tick_counter++;

        SDL_Event event;

        Input::text_input = {};

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
              case SDL_QUIT:
                exit_requested = 1;
                break;

              case SDL_KEYDOWN:
                Input::keyboard_keys.Set(Input::repeated, event.key.keysym.scancode);
                if (event.key.repeat)
                    break;
                Input::keyboard_keys.Set(Input::pressed, event.key.keysym.scancode);
                break;
              case SDL_KEYUP:
                if (event.key.repeat) // Sic! We don't care about repeated releases.
                    break;
                Input::keyboard_keys.Set(Input::released, event.key.keysym.scancode);
                break;

              case SDL_MOUSEBUTTONDOWN:
                Input::mouse_keys.Set(Input::pressed, event.button.button);
                break;
              case SDL_MOUSEBUTTONUP:
                Input::mouse_keys.Set(Input::released, event.button.button);
                break;

              case SDL_MOUSEMOTION:
                Input::mouse_vecs.Set(Input::vec_mouse_pos, {event.motion.x, event.motion.y});
                Input::mouse_vecs.Accumulate(Input::vec_mouse_pos_rel, {event.motion.xrel, event.motion.yrel});
                break;

              case SDL_MOUSEWHEEL:
                Input::mouse_vecs.Accumulate(Input::vec_mouse_wheel, {event.wheel.x, -event.wheel.y});
                break;

              case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                  case SDL_WINDOWEVENT_CLOSE:
                    if (auto ptr = Window::FromID(event.window.windowID); ptr)
                        ptr->closure_requested = 1;
                    break;
                  case SDL_WINDOWEVENT_RESIZED:
                    if (auto ptr = Window::FromID(event.window.windowID); ptr)
                        ptr->size_changed = 1;
                    break;
                }
                break;

              case SDL_TEXTINPUT:
                Input::text_input += event.text.text;
                break;
            }
        }
    }

    void SetErrorHandlers()
    {
        auto handler = [](int sig)
        {
            switch (sig)
            {
              case SIGABRT:
              case SIGINT:
              case SIGTERM:
                exit_requested = 1;
                break;
              case SIGFPE:
                Program::Error("Signal: Floating point exception.", sig);
                break;
              case SIGILL:
                Program::Error("Signal: Illegal instruction.", sig);
                break;
              case SIGSEGV:
                Program::Error("Signal: Segmentation fault.", sig);
                break;
            }
        };
        for (int sig : {SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM})
            std::signal(sig, handler);

        std::set_terminate([]
        {
            std::exception_ptr e = std::current_exception();
            if (e)
            {
                try
                {
                    std::rethrow_exception(e);
                }
                catch (std::exception &e)
                {
                    Program::Error(Str("Exception: ", e.what()));
                }
                catch (...)
                {
                    Program::Error("Unknown exception.");
                }
            }
            Program::Error("Terminated.");
        });
    }
}
