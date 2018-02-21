#ifndef MAT_H_INCLUDED
#define MAT_H_INCLUDED

// Version 2.5.8 by HolyBlackCat

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <ios>
#include <istream>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


namespace Math
{
    inline namespace Vector // prototypes
    {
        template <unsigned int D, typename T> struct vec;
        template <unsigned int W, unsigned int H, typename T> using mat = vec<W, vec<H, T>>;
    }

    inline namespace Utility
    {
        struct custom_operator_base_t {constexpr custom_operator_base_t() {}};

        template <typename T> struct type_category
        {
            static constexpr bool vec = 0,
                                  mat = 0,
                                  vec_or_mat = 0,
                                  io = std::is_base_of_v<std::ios_base,T>,
                                  op = std::is_base_of_v<custom_operator_base_t,T>,
                                  io_or_op = io || op,
                                  other = !io_or_op;
        };
        template <unsigned int D, typename T> struct type_category<vec<D,T>>
        {
            static constexpr bool vec = 1,
                                  mat = 0,
                                  vec_or_mat = 1,
                                  io = 0,
                                  op = 0,
                                  io_or_op = 0,
                                  other = 0;
        };
        template <unsigned int W, unsigned int H, typename T> struct type_category<mat<W,H,T>>
        {
            static constexpr bool vec = 0,
                                  mat = 1,
                                  vec_or_mat = 1,
                                  io = 0,
                                  op = 0,
                                  io_or_op = 0,
                                  other = 0;
        };

        template <typename T> struct base_type_t_impl {using type = T;};
        template <unsigned int D, typename T> struct base_type_t_impl<vec<D,T>> {using type = typename vec<D,T>::type;};
        template <typename T> using base_type_t = typename base_type_t_impl<T>::type;

        template <typename T, typename TT> struct change_base_type_t_impl {using type = base_type_t<TT>;};
        template <unsigned int D, typename T, typename TT> struct change_base_type_t_impl<vec<D,T>,TT> {using type = vec<D,base_type_t<TT>>;};
        template <unsigned int W, unsigned int H, typename T, typename TT> struct change_base_type_t_impl<mat<W,H,T>,TT> {using type = mat<W,H,base_type_t<TT>>;};
        template <typename T, typename TT> using change_base_type_t = typename change_base_type_t_impl<T,TT>::type;

        template <typename T> using floating_point_t = std::conditional_t<std::is_floating_point_v<base_type_t<T>>, T, change_base_type_t<T, double>>;

        template <typename ...P> struct larger_type_t_impl {using type = void;};
        template <typename ...P> using larger_type_t = std::enable_if_t<!std::is_void_v<typename larger_type_t_impl<P...>::type>, typename larger_type_t_impl<P...>::type>;
        template <typename T, typename ...P> struct larger_type_t_impl<T, P...> {using type = larger_type_t<T, larger_type_t<P...>>;};
        template <typename T> struct larger_type_t_impl<T> {using type = T;};
        template <typename T, typename TT> struct larger_type_t_impl<T, TT>
        {
            template <bool A, typename B, typename C> using con = std::conditional_t<A,B,C>;
            using type =
            con< !std::is_arithmetic_v<T> || !std::is_arithmetic_v<TT>,
                void,
                con< std::is_integral_v<T> == std::is_integral_v<TT>,
                    con< (sizeof (T) == sizeof (TT)),
                        con< std::is_same_v<T, TT>,
                            T,
                            void
                        >,
                        con< (sizeof(T) > sizeof (TT)),
                           T,
                           TT
                        >>,
                    con< std::is_floating_point_v<T>,
                       T,
                       TT
                    >
                >
            >;
        };
        template <unsigned int D, typename T, typename TT> struct larger_type_t_impl<vec<D,T>, TT>
        {
            using type = change_base_type_t<vec<D,T>, larger_type_t<base_type_t<vec<D,T>>, TT>>;
        };
        template <unsigned int D, typename T, typename TT> struct larger_type_t_impl<TT, vec<D,T>>
        {
            using type = change_base_type_t<vec<D,T>, larger_type_t<base_type_t<vec<D,T>>, TT>>;
        };
        template <unsigned int D, typename T, typename TT> struct larger_type_t_impl<vec<D,T>, vec<D,TT>>
        {
            using type = vec<D, larger_type_t<T,TT>>;
        };

        inline void hash_combine(std::size_t &a, std::size_t b)
        {
            a ^= b + 0x9e3779b9 + (a << 6) + (a >> 2); // From Boost.
        }
    }

    inline namespace Strings
    {
        template <typename T,
                  int length = 0,
                  int precision = -1,
                  char format_ch = (std::is_floating_point_v<std::remove_extent_t<T>> ? 'g' :
                                    std::is_signed_v        <std::remove_extent_t<T>> ? 'i' :
                                    /* else */                                          'u'),
                  char ...flags>
        [[nodiscard]] std::string num_to_string(std::remove_extent_t<T> param)
        {
            char buffer[(std::is_array_v<T> ? std::extent_v<T> : 64) + 1];

            using type = std::remove_extent_t<T>;
            static_assert(std::is_arithmetic_v<type>, "The type must be arithmetic.");

            static_assert(
            []() constexpr -> bool
            {
                if constexpr (sizeof...(flags))
                {
                    char array[] = {flags...};
                    for (unsigned i = 0; i < sizeof...(flags); i++)
                    {
                        if (array[i] != '+'
                         && array[i] != ' '
                         && array[i] != '#'
                         && array[i] != '0')
                            return 0;
                        for (unsigned j = 0; j < i; j++)
                            if (array[i] == array[j])
                                return 0;
                    }
                }
                return 1;
            }
            (), "Invalid format flags.");

            if constexpr (std::is_floating_point_v<type>)
            {
                static_assert(format_ch == 'f' ||
                              format_ch == 'F' ||
                              format_ch == 'e' ||
                              format_ch == 'E' ||
                              format_ch == 'a' ||
                              format_ch == 'A' ||
                              format_ch == 'g' ||
                              format_ch == 'G', "Invalid format char for the type.");
                static constexpr char format[] = {'%', flags..., '*', '.', '*', std::is_same_v<type, long double> ? 'L' : 'l', format_ch, '\0'};
                std::snprintf(buffer, sizeof buffer, format, length, precision, param);
            }
            else
            {
                if constexpr (std::is_signed_v<type>)
                    static_assert(format_ch == 'd' ||
                                  format_ch == 'i', "Invalid format char for the type.");
                else
                    static_assert(format_ch == 'u' ||
                                  format_ch == 'o' ||
                                  format_ch == 'x' ||
                                  format_ch == 'X', "Invalid format char for the type.");

                using signed_type = typename std::conditional_t<std::is_same_v<type, bool>, std::enable_if<1, bool>, std::make_signed<type>>::type;
                if constexpr (std::is_same_v<signed_type, long long>)
                {
                    static constexpr char format[] = {'%', flags..., '*', '.', '*', 'l', 'l', format_ch, '\0'};
                    std::snprintf(buffer, sizeof buffer, format, length, precision, param);
                }
                else if constexpr (std::is_same_v<signed_type, long>)
                {
                    static constexpr char format[] = {'%', flags..., '*', '.', '*', 'l', format_ch, '\0'};
                    std::snprintf(buffer, sizeof buffer, format, length, precision, param);
                }
                else
                {
                    static constexpr char format[] = {'%', flags..., '*', '.', '*', format_ch, '\0'};
                    std::snprintf(buffer, sizeof buffer, format, length, precision, param);
                }
            }

            return buffer;
        }


        template <typename T> [[nodiscard]] std::enable_if_t<std::is_floating_point_v<T>, T> num_from_string_mid(const char *ptr, int *chars_consumed = 0, int /*base*/ = 0)
        {
            if (std::isspace(*ptr))
            {
                if (chars_consumed)
                    *chars_consumed = 0;
                return 0;
            }

            const char *end;
            T value;

            if constexpr (std::is_same_v<T, float>)
                value = std::strtof(ptr, (char **)&end);
            else if constexpr (std::is_same_v<T, double>)
                value = std::strtod(ptr, (char **)&end);
            else
                value = std::strtold(ptr, (char **)&end);

            if (ptr == end)
            {
                if (chars_consumed)
                    *chars_consumed = 0;
                return 0;
            }

            if (chars_consumed)
                *chars_consumed = end - ptr;
            return value;
        }
        template <typename T> [[nodiscard]] std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, T> num_from_string_mid(const char *ptr, int *chars_consumed = 0, int base = 0)
        {
            if (std::isspace(*ptr))
            {
                if (chars_consumed)
                    *chars_consumed = 0;
                return 0;
            }

            const char *end;
            T value;

            if constexpr (std::is_signed_v<T>)
            {
                if constexpr (sizeof (T) <= sizeof (long))
                    value = std::strtol(ptr, (char **)&end, base);
                else
                    value = std::strtoll(ptr, (char **)&end, base);
            }
            else
            {
                if constexpr (sizeof (T) <= sizeof (unsigned long))
                    value = std::strtoul(ptr, (char **)&end, base);
                else
                    value = std::strtoull(ptr, (char **)&end, base);
            }

            if (ptr == end)
            {
                if (chars_consumed)
                    *chars_consumed = 0;
                return 0;
            }

            if (chars_consumed)
                *chars_consumed = end - ptr;
            return value;
        }
        template <typename T> [[nodiscard]] std::enable_if_t<std::is_same_v<T, bool>, bool> num_from_string_mid(const char *ptr, int *chars_consumed = 0, int /*base*/ = 0)
        {
            switch (*ptr)
            {
              default:
                if (!strncmp(ptr, "true", 4))
                {
                    if (chars_consumed)
                        *chars_consumed = 4;
                    return 1;
                }
                if (!strncmp(ptr, "false", 5))
                {
                    if (chars_consumed)
                        *chars_consumed = 5;
                    return 0;
                }
                if (chars_consumed)
                    *chars_consumed = 0;
                return 0;
              case '0':
              case '1':
                if (chars_consumed)
                    *chars_consumed = 1;
                return *ptr - '0';
            }
        }

        template <typename T> [[nodiscard]] T num_from_string(const char *ptr, bool *success = 0, int base = 0)
        {
            int ch_con;
            T ret = num_from_string<T>(ptr, &ch_con, base);
            ch_con = ptr[ch_con] == '\0';
            if (success) *success = ch_con;
            if (ch_con)
                return ret;
            else
                return 0;
        }
    }

