#include "everything.h"

Events::AutoErrorHandlers error_handlers;

Window win("TAU++", ivec2(1024,768), Window::Settings{}.GlVersion(2,1).MinSize(ivec2(640,480)).Resizable());
Timing::TickStabilizer tick_stabilizer(60);

int main(int, char **)
{
    uint64_t frame_start = Timing::Clock();

    while (1)
    {
        uint64_t time = Timing::Clock(), frame_delta = time - frame_start;
        frame_start = time;

        while (tick_stabilizer.Tick(frame_delta))
        {
            Events::Process();
            if (win.size_changed)
            {
                win.size_changed = 0;
                Graphics::Viewport(win.Size());
            }
        }

        win.Swap();
    }
}
