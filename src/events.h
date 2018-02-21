#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include <cstdint>

#include "mat.h"

namespace Events
{
    // If this flag isn't handled, the program will be closed at the beginning of the next event tick.
    // If only one window is opened, this will be set when user tries to close it (but other exit reasons do exist).
    bool ExitRequested();
    void IgnoreExitRequest(); // Resets the above flag.
    void MakeExitRequest(); // Sets the above flag.

    using Time_t = uint32_t;
    Time_t Time();

    namespace Input
    {
        enum DeviceType {no_device, keyboard, mouse};
        using DeviceID = uint32_t;
        enum Action {pressed, released, repeated};
        using Index = int; // 0 is reserved for 'any key/button'.
        Time_t KeyTime(DeviceType, DeviceID, Action, Index);
        Index LastKey(DeviceType, DeviceID, Action);

        using VecIndex = int;
        enum MouseVectors {vec_mouse_pos, vec_mouse_pos_rel, vec_mouse_wheel, vec_mouse_count/*enum size*/};

        ivec2 Vector(DeviceType, DeviceID, VecIndex);
        Time_t VectorTime(DeviceType, DeviceID, VecIndex);

        const std::string &Text();
    }

    void Process();

    void SetErrorHandlers();

    struct AutoErrorHandlers // This can be used as a global variable type to set the handlers early.
    {
        AutoErrorHandlers() {SetErrorHandlers();}
    };
}

#endif
