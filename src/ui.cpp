#include "ui.h"

#include <SDL2/SDL.h>

namespace UI
{
    void MessageBox(std::string title, std::string text, MessageBoxType type)
    {
        int type_value;
        switch (type)
        {
          case info:
          default:
            type_value = SDL_MESSAGEBOX_INFORMATION;
            break;
          case warning:
            type_value = SDL_MESSAGEBOX_WARNING;
            break;
          case error:
            type_value = SDL_MESSAGEBOX_ERROR;
            break;
        }
        SDL_ShowSimpleMessageBox(type_value, title.c_str(), text.c_str(), 0);
    }
}
