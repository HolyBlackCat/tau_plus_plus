#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include <string>

namespace UI
{
    enum MessageBoxType
    {
        info,
        warning,
        error,
    };
    void MessageBox(std::string title, std::string text, MessageBoxType type = info);
}

#endif
