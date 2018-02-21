#ifndef PLATFORM_H_INCLUDED

#include <SDL2/SDL_endian.h>

#if defined(ANDROID) || defined(__ANDROID__)
#  define PLATFORM_MOBILE
#else
#  define PLATFORM_PC
#endif

#ifdef PLATFORM_PC
#  define OnPC(...) __VA_ARGS__
#  define IsOnPC 1
#else
#  define OnPC(...)
#  define IsOnPC 0
#endif

#ifdef PLATFORM_MOBILE
#  define OnMobile(...) __VA_ARGS__
#  define IsOnMobile 1
#else
#  define OnMobile(...)
#  define IsOnMobile 0
#endif


#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#  define PLATFORM_BIG_ENDIAN
#elif SDL_BYTEORDER == SDL_LIL_ENDIAN
#  define PLATFORM_LIL_ENDIAN
#else
#  error Invalid endianness.
#endif

#endif
