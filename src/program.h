#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include <string>

#include "strings.h"

namespace Program
{
    void CleanUpOnExit(bool);

    [[noreturn]] void Exit(int = 0);
    [[noreturn]] void Error(std::string, int = 1);
    void SetErrorMessageBoxCaption(std::string);
    void SetErrorMessageBoxPrefix(std::string);
}

#ifdef NDEBUG
#  define DebugAssert(text_, /*expr*/...) do {} while (0)
#else
#  define DebugAssert(text_, /*expr*/...) do {if (!bool(__VA_ARGS__)) ::Program::Error(Str("Assertion failed: ", text_));} while (0)
#endif

#endif
