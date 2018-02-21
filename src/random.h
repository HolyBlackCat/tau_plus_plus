#ifndef RANDOM_H_INCLUDED
#define RANDOM_H_INCLUDED

#include <ctime>
#include <type_traits>
#include <random>

#include "program.h"


namespace Rand
{
    inline std::mt19937 &Generator()
    {
        static std::mt19937 generator(std::time(0)); // We don't use `std::random_device` because it's broken on MinGW.
        return generator;
    }

    // `std::enable_if_t<1,T>` is there to make sure `T` is never deduced.

    // `0 <= returned_value < x`. (If `x < 0`, returns -random_int(-x). If `x == 0`, returns 0.) Sic! Unlike `random_real`, `x` can not be returned unless it's `0`.
    template <typename T = int> T random_int(std::enable_if_t<1,T> x)
    {
        static_assert(std::is_integral_v<T>, "The template parameter must be integral.");
        if (x > 0)
            return std::uniform_int_distribution<T>(0, x-1)(Generator());
        else if (x < 0)
            return std::uniform_int_distribution<T>(1-x, 0)(Generator());
        else
            return 0;
    }

    // `a <= returned_value <= b`. (If `b < a`, they are swapped.)
    template <typename T = int> T random_int_range(std::enable_if_t<1,T> a, std::enable_if_t<1,T> b)
    {
        static_assert(std::is_integral_v<T>, "The template parameter must be integral.");
        // We don't handle `a == b` separately, it seems to be well-defined.
        if (a < b)
            return std::uniform_int_distribution<T>(a, b)(Generator());
        else
            return std::uniform_int_distribution<T>(b, a)(Generator());
    }

    // `-x <= returned_value <= x`. (If `x < 0`, it's negated.)
    template <typename T = int> T random_int_range(std::enable_if_t<1,T> x)
    {
        static_assert(std::is_integral_v<T> && std::is_signed_v<T>, "The template parameter must be integral and signed.");
        return random_int_range<T>(-x, x);
    }

    // `0 <= returned_value <= x`. (If `x < 0`, returns -random_int(-x).) Sic! Unlike `random_int`, `x` can be returned.
    template <typename T = float> T random_real(std::enable_if_t<1,T> x)
    {
        static_assert(std::is_floating_point_v<T>, "The template parameter must be floating-point.");
        if (x > 0)
            return std::uniform_real_distribution<T>(0, x)(Generator());
        else if (x < 0)
            return std::uniform_real_distribution<T>(x, 0)(Generator());
        else
            return 0;
    }

    // `a <= returned_value <= b`. (If `b < a`, they are swapped.)
    template <typename T = float> T random_real_range(std::enable_if_t<1,T> a, std::enable_if_t<1,T> b)
    {
        static_assert(std::is_floating_point_v<T>, "The template parameter must be floating-point.");
        if (a < b)
            return std::uniform_real_distribution<T>(a, b)(Generator());
        else if (b < a)
            return std::uniform_real_distribution<T>(b, a)(Generator());
        else // We need to handle `a == b` separately. It doesn't seem to be well defined for reals (it is for integers, though).
            return a;
    }

    // `-x <= returned_value <= x`. (If `x < 0`, it's negated.)
    template <typename T = float> T random_real_range(std::enable_if_t<1,T> x)
    {
        return random_real_range<T>(-x, x);
    }
}

using Rand::random_int;
using Rand::random_int_range;
using Rand::random_real;
using Rand::random_real_range;

#endif