    inline namespace Vector
    {
        template <typename T> using vec2 = vec<2, T>; template <typename T> using vec3 = vec<3, T>; template <typename T> using vec4 = vec<4, T>;
        template <typename T> using mat2x2 = mat<2,2,T>; template <typename T> using mat3x2 = mat<3,2,T>; template <typename T> using mat4x2 = mat<4,2,T>;
        template <typename T> using mat2x3 = mat<2,3,T>; template <typename T> using mat3x3 = mat<3,3,T>; template <typename T> using mat4x3 = mat<4,3,T>;
        template <typename T> using mat2x4 = mat<2,4,T>; template <typename T> using mat3x4 = mat<3,4,T>; template <typename T> using mat4x4 = mat<4,4,T>;
        template <typename T> using mat2 = mat2x2<T>; template <typename T> using mat3 = mat3x3<T>; template <typename T> using mat4 = mat4x4<T>;

        template <unsigned int D> using bvec = vec<D,bool>;
        template <unsigned int W, unsigned int H> using bmat = mat<W,H,bool>;
        using bvec2 = vec<2,bool>; using bvec3 = vec<3,bool>; using bvec4 = vec<4,bool>;
        using bmat2x2 = mat<2,2,bool>; using bmat3x2 = mat<3,2,bool>; using bmat4x2 = mat<4,2,bool>;
        using bmat2x3 = mat<2,3,bool>; using bmat3x3 = mat<3,3,bool>; using bmat4x3 = mat<4,3,bool>;
        using bmat2x4 = mat<2,4,bool>; using bmat3x4 = mat<3,4,bool>; using bmat4x4 = mat<4,4,bool>;
        using bmat2 = bmat2x2; using bmat3 = bmat3x3; using bmat4 = bmat4x4;

        template <unsigned int D> using cvec = vec<D,char>;
        template <unsigned int W, unsigned int H> using cmat = mat<W,H,char>;
        using cvec2 = vec<2,char>; using cvec3 = vec<3,char>; using cvec4 = vec<4,char>;
        using cmat2x2 = mat<2,2,char>; using cmat3x2 = mat<3,2,char>; using cmat4x2 = mat<4,2,char>;
        using cmat2x3 = mat<2,3,char>; using cmat3x3 = mat<3,3,char>; using cmat4x3 = mat<4,3,char>;
        using cmat2x4 = mat<2,4,char>; using cmat3x4 = mat<3,4,char>; using cmat4x4 = mat<4,4,char>;
        using cmat2 = cmat2x2; using cmat3 = cmat3x3; using cmat4 = cmat4x4;

        template <unsigned int D> using ucvec = vec<D,unsigned char>;
        template <unsigned int W, unsigned int H> using ucmat = mat<W,H,unsigned char>;
        using ucvec2 = vec<2,unsigned char>; using ucvec3 = vec<3,unsigned char>; using ucvec4 = vec<4,unsigned char>;
        using ucmat2x2 = mat<2,2,unsigned char>; using ucmat3x2 = mat<3,2,unsigned char>; using ucmat4x2 = mat<4,2,unsigned char>;
        using ucmat2x3 = mat<2,3,unsigned char>; using ucmat3x3 = mat<3,3,unsigned char>; using ucmat4x3 = mat<4,3,unsigned char>;
        using ucmat2x4 = mat<2,4,unsigned char>; using ucmat3x4 = mat<3,4,unsigned char>; using ucmat4x4 = mat<4,4,unsigned char>;
        using ucmat2 = ucmat2x2; using ucmat3 = ucmat3x3; using ucmat4 = ucmat4x4;

        template <unsigned int D> using scvec = vec<D,signed char>;
        template <unsigned int W, unsigned int H> using scmat = mat<W,H,signed char>;
        using scvec2 = vec<2,signed char>; using scvec3 = vec<3,signed char>; using scvec4 = vec<4,signed char>;
        using scmat2x2 = mat<2,2,signed char>; using scmat3x2 = mat<3,2,signed char>; using scmat4x2 = mat<4,2,signed char>;
        using scmat2x3 = mat<2,3,signed char>; using scmat3x3 = mat<3,3,signed char>; using scmat4x3 = mat<4,3,signed char>;
        using scmat2x4 = mat<2,4,signed char>; using scmat3x4 = mat<3,4,signed char>; using scmat4x4 = mat<4,4,signed char>;
        using scmat2 = scmat2x2; using scmat3 = scmat3x3; using scmat4 = scmat4x4;

        template <unsigned int D> using svec = vec<D,short>;
        template <unsigned int W, unsigned int H> using smat = mat<W,H,short>;
        using svec2 = vec<2,short>; using svec3 = vec<3,short>; using svec4 = vec<4,short>;
        using smat2x2 = mat<2,2,short>; using smat3x2 = mat<3,2,short>; using smat4x2 = mat<4,2,short>;
        using smat2x3 = mat<2,3,short>; using smat3x3 = mat<3,3,short>; using smat4x3 = mat<4,3,short>;
        using smat2x4 = mat<2,4,short>; using smat3x4 = mat<3,4,short>; using smat4x4 = mat<4,4,short>;
        using smat2 = smat2x2; using smat3 = smat3x3; using smat4 = smat4x4;

        template <unsigned int D> using usvec = vec<D,unsigned short>;
        template <unsigned int W, unsigned int H> using usmat = mat<W,H,unsigned short>;
        using usvec2 = vec<2,unsigned short>; using usvec3 = vec<3,unsigned short>; using usvec4 = vec<4,unsigned short>;
        using usmat2x2 = mat<2,2,unsigned short>; using usmat3x2 = mat<3,2,unsigned short>; using usmat4x2 = mat<4,2,unsigned short>;
        using usmat2x3 = mat<2,3,unsigned short>; using usmat3x3 = mat<3,3,unsigned short>; using usmat4x3 = mat<4,3,unsigned short>;
        using usmat2x4 = mat<2,4,unsigned short>; using usmat3x4 = mat<3,4,unsigned short>; using usmat4x4 = mat<4,4,unsigned short>;
        using usmat2 = usmat2x2; using usmat3 = usmat3x3; using usmat4 = usmat4x4;

        template <unsigned int D> using ivec = vec<D,int>;
        template <unsigned int W, unsigned int H> using imat = mat<W,H,int>;
        using ivec2 = vec<2,int>; using ivec3 = vec<3,int>; using ivec4 = vec<4,int>;
        using imat2x2 = mat<2,2,int>; using imat3x2 = mat<3,2,int>; using imat4x2 = mat<4,2,int>;
        using imat2x3 = mat<2,3,int>; using imat3x3 = mat<3,3,int>; using imat4x3 = mat<4,3,int>;
        using imat2x4 = mat<2,4,int>; using imat3x4 = mat<3,4,int>; using imat4x4 = mat<4,4,int>;
        using imat2 = imat2x2; using imat3 = imat3x3; using imat4 = imat4x4;

        template <unsigned int D> using uvec = vec<D,unsigned int>;
        template <unsigned int W, unsigned int H> using umat = mat<W,H,unsigned int>;
        using uvec2 = vec<2,unsigned int>; using uvec3 = vec<3,unsigned int>; using uvec4 = vec<4,unsigned int>;
        using umat2x2 = mat<2,2,unsigned int>; using umat3x2 = mat<3,2,unsigned int>; using umat4x2 = mat<4,2,unsigned int>;
        using umat2x3 = mat<2,3,unsigned int>; using umat3x3 = mat<3,3,unsigned int>; using umat4x3 = mat<4,3,unsigned int>;
        using umat2x4 = mat<2,4,unsigned int>; using umat3x4 = mat<3,4,unsigned int>; using umat4x4 = mat<4,4,unsigned int>;
        using umat2 = umat2x2; using umat3 = umat3x3; using umat4 = umat4x4;

        template <unsigned int D> using lvec = vec<D,long>;
        template <unsigned int W, unsigned int H> using lmat = mat<W,H,long>;
        using lvec2 = vec<2,long>; using lvec3 = vec<3,long>; using lvec4 = vec<4,long>;
        using lmat2x2 = mat<2,2,long>; using lmat3x2 = mat<3,2,long>; using lmat4x2 = mat<4,2,long>;
        using lmat2x3 = mat<2,3,long>; using lmat3x3 = mat<3,3,long>; using lmat4x3 = mat<4,3,long>;
        using lmat2x4 = mat<2,4,long>; using lmat3x4 = mat<3,4,long>; using lmat4x4 = mat<4,4,long>;
        using lmat2 = lmat2x2; using lmat3 = lmat3x3; using lmat4 = lmat4x4;

        template <unsigned int D> using ulvec = vec<D,unsigned long>;
        template <unsigned int W, unsigned int H> using ulmat = mat<W,H,unsigned long>;
        using ulvec2 = vec<2,unsigned long>; using ulvec3 = vec<3,unsigned long>; using ulvec4 = vec<4,unsigned long>;
        using ulmat2x2 = mat<2,2,unsigned long>; using ulmat3x2 = mat<3,2,unsigned long>; using ulmat4x2 = mat<4,2,unsigned long>;
        using ulmat2x3 = mat<2,3,unsigned long>; using ulmat3x3 = mat<3,3,unsigned long>; using ulmat4x3 = mat<4,3,unsigned long>;
        using ulmat2x4 = mat<2,4,unsigned long>; using ulmat3x4 = mat<3,4,unsigned long>; using ulmat4x4 = mat<4,4,unsigned long>;
        using ulmat2 = ulmat2x2; using ulmat3 = ulmat3x3; using ulmat4 = ulmat4x4;

        template <unsigned int D> using llvec = vec<D,long long>;
        template <unsigned int W, unsigned int H> using llmat = mat<W,H,long long>;
        using llvec2 = vec<2,long long>; using llvec3 = vec<3,long long>; using llvec4 = vec<4,long long>;
        using llmat2x2 = mat<2,2,long long>; using llmat3x2 = mat<3,2,long long>; using llmat4x2 = mat<4,2,long long>;
        using llmat2x3 = mat<2,3,long long>; using llmat3x3 = mat<3,3,long long>; using llmat4x3 = mat<4,3,long long>;
        using llmat2x4 = mat<2,4,long long>; using llmat3x4 = mat<3,4,long long>; using llmat4x4 = mat<4,4,long long>;
        using llmat2 = llmat2x2; using llmat3 = llmat3x3; using llmat4 = llmat4x4;

        template <unsigned int D> using ullvec = vec<D,unsigned long long>;
        template <unsigned int W, unsigned int H> using ullmat = mat<W,H,unsigned long long>;
        using ullvec2 = vec<2,unsigned long long>; using ullvec3 = vec<3,unsigned long long>; using ullvec4 = vec<4,unsigned long long>;
        using ullmat2x2 = mat<2,2,unsigned long long>; using ullmat3x2 = mat<3,2,unsigned long long>; using ullmat4x2 = mat<4,2,unsigned long long>;
        using ullmat2x3 = mat<2,3,unsigned long long>; using ullmat3x3 = mat<3,3,unsigned long long>; using ullmat4x3 = mat<4,3,unsigned long long>;
        using ullmat2x4 = mat<2,4,unsigned long long>; using ullmat3x4 = mat<3,4,unsigned long long>; using ullmat4x4 = mat<4,4,unsigned long long>;
        using ullmat2 = ullmat2x2; using ullmat3 = ullmat3x3; using ullmat4 = ullmat4x4;

        template <unsigned int D> using fvec = vec<D,float>;
        template <unsigned int W, unsigned int H> using fmat = mat<W,H,float>;
        using fvec2 = vec<2,float>; using fvec3 = vec<3,float>; using fvec4 = vec<4,float>;
        using fmat2x2 = mat<2,2,float>; using fmat3x2 = mat<3,2,float>; using fmat4x2 = mat<4,2,float>;
        using fmat2x3 = mat<2,3,float>; using fmat3x3 = mat<3,3,float>; using fmat4x3 = mat<4,3,float>;
        using fmat2x4 = mat<2,4,float>; using fmat3x4 = mat<3,4,float>; using fmat4x4 = mat<4,4,float>;
        using fmat2 = fmat2x2; using fmat3 = fmat3x3; using fmat4 = fmat4x4;

        template <unsigned int D> using dvec = vec<D,double>;
        template <unsigned int W, unsigned int H> using dmat = mat<W,H,double>;
        using dvec2 = vec<2,double>; using dvec3 = vec<3,double>; using dvec4 = vec<4,double>;
        using dmat2x2 = mat<2,2,double>; using dmat3x2 = mat<3,2,double>; using dmat4x2 = mat<4,2,double>;
        using dmat2x3 = mat<2,3,double>; using dmat3x3 = mat<3,3,double>; using dmat4x3 = mat<4,3,double>;
        using dmat2x4 = mat<2,4,double>; using dmat3x4 = mat<3,4,double>; using dmat4x4 = mat<4,4,double>;
        using dmat2 = dmat2x2; using dmat3 = dmat3x3; using dmat4 = dmat4x4;

        template <unsigned int D> using ldvec = vec<D,long double>;
        template <unsigned int W, unsigned int H> using ldmat = mat<W,H,long double>;
        using ldvec2 = vec<2,long double>; using ldvec3 = vec<3,long double>; using ldvec4 = vec<4,long double>;
        using ldmat2x2 = mat<2,2,long double>; using ldmat3x2 = mat<3,2,long double>; using ldmat4x2 = mat<4,2,long double>;
        using ldmat2x3 = mat<2,3,long double>; using ldmat3x3 = mat<3,3,long double>; using ldmat4x3 = mat<4,3,long double>;
        using ldmat2x4 = mat<2,4,long double>; using ldmat3x4 = mat<3,4,long double>; using ldmat4x4 = mat<4,4,long double>;
        using ldmat2 = ldmat2x2; using ldmat3 = ldmat3x3; using ldmat4 = ldmat4x4;

        template <unsigned int D> using i8vec = vec<D,int8_t>;
        template <unsigned int W, unsigned int H> using i8mat = mat<W,H,int8_t>;
        using i8vec2 = vec<2,int8_t>; using i8vec3 = vec<3,int8_t>; using i8vec4 = vec<4,int8_t>;
        using i8mat2x2 = mat<2,2,int8_t>; using i8mat3x2 = mat<3,2,int8_t>; using i8mat4x2 = mat<4,2,int8_t>;
        using i8mat2x3 = mat<2,3,int8_t>; using i8mat3x3 = mat<3,3,int8_t>; using i8mat4x3 = mat<4,3,int8_t>;
        using i8mat2x4 = mat<2,4,int8_t>; using i8mat3x4 = mat<3,4,int8_t>; using i8mat4x4 = mat<4,4,int8_t>;
        using i8mat2 = i8mat2x2; using i8mat3 = i8mat3x3; using i8mat4 = i8mat4x4;

        template <unsigned int D> using u8vec = vec<D,uint8_t>;
        template <unsigned int W, unsigned int H> using u8mat = mat<W,H,uint8_t>;
        using u8vec2 = vec<2,uint8_t>; using u8vec3 = vec<3,uint8_t>; using u8vec4 = vec<4,uint8_t>;
        using u8mat2x2 = mat<2,2,uint8_t>; using u8mat3x2 = mat<3,2,uint8_t>; using u8mat4x2 = mat<4,2,uint8_t>;
        using u8mat2x3 = mat<2,3,uint8_t>; using u8mat3x3 = mat<3,3,uint8_t>; using u8mat4x3 = mat<4,3,uint8_t>;
        using u8mat2x4 = mat<2,4,uint8_t>; using u8mat3x4 = mat<3,4,uint8_t>; using u8mat4x4 = mat<4,4,uint8_t>;
        using u8mat2 = u8mat2x2; using u8mat3 = u8mat3x3; using u8mat4 = u8mat4x4;

        template <unsigned int D> using i16vec = vec<D,int16_t>;
        template <unsigned int W, unsigned int H> using i16mat = mat<W,H,int16_t>;
        using i16vec2 = vec<2,int16_t>; using i16vec3 = vec<3,int16_t>; using i16vec4 = vec<4,int16_t>;
        using i16mat2x2 = mat<2,2,int16_t>; using i16mat3x2 = mat<3,2,int16_t>; using i16mat4x2 = mat<4,2,int16_t>;
        using i16mat2x3 = mat<2,3,int16_t>; using i16mat3x3 = mat<3,3,int16_t>; using i16mat4x3 = mat<4,3,int16_t>;
        using i16mat2x4 = mat<2,4,int16_t>; using i16mat3x4 = mat<3,4,int16_t>; using i16mat4x4 = mat<4,4,int16_t>;
        using i16mat2 = i16mat2x2; using i16mat3 = i16mat3x3; using i16mat4 = i16mat4x4;

        template <unsigned int D> using u16vec = vec<D,uint16_t>;
        template <unsigned int W, unsigned int H> using u16mat = mat<W,H,uint16_t>;
        using u16vec2 = vec<2,uint16_t>; using u16vec3 = vec<3,uint16_t>; using u16vec4 = vec<4,uint16_t>;
        using u16mat2x2 = mat<2,2,uint16_t>; using u16mat3x2 = mat<3,2,uint16_t>; using u16mat4x2 = mat<4,2,uint16_t>;
        using u16mat2x3 = mat<2,3,uint16_t>; using u16mat3x3 = mat<3,3,uint16_t>; using u16mat4x3 = mat<4,3,uint16_t>;
        using u16mat2x4 = mat<2,4,uint16_t>; using u16mat3x4 = mat<3,4,uint16_t>; using u16mat4x4 = mat<4,4,uint16_t>;
        using u16mat2 = u16mat2x2; using u16mat3 = u16mat3x3; using u16mat4 = u16mat4x4;

        template <unsigned int D> using i32vec = vec<D,int32_t>;
        template <unsigned int W, unsigned int H> using i32mat = mat<W,H,int32_t>;
        using i32vec2 = vec<2,int32_t>; using i32vec3 = vec<3,int32_t>; using i32vec4 = vec<4,int32_t>;
        using i32mat2x2 = mat<2,2,int32_t>; using i32mat3x2 = mat<3,2,int32_t>; using i32mat4x2 = mat<4,2,int32_t>;
        using i32mat2x3 = mat<2,3,int32_t>; using i32mat3x3 = mat<3,3,int32_t>; using i32mat4x3 = mat<4,3,int32_t>;
        using i32mat2x4 = mat<2,4,int32_t>; using i32mat3x4 = mat<3,4,int32_t>; using i32mat4x4 = mat<4,4,int32_t>;
        using i32mat2 = i32mat2x2; using i32mat3 = i32mat3x3; using i32mat4 = i32mat4x4;

        template <unsigned int D> using u32vec = vec<D,uint32_t>;
        template <unsigned int W, unsigned int H> using u32mat = mat<W,H,uint32_t>;
        using u32vec2 = vec<2,uint32_t>; using u32vec3 = vec<3,uint32_t>; using u32vec4 = vec<4,uint32_t>;
        using u32mat2x2 = mat<2,2,uint32_t>; using u32mat3x2 = mat<3,2,uint32_t>; using u32mat4x2 = mat<4,2,uint32_t>;
        using u32mat2x3 = mat<2,3,uint32_t>; using u32mat3x3 = mat<3,3,uint32_t>; using u32mat4x3 = mat<4,3,uint32_t>;
        using u32mat2x4 = mat<2,4,uint32_t>; using u32mat3x4 = mat<3,4,uint32_t>; using u32mat4x4 = mat<4,4,uint32_t>;
        using u32mat2 = u32mat2x2; using u32mat3 = u32mat3x3; using u32mat4 = u32mat4x4;

        template <unsigned int D> using i64vec = vec<D,int64_t>;
        template <unsigned int W, unsigned int H> using i64mat = mat<W,H,int64_t>;
        using i64vec2 = vec<2,int64_t>; using i64vec3 = vec<3,int64_t>; using i64vec4 = vec<4,int64_t>;
        using i64mat2x2 = mat<2,2,int64_t>; using i64mat3x2 = mat<3,2,int64_t>; using i64mat4x2 = mat<4,2,int64_t>;
        using i64mat2x3 = mat<2,3,int64_t>; using i64mat3x3 = mat<3,3,int64_t>; using i64mat4x3 = mat<4,3,int64_t>;
        using i64mat2x4 = mat<2,4,int64_t>; using i64mat3x4 = mat<3,4,int64_t>; using i64mat4x4 = mat<4,4,int64_t>;
        using i64mat2 = i64mat2x2; using i64mat3 = i64mat3x3; using i64mat4 = i64mat4x4;

        template <unsigned int D> using u64vec = vec<D,uint64_t>;
        template <unsigned int W, unsigned int H> using u64mat = mat<W,H,uint64_t>;
        using u64vec2 = vec<2,uint64_t>; using u64vec3 = vec<3,uint64_t>; using u64vec4 = vec<4,uint64_t>;
        using u64mat2x2 = mat<2,2,uint64_t>; using u64mat3x2 = mat<3,2,uint64_t>; using u64mat4x2 = mat<4,2,uint64_t>;
        using u64mat2x3 = mat<2,3,uint64_t>; using u64mat3x3 = mat<3,3,uint64_t>; using u64mat4x3 = mat<4,3,uint64_t>;
        using u64mat2x4 = mat<2,4,uint64_t>; using u64mat3x4 = mat<3,4,uint64_t>; using u64mat4x4 = mat<4,4,uint64_t>;
        using u64mat2 = u64mat2x2; using u64mat3 = u64mat3x3; using u64mat4 = u64mat4x4;

        template <typename T> struct vec<2,T> // vec2
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int size = 2;
            static constexpr bool is_floating_point = std::is_floating_point_v<T>;
            union {T x, r, s;};
            union {T y, g, t;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py) : x(px), y(py) {}
            template <typename TT> constexpr vec(const vec2<TT> &obj) : x(obj.x), y(obj.y) {}
            [[nodiscard]] constexpr auto sum() const {return x + y;}
            [[nodiscard]] constexpr auto product() const {return x * y;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o};}
            template <typename TT> [[nodiscard]] constexpr vec2<TT> to() const {return {TT(x),TT(y)};}
            [[nodiscard]] constexpr auto len_sqr() const {return x*x + y*y;}
            [[nodiscard]] constexpr auto len() const {return std::sqrt(len_sqr());}
            template <typename TT> [[nodiscard]] constexpr auto dot(const vec2<TT> &o) const {return x*o.x + y*o.y;}
            template <typename TT> [[nodiscard]] constexpr auto cross(const vec2<TT> &o) const -> decltype(x * o.y - y * o.x) {return x * o.y - y * o.x;}
            [[nodiscard]] constexpr floating_point_t<T> ratio() const {return floating_point_t<T>(x) / floating_point_t<T>(y);}
            [[nodiscard]] constexpr auto norm() const -> vec2<decltype(T{}/len())> {auto l = len(); if (l == 0) return vec2<T>(0); else return *this / l;}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const vec2<P> &... p) const -> vec2<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {func(x, p.x...), func(y, p.y...)};}
            [[nodiscard]] constexpr vec3<T> to_vec3(T pz) const {return {x, y, pz};}
            [[nodiscard]] constexpr vec4<T> to_vec4(T pz, T pw) const {return {x, y, pz, pw};}
            [[nodiscard]] constexpr vec3<T> to_vec3() const {return to_vec3(T{});}
            [[nodiscard]] constexpr vec4<T> to_vec4() const {return to_vec4(T{}, T{});}
            [[nodiscard]] constexpr bool none() const {return !(x || y);}
            [[nodiscard]] constexpr bool any() const {return x || y;}
            [[nodiscard]] constexpr bool all() const {return x && y;}
            template <typename TT> [[nodiscard]] constexpr larger_type_t<T,TT> mul(const vec2<TT> &o) const {return {x*o.x+y*o.y};}
            template <typename TT> [[nodiscard]] constexpr vec2<larger_type_t<T,TT>> mul(const mat2x2<TT> &o) const {return {x*o.x.x+y*o.x.y, x*o.y.x+y*o.y.y};}
            template <typename TT> [[nodiscard]] constexpr vec3<larger_type_t<T,TT>> mul(const mat3x2<TT> &o) const {return {x*o.x.x+y*o.x.y, x*o.y.x+y*o.y.y, x*o.z.x+y*o.z.y};}
            template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const mat4x2<TT> &o) const {return {x*o.x.x+y*o.x.y, x*o.y.x+y*o.y.y, x*o.z.x+y*o.z.y, x*o.w.x+y*o.w.y};}
            [[nodiscard]] constexpr T min() const {return std::min({x,y});}
            [[nodiscard]] constexpr T max() const {return std::max({x,y});}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x) + sep + f(y) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("[ "," "," ]",num_to_string<T[12],12,4,'g','#'>); else return to_string("[ "," "," ]",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", "]", base);}
        };
        template <typename T> struct vec<3,T> // vec3
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int size = 3;
            static constexpr bool is_floating_point = std::is_floating_point_v<T>;
            union {T x, r, s;};
            union {T y, g, t;};
            union {T z, b, p;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz) : x(px), y(py), z(pz) {}
            template <typename TT> constexpr vec(const vec3<TT> &obj) : x(obj.x), y(obj.y), z(obj.z) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z;}
            [[nodiscard]] constexpr auto product() const {return x * y * z;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o};}
            template <typename TT> [[nodiscard]] constexpr vec3<TT> to() const {return {TT(x),TT(y),TT(z)};}
            [[nodiscard]] constexpr auto len_sqr() const {return x*x + y*y + z*z;}
            [[nodiscard]] constexpr auto len() const {return std::sqrt(len_sqr());}
            template <typename TT> [[nodiscard]] constexpr auto dot(const vec3<TT> &o) const {return x*o.x + y*o.y + z*o.z;}
            template <typename TT> [[nodiscard]] constexpr auto cross(const vec3<TT> &o) const -> vec3<decltype(y * o.z - z * o.y)> {return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};}
            [[nodiscard]] constexpr auto norm() const -> vec3<decltype(T{}/len())> {auto l = len(); if (l == 0) return vec3<T>(0); else return *this / l;}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const vec3<P> &... p) const -> vec3<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {func(x, p.x...), func(y, p.y...), func(z, p.z...)};}
            [[nodiscard]] constexpr vec2<T> to_vec2() const {return {x, y};}
            [[nodiscard]] constexpr vec4<T> to_vec4(T pw) const {return {x, y, z, pw};}
            [[nodiscard]] constexpr vec4<T> to_vec4() const {return to_vec4(T{});}
            [[nodiscard]] constexpr bool none() const {return !(x || y || z);}
            [[nodiscard]] constexpr bool any() const {return x || y || z;}
            [[nodiscard]] constexpr bool all() const {return x && y && z;}
            template <typename TT> [[nodiscard]] constexpr larger_type_t<T,TT> mul(const vec3<TT> &o) const {return {x*o.x+y*o.y+z*o.z};}
            template <typename TT> [[nodiscard]] constexpr vec2<larger_type_t<T,TT>> mul(const mat2x3<TT> &o) const {return {x*o.x.x+y*o.x.y+z*o.x.z, x*o.y.x+y*o.y.y+z*o.y.z};}
            template <typename TT> [[nodiscard]] constexpr vec3<larger_type_t<T,TT>> mul(const mat3x3<TT> &o) const {return {x*o.x.x+y*o.x.y+z*o.x.z, x*o.y.x+y*o.y.y+z*o.y.z, x*o.z.x+y*o.z.y+z*o.z.z};}
            template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const mat4x3<TT> &o) const {return {x*o.x.x+y*o.x.y+z*o.x.z, x*o.y.x+y*o.y.y+z*o.y.z, x*o.z.x+y*o.z.y+z*o.z.z, x*o.w.x+y*o.w.y+z*o.w.z};}
            [[nodiscard]] constexpr T min() const {return std::min({x,y,z});}
            [[nodiscard]] constexpr T max() const {return std::max({x,y,z});}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x) + sep + f(y) + sep + f(z) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("[ "," "," ]",num_to_string<T[12],12,4,'g','#'>); else return to_string("[ "," "," ]",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", "]", base);}
        };
        template <typename T> struct vec<4,T> // vec4
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int size = 4;
            static constexpr bool is_floating_point = std::is_floating_point_v<T>;
            union {T x, r, s;};
            union {T y, g, t;};
            union {T z, b, p;};
            union {T w, a, q;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z) || bool(w); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj), w(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz, decltype(x) pw) : x(px), y(py), z(pz), w(pw) {}
            template <typename TT> constexpr vec(const vec4<TT> &obj) : x(obj.x), y(obj.y), z(obj.z), w(obj.w) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z + w;}
            [[nodiscard]] constexpr auto product() const {return x * y * z * w;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_w(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_w(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_w(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_w(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_w(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_a(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_a(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_a(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_a(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_a(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_q(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_q(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_q(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_q(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_q(T o) const {return {x, y, z, w/o};}
            template <typename TT> [[nodiscard]] constexpr vec4<TT> to() const {return {TT(x),TT(y),TT(z),TT(w)};}
            [[nodiscard]] constexpr auto len_sqr() const {return x*x + y*y + z*z + w*w;}
            [[nodiscard]] constexpr auto len() const {return std::sqrt(len_sqr());}
            template <typename TT> [[nodiscard]] constexpr auto dot(const vec4<TT> &o) const {return x*o.x + y*o.y + z*o.z + w*o.w;}
            [[nodiscard]] constexpr auto norm() const -> vec4<decltype(T{}/len())> {auto l = len(); if (l == 0) return vec4<T>(0); else return *this / l;}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const vec4<P> &... p) const -> vec4<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {func(x, p.x...), func(y, p.y...), func(z, p.z...), func(w, p.w...)};}
            [[nodiscard]] constexpr vec2<T> to_vec2() const {return {x, y};}
            [[nodiscard]] constexpr vec3<T> to_vec3() const {return {x, y, z};}
            [[nodiscard]] constexpr bool none() const {return !(x || y || z || w);}
            [[nodiscard]] constexpr bool any() const {return x || y || z || w;}
            [[nodiscard]] constexpr bool all() const {return x && y && z && w;}
            template <typename TT> [[nodiscard]] constexpr larger_type_t<T,TT> mul(const vec4<TT> &o) const {return {x*o.x+y*o.y+z*o.z+w*o.w};}
            template <typename TT> [[nodiscard]] constexpr vec2<larger_type_t<T,TT>> mul(const mat2x4<TT> &o) const {return {x*o.x.x+y*o.x.y+z*o.x.z+w*o.x.w, x*o.y.x+y*o.y.y+z*o.y.z+w*o.y.w};}
            template <typename TT> [[nodiscard]] constexpr vec3<larger_type_t<T,TT>> mul(const mat3x4<TT> &o) const {return {x*o.x.x+y*o.x.y+z*o.x.z+w*o.x.w, x*o.y.x+y*o.y.y+z*o.y.z+w*o.y.w, x*o.z.x+y*o.z.y+z*o.z.z+w*o.z.w};}
            template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const mat4x4<TT> &o) const {return {x*o.x.x+y*o.x.y+z*o.x.z+w*o.x.w, x*o.y.x+y*o.y.y+z*o.y.z+w*o.y.w, x*o.z.x+y*o.z.y+z*o.z.z+w*o.z.w, x*o.w.x+y*o.w.y+z*o.w.z+w*o.w.w};}
            [[nodiscard]] constexpr T min() const {return std::min({x,y,z,w});}
            [[nodiscard]] constexpr T max() const {return std::max({x,y,z,w});}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x) + sep + f(y) + sep + f(z) + sep + f(w) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("[ "," "," ]",num_to_string<T[12],12,4,'g','#'>); else return to_string("[ "," "," ]",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", "]", base);}
        };
        template <typename T> struct vec<2,vec<2,T>> // mat2x2
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 2, height = 2;
            static constexpr bool is_floating_point = vec2<T>::is_floating_point;
            union {vec2<T> x, r, s;};
            union {vec2<T> y, g, t;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py) : x(px), y(py) {}
            template <typename TT> constexpr vec(const vec2<TT> &obj) : x(obj.x), y(obj.y) {}
            [[nodiscard]] constexpr auto sum() const {return x + y;}
            [[nodiscard]] constexpr auto product() const {return x * y;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o};}
            constexpr vec(T xx, T yx, T xy, T yy) : x(xx,xy), y(yx,yy) {}
            template <typename TT> [[nodiscard]] constexpr mat2x2<TT> to() const {return {TT(x.x),TT(y.x),TT(x.y),TT(y.y)};}
            [[nodiscard]] constexpr mat2x2<T> transpose() const {return {x.x,x.y,y.x,y.y};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 1};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v)
            {
                return {v.x, 0,
                        0, v.y};
            }
            [[nodiscard]] static constexpr vec rotate2D(T angle)
            {
                static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");
                T c = std::cos(angle);
                T s = std::sin(angle);
                return {c, -s,
                        s, c};
            }
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,0,x.y,y.y,0};}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,0,0,x.y,y.y,0,0};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,0,0};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,0,x.y,y.y,0,0,0,1};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,0,0,x.y,y.y,0,0,0,0,1,0};}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,0,0,0,0};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,0,x.y,y.y,0,0,0,1,0,0,0};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,0,0,x.y,y.y,0,0,0,0,1,0,0,0,0,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat2x2<P> &... p) const -> mat2x2<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || x.y || y.y);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || x.y || y.y;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && x.y && y.y;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,y.x,y.y});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,y.x,y.y});}
            [[nodiscard]] mat2<T> inverse() const
            {
                mat2<T> inv;
                inv.x.x =  y.y;
                inv.y.x = -y.x;
                inv.x.y = -x.y;
                inv.y.y =  x.x;
                T det = x.x * inv.x.x + x.y * inv.y.x;
                if (det == 0)
                    return mat2<T>::identity();
                det = 1.0f / det;
                return inv * det;
            }
            template <typename TT> [[nodiscard]] constexpr vec2<larger_type_t<T,TT>> mul(const vec2<TT> &o) const {return {x.x*o.x+y.x*o.y, x.y*o.x+y.y*o.y};}
            template <typename TT> [[nodiscard]] constexpr mat2x2<larger_type_t<T,TT>> mul(const mat2x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y};}
            template <typename TT> [[nodiscard]] constexpr mat3x2<larger_type_t<T,TT>> mul(const mat3x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.x*o.z.x+y.x*o.z.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.y*o.z.x+y.y*o.z.y};}
            template <typename TT> [[nodiscard]] constexpr mat4x2<larger_type_t<T,TT>> mul(const mat4x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.x*o.z.x+y.x*o.z.y, x.x*o.w.x+y.x*o.w.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.y*o.z.x+y.y*o.z.y, x.y*o.w.x+y.y*o.w.y};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + row_sep + f(x.y) + sep + f(y.y) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<3,vec<2,T>> // mat3x2
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 3, height = 2;
            static constexpr bool is_floating_point = vec2<T>::is_floating_point;
            union {vec2<T> x, r, s;};
            union {vec2<T> y, g, t;};
            union {vec2<T> z, b, p;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz) : x(px), y(py), z(pz) {}
            template <typename TT> constexpr vec(const vec3<TT> &obj) : x(obj.x), y(obj.y), z(obj.z) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z;}
            [[nodiscard]] constexpr auto product() const {return x * y * z;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o};}
            constexpr vec(T xx, T yx, T zx, T xy, T yy, T zy) : x(xx,xy), y(yx,yy), z(zx,zy) {}
            template <typename TT> [[nodiscard]] constexpr mat3x2<TT> to() const {return {TT(x.x),TT(y.x),TT(z.x),TT(x.y),TT(y.y),TT(z.y)};}
            [[nodiscard]] constexpr mat2x3<T> transpose() const {return {x.x,x.y,y.x,y.y,z.x,z.y};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 0, 1, 0};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat3x2();}
            [[nodiscard]] static constexpr vec ortho2D(const vec2<T> &min, const vec2<T> &max)
            {
                static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");
                return {2 / (max.x - min.x), 0, (min.x + max.x) / (min.x - max.x),
                        0, 2 / (max.y - min.y), (min.y + max.y) / (min.y - max.y)};
            }
            [[nodiscard]] static constexpr vec translate2D(const vec2<T> &v)
            {
                return {1, 0, v.x,
                        0, 1, v.y};
            }
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat3x2();}
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,0,0};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,z.x,x.y,y.y,z.y,0,0,1};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0,0,0,1,0};}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,0,0,0,0};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,z.x,x.y,y.y,z.y,0,0,1,0,0,0};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0,0,0,1,0,0,0,0,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat3x2<P> &... p) const -> mat3x2<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...), z.apply(func, p.z...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || z.x || x.y || y.y || z.y);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || z.x || x.y || y.y || z.y;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && z.x && x.y && y.y && z.y;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,y.x,y.y,z.x,z.y});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,y.x,y.y,z.x,z.y});}
            template <typename TT> [[nodiscard]] constexpr vec2<larger_type_t<T,TT>> mul(const vec3<TT> &o) const {return {x.x*o.x+y.x*o.y+z.x*o.z, x.y*o.x+y.y*o.y+z.y*o.z};}
            template <typename TT> [[nodiscard]] constexpr mat2x2<larger_type_t<T,TT>> mul(const mat2x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z};}
            template <typename TT> [[nodiscard]] constexpr mat3x2<larger_type_t<T,TT>> mul(const mat3x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z};}
            template <typename TT> [[nodiscard]] constexpr mat4x2<larger_type_t<T,TT>> mul(const mat4x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z, x.x*o.w.x+y.x*o.w.y+z.x*o.w.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z, x.y*o.w.x+y.y*o.w.y+z.y*o.w.z};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + sep + f(z.x) + row_sep + f(x.y) + sep + f(y.y) + sep + f(z.y) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<4,vec<2,T>> // mat4x2
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 4, height = 2;
            static constexpr bool is_floating_point = vec2<T>::is_floating_point;
            union {vec2<T> x, r, s;};
            union {vec2<T> y, g, t;};
            union {vec2<T> z, b, p;};
            union {vec2<T> w, a, q;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z) || bool(w); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj), w(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz, decltype(x) pw) : x(px), y(py), z(pz), w(pw) {}
            template <typename TT> constexpr vec(const vec4<TT> &obj) : x(obj.x), y(obj.y), z(obj.z), w(obj.w) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z + w;}
            [[nodiscard]] constexpr auto product() const {return x * y * z * w;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_w(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_w(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_w(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_w(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_w(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_a(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_a(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_a(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_a(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_a(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_q(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_q(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_q(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_q(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_q(T o) const {return {x, y, z, w/o};}
            constexpr vec(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy) : x(xx,xy), y(yx,yy), z(zx,zy), w(wx,wy) {}
            template <typename TT> [[nodiscard]] constexpr mat4x2<TT> to() const {return {TT(x.x),TT(y.x),TT(z.x),TT(w.x),TT(x.y),TT(y.y),TT(z.y),TT(w.y)};}
            [[nodiscard]] constexpr mat2x4<T> transpose() const {return {x.x,x.y,y.x,y.y,z.x,z.y,w.x,w.y};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 0, 0, 1, 0, 0};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat4x2();}
            [[nodiscard]] static constexpr vec ortho2D(const vec2<T> &min, const vec2<T> &max) {return mat3x2<T>::ortho2D(min, max).to_mat4x2();}
            [[nodiscard]] static constexpr vec translate2D(const vec2<T> &v) {return mat3x2<T>::translate2D(v).to_mat4x2();}
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat4x2();}
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,z.x,x.y,y.y,z.y};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,0,0};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,z.x,x.y,y.y,z.y,0,0,1};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,z.x,w.x,x.y,y.y,z.y,w.y,0,0,1,0};}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,0,0,0,0};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,z.x,x.y,y.y,z.y,0,0,1,0,0,0};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,z.x,w.x,x.y,y.y,z.y,w.y,0,0,1,0,0,0,0,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat4x2<P> &... p) const -> mat4x2<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...), z.apply(func, p.z...), w.apply(func, p.w...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || z.x || w.x || x.y || y.y || z.y || w.y);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || z.x || w.x || x.y || y.y || z.y || w.y;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && z.x && w.x && x.y && y.y && z.y && w.y;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,y.x,y.y,z.x,z.y,w.x,w.y});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,y.x,y.y,z.x,z.y,w.x,w.y});}
            template <typename TT> [[nodiscard]] constexpr vec2<larger_type_t<T,TT>> mul(const vec4<TT> &o) const {return {x.x*o.x+y.x*o.y+z.x*o.z+w.x*o.w, x.y*o.x+y.y*o.y+z.y*o.z+w.y*o.w};}
            template <typename TT> [[nodiscard]] constexpr mat2x2<larger_type_t<T,TT>> mul(const mat2x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w};}
            template <typename TT> [[nodiscard]] constexpr mat3x2<larger_type_t<T,TT>> mul(const mat3x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z+w.x*o.z.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z+w.y*o.z.w};}
            template <typename TT> [[nodiscard]] constexpr mat4x2<larger_type_t<T,TT>> mul(const mat4x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z+w.x*o.z.w, x.x*o.w.x+y.x*o.w.y+z.x*o.w.z+w.x*o.w.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z+w.y*o.z.w, x.y*o.w.x+y.y*o.w.y+z.y*o.w.z+w.y*o.w.w};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + sep + f(z.x) + sep + f(w.x) + row_sep + f(x.y) + sep + f(y.y) + sep + f(z.y) + sep + f(w.y) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<2,vec<3,T>> // mat2x3
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 2, height = 3;
            static constexpr bool is_floating_point = vec3<T>::is_floating_point;
            union {vec3<T> x, r, s;};
            union {vec3<T> y, g, t;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py) : x(px), y(py) {}
            template <typename TT> constexpr vec(const vec2<TT> &obj) : x(obj.x), y(obj.y) {}
            [[nodiscard]] constexpr auto sum() const {return x + y;}
            [[nodiscard]] constexpr auto product() const {return x * y;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o};}
            constexpr vec(T xx, T yx, T xy, T yy, T xz, T yz) : x(xx,xy,xz), y(yx,yy,yz) {}
            template <typename TT> [[nodiscard]] constexpr mat2x3<TT> to() const {return {TT(x.x),TT(y.x),TT(x.y),TT(y.y),TT(x.z),TT(y.z)};}
            [[nodiscard]] constexpr mat3x2<T> transpose() const {return {x.x,x.y,x.z,y.x,y.y,y.z};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 1, 0, 0};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat2x3();}
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat2x3();}
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,0,x.y,y.y,0};}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,0,0,x.y,y.y,0,0};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,0,x.y,y.y,0,x.z,y.z,1};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,0,0,x.y,y.y,0,0,x.z,y.z,1,0};}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,x.z,y.z,0,0};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,0,x.y,y.y,0,x.z,y.z,1,0,0,0};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,0,0,x.y,y.y,0,0,x.z,y.z,1,0,0,0,0,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat2x3<P> &... p) const -> mat2x3<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || x.y || y.y || x.z || y.z);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || x.y || y.y || x.z || y.z;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && x.y && y.y && x.z && y.z;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,x.z,y.x,y.y,y.z});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,x.z,y.x,y.y,y.z});}
            template <typename TT> [[nodiscard]] constexpr vec3<larger_type_t<T,TT>> mul(const vec2<TT> &o) const {return {x.x*o.x+y.x*o.y, x.y*o.x+y.y*o.y, x.z*o.x+y.z*o.y};}
            template <typename TT> [[nodiscard]] constexpr mat2x3<larger_type_t<T,TT>> mul(const mat2x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.z*o.x.x+y.z*o.x.y, x.z*o.y.x+y.z*o.y.y};}
            template <typename TT> [[nodiscard]] constexpr mat3x3<larger_type_t<T,TT>> mul(const mat3x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.x*o.z.x+y.x*o.z.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.y*o.z.x+y.y*o.z.y, x.z*o.x.x+y.z*o.x.y, x.z*o.y.x+y.z*o.y.y, x.z*o.z.x+y.z*o.z.y};}
            template <typename TT> [[nodiscard]] constexpr mat4x3<larger_type_t<T,TT>> mul(const mat4x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.x*o.z.x+y.x*o.z.y, x.x*o.w.x+y.x*o.w.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.y*o.z.x+y.y*o.z.y, x.y*o.w.x+y.y*o.w.y, x.z*o.x.x+y.z*o.x.y, x.z*o.y.x+y.z*o.y.y, x.z*o.z.x+y.z*o.z.y, x.z*o.w.x+y.z*o.w.y};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + row_sep + f(x.y) + sep + f(y.y) + row_sep + f(x.z) + sep + f(y.z) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<3,vec<3,T>> // mat3x3
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 3, height = 3;
            static constexpr bool is_floating_point = vec3<T>::is_floating_point;
            union {vec3<T> x, r, s;};
            union {vec3<T> y, g, t;};
            union {vec3<T> z, b, p;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz) : x(px), y(py), z(pz) {}
            template <typename TT> constexpr vec(const vec3<TT> &obj) : x(obj.x), y(obj.y), z(obj.z) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z;}
            [[nodiscard]] constexpr auto product() const {return x * y * z;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o};}
            constexpr vec(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz) : x(xx,xy,xz), y(yx,yy,yz), z(zx,zy,zz) {}
            template <typename TT> [[nodiscard]] constexpr mat3x3<TT> to() const {return {TT(x.x),TT(y.x),TT(z.x),TT(x.y),TT(y.y),TT(z.y),TT(x.z),TT(y.z),TT(z.z)};}
            [[nodiscard]] constexpr mat3x3<T> transpose() const {return {x.x,x.y,x.z,y.x,y.y,y.z,z.x,z.y,z.z};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 0, 1, 0, 0, 0, 1};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat3x3();}
            [[nodiscard]] static constexpr vec scale(const vec3<T> &v)
            {
                return {v.x, 0, 0,
                        0, v.y, 0,
                        0, 0, v.z};
            }
            [[nodiscard]] static constexpr vec ortho2D(const vec2<T> &min, const vec2<T> &max) {return mat3x2<T>::ortho2D(min, max).to_mat3x3();}
            [[nodiscard]] static constexpr vec translate2D(const vec2<T> &v) {return mat3x2<T>::translate2D(v).to_mat3x3();}
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat3x3();}
            [[nodiscard]] static constexpr vec rotate_with_normalized_axis(const vec3<T> &axis, T angle)
            {
                static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");
                T c = std::cos(angle);
                T s = std::sin(angle);
                return {axis.x * axis.x * (1 - c) + c, axis.x * axis.y * (1 - c) - axis.z * s, axis.x * axis.z * (1 - c) + axis.y * s,
                        axis.y * axis.x * (1 - c) + axis.z * s, axis.y * axis.y * (1 - c) + c, axis.y * axis.z * (1 - c) - axis.x * s,
                        axis.x * axis.z * (1 - c) - axis.y * s, axis.y * axis.z * (1 - c) + axis.x * s, axis.z * axis.z * (1 - c) + c};
            }
            [[nodiscard]] static constexpr vec rotate(const vec3<T> &axis, T angle)
            {
                static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");
                return rotate_with_normalized_axis(axis.norm(), angle);
            }
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,z.x,x.y,y.y,z.y};}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,x.z,y.z};}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0,x.z,y.z,z.z,0};}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,x.z,y.z,0,0};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z,0,0,0};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0,x.z,y.z,z.z,0,0,0,0,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat3x3<P> &... p) const -> mat3x3<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...), z.apply(func, p.z...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || z.x || x.y || y.y || z.y || x.z || y.z || z.z);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || z.x || x.y || y.y || z.y || x.z || y.z || z.z;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && z.x && x.y && y.y && z.y && x.z && y.z && z.z;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,x.z,y.x,y.y,y.z,z.x,z.y,z.z});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,x.z,y.x,y.y,y.z,z.x,z.y,z.z});}
            [[nodiscard]] mat3<T> inverse() const
            {
                mat3<T> inv;
                inv.x.x = y.y * z.z -
                          z.y * y.z;
                inv.y.x = z.x * y.z -
                          y.x * z.z;
                inv.z.x = y.x * z.y -
                          z.x * y.y;
                inv.x.y = z.y * x.z -
                          x.y * z.z;
                inv.y.y = x.x * z.z -
                          z.x * x.z;
                inv.z.y = z.x * x.y -
                          x.x * z.y;
                inv.x.z = x.y * y.z -
                          y.y * x.z;
                inv.y.z = y.x * x.z -
                          x.x * y.z;
                inv.z.z = x.x * y.y -
                          y.x * x.y;
                T det = x.x * inv.x.x + x.y * inv.y.x + x.z * inv.z.x;
                if (det == 0)
                    return mat3<T>::identity();
                det = 1.0f / det;
                return inv * det;
            }
            template <typename TT> [[nodiscard]] constexpr vec3<larger_type_t<T,TT>> mul(const vec3<TT> &o) const {return {x.x*o.x+y.x*o.y+z.x*o.z, x.y*o.x+y.y*o.y+z.y*o.z, x.z*o.x+y.z*o.y+z.z*o.z};}
            template <typename TT> [[nodiscard]] constexpr mat2x3<larger_type_t<T,TT>> mul(const mat2x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z};}
            template <typename TT> [[nodiscard]] constexpr mat3x3<larger_type_t<T,TT>> mul(const mat3x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z};}
            template <typename TT> [[nodiscard]] constexpr mat4x3<larger_type_t<T,TT>> mul(const mat4x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z, x.x*o.w.x+y.x*o.w.y+z.x*o.w.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z, x.y*o.w.x+y.y*o.w.y+z.y*o.w.z, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z, x.z*o.w.x+y.z*o.w.y+z.z*o.w.z};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + sep + f(z.x) + row_sep + f(x.y) + sep + f(y.y) + sep + f(z.y) + row_sep + f(x.z) + sep + f(y.z) + sep + f(z.z) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<4,vec<3,T>> // mat4x3
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 4, height = 3;
            static constexpr bool is_floating_point = vec3<T>::is_floating_point;
            union {vec3<T> x, r, s;};
            union {vec3<T> y, g, t;};
            union {vec3<T> z, b, p;};
            union {vec3<T> w, a, q;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z) || bool(w); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj), w(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz, decltype(x) pw) : x(px), y(py), z(pz), w(pw) {}
            template <typename TT> constexpr vec(const vec4<TT> &obj) : x(obj.x), y(obj.y), z(obj.z), w(obj.w) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z + w;}
            [[nodiscard]] constexpr auto product() const {return x * y * z * w;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_w(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_w(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_w(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_w(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_w(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_a(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_a(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_a(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_a(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_a(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_q(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_q(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_q(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_q(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_q(T o) const {return {x, y, z, w/o};}
            constexpr vec(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy, T xz, T yz, T zz, T wz) : x(xx,xy,xz), y(yx,yy,yz), z(zx,zy,zz), w(wx,wy,wz) {}
            template <typename TT> [[nodiscard]] constexpr mat4x3<TT> to() const {return {TT(x.x),TT(y.x),TT(z.x),TT(w.x),TT(x.y),TT(y.y),TT(z.y),TT(w.y),TT(x.z),TT(y.z),TT(z.z),TT(w.z)};}
            [[nodiscard]] constexpr mat3x4<T> transpose() const {return {x.x,x.y,x.z,y.x,y.y,y.z,z.x,z.y,z.z,w.x,w.y,w.z};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat4x3();}
            [[nodiscard]] static constexpr vec scale(const vec3<T> &v) {return mat3x3<T>::scale(v).to_mat4x3();}
            [[nodiscard]] static constexpr vec ortho2D(const vec2<T> &min, const vec2<T> &max) {return mat3x2<T>::ortho2D(min, max).to_mat4x3();}
            [[nodiscard]] static constexpr vec ortho(const vec2<T> &min, const vec2<T> &max, T near, T far)
            {
                static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");
                return {2 / (max.x - min.x), 0, 0, (min.x + max.x) / (min.x - max.x),
                        0, 2 / (max.y - min.y), 0, (min.y + max.y) / (min.y - max.y),
                        0, 0, 2 / (near - far), (near + far) / (near - far)};
            }
            [[nodiscard]] static constexpr vec look_at(const vec3<T> &src, const vec3<T> &dst, const vec3<T> &local_up)
            {
                static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");
                vec3<T> v3 = (src-dst).norm();
                vec3<T> v1 = local_up.cross(v3).norm();
                vec3<T> v2 = v3.cross(v1);
                return {v1.x, v1.y, v1.z, -src.x*v1.x - src.y*v1.y - src.z*v1.z,
                        v2.x, v2.y, v2.z, -src.x*v2.x - src.y*v2.y - src.z*v2.z,
                        v3.x, v3.y, v3.z, -src.x*v3.x - src.y*v3.y - src.z*v3.z};
            }
            [[nodiscard]] static constexpr vec translate2D(const vec2<T> &v) {return mat3x2<T>::translate2D(v).to_mat4x3();}
            [[nodiscard]] static constexpr vec translate(const vec3<T> &v)
            {
                return {1, 0, 0, v.x,
                        0, 1, 0, v.y,
                        0, 0, 1, v.z};
            }
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat4x3();}
            [[nodiscard]] static constexpr vec rotate_with_normalized_axis(const vec3<T> &axis, T angle) {return mat3x3<T>::rotate_with_normalized_axis(axis, angle).to_mat4x3();}
            [[nodiscard]] static constexpr vec rotate(const vec3<T> &axis, T angle) {return mat3x3<T>::rotate(axis, angle).to_mat4x3();}
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,z.x,x.y,y.y,z.y};}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,z.x,w.x,x.y,y.y,z.y,w.y};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,x.z,y.z};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,x.z,y.z,0,0};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z,0,0,0};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,z.x,w.x,x.y,y.y,z.y,w.y,x.z,y.z,z.z,w.z,0,0,0,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat4x3<P> &... p) const -> mat4x3<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...), z.apply(func, p.z...), w.apply(func, p.w...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || z.x || w.x || x.y || y.y || z.y || w.y || x.z || y.z || z.z || w.z);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || z.x || w.x || x.y || y.y || z.y || w.y || x.z || y.z || z.z || w.z;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && z.x && w.x && x.y && y.y && z.y && w.y && x.z && y.z && z.z && w.z;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,x.z,y.x,y.y,y.z,z.x,z.y,z.z,w.x,w.y,w.z});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,x.z,y.x,y.y,y.z,z.x,z.y,z.z,w.x,w.y,w.z});}
            template <typename TT> [[nodiscard]] constexpr vec3<larger_type_t<T,TT>> mul(const vec4<TT> &o) const {return {x.x*o.x+y.x*o.y+z.x*o.z+w.x*o.w, x.y*o.x+y.y*o.y+z.y*o.z+w.y*o.w, x.z*o.x+y.z*o.y+z.z*o.z+w.z*o.w};}
            template <typename TT> [[nodiscard]] constexpr mat2x3<larger_type_t<T,TT>> mul(const mat2x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z+w.z*o.x.w, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z+w.z*o.y.w};}
            template <typename TT> [[nodiscard]] constexpr mat3x3<larger_type_t<T,TT>> mul(const mat3x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z+w.x*o.z.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z+w.y*o.z.w, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z+w.z*o.x.w, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z+w.z*o.y.w, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z+w.z*o.z.w};}
            template <typename TT> [[nodiscard]] constexpr mat4x3<larger_type_t<T,TT>> mul(const mat4x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z+w.x*o.z.w, x.x*o.w.x+y.x*o.w.y+z.x*o.w.z+w.x*o.w.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z+w.y*o.z.w, x.y*o.w.x+y.y*o.w.y+z.y*o.w.z+w.y*o.w.w, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z+w.z*o.x.w, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z+w.z*o.y.w, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z+w.z*o.z.w, x.z*o.w.x+y.z*o.w.y+z.z*o.w.z+w.z*o.w.w};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + sep + f(z.x) + sep + f(w.x) + row_sep + f(x.y) + sep + f(y.y) + sep + f(z.y) + sep + f(w.y) + row_sep + f(x.z) + sep + f(y.z) + sep + f(z.z) + sep + f(w.z) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<2,vec<4,T>> // mat2x4
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 2, height = 4;
            static constexpr bool is_floating_point = vec4<T>::is_floating_point;
            union {vec4<T> x, r, s;};
            union {vec4<T> y, g, t;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py) : x(px), y(py) {}
            template <typename TT> constexpr vec(const vec2<TT> &obj) : x(obj.x), y(obj.y) {}
            [[nodiscard]] constexpr auto sum() const {return x + y;}
            [[nodiscard]] constexpr auto product() const {return x * y;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o};}
            constexpr vec(T xx, T yx, T xy, T yy, T xz, T yz, T xw, T yw) : x(xx,xy,xz,xw), y(yx,yy,yz,yw) {}
            template <typename TT> [[nodiscard]] constexpr mat2x4<TT> to() const {return {TT(x.x),TT(y.x),TT(x.y),TT(y.y),TT(x.z),TT(y.z),TT(x.w),TT(y.w)};}
            [[nodiscard]] constexpr mat4x2<T> transpose() const {return {x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 1, 0, 0, 0, 0};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat2x4();}
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat2x4();}
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,0,x.y,y.y,0};}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,0,0,x.y,y.y,0,0};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,x.z,y.z};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,0,x.y,y.y,0,x.z,y.z,1};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,0,0,x.y,y.y,0,0,x.z,y.z,1,0};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,0,x.y,y.y,0,x.z,y.z,1,x.w,y.w,0};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,0,0,x.y,y.y,0,0,x.z,y.z,1,0,x.w,y.w,0,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat2x4<P> &... p) const -> mat2x4<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || x.y || y.y || x.z || y.z || x.w || y.w);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || x.y || y.y || x.z || y.z || x.w || y.w;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && x.y && y.y && x.z && y.z && x.w && y.w;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w});}
            template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const vec2<TT> &o) const {return {x.x*o.x+y.x*o.y, x.y*o.x+y.y*o.y, x.z*o.x+y.z*o.y, x.w*o.x+y.w*o.y};}
            template <typename TT> [[nodiscard]] constexpr mat2x4<larger_type_t<T,TT>> mul(const mat2x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.z*o.x.x+y.z*o.x.y, x.z*o.y.x+y.z*o.y.y, x.w*o.x.x+y.w*o.x.y, x.w*o.y.x+y.w*o.y.y};}
            template <typename TT> [[nodiscard]] constexpr mat3x4<larger_type_t<T,TT>> mul(const mat3x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.x*o.z.x+y.x*o.z.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.y*o.z.x+y.y*o.z.y, x.z*o.x.x+y.z*o.x.y, x.z*o.y.x+y.z*o.y.y, x.z*o.z.x+y.z*o.z.y, x.w*o.x.x+y.w*o.x.y, x.w*o.y.x+y.w*o.y.y, x.w*o.z.x+y.w*o.z.y};}
            template <typename TT> [[nodiscard]] constexpr mat4x4<larger_type_t<T,TT>> mul(const mat4x2<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y, x.x*o.y.x+y.x*o.y.y, x.x*o.z.x+y.x*o.z.y, x.x*o.w.x+y.x*o.w.y, x.y*o.x.x+y.y*o.x.y, x.y*o.y.x+y.y*o.y.y, x.y*o.z.x+y.y*o.z.y, x.y*o.w.x+y.y*o.w.y, x.z*o.x.x+y.z*o.x.y, x.z*o.y.x+y.z*o.y.y, x.z*o.z.x+y.z*o.z.y, x.z*o.w.x+y.z*o.w.y, x.w*o.x.x+y.w*o.x.y, x.w*o.y.x+y.w*o.y.y, x.w*o.z.x+y.w*o.z.y, x.w*o.w.x+y.w*o.w.y};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + row_sep + f(x.y) + sep + f(y.y) + row_sep + f(x.z) + sep + f(y.z) + row_sep + f(x.w) + sep + f(y.w) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<3,vec<4,T>> // mat3x4
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 3, height = 4;
            static constexpr bool is_floating_point = vec4<T>::is_floating_point;
            union {vec4<T> x, r, s;};
            union {vec4<T> y, g, t;};
            union {vec4<T> z, b, p;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz) : x(px), y(py), z(pz) {}
            template <typename TT> constexpr vec(const vec3<TT> &obj) : x(obj.x), y(obj.y), z(obj.z) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z;}
            [[nodiscard]] constexpr auto product() const {return x * y * z;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o};}
            constexpr vec(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz, T xw, T yw, T zw) : x(xx,xy,xz,xw), y(yx,yy,yz,yw), z(zx,zy,zz,zw) {}
            template <typename TT> [[nodiscard]] constexpr mat3x4<TT> to() const {return {TT(x.x),TT(y.x),TT(z.x),TT(x.y),TT(y.y),TT(z.y),TT(x.z),TT(y.z),TT(z.z),TT(x.w),TT(y.w),TT(z.w)};}
            [[nodiscard]] constexpr mat4x3<T> transpose() const {return {x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w,z.x,z.y,z.z,z.w};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat3x4();}
            [[nodiscard]] static constexpr vec scale(const vec3<T> &v) {return mat3x3<T>::scale(v).to_mat3x4();}
            [[nodiscard]] static constexpr vec ortho2D(const vec2<T> &min, const vec2<T> &max) {return mat3x2<T>::ortho2D(min, max).to_mat3x4();}
            [[nodiscard]] static constexpr vec translate2D(const vec2<T> &v) {return mat3x2<T>::translate2D(v).to_mat3x4();}
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat3x4();}
            [[nodiscard]] static constexpr vec rotate_with_normalized_axis(const vec3<T> &axis, T angle) {return mat3x3<T>::rotate_with_normalized_axis(axis, angle).to_mat3x4();}
            [[nodiscard]] static constexpr vec rotate(const vec3<T> &axis, T angle) {return mat3x3<T>::rotate(axis, angle).to_mat3x4();}
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,z.x,x.y,y.y,z.y};}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,x.z,y.z};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0,x.z,y.z,z.z,0};}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,x.z,y.z,x.w,y.w};}
            [[nodiscard]] constexpr mat4x4<T> to_mat4x4() const {return {x.x,y.x,z.x,0,x.y,y.y,z.y,0,x.z,y.z,z.z,0,x.w,y.w,z.w,1};}
            [[nodiscard]] constexpr mat4<T> to_mat4() const {return to_mat4x4();}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat3x4<P> &... p) const -> mat3x4<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...), z.apply(func, p.z...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || z.x || x.y || y.y || z.y || x.z || y.z || z.z || x.w || y.w || z.w);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || z.x || x.y || y.y || z.y || x.z || y.z || z.z || x.w || y.w || z.w;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && z.x && x.y && y.y && z.y && x.z && y.z && z.z && x.w && y.w && z.w;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w,z.x,z.y,z.z,z.w});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w,z.x,z.y,z.z,z.w});}
            template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const vec3<TT> &o) const {return {x.x*o.x+y.x*o.y+z.x*o.z, x.y*o.x+y.y*o.y+z.y*o.z, x.z*o.x+y.z*o.y+z.z*o.z, x.w*o.x+y.w*o.y+z.w*o.z};}
            template <typename TT> [[nodiscard]] constexpr mat2x4<larger_type_t<T,TT>> mul(const mat2x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z, x.w*o.x.x+y.w*o.x.y+z.w*o.x.z, x.w*o.y.x+y.w*o.y.y+z.w*o.y.z};}
            template <typename TT> [[nodiscard]] constexpr mat3x4<larger_type_t<T,TT>> mul(const mat3x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z, x.w*o.x.x+y.w*o.x.y+z.w*o.x.z, x.w*o.y.x+y.w*o.y.y+z.w*o.y.z, x.w*o.z.x+y.w*o.z.y+z.w*o.z.z};}
            template <typename TT> [[nodiscard]] constexpr mat4x4<larger_type_t<T,TT>> mul(const mat4x3<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z, x.x*o.w.x+y.x*o.w.y+z.x*o.w.z, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z, x.y*o.w.x+y.y*o.w.y+z.y*o.w.z, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z, x.z*o.w.x+y.z*o.w.y+z.z*o.w.z, x.w*o.x.x+y.w*o.x.y+z.w*o.x.z, x.w*o.y.x+y.w*o.y.y+z.w*o.y.z, x.w*o.z.x+y.w*o.z.y+z.w*o.z.z, x.w*o.w.x+y.w*o.w.y+z.w*o.w.z};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + sep + f(z.x) + row_sep + f(x.y) + sep + f(y.y) + sep + f(z.y) + row_sep + f(x.z) + sep + f(y.z) + sep + f(z.z) + row_sep + f(x.w) + sep + f(y.w) + sep + f(z.w) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };
        template <typename T> struct vec<4,vec<4,T>> // mat4x4
        {
            static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
            static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
            using type = T;
            static constexpr int width = 4, height = 4;
            static constexpr bool is_floating_point = vec4<T>::is_floating_point;
            union {vec4<T> x, r, s;};
            union {vec4<T> y, g, t;};
            union {vec4<T> z, b, p;};
            union {vec4<T> w, a, q;};
            template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: static T ret; ret = {}; return ret;}}
            template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {case 0: return x; case 1: return y; case 2: return z; case 3: return w; default: return {};}}
            [[nodiscard]] explicit constexpr operator bool() const {return bool(x) || bool(y) || bool(z) || bool(w); static_assert(!std::is_same_v<base_type_t<vec>, bool>, "Use .none(), .any(), or .all() for vectors/matrices of bool.");}
            vec() = default;
            explicit constexpr vec(T obj) : x(obj), y(obj), z(obj), w(obj) {}
            constexpr vec(decltype(x) px, decltype(x) py, decltype(x) pz, decltype(x) pw) : x(px), y(py), z(pz), w(pw) {}
            template <typename TT> constexpr vec(const vec4<TT> &obj) : x(obj.x), y(obj.y), z(obj.z), w(obj.w) {}
            [[nodiscard]] constexpr auto sum() const {return x + y + z + w;}
            [[nodiscard]] constexpr auto product() const {return x * y * z * w;}
            [[nodiscard]] constexpr T *as_array() {return (T *)this;}
            [[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}
            [[nodiscard]] constexpr vec set_x(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_x(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_x(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_x(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_x(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_y(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_y(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_y(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_y(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_y(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_z(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_z(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_z(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_z(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_z(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_w(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_w(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_w(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_w(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_w(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_r(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_r(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_r(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_r(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_r(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_g(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_g(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_g(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_g(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_g(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_b(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_b(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_b(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_b(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_b(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_a(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_a(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_a(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_a(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_a(T o) const {return {x, y, z, w/o};}
            [[nodiscard]] constexpr vec set_s(T o) const {return {o, y, z, w};} [[nodiscard]] constexpr vec add_s(T o) const {return {x+o, y, z, w};} [[nodiscard]] constexpr vec sub_s(T o) const {return {x-o, y, z, w};} [[nodiscard]] constexpr vec mul_s(T o) const {return {x*o, y, z, w};} [[nodiscard]] constexpr vec div_s(T o) const {return {x/o, y, z, w};}
            [[nodiscard]] constexpr vec set_t(T o) const {return {x, o, z, w};} [[nodiscard]] constexpr vec add_t(T o) const {return {x, y+o, z, w};} [[nodiscard]] constexpr vec sub_t(T o) const {return {x, y-o, z, w};} [[nodiscard]] constexpr vec mul_t(T o) const {return {x, y*o, z, w};} [[nodiscard]] constexpr vec div_t(T o) const {return {x, y/o, z, w};}
            [[nodiscard]] constexpr vec set_p(T o) const {return {x, y, o, w};} [[nodiscard]] constexpr vec add_p(T o) const {return {x, y, z+o, w};} [[nodiscard]] constexpr vec sub_p(T o) const {return {x, y, z-o, w};} [[nodiscard]] constexpr vec mul_p(T o) const {return {x, y, z*o, w};} [[nodiscard]] constexpr vec div_p(T o) const {return {x, y, z/o, w};}
            [[nodiscard]] constexpr vec set_q(T o) const {return {x, y, z, o};} [[nodiscard]] constexpr vec add_q(T o) const {return {x, y, z, w+o};} [[nodiscard]] constexpr vec sub_q(T o) const {return {x, y, z, w-o};} [[nodiscard]] constexpr vec mul_q(T o) const {return {x, y, z, w*o};} [[nodiscard]] constexpr vec div_q(T o) const {return {x, y, z, w/o};}
            constexpr vec(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy, T xz, T yz, T zz, T wz, T xw, T yw, T zw, T ww) : x(xx,xy,xz,xw), y(yx,yy,yz,yw), z(zx,zy,zz,zw), w(wx,wy,wz,ww) {}
            template <typename TT> [[nodiscard]] constexpr mat4x4<TT> to() const {return {TT(x.x),TT(y.x),TT(z.x),TT(w.x),TT(x.y),TT(y.y),TT(z.y),TT(w.y),TT(x.z),TT(y.z),TT(z.z),TT(w.z),TT(x.w),TT(y.w),TT(z.w),TT(w.w)};}
            [[nodiscard]] constexpr mat4x4<T> transpose() const {return {x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w,z.x,z.y,z.z,z.w,w.x,w.y,w.z,w.w};}
            [[nodiscard]] static constexpr vec identity() {return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};}
            [[nodiscard]] static constexpr vec scale(const vec2<T> &v) {return mat2x2<T>::scale(v).to_mat4x4();}
            [[nodiscard]] static constexpr vec scale(const vec3<T> &v) {return mat3x3<T>::scale(v).to_mat4x4();}
            [[nodiscard]] static constexpr vec scale(const vec4<T> &v)
            {
                return {v.x, 0, 0, 0,
                        0, v.y, 0, 0,
                        0, 0, v.z, 0,
                        0, 0, 0, v.w};
            }
            [[nodiscard]] static constexpr vec ortho2D(const vec2<T> &min, const vec2<T> &max) {return mat3x2<T>::ortho2D(min, max).to_mat4x4();}
            [[nodiscard]] static constexpr vec ortho(const vec2<T> &min, const vec2<T> &max, T near, T far) {return mat4x3<T>::ortho(min, max, near, far).to_mat4x4();}
            [[nodiscard]] static constexpr vec look_at(const vec3<T> &src, const vec3<T> &dst, const vec3<T> &local_up) {return mat4x3<T>::look_at(src, dst, local_up).to_mat4x4();}
            [[nodiscard]] static constexpr vec translate2D(const vec2<T> &v) {return mat3x2<T>::translate2D(v).to_mat4x4();}
            [[nodiscard]] static constexpr vec translate(const vec3<T> &v) {return mat4x3<T>::translate(v).to_mat4x4();}
            [[nodiscard]] static constexpr vec rotate2D(T angle) {return mat2x2<T>::rotate2D(angle).to_mat4x4();}
            [[nodiscard]] static constexpr vec rotate_with_normalized_axis(const vec3<T> &axis, T angle) {return mat3x3<T>::rotate_with_normalized_axis(axis, angle).to_mat4x4();}
            [[nodiscard]] static constexpr vec rotate(const vec3<T> &axis, T angle) {return mat3x3<T>::rotate(axis, angle).to_mat4x4();}
            [[nodiscard]] static constexpr vec perspective(T yfov, T wh_aspect, T near, T far)
            {
                static_assert(is_floating_point, "This function only makes sense for floating-point matrices.");
                yfov = (T)1 / std::tan(yfov / 2);
                return {yfov / wh_aspect , 0    , 0                           , 0                             ,
                        0                , yfov , 0                           , 0                             ,
                        0                , 0    , (near + far) / (near - far) , 2 * near * far / (near - far) ,
                        0                , 0    , -1                          , 0                             };
            }
            [[nodiscard]] constexpr mat2x2<T> to_mat2x2() const {return {x.x,y.x,x.y,y.y};}
            [[nodiscard]] constexpr mat2<T> to_mat2() const {return to_mat2x2();}
            [[nodiscard]] constexpr mat3x2<T> to_mat3x2() const {return {x.x,y.x,z.x,x.y,y.y,z.y};}
            [[nodiscard]] constexpr mat4x2<T> to_mat4x2() const {return {x.x,y.x,z.x,w.x,x.y,y.y,z.y,w.y};}
            [[nodiscard]] constexpr mat2x3<T> to_mat2x3() const {return {x.x,y.x,x.y,y.y,x.z,y.z};}
            [[nodiscard]] constexpr mat3x3<T> to_mat3x3() const {return {x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z};}
            [[nodiscard]] constexpr mat3<T> to_mat3() const {return to_mat3x3();}
            [[nodiscard]] constexpr mat4x3<T> to_mat4x3() const {return {x.x,y.x,z.x,w.x,x.y,y.y,z.y,w.y,x.z,y.z,z.z,w.z};}
            [[nodiscard]] constexpr mat2x4<T> to_mat2x4() const {return {x.x,y.x,x.y,y.y,x.z,y.z,x.w,y.w};}
            [[nodiscard]] constexpr mat3x4<T> to_mat3x4() const {return {x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z,x.w,y.w,z.w};}
            template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat4x4<P> &... p) const -> mat4x4<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {x.apply(func, p.x...), y.apply(func, p.y...), z.apply(func, p.z...), w.apply(func, p.w...)};}
            [[nodiscard]] constexpr bool none() const {return !(x.x || y.x || z.x || w.x || x.y || y.y || z.y || w.y || x.z || y.z || z.z || w.z || x.w || y.w || z.w || w.w);}
            [[nodiscard]] constexpr bool any() const {return x.x || y.x || z.x || w.x || x.y || y.y || z.y || w.y || x.z || y.z || z.z || w.z || x.w || y.w || z.w || w.w;}
            [[nodiscard]] constexpr bool all() const {return x.x && y.x && z.x && w.x && x.y && y.y && z.y && w.y && x.z && y.z && z.z && w.z && x.w && y.w && z.w && w.w;}
            [[nodiscard]] constexpr T min() const {return std::min({x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w,z.x,z.y,z.z,z.w,w.x,w.y,w.z,w.w});}
            [[nodiscard]] constexpr T max() const {return std::max({x.x,x.y,x.z,x.w,y.x,y.y,y.z,y.w,z.x,z.y,z.z,z.w,w.x,w.y,w.z,w.w});}
            [[nodiscard]] mat4<T> inverse() const
            {
                mat4<T> inv;
                inv.x.x = y.y * z.z * w.w -
                          y.y * z.w * w.z -
                          z.y * y.z * w.w +
                          z.y * y.w * w.z +
                          w.y * y.z * z.w -
                          w.y * y.w * z.z;
                inv.y.x = y.x * z.w * w.z -
                          y.x * z.z * w.w +
                          z.x * y.z * w.w -
                          z.x * y.w * w.z -
                          w.x * y.z * z.w +
                          w.x * y.w * z.z;
                inv.z.x = y.x * z.y * w.w -
                          y.x * z.w * w.y -
                          z.x * y.y * w.w +
                          z.x * y.w * w.y +
                          w.x * y.y * z.w -
                          w.x * y.w * z.y;
                inv.w.x = y.x * z.z * w.y -
                          y.x * z.y * w.z +
                          z.x * y.y * w.z -
                          z.x * y.z * w.y -
                          w.x * y.y * z.z +
                          w.x * y.z * z.y;
                inv.x.y = x.y * z.w * w.z -
                          x.y * z.z * w.w +
                          z.y * x.z * w.w -
                          z.y * x.w * w.z -
                          w.y * x.z * z.w +
                          w.y * x.w * z.z;
                inv.y.y = x.x * z.z * w.w -
                          x.x * z.w * w.z -
                          z.x * x.z * w.w +
                          z.x * x.w * w.z +
                          w.x * x.z * z.w -
                          w.x * x.w * z.z;
                inv.z.y = x.x * z.w * w.y -
                          x.x * z.y * w.w +
                          z.x * x.y * w.w -
                          z.x * x.w * w.y -
                          w.x * x.y * z.w +
                          w.x * x.w * z.y;
                inv.w.y = x.x * z.y * w.z -
                          x.x * z.z * w.y -
                          z.x * x.y * w.z +
                          z.x * x.z * w.y +
                          w.x * x.y * z.z -
                          w.x * x.z * z.y;
                inv.x.z = x.y * y.z * w.w -
                          x.y * y.w * w.z -
                          y.y * x.z * w.w +
                          y.y * x.w * w.z +
                          w.y * x.z * y.w -
                          w.y * x.w * y.z;
                inv.y.z = x.x * y.w * w.z -
                          x.x * y.z * w.w +
                          y.x * x.z * w.w -
                          y.x * x.w * w.z -
                          w.x * x.z * y.w +
                          w.x * x.w * y.z;
                inv.z.z = x.x * y.y * w.w -
                          x.x * y.w * w.y -
                          y.x * x.y * w.w +
                          y.x * x.w * w.y +
                          w.x * x.y * y.w -
                          w.x * x.w * y.y;
                inv.w.z = x.x * y.z * w.y -
                          x.x * y.y * w.z +
                          y.x * x.y * w.z -
                          y.x * x.z * w.y -
                          w.x * x.y * y.z +
                          w.x * x.z * y.y;
                inv.x.w = x.y * y.w * z.z -
                          x.y * y.z * z.w +
                          y.y * x.z * z.w -
                          y.y * x.w * z.z -
                          z.y * x.z * y.w +
                          z.y * x.w * y.z;
                inv.y.w = x.x * y.z * z.w -
                          x.x * y.w * z.z -
                          y.x * x.z * z.w +
                          y.x * x.w * z.z +
                          z.x * x.z * y.w -
                          z.x * x.w * y.z;
                inv.z.w = x.x * y.w * z.y -
                          x.x * y.y * z.w +
                          y.x * x.y * z.w -
                          y.x * x.w * z.y -
                          z.x * x.y * y.w +
                          z.x * x.w * y.y;
                inv.w.w = x.x * y.y * z.z -
                          x.x * y.z * z.y -
                          y.x * x.y * z.z +
                          y.x * x.z * z.y +
                          z.x * x.y * y.z -
                          z.x * x.z * y.y;
                T det = x.x * inv.x.x + x.y * inv.y.x + x.z * inv.z.x + x.w * inv.w.x;
                if (det == 0)
                    return mat4<T>::identity();
                det = 1.0f / det;
                return inv * det;
            }
            template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const vec4<TT> &o) const {return {x.x*o.x+y.x*o.y+z.x*o.z+w.x*o.w, x.y*o.x+y.y*o.y+z.y*o.z+w.y*o.w, x.z*o.x+y.z*o.y+z.z*o.z+w.z*o.w, x.w*o.x+y.w*o.y+z.w*o.z+w.w*o.w};}
            template <typename TT> [[nodiscard]] constexpr mat2x4<larger_type_t<T,TT>> mul(const mat2x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z+w.z*o.x.w, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z+w.z*o.y.w, x.w*o.x.x+y.w*o.x.y+z.w*o.x.z+w.w*o.x.w, x.w*o.y.x+y.w*o.y.y+z.w*o.y.z+w.w*o.y.w};}
            template <typename TT> [[nodiscard]] constexpr mat3x4<larger_type_t<T,TT>> mul(const mat3x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z+w.x*o.z.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z+w.y*o.z.w, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z+w.z*o.x.w, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z+w.z*o.y.w, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z+w.z*o.z.w, x.w*o.x.x+y.w*o.x.y+z.w*o.x.z+w.w*o.x.w, x.w*o.y.x+y.w*o.y.y+z.w*o.y.z+w.w*o.y.w, x.w*o.z.x+y.w*o.z.y+z.w*o.z.z+w.w*o.z.w};}
            template <typename TT> [[nodiscard]] constexpr mat4x4<larger_type_t<T,TT>> mul(const mat4x4<TT> &o) const {return {x.x*o.x.x+y.x*o.x.y+z.x*o.x.z+w.x*o.x.w, x.x*o.y.x+y.x*o.y.y+z.x*o.y.z+w.x*o.y.w, x.x*o.z.x+y.x*o.z.y+z.x*o.z.z+w.x*o.z.w, x.x*o.w.x+y.x*o.w.y+z.x*o.w.z+w.x*o.w.w, x.y*o.x.x+y.y*o.x.y+z.y*o.x.z+w.y*o.x.w, x.y*o.y.x+y.y*o.y.y+z.y*o.y.z+w.y*o.y.w, x.y*o.z.x+y.y*o.z.y+z.y*o.z.z+w.y*o.z.w, x.y*o.w.x+y.y*o.w.y+z.y*o.w.z+w.y*o.w.w, x.z*o.x.x+y.z*o.x.y+z.z*o.x.z+w.z*o.x.w, x.z*o.y.x+y.z*o.y.y+z.z*o.y.z+w.z*o.y.w, x.z*o.z.x+y.z*o.z.y+z.z*o.z.z+w.z*o.z.w, x.z*o.w.x+y.z*o.w.y+z.z*o.w.z+w.z*o.w.w, x.w*o.x.x+y.w*o.x.y+z.w*o.x.z+w.w*o.x.w, x.w*o.y.x+y.w*o.y.y+z.w*o.y.z+w.w*o.y.w, x.w*o.z.x+y.w*o.z.y+z.w*o.z.z+w.w*o.z.w, x.w*o.w.x+y.w*o.w.y+z.w*o.w.z+w.w*o.w.w};}
            [[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start + f(x.x) + sep + f(y.x) + sep + f(z.x) + sep + f(w.x) + row_sep + f(x.y) + sep + f(y.y) + sep + f(z.y) + sep + f(w.y) + row_sep + f(x.z) + sep + f(y.z) + sep + f(z.z) + sep + f(w.z) + row_sep + f(x.w) + sep + f(y.w) + sep + f(z.w) + sep + f(w.w) + end;}
            [[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string("[", ",", ";", "]", f);}
            [[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,4,'g','#'>); else return to_string("/ "," "," |\n| "," /",num_to_string<T[12],12,-1>);}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
            {
                if (chars_consumed) *chars_consumed = 0;
                if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
                ret.x.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.x = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.y = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.z = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} pos += sep.size();
                ret.x.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.y.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.z.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} pos += sep.size();
                ret.w.w = num_from_string_mid<T>(src+pos, &offset, base); if (!offset) {return {};} pos += offset;
                if (strncmp(src+pos, end.c_str(), end.size())) {return {};} pos += end.size();
                if (chars_consumed) *chars_consumed = pos;
                return ret;
            }
            [[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}
            [[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, "[", ",", ";", "]", base);}
            [[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, "[", ",", ";", "]", base);}
        };

        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}+T2{})> operator+(const vec2<T1> &first, const vec2<T2> &second) {return {first.x+second.x,first.y+second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}+T2{})> operator+(const vec2<T1> &first, const T2 &second) {return {first.x+second,first.y+second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}+T2{})> operator+(const T1 &first, const vec2<T2> &second) {return {first+second.x,first+second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}-T2{})> operator-(const vec2<T1> &first, const vec2<T2> &second) {return {first.x-second.x,first.y-second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}-T2{})> operator-(const vec2<T1> &first, const T2 &second) {return {first.x-second,first.y-second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}-T2{})> operator-(const T1 &first, const vec2<T2> &second) {return {first-second.x,first-second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}*T2{})> operator*(const vec2<T1> &first, const vec2<T2> &second) {return {first.x*second.x,first.y*second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}*T2{})> operator*(const vec2<T1> &first, const T2 &second) {return {first.x*second,first.y*second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}*T2{})> operator*(const T1 &first, const vec2<T2> &second) {return {first*second.x,first*second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}/T2{})> operator/(const vec2<T1> &first, const vec2<T2> &second) {return {first.x/second.x,first.y/second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}/T2{})> operator/(const vec2<T1> &first, const T2 &second) {return {first.x/second,first.y/second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}/T2{})> operator/(const T1 &first, const vec2<T2> &second) {return {first/second.x,first/second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}%T2{})> operator%(const vec2<T1> &first, const vec2<T2> &second) {return {first.x%second.x,first.y%second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}%T2{})> operator%(const vec2<T1> &first, const T2 &second) {return {first.x%second,first.y%second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}%T2{})> operator%(const T1 &first, const vec2<T2> &second) {return {first%second.x,first%second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}^T2{})> operator^(const vec2<T1> &first, const vec2<T2> &second) {return {first.x^second.x,first.y^second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}^T2{})> operator^(const vec2<T1> &first, const T2 &second) {return {first.x^second,first.y^second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}^T2{})> operator^(const T1 &first, const vec2<T2> &second) {return {first^second.x,first^second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}&T2{})> operator&(const vec2<T1> &first, const vec2<T2> &second) {return {first.x&second.x,first.y&second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}&T2{})> operator&(const vec2<T1> &first, const T2 &second) {return {first.x&second,first.y&second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}&T2{})> operator&(const T1 &first, const vec2<T2> &second) {return {first&second.x,first&second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}|T2{})> operator|(const vec2<T1> &first, const vec2<T2> &second) {return {first.x|second.x,first.y|second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}|T2{})> operator|(const vec2<T1> &first, const T2 &second) {return {first.x|second,first.y|second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}|T2{})> operator|(const T1 &first, const vec2<T2> &second) {return {first|second.x,first|second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}<<T2{})> operator<<(const vec2<T1> &first, const vec2<T2> &second) {return {first.x<<second.x,first.y<<second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}<<T2{})> operator<<(const vec2<T1> &first, const T2 &second) {return {first.x<<second,first.y<<second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}<<T2{})> operator<<(const T1 &first, const vec2<T2> &second) {return {first<<second.x,first<<second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}>>T2{})> operator>>(const vec2<T1> &first, const vec2<T2> &second) {return {first.x>>second.x,first.y>>second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}>>T2{})> operator>>(const vec2<T1> &first, const T2 &second) {return {first.x>>second,first.y>>second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}>>T2{})> operator>>(const T1 &first, const vec2<T2> &second) {return {first>>second.x,first>>second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}<T2{})> operator<(const vec2<T1> &first, const vec2<T2> &second) {return {first.x<second.x,first.y<second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}<T2{})> operator<(const vec2<T1> &first, const T2 &second) {return {first.x<second,first.y<second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}<T2{})> operator<(const T1 &first, const vec2<T2> &second) {return {first<second.x,first<second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}>T2{})> operator>(const vec2<T1> &first, const vec2<T2> &second) {return {first.x>second.x,first.y>second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}>T2{})> operator>(const vec2<T1> &first, const T2 &second) {return {first.x>second,first.y>second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}>T2{})> operator>(const T1 &first, const vec2<T2> &second) {return {first>second.x,first>second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}<=T2{})> operator<=(const vec2<T1> &first, const vec2<T2> &second) {return {first.x<=second.x,first.y<=second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}<=T2{})> operator<=(const vec2<T1> &first, const T2 &second) {return {first.x<=second,first.y<=second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}<=T2{})> operator<=(const T1 &first, const vec2<T2> &second) {return {first<=second.x,first<=second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec2<decltype(T1{}>=T2{})> operator>=(const vec2<T1> &first, const vec2<T2> &second) {return {first.x>=second.x,first.y>=second.y};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}>=T2{})> operator>=(const vec2<T1> &first, const T2 &second) {return {first.x>=second,first.y>=second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec2<decltype(T1{}>=T2{})> operator>=(const T1 &first, const vec2<T2> &second) {return {first>=second.x,first>=second.y};}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const vec2<T1> &first, const vec2<T2> &second) {return bool(first) && bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const vec2<T1> &first, const T2 &second) {return bool(first) && second;}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const T1 &first, const vec2<T2> &second) {return first && bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const vec2<T1> &first, const vec2<T2> &second) {return bool(first) || bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const vec2<T1> &first, const T2 &second) {return bool(first) || second;}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const T1 &first, const vec2<T2> &second) {return first || bool(second);}
        template <typename T> [[nodiscard]] constexpr vec2<decltype(~T{})> operator~(const vec2<T> &object) {return {~object.x,~object.y};}
        template <typename T> [[nodiscard]] constexpr vec2<decltype(+T{})> operator+(const vec2<T> &object) {return {+object.x,+object.y};}
        template <typename T> [[nodiscard]] constexpr vec2<decltype(-T{})> operator-(const vec2<T> &object) {return {-object.x,-object.y};}
        template <typename T> [[nodiscard]] constexpr vec2<decltype(!T{})> operator!(const vec2<T> &object) {return !bool(object);}
        template <typename T> constexpr vec2<T> &operator++(vec2<T> &object) {++object.x; ++object.y; return object;}
        template <typename T> constexpr vec2<T> operator++(vec2<T> &object, int) {return {object.x++,object.y++};}
        template <typename T> constexpr vec2<T> &operator--(vec2<T> &object) {--object.x; --object.y; return object;}
        template <typename T> constexpr vec2<T> operator--(vec2<T> &object, int) {return {object.x--,object.y--};}
        template <typename T1, typename T2> constexpr vec2<T1> &operator+=(vec2<T1> &first, const vec2<T2> &second) {first.x += second.x; first.y += second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator+=(vec2<T1> &first, const T2 &second) {first.x += second; first.y += second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator-=(vec2<T1> &first, const vec2<T2> &second) {first.x -= second.x; first.y -= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator-=(vec2<T1> &first, const T2 &second) {first.x -= second; first.y -= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator*=(vec2<T1> &first, const vec2<T2> &second) {first.x *= second.x; first.y *= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator*=(vec2<T1> &first, const T2 &second) {first.x *= second; first.y *= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator/=(vec2<T1> &first, const vec2<T2> &second) {first.x /= second.x; first.y /= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator/=(vec2<T1> &first, const T2 &second) {first.x /= second; first.y /= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator%=(vec2<T1> &first, const vec2<T2> &second) {first.x %= second.x; first.y %= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator%=(vec2<T1> &first, const T2 &second) {first.x %= second; first.y %= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator^=(vec2<T1> &first, const vec2<T2> &second) {first.x ^= second.x; first.y ^= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator^=(vec2<T1> &first, const T2 &second) {first.x ^= second; first.y ^= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator&=(vec2<T1> &first, const vec2<T2> &second) {first.x &= second.x; first.y &= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator&=(vec2<T1> &first, const T2 &second) {first.x &= second; first.y &= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator|=(vec2<T1> &first, const vec2<T2> &second) {first.x |= second.x; first.y |= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator|=(vec2<T1> &first, const T2 &second) {first.x |= second; first.y |= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator<<=(vec2<T1> &first, const vec2<T2> &second) {first.x <<= second.x; first.y <<= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator<<=(vec2<T1> &first, const T2 &second) {first.x <<= second; first.y <<= second; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator>>=(vec2<T1> &first, const vec2<T2> &second) {first.x >>= second.x; first.y >>= second.y; return first;}
        template <typename T1, typename T2> constexpr vec2<T1> &operator>>=(vec2<T1> &first, const T2 &second) {first.x >>= second; first.y >>= second; return first;}

        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const vec2<T1> &first, const vec2<T2> &second) {return (first.x == second.x) && (first.y == second.y);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const vec2<T1> &first, const T2 &second) {return (first.x == second) && (first.y == second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const T1 &first, const vec2<T2> &second) {return (first == second.x) && (first == second.y);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const vec2<T1> &first, const vec2<T2> &second) {return (first.x != second.x) || (first.y != second.y);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const vec2<T1> &first, const T2 &second) {return (first.x != second) || (first.y != second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const T1 &first, const vec2<T2> &second) {return (first != second.x) || (first != second.y);}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}+T2{})> operator+(const vec3<T1> &first, const vec3<T2> &second) {return {first.x+second.x,first.y+second.y,first.z+second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}+T2{})> operator+(const vec3<T1> &first, const T2 &second) {return {first.x+second,first.y+second,first.z+second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}+T2{})> operator+(const T1 &first, const vec3<T2> &second) {return {first+second.x,first+second.y,first+second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}-T2{})> operator-(const vec3<T1> &first, const vec3<T2> &second) {return {first.x-second.x,first.y-second.y,first.z-second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}-T2{})> operator-(const vec3<T1> &first, const T2 &second) {return {first.x-second,first.y-second,first.z-second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}-T2{})> operator-(const T1 &first, const vec3<T2> &second) {return {first-second.x,first-second.y,first-second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}*T2{})> operator*(const vec3<T1> &first, const vec3<T2> &second) {return {first.x*second.x,first.y*second.y,first.z*second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}*T2{})> operator*(const vec3<T1> &first, const T2 &second) {return {first.x*second,first.y*second,first.z*second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}*T2{})> operator*(const T1 &first, const vec3<T2> &second) {return {first*second.x,first*second.y,first*second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}/T2{})> operator/(const vec3<T1> &first, const vec3<T2> &second) {return {first.x/second.x,first.y/second.y,first.z/second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}/T2{})> operator/(const vec3<T1> &first, const T2 &second) {return {first.x/second,first.y/second,first.z/second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}/T2{})> operator/(const T1 &first, const vec3<T2> &second) {return {first/second.x,first/second.y,first/second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}%T2{})> operator%(const vec3<T1> &first, const vec3<T2> &second) {return {first.x%second.x,first.y%second.y,first.z%second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}%T2{})> operator%(const vec3<T1> &first, const T2 &second) {return {first.x%second,first.y%second,first.z%second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}%T2{})> operator%(const T1 &first, const vec3<T2> &second) {return {first%second.x,first%second.y,first%second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}^T2{})> operator^(const vec3<T1> &first, const vec3<T2> &second) {return {first.x^second.x,first.y^second.y,first.z^second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}^T2{})> operator^(const vec3<T1> &first, const T2 &second) {return {first.x^second,first.y^second,first.z^second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}^T2{})> operator^(const T1 &first, const vec3<T2> &second) {return {first^second.x,first^second.y,first^second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}&T2{})> operator&(const vec3<T1> &first, const vec3<T2> &second) {return {first.x&second.x,first.y&second.y,first.z&second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}&T2{})> operator&(const vec3<T1> &first, const T2 &second) {return {first.x&second,first.y&second,first.z&second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}&T2{})> operator&(const T1 &first, const vec3<T2> &second) {return {first&second.x,first&second.y,first&second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}|T2{})> operator|(const vec3<T1> &first, const vec3<T2> &second) {return {first.x|second.x,first.y|second.y,first.z|second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}|T2{})> operator|(const vec3<T1> &first, const T2 &second) {return {first.x|second,first.y|second,first.z|second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}|T2{})> operator|(const T1 &first, const vec3<T2> &second) {return {first|second.x,first|second.y,first|second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}<<T2{})> operator<<(const vec3<T1> &first, const vec3<T2> &second) {return {first.x<<second.x,first.y<<second.y,first.z<<second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}<<T2{})> operator<<(const vec3<T1> &first, const T2 &second) {return {first.x<<second,first.y<<second,first.z<<second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}<<T2{})> operator<<(const T1 &first, const vec3<T2> &second) {return {first<<second.x,first<<second.y,first<<second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}>>T2{})> operator>>(const vec3<T1> &first, const vec3<T2> &second) {return {first.x>>second.x,first.y>>second.y,first.z>>second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}>>T2{})> operator>>(const vec3<T1> &first, const T2 &second) {return {first.x>>second,first.y>>second,first.z>>second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}>>T2{})> operator>>(const T1 &first, const vec3<T2> &second) {return {first>>second.x,first>>second.y,first>>second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}<T2{})> operator<(const vec3<T1> &first, const vec3<T2> &second) {return {first.x<second.x,first.y<second.y,first.z<second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}<T2{})> operator<(const vec3<T1> &first, const T2 &second) {return {first.x<second,first.y<second,first.z<second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}<T2{})> operator<(const T1 &first, const vec3<T2> &second) {return {first<second.x,first<second.y,first<second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}>T2{})> operator>(const vec3<T1> &first, const vec3<T2> &second) {return {first.x>second.x,first.y>second.y,first.z>second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}>T2{})> operator>(const vec3<T1> &first, const T2 &second) {return {first.x>second,first.y>second,first.z>second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}>T2{})> operator>(const T1 &first, const vec3<T2> &second) {return {first>second.x,first>second.y,first>second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}<=T2{})> operator<=(const vec3<T1> &first, const vec3<T2> &second) {return {first.x<=second.x,first.y<=second.y,first.z<=second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}<=T2{})> operator<=(const vec3<T1> &first, const T2 &second) {return {first.x<=second,first.y<=second,first.z<=second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}<=T2{})> operator<=(const T1 &first, const vec3<T2> &second) {return {first<=second.x,first<=second.y,first<=second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec3<decltype(T1{}>=T2{})> operator>=(const vec3<T1> &first, const vec3<T2> &second) {return {first.x>=second.x,first.y>=second.y,first.z>=second.z};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}>=T2{})> operator>=(const vec3<T1> &first, const T2 &second) {return {first.x>=second,first.y>=second,first.z>=second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec3<decltype(T1{}>=T2{})> operator>=(const T1 &first, const vec3<T2> &second) {return {first>=second.x,first>=second.y,first>=second.z};}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const vec3<T1> &first, const vec3<T2> &second) {return bool(first) && bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const vec3<T1> &first, const T2 &second) {return bool(first) && second;}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const T1 &first, const vec3<T2> &second) {return first && bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const vec3<T1> &first, const vec3<T2> &second) {return bool(first) || bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const vec3<T1> &first, const T2 &second) {return bool(first) || second;}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const T1 &first, const vec3<T2> &second) {return first || bool(second);}
        template <typename T> [[nodiscard]] constexpr vec3<decltype(~T{})> operator~(const vec3<T> &object) {return {~object.x,~object.y,~object.z};}
        template <typename T> [[nodiscard]] constexpr vec3<decltype(+T{})> operator+(const vec3<T> &object) {return {+object.x,+object.y,+object.z};}
        template <typename T> [[nodiscard]] constexpr vec3<decltype(-T{})> operator-(const vec3<T> &object) {return {-object.x,-object.y,-object.z};}
        template <typename T> [[nodiscard]] constexpr vec3<decltype(!T{})> operator!(const vec3<T> &object) {return !bool(object);}
        template <typename T> constexpr vec3<T> &operator++(vec3<T> &object) {++object.x; ++object.y; ++object.z; return object;}
        template <typename T> constexpr vec3<T> operator++(vec3<T> &object, int) {return {object.x++,object.y++,object.z++};}
        template <typename T> constexpr vec3<T> &operator--(vec3<T> &object) {--object.x; --object.y; --object.z; return object;}
        template <typename T> constexpr vec3<T> operator--(vec3<T> &object, int) {return {object.x--,object.y--,object.z--};}
        template <typename T1, typename T2> constexpr vec3<T1> &operator+=(vec3<T1> &first, const vec3<T2> &second) {first.x += second.x; first.y += second.y; first.z += second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator+=(vec3<T1> &first, const T2 &second) {first.x += second; first.y += second; first.z += second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator-=(vec3<T1> &first, const vec3<T2> &second) {first.x -= second.x; first.y -= second.y; first.z -= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator-=(vec3<T1> &first, const T2 &second) {first.x -= second; first.y -= second; first.z -= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator*=(vec3<T1> &first, const vec3<T2> &second) {first.x *= second.x; first.y *= second.y; first.z *= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator*=(vec3<T1> &first, const T2 &second) {first.x *= second; first.y *= second; first.z *= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator/=(vec3<T1> &first, const vec3<T2> &second) {first.x /= second.x; first.y /= second.y; first.z /= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator/=(vec3<T1> &first, const T2 &second) {first.x /= second; first.y /= second; first.z /= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator%=(vec3<T1> &first, const vec3<T2> &second) {first.x %= second.x; first.y %= second.y; first.z %= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator%=(vec3<T1> &first, const T2 &second) {first.x %= second; first.y %= second; first.z %= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator^=(vec3<T1> &first, const vec3<T2> &second) {first.x ^= second.x; first.y ^= second.y; first.z ^= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator^=(vec3<T1> &first, const T2 &second) {first.x ^= second; first.y ^= second; first.z ^= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator&=(vec3<T1> &first, const vec3<T2> &second) {first.x &= second.x; first.y &= second.y; first.z &= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator&=(vec3<T1> &first, const T2 &second) {first.x &= second; first.y &= second; first.z &= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator|=(vec3<T1> &first, const vec3<T2> &second) {first.x |= second.x; first.y |= second.y; first.z |= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator|=(vec3<T1> &first, const T2 &second) {first.x |= second; first.y |= second; first.z |= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator<<=(vec3<T1> &first, const vec3<T2> &second) {first.x <<= second.x; first.y <<= second.y; first.z <<= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator<<=(vec3<T1> &first, const T2 &second) {first.x <<= second; first.y <<= second; first.z <<= second; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator>>=(vec3<T1> &first, const vec3<T2> &second) {first.x >>= second.x; first.y >>= second.y; first.z >>= second.z; return first;}
        template <typename T1, typename T2> constexpr vec3<T1> &operator>>=(vec3<T1> &first, const T2 &second) {first.x >>= second; first.y >>= second; first.z >>= second; return first;}

        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const vec3<T1> &first, const vec3<T2> &second) {return (first.x == second.x) && (first.y == second.y) && (first.z == second.z);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const vec3<T1> &first, const T2 &second) {return (first.x == second) && (first.y == second) && (first.z == second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const T1 &first, const vec3<T2> &second) {return (first == second.x) && (first == second.y) && (first == second.z);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const vec3<T1> &first, const vec3<T2> &second) {return (first.x != second.x) || (first.y != second.y) || (first.z != second.z);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const vec3<T1> &first, const T2 &second) {return (first.x != second) || (first.y != second) || (first.z != second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const T1 &first, const vec3<T2> &second) {return (first != second.x) || (first != second.y) || (first != second.z);}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}+T2{})> operator+(const vec4<T1> &first, const vec4<T2> &second) {return {first.x+second.x,first.y+second.y,first.z+second.z,first.w+second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}+T2{})> operator+(const vec4<T1> &first, const T2 &second) {return {first.x+second,first.y+second,first.z+second,first.w+second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}+T2{})> operator+(const T1 &first, const vec4<T2> &second) {return {first+second.x,first+second.y,first+second.z,first+second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}-T2{})> operator-(const vec4<T1> &first, const vec4<T2> &second) {return {first.x-second.x,first.y-second.y,first.z-second.z,first.w-second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}-T2{})> operator-(const vec4<T1> &first, const T2 &second) {return {first.x-second,first.y-second,first.z-second,first.w-second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}-T2{})> operator-(const T1 &first, const vec4<T2> &second) {return {first-second.x,first-second.y,first-second.z,first-second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}*T2{})> operator*(const vec4<T1> &first, const vec4<T2> &second) {return {first.x*second.x,first.y*second.y,first.z*second.z,first.w*second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}*T2{})> operator*(const vec4<T1> &first, const T2 &second) {return {first.x*second,first.y*second,first.z*second,first.w*second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}*T2{})> operator*(const T1 &first, const vec4<T2> &second) {return {first*second.x,first*second.y,first*second.z,first*second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}/T2{})> operator/(const vec4<T1> &first, const vec4<T2> &second) {return {first.x/second.x,first.y/second.y,first.z/second.z,first.w/second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}/T2{})> operator/(const vec4<T1> &first, const T2 &second) {return {first.x/second,first.y/second,first.z/second,first.w/second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}/T2{})> operator/(const T1 &first, const vec4<T2> &second) {return {first/second.x,first/second.y,first/second.z,first/second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}%T2{})> operator%(const vec4<T1> &first, const vec4<T2> &second) {return {first.x%second.x,first.y%second.y,first.z%second.z,first.w%second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}%T2{})> operator%(const vec4<T1> &first, const T2 &second) {return {first.x%second,first.y%second,first.z%second,first.w%second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}%T2{})> operator%(const T1 &first, const vec4<T2> &second) {return {first%second.x,first%second.y,first%second.z,first%second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}^T2{})> operator^(const vec4<T1> &first, const vec4<T2> &second) {return {first.x^second.x,first.y^second.y,first.z^second.z,first.w^second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}^T2{})> operator^(const vec4<T1> &first, const T2 &second) {return {first.x^second,first.y^second,first.z^second,first.w^second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}^T2{})> operator^(const T1 &first, const vec4<T2> &second) {return {first^second.x,first^second.y,first^second.z,first^second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}&T2{})> operator&(const vec4<T1> &first, const vec4<T2> &second) {return {first.x&second.x,first.y&second.y,first.z&second.z,first.w&second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}&T2{})> operator&(const vec4<T1> &first, const T2 &second) {return {first.x&second,first.y&second,first.z&second,first.w&second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}&T2{})> operator&(const T1 &first, const vec4<T2> &second) {return {first&second.x,first&second.y,first&second.z,first&second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}|T2{})> operator|(const vec4<T1> &first, const vec4<T2> &second) {return {first.x|second.x,first.y|second.y,first.z|second.z,first.w|second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}|T2{})> operator|(const vec4<T1> &first, const T2 &second) {return {first.x|second,first.y|second,first.z|second,first.w|second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}|T2{})> operator|(const T1 &first, const vec4<T2> &second) {return {first|second.x,first|second.y,first|second.z,first|second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}<<T2{})> operator<<(const vec4<T1> &first, const vec4<T2> &second) {return {first.x<<second.x,first.y<<second.y,first.z<<second.z,first.w<<second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}<<T2{})> operator<<(const vec4<T1> &first, const T2 &second) {return {first.x<<second,first.y<<second,first.z<<second,first.w<<second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}<<T2{})> operator<<(const T1 &first, const vec4<T2> &second) {return {first<<second.x,first<<second.y,first<<second.z,first<<second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}>>T2{})> operator>>(const vec4<T1> &first, const vec4<T2> &second) {return {first.x>>second.x,first.y>>second.y,first.z>>second.z,first.w>>second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}>>T2{})> operator>>(const vec4<T1> &first, const T2 &second) {return {first.x>>second,first.y>>second,first.z>>second,first.w>>second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}>>T2{})> operator>>(const T1 &first, const vec4<T2> &second) {return {first>>second.x,first>>second.y,first>>second.z,first>>second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}<T2{})> operator<(const vec4<T1> &first, const vec4<T2> &second) {return {first.x<second.x,first.y<second.y,first.z<second.z,first.w<second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}<T2{})> operator<(const vec4<T1> &first, const T2 &second) {return {first.x<second,first.y<second,first.z<second,first.w<second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}<T2{})> operator<(const T1 &first, const vec4<T2> &second) {return {first<second.x,first<second.y,first<second.z,first<second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}>T2{})> operator>(const vec4<T1> &first, const vec4<T2> &second) {return {first.x>second.x,first.y>second.y,first.z>second.z,first.w>second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}>T2{})> operator>(const vec4<T1> &first, const T2 &second) {return {first.x>second,first.y>second,first.z>second,first.w>second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}>T2{})> operator>(const T1 &first, const vec4<T2> &second) {return {first>second.x,first>second.y,first>second.z,first>second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}<=T2{})> operator<=(const vec4<T1> &first, const vec4<T2> &second) {return {first.x<=second.x,first.y<=second.y,first.z<=second.z,first.w<=second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}<=T2{})> operator<=(const vec4<T1> &first, const T2 &second) {return {first.x<=second,first.y<=second,first.z<=second,first.w<=second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}<=T2{})> operator<=(const T1 &first, const vec4<T2> &second) {return {first<=second.x,first<=second.y,first<=second.z,first<=second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr vec4<decltype(T1{}>=T2{})> operator>=(const vec4<T1> &first, const vec4<T2> &second) {return {first.x>=second.x,first.y>=second.y,first.z>=second.z,first.w>=second.w};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}>=T2{})> operator>=(const vec4<T1> &first, const T2 &second) {return {first.x>=second,first.y>=second,first.z>=second,first.w>=second};}
        template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec4<decltype(T1{}>=T2{})> operator>=(const T1 &first, const vec4<T2> &second) {return {first>=second.x,first>=second.y,first>=second.z,first>=second.w};}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const vec4<T1> &first, const vec4<T2> &second) {return bool(first) && bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const vec4<T1> &first, const T2 &second) {return bool(first) && second;}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator&&(const T1 &first, const vec4<T2> &second) {return first && bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const vec4<T1> &first, const vec4<T2> &second) {return bool(first) || bool(second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const vec4<T1> &first, const T2 &second) {return bool(first) || second;}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator||(const T1 &first, const vec4<T2> &second) {return first || bool(second);}
        template <typename T> [[nodiscard]] constexpr vec4<decltype(~T{})> operator~(const vec4<T> &object) {return {~object.x,~object.y,~object.z,~object.w};}
        template <typename T> [[nodiscard]] constexpr vec4<decltype(+T{})> operator+(const vec4<T> &object) {return {+object.x,+object.y,+object.z,+object.w};}
        template <typename T> [[nodiscard]] constexpr vec4<decltype(-T{})> operator-(const vec4<T> &object) {return {-object.x,-object.y,-object.z,-object.w};}
        template <typename T> [[nodiscard]] constexpr vec4<decltype(!T{})> operator!(const vec4<T> &object) {return !bool(object);}
        template <typename T> constexpr vec4<T> &operator++(vec4<T> &object) {++object.x; ++object.y; ++object.z; ++object.w; return object;}
        template <typename T> constexpr vec4<T> operator++(vec4<T> &object, int) {return {object.x++,object.y++,object.z++,object.w++};}
        template <typename T> constexpr vec4<T> &operator--(vec4<T> &object) {--object.x; --object.y; --object.z; --object.w; return object;}
        template <typename T> constexpr vec4<T> operator--(vec4<T> &object, int) {return {object.x--,object.y--,object.z--,object.w--};}
        template <typename T1, typename T2> constexpr vec4<T1> &operator+=(vec4<T1> &first, const vec4<T2> &second) {first.x += second.x; first.y += second.y; first.z += second.z; first.w += second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator+=(vec4<T1> &first, const T2 &second) {first.x += second; first.y += second; first.z += second; first.w += second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator-=(vec4<T1> &first, const vec4<T2> &second) {first.x -= second.x; first.y -= second.y; first.z -= second.z; first.w -= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator-=(vec4<T1> &first, const T2 &second) {first.x -= second; first.y -= second; first.z -= second; first.w -= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator*=(vec4<T1> &first, const vec4<T2> &second) {first.x *= second.x; first.y *= second.y; first.z *= second.z; first.w *= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator*=(vec4<T1> &first, const T2 &second) {first.x *= second; first.y *= second; first.z *= second; first.w *= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator/=(vec4<T1> &first, const vec4<T2> &second) {first.x /= second.x; first.y /= second.y; first.z /= second.z; first.w /= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator/=(vec4<T1> &first, const T2 &second) {first.x /= second; first.y /= second; first.z /= second; first.w /= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator%=(vec4<T1> &first, const vec4<T2> &second) {first.x %= second.x; first.y %= second.y; first.z %= second.z; first.w %= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator%=(vec4<T1> &first, const T2 &second) {first.x %= second; first.y %= second; first.z %= second; first.w %= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator^=(vec4<T1> &first, const vec4<T2> &second) {first.x ^= second.x; first.y ^= second.y; first.z ^= second.z; first.w ^= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator^=(vec4<T1> &first, const T2 &second) {first.x ^= second; first.y ^= second; first.z ^= second; first.w ^= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator&=(vec4<T1> &first, const vec4<T2> &second) {first.x &= second.x; first.y &= second.y; first.z &= second.z; first.w &= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator&=(vec4<T1> &first, const T2 &second) {first.x &= second; first.y &= second; first.z &= second; first.w &= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator|=(vec4<T1> &first, const vec4<T2> &second) {first.x |= second.x; first.y |= second.y; first.z |= second.z; first.w |= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator|=(vec4<T1> &first, const T2 &second) {first.x |= second; first.y |= second; first.z |= second; first.w |= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator<<=(vec4<T1> &first, const vec4<T2> &second) {first.x <<= second.x; first.y <<= second.y; first.z <<= second.z; first.w <<= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator<<=(vec4<T1> &first, const T2 &second) {first.x <<= second; first.y <<= second; first.z <<= second; first.w <<= second; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator>>=(vec4<T1> &first, const vec4<T2> &second) {first.x >>= second.x; first.y >>= second.y; first.z >>= second.z; first.w >>= second.w; return first;}
        template <typename T1, typename T2> constexpr vec4<T1> &operator>>=(vec4<T1> &first, const T2 &second) {first.x >>= second; first.y >>= second; first.z >>= second; first.w >>= second; return first;}

        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const vec4<T1> &first, const vec4<T2> &second) {return (first.x == second.x) && (first.y == second.y) && (first.z == second.z) && (first.w == second.w);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const vec4<T1> &first, const T2 &second) {return (first.x == second) && (first.y == second) && (first.z == second) && (first.w == second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator==(const T1 &first, const vec4<T2> &second) {return (first == second.x) && (first == second.y) && (first == second.z) && (first == second.w);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const vec4<T1> &first, const vec4<T2> &second) {return (first.x != second.x) || (first.y != second.y) || (first.z != second.z) || (first.w != second.w);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const vec4<T1> &first, const T2 &second) {return (first.x != second) || (first.y != second) || (first.z != second) || (first.w != second);}
        template <typename T1, typename T2> [[nodiscard]] constexpr bool operator!=(const T1 &first, const vec4<T2> &second) {return (first != second.x) || (first != second.y) || (first != second.z) || (first != second.w);}

        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const vec2<T> &vector) {stream << '[' << vector.x << ',' << vector.y << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const vec3<T> &vector) {stream << '[' << vector.x << ',' << vector.y << ',' << vector.z << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const vec4<T> &vector) {stream << '[' << vector.x << ',' << vector.y << ',' << vector.z << ',' << vector.w << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat2x2<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ';' << matrix.x.y << ',' << matrix.y.y << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat3x2<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ',' << matrix.z.x << ';' << matrix.x.y << ',' << matrix.y.y << ',' << matrix.z.y << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat4x2<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ',' << matrix.z.x << ',' << matrix.w.x << ';' << matrix.x.y << ',' << matrix.y.y << ',' << matrix.z.y << ',' << matrix.w.y << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat2x3<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ';' << matrix.x.y << ',' << matrix.y.y << ';' << matrix.x.z << ',' << matrix.y.z << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat3x3<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ',' << matrix.z.x << ';' << matrix.x.y << ',' << matrix.y.y << ',' << matrix.z.y << ';' << matrix.x.z << ',' << matrix.y.z << ',' << matrix.z.z << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat4x3<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ',' << matrix.z.x << ',' << matrix.w.x << ';' << matrix.x.y << ',' << matrix.y.y << ',' << matrix.z.y << ',' << matrix.w.y << ';' << matrix.x.z << ',' << matrix.y.z << ',' << matrix.z.z << ',' << matrix.w.z << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat2x4<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ';' << matrix.x.y << ',' << matrix.y.y << ';' << matrix.x.z << ',' << matrix.y.z << ';' << matrix.x.w << ',' << matrix.y.w << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat3x4<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ',' << matrix.z.x << ';' << matrix.x.y << ',' << matrix.y.y << ',' << matrix.z.y << ';' << matrix.x.z << ',' << matrix.y.z << ',' << matrix.z.z << ';' << matrix.x.w << ',' << matrix.y.w << ',' << matrix.z.w << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat4x4<T> &matrix) {stream << '[' << matrix.x.x << ',' << matrix.y.x << ',' << matrix.z.x << ',' << matrix.w.x << ';' << matrix.x.y << ',' << matrix.y.y << ',' << matrix.z.y << ',' << matrix.w.y << ';' << matrix.x.z << ',' << matrix.y.z << ',' << matrix.z.z << ',' << matrix.w.z << ';' << matrix.x.w << ',' << matrix.y.w << ',' << matrix.z.w << ',' << matrix.w.w << ']'; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, vec2<T> &vector) {stream >> vector.x >> vector.y; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, vec3<T> &vector) {stream >> vector.x >> vector.y >> vector.z; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, vec4<T> &vector) {stream >> vector.x >> vector.y >> vector.z >> vector.w; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat2x2<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.x.y >> matrix.y.y; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat3x2<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.z.x >> matrix.x.y >> matrix.y.y >> matrix.z.y; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat4x2<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.z.x >> matrix.w.x >> matrix.x.y >> matrix.y.y >> matrix.z.y >> matrix.w.y; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat2x3<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.x.y >> matrix.y.y >> matrix.x.z >> matrix.y.z; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat3x3<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.z.x >> matrix.x.y >> matrix.y.y >> matrix.z.y >> matrix.x.z >> matrix.y.z >> matrix.z.z; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat4x3<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.z.x >> matrix.w.x >> matrix.x.y >> matrix.y.y >> matrix.z.y >> matrix.w.y >> matrix.x.z >> matrix.y.z >> matrix.z.z >> matrix.w.z; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat2x4<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.x.y >> matrix.y.y >> matrix.x.z >> matrix.y.z >> matrix.x.w >> matrix.y.w; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat3x4<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.z.x >> matrix.x.y >> matrix.y.y >> matrix.z.y >> matrix.x.z >> matrix.y.z >> matrix.z.z >> matrix.x.w >> matrix.y.w >> matrix.z.w; return stream;}
        template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat4x4<T> &matrix) {stream >> matrix.x.x >> matrix.y.x >> matrix.z.x >> matrix.w.x >> matrix.x.y >> matrix.y.y >> matrix.z.y >> matrix.w.y >> matrix.x.z >> matrix.y.z >> matrix.z.z >> matrix.w.z >> matrix.x.w >> matrix.y.w >> matrix.z.w >> matrix.w.w; return stream;}
    }

    inline namespace Quaternion
    {
        template <typename T> struct quat
        {
            static_assert(std::is_floating_point_v<T>, "Type must be floating-point.");

            vec4<T> vec;

            constexpr quat() : vec{0,0,0,1} {}
            constexpr quat(const vec4<T> &o) : vec(o) {}

            [[nodiscard]] static quat around_norm_axis(const vec3<T> &v, T angle)
            {
                return quat({v.x * std::sin(angle / 2.f), v.y * std::sin(angle / 2.f), v.z * std::sin(angle / 2.f), std::cos(angle / 2.f)});
            }
            [[nodiscard]] static quat around_axis(const vec3<T> &v, T angle)
            {
                return around_norm_axis(v.norm(), angle);
            }

            void normalize()
            {
                vec = vec.norm();
            }

            template <typename TT> [[nodiscard]] constexpr quat<larger_type_t<T,TT>> mul(const quat<TT> &o) const
            {
                return quat({vec.w*o.vec.x + vec.x*o.vec.w + vec.y*o.vec.z - vec.z*o.vec.y,
                             vec.w*o.vec.y - vec.x*o.vec.z + vec.y*o.vec.w + vec.z*o.vec.x,
                             vec.w*o.vec.z + vec.x*o.vec.y - vec.y*o.vec.x + vec.z*o.vec.w,
                             vec.w*o.vec.w - vec.x*o.vec.x - vec.y*o.vec.y - vec.z*o.vec.z});
            }
            [[nodiscard]] quat combine(const quat &o, T fac) const
            {
                return quat(vec.interpolate(o.vec, fac));
            }

            [[nodiscard]] vec3<T> get_axis() const
            {
                return vec.to_vec3().norm();
            }
            [[nodiscard]] constexpr vec3<T> get_not_norm_axis() const
            {
                return vec.to_vec3();
            }
            [[nodiscard]] T get_angle() const
            {
                return std::atan2(vec.to_vec3().len(), vec.w) * 2;
            }

            [[nodiscard]] quat set_angle(T n) const
            {
                return from_axis_and_angle(get_not_norm_axis(), n);
            }

            template <typename TT> [[nodiscard]] constexpr vec3<larger_type_t<T,TT>> mul(const vec3<TT> &in) const // Complexity: 24x`*` 17x`+-`
            {
                float newx = vec.w*in.x + vec.y*in.z - vec.z*in.y;
                float newy = vec.w*in.y - vec.x*in.z + vec.z*in.x;
                float newz = vec.w*in.z + vec.x*in.y - vec.y*in.x;
                float neww = vec.x*in.x + vec.y*in.y + vec.z*in.z;

                return {newx*vec.w + neww*vec.x - newy*vec.z + newz*vec.y,
                        neww*vec.y + newx*vec.z + newy*vec.w - newz*vec.x,
                        neww*vec.z - newx*vec.y + newy*vec.x + newz*vec.w};
            }

            template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const vec4<TT> &in) const // Leaves in.w unchanged. Complexity: 24x`*` 17x`+-`
            {
                    return mul(in.to_vec3).to_vec4(in.w);
            }

            [[nodiscard]] constexpr mat3<T> make_mat3() const // The quaternion must be normalized! Complexity: 18x`*` 12x`+-` (+ multiplication 9x`*` 6x`+-`)
            {
                return {1 - 2*vec.y*vec.y - 2*vec.z*vec.z, 2*vec.x*vec.y - 2*vec.z*vec.w, 2*vec.x*vec.z + 2*vec.y*vec.w,
                        2*vec.x*vec.y + 2*vec.z*vec.w, 1 - 2*vec.x*vec.x - 2*vec.z*vec.z, 2*vec.y*vec.z - 2*vec.x*vec.w,
                        2*vec.x*vec.z - 2*vec.y*vec.w, 2*vec.y*vec.z + 2*vec.x*vec.w, 1 - 2*vec.x*vec.x - 2*vec.y*vec.y};
            }

            [[nodiscard]] constexpr mat4<T> make_mat4() const // The quaternion must be normalized! Complexity: 18x`*` 12x`+-` (+ multiplication 9x`*` 6x`+-`)
            {
                return make_mat3().to_mat4();
            }
        };

        using fquat = quat<float>;
        using dquat = quat<double>;
        using ldquat = quat<long double>;
    }

    inline namespace Operators
    {
        template <typename T> struct dot_operator_impl_expression_t : private custom_operator_base_t
        {
            T first_arg;
            template <typename TT> constexpr dot_operator_impl_expression_t(TT &&obj) : first_arg((TT &&) obj) {}
            template <typename TT> [[nodiscard]] constexpr auto operator/(TT &&obj) const {return first_arg.dot((TT &&) obj);}
        };
        template <typename T> struct cross_operator_impl_expression_t : private custom_operator_base_t
        {
            T first_arg;
            template <typename TT> constexpr cross_operator_impl_expression_t(TT &&obj) : first_arg((TT &&) obj) {}
            template <typename TT> [[nodiscard]] constexpr auto operator/(TT &&obj) const {return first_arg.cross((TT &&) obj);}
        };
        template <typename T> struct mul_operator_impl_expression_t : private custom_operator_base_t
        {
            T first_arg;
            template <typename TT> constexpr mul_operator_impl_expression_t(TT &&obj) : first_arg((TT &&) obj) {}
            template <typename TT> [[nodiscard]] constexpr auto operator/(TT &&obj) const {return first_arg.mul((TT &&) obj);}
        };

        struct dot_operator_impl_t : private custom_operator_base_t
        {
            constexpr dot_operator_impl_t(){}
        };
        constexpr dot_operator_impl_t dot;
        template <typename T> [[nodiscard]] constexpr auto operator/(T &&obj, decltype(dot)) {return dot_operator_impl_expression_t<T>((T &&) obj);}
        struct cross_operator_impl_t : private custom_operator_base_t
        {
            constexpr cross_operator_impl_t(){}
        };
        constexpr cross_operator_impl_t cross;
        template <typename T> [[nodiscard]] constexpr auto operator/(T &&obj, decltype(cross)) {return cross_operator_impl_expression_t<T>((T &&) obj);}
        struct mul_operator_impl_t : private custom_operator_base_t
        {
            constexpr mul_operator_impl_t(){}
        };
        constexpr mul_operator_impl_t mul;
        template <typename T> [[nodiscard]] constexpr auto operator/(T &&obj, decltype(mul)) {return mul_operator_impl_expression_t<T>((T &&) obj);}
    }

    inline namespace Misc
    {
        template <typename T> [[nodiscard]] constexpr T pi() {return T(3.14159265358979323846l);}
        constexpr float       f_pi  = pi<float>();
        constexpr double      d_pi  = pi<double>();
        constexpr long double ld_pi = pi<long double>();

        template <typename T> [[nodiscard]] floating_point_t<T> to_rad(T in)
        {
            using T_fp = floating_point_t<T>;
            return in * pi<T_fp>() / (T_fp)180;
        }
        template <typename T> [[nodiscard]] floating_point_t<T> to_deg(T in)
        {
            using T_fp = floating_point_t<T>;
            return in * (T_fp)180 / pi<T_fp>();
        }

        template <typename T, typename TT> [[nodiscard]] constexpr T ipow(T a, TT b)
        {
            static_assert(std::is_integral_v<TT>, "Non integral power makes no sense for this function.");
            T ret = 1;
            while (b-- > 0)
                ret *= a;
            return ret;
        }

        template <typename T, typename Min, typename Max> [[nodiscard]] constexpr T clamp(T val, Min min, Max max)
        {
            static_assert(std::is_arithmetic_v<T> &&
                          std::is_arithmetic_v<Min> &&
                          std::is_arithmetic_v<Max>, "Non arithmetic arguments make no sense for this function.");
            if (val > max) return max;
            if (val < min) return min;
            return val;
        }
        template <typename T, unsigned int D, typename Min, typename Max> [[nodiscard]] constexpr vec<D,T> clamp(const vec<D,T> &val, Min min, Max max)
        {
            using v = vec<D,T>;
            static_assert(!type_category<Min>::vec_or_mat || std::is_same_v<change_base_type_t<v,Min>, Min>, "Second argument must be a scalar or must have the same dimensions as the first one.");
            static_assert(!type_category<Max>::vec_or_mat || std::is_same_v<change_base_type_t<v,Max>, Max>, "Third argument must be a scalar or must have the same dimensions as the first one.");
            return val.apply(clamp<base_type_t<v>, base_type_t<Min>, base_type_t<Max>>, change_base_type_t<v,Min>(min), change_base_type_t<v,Max>(max));
        }
        template <typename T> [[nodiscard]] constexpr T clamp(T val)
        {
            return clamp(val, 0, 1);
        }

        template <typename T, typename Min, typename Max> constexpr void clamp_assign(T &val, Min min, Max max)
        {
            val = clamp(val, min, max);
        }
        template <typename T, typename Min, typename Max> constexpr void clamp_assign(T &val)
        {
            val = clamp(val);
        }

        template <typename T> [[nodiscard]] constexpr change_base_type_t<T,int> sign(T val)
        {
            return (val > 0) - (val < 0);
        }

        template <typename I = int, typename F> [[nodiscard]] I iround(F x)
        {
            static_assert(std::is_floating_point_v<F>, "Argument type must be floating-point.");
            static_assert(std::is_integral_v<I> && std::is_signed_v<I>, "Template argument must be integral and signed.");
            if constexpr (sizeof (I) <= sizeof (long))
                return std::lround(x);
            else
                return std::llround(x);
        }
        template <typename I = int, typename T, unsigned int D> [[nodiscard]] change_base_type_t<vec<D,T>,I> iround(const vec<D,T> &val)
        {
            return val.apply(iround<I, base_type_t<vec<D,T>>>);
        }

        template <typename T> [[nodiscard]] constexpr T smoothstep(T x)
        {
            static_assert(std::is_floating_point_v<base_type_t<T>>, "Argument type must be floating-point.");
            return 3 * x*x - 2 * x*x*x;
        }

        template <typename T> [[nodiscard]] T constexpr abs(T x)
        {
            return (x >= 0 ? x : -x);
        }
        template <typename T, unsigned int D> vec<D,T> abs(const vec<D,T> &val)
        {
            return val.apply(abs<base_type_t<vec<D,T>>>);
        }

        template <typename T> [[nodiscard]] T floor(T x)
        {
            static_assert(std::is_floating_point_v<T>, "Argument type must be floating-point.");
            return std::floor(x);
        }
        template <typename T, unsigned int D> [[nodiscard]] vec<D,T> floor(const vec<D,T> &val)
        {
            return val.apply(floor<base_type_t<vec<D,T>>>);
        }

        template <typename T> [[nodiscard]] T ceil(T x)
        {
            static_assert(std::is_floating_point_v<T>, "Argument type must be floating-point.");
            return std::ceil(x);
        }
        template <typename T, unsigned int D> [[nodiscard]] vec<D,T> ceil(const vec<D,T> &val)
        {
            return val.apply(ceil<base_type_t<vec<D,T>>>);
        }

        template <typename T> [[nodiscard]] T trunc(T x)
        {
            static_assert(std::is_floating_point_v<T>, "Argument type must be floating-point.");
            return std::trunc(x);
        }
        template <typename T, unsigned int D> [[nodiscard]] vec<D,T> trunc(const vec<D,T> &val)
        {
            return val.apply(trunc<base_type_t<vec<D,T>>>);
        }

        template <typename T> [[nodiscard]] T frac(T x)
        {
            static_assert(std::is_floating_point_v<T>, "Argument type must be floating-point.");
            return std::modf(x, 0);
        }
        template <typename T, unsigned int D> [[nodiscard]] vec<D,T> frac(const vec<D,T> &val)
        {
            return val.apply(frac<base_type_t<vec<D,T>>>);
        }

        template <typename T, typename TT> [[nodiscard]] constexpr T div_ex(T a, TT b)
        {
            static_assert(std::is_integral_v<T> &&
                          std::is_integral_v<TT>, "Parameter types must be integral scalars, vectors, or matrices.");
            if (a >= 0)
                return a / b;
            else
                return (a + 1) / b - sign(b);
        }
        template <typename T, typename TT, unsigned int D> [[nodiscard]] vec<D,T> div_ex(const vec<D,T> &a, TT b)
        {
            return a.apply(div_ex<base_type_t<vec<D,T>>, base_type_t<TT>>, change_base_type_t<vec<D,T>,TT>(b));
        }

        template <typename T, typename TT> [[nodiscard]] constexpr T mod_ex(T a, TT b)
        {
            static_assert(std::is_integral_v<T> &&
                          std::is_integral_v<TT>, "Parameter types must be integral scalars, vectors, or matrices.");
            if (a >= 0)
                return a % b;
            else
                return abs(b) - 1 + (a + 1) % b;
        }
        template <typename T, typename TT, unsigned int D> [[nodiscard]] vec<D,T> mod_ex(const vec<D,T> &a, TT b)
        {
            return a.apply(mod_ex<base_type_t<vec<D,T>>, base_type_t<TT>>, change_base_type_t<vec<D,T>,TT>(b));
        }

        template <typename T1, typename T2> [[nodiscard]] constexpr larger_type_t<T1,T2> min(T1 a, T2 b) {return (a > b ? b : a);}
        template <typename T1, typename T2, unsigned int D1> [[nodiscard]] constexpr larger_type_t<vec<D1,T1>,T2> min(const vec<D1,T1> &a, T2 b) {return a.apply(min<base_type_t<vec<D1,T1>>, T2>, change_base_type_t<vec<D1,T1>,T2>(b));}
        template <typename T1, typename T2, unsigned int D2> [[nodiscard]] constexpr larger_type_t<T1,vec<D2,T2>> min(T1 a, const vec<D2,T2> &b) {return b.apply(min<T1, base_type_t<vec<D2,T2>>>, change_base_type_t<vec<D2,T2>,T1>(a));}
        template <typename T1, typename T2, unsigned int D1, unsigned int D2> [[nodiscard]] constexpr larger_type_t<vec<D1,T1>,vec<D2,T2>> min(const vec<D1,T1> &a, const vec<D2,T2> &b) {return a.apply(min<base_type_t<vec<D1,T1>>, base_type_t<vec<D2,T2>>>, b);}

        template <typename T1, typename T2> [[nodiscard]] constexpr larger_type_t<T1,T2> max(T1 a, T2 b) {return (b > a ? b : a);}
        template <typename T1, typename T2, unsigned int D1> [[nodiscard]] constexpr larger_type_t<vec<D1,T1>,T2> max(const vec<D1,T1> &a, T2 b) {return a.apply(max<base_type_t<vec<D1,T1>>, T2>, change_base_type_t<vec<D1,T1>,T2>(b));}
        template <typename T1, typename T2, unsigned int D2> [[nodiscard]] constexpr larger_type_t<T1,vec<D2,T2>> max(T1 a, const vec<D2,T2> &b) {return b.apply(max<T1, base_type_t<vec<D2,T2>>>, change_base_type_t<vec<D2,T2>,T1>(a));}
        template <typename T1, typename T2, unsigned int D1, unsigned int D2> [[nodiscard]] constexpr larger_type_t<vec<D1,T1>,vec<D2,T2>> max(const vec<D1,T1> &a, const vec<D2,T2> &b) {return a.apply(max<base_type_t<vec<D1,T1>>, base_type_t<vec<D2,T2>>>, b);}
    }

    namespace Export
    {
        using namespace Vector;
        using namespace Quaternion;
        using namespace Operators;
        using namespace Misc;

        using Strings::num_to_string;
        using Strings::num_from_string_mid;
    }
}

namespace std
{
    template <typename T> struct less<Math::vec2<T>>
    {
        using result_type = bool;
        using first_argument_type = Math::vec2<T>;
        using second_argument_type = Math::vec2<T>;
        constexpr bool operator()(const Math::vec2<T> &a, const Math::vec2<T> &b) const
        {
            if (a.x < b.x) return 1;
            if (a.x > b.x) return 0;
            if (a.y < b.y) return 1;
            if (a.y > b.y) return 0;
            return 0;
        }
    };
    template <typename T> struct less<Math::vec3<T>>
    {
        using result_type = bool;
        using first_argument_type = Math::vec3<T>;
        using second_argument_type = Math::vec3<T>;
        constexpr bool operator()(const Math::vec3<T> &a, const Math::vec3<T> &b) const
        {
            if (a.x < b.x) return 1;
            if (a.x > b.x) return 0;
            if (a.y < b.y) return 1;
            if (a.y > b.y) return 0;
            if (a.z < b.z) return 1;
            if (a.z > b.z) return 0;
            return 0;
        }
    };
    template <typename T> struct less<Math::vec4<T>>
    {
        using result_type = bool;
        using first_argument_type = Math::vec4<T>;
        using second_argument_type = Math::vec4<T>;
        constexpr bool operator()(const Math::vec4<T> &a, const Math::vec4<T> &b) const
        {
            if (a.x < b.x) return 1;
            if (a.x > b.x) return 0;
            if (a.y < b.y) return 1;
            if (a.y > b.y) return 0;
            if (a.z < b.z) return 1;
            if (a.z > b.z) return 0;
            if (a.w < b.w) return 1;
            if (a.w > b.w) return 0;
            return 0;
        }
    };

    template <typename T> struct hash<Math::vec2<T>>
    {
        using result_type = std::size_t;
        using argument_type = Math::vec2<T>;
        std::size_t operator()(const Math::vec2<T> &v) const
        {
            std::size_t ret = std::hash<decltype(v.x)>{}(v.x);
            Math::hash_combine(ret, std::hash<decltype(v.x)>{}(v.y));
            return ret;
        }
    };
    template <typename T> struct hash<Math::vec3<T>>
    {
        using result_type = std::size_t;
        using argument_type = Math::vec3<T>;
        std::size_t operator()(const Math::vec3<T> &v) const
        {
            std::size_t ret = std::hash<decltype(v.x)>{}(v.x);
            Math::hash_combine(ret, std::hash<decltype(v.x)>{}(v.y));
            Math::hash_combine(ret, std::hash<decltype(v.x)>{}(v.z));
            return ret;
        }
    };
    template <typename T> struct hash<Math::vec4<T>>
    {
        using result_type = std::size_t;
        using argument_type = Math::vec4<T>;
        std::size_t operator()(const Math::vec4<T> &v) const
        {
            std::size_t ret = std::hash<decltype(v.x)>{}(v.x);
            Math::hash_combine(ret, std::hash<decltype(v.x)>{}(v.y));
            Math::hash_combine(ret, std::hash<decltype(v.x)>{}(v.z));
            Math::hash_combine(ret, std::hash<decltype(v.x)>{}(v.w));
            return ret;
        }
    };
}

using namespace Math::Export;

#endif
