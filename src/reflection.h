#ifndef REFLECTION_H_INCLUDED
#define REFLECTION_H_INCLUDED

#include <algorithm>
#include <bitset>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mat.h"
#include "platform.h"
#include "preprocessor.h"

#ifdef PLATFORM_BIG_ENDIAN
#  define REFLECTION_NONNATIVE_ENDIANNESS
#endif

namespace Reflection
{
    template <std::size_t I> using index_const = std::integral_constant<std::size_t, I>;


    namespace Strings
    {
        inline constexpr bool is_digit(char ch)
        {
            return ch >= '0' && ch <= '9';
        }
        inline constexpr bool is_hex_digit(char ch)
        {
            return is_digit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
        }
        inline constexpr bool is_alphanum(char ch)
        {
            return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || is_digit(ch) || ch == '_';
        }
        inline constexpr int hex_digit_to_number(char ch)
        {
                 if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            else                             return ch - '0';
        }

        // Returns a number of characters required to represent an escape sequence for a character.
        // 1 is returned if no escape is needed.
        // 2 is returned if \@ escape is needed.
        // 4 is returned if \x@@ escape is needed.
        inline int char_escape_len(char c)
        {
            switch (c)
            {
              case '\0':
              case '\"':
              case '\a':
              case '\b':
              case '\f':
              case '\n':
              case '\r':
              case '\t':
              case '\v':
                return 2;
              default:
                if ((c >= '\0' && c < ' ') || c == 127)
                    return 4;
                else
                    return 1;
            }
        }

        inline std::string char_escape_seq(char c)
        {
            char tmp[5]{};
            switch (char_escape_len(c))
            {
              case 1:
                tmp[0] = c;
                tmp[1] = '\0';
                break;
              case 2:
                tmp[0] = '\\';
                tmp[2] = '\0';
                switch (c)
                {
                    case '\0': tmp[1] = '0'; break;
                    case '\"': tmp[1] = '"'; break;
                    case '\a': tmp[1] = 'a'; break;
                    case '\b': tmp[1] = 'b'; break;
                    case '\f': tmp[1] = 'f'; break;
                    case '\n': tmp[1] = 'n'; break;
                    case '\r': tmp[1] = 'r'; break;
                    case '\t': tmp[1] = 't'; break;
                    case '\v': tmp[1] = 'v'; break;
                    default: tmp[0] = c; tmp[1] = '\0'; break;
                }
                break;
              case 4:
                tmp[0] = '\\';
                tmp[1] = 'x';
                tmp[2] = (unsigned char)c / 16 + '0';
                tmp[3] = (unsigned char)c % 16 + '0';
                tmp[4] = '\0';
                break;
            }
            return tmp;
        }

        inline char char_from_escape(const char *ptr, const char **new_ptr = 0)
        {
            if (*ptr != '\\')
            {
                if (new_ptr)
                    *new_ptr = ptr + 1;
                return *ptr;
            }

            switch (ptr[1])
            {
              case '0': if (new_ptr) *new_ptr = ptr + 2; return '\0';
              case '"': if (new_ptr) *new_ptr = ptr + 2; return '\"';
              case 'a': if (new_ptr) *new_ptr = ptr + 2; return '\a';
              case 'b': if (new_ptr) *new_ptr = ptr + 2; return '\b';
              case 'f': if (new_ptr) *new_ptr = ptr + 2; return '\f';
              case 'n': if (new_ptr) *new_ptr = ptr + 2; return '\n';
              case 'r': if (new_ptr) *new_ptr = ptr + 2; return '\r';
              case 't': if (new_ptr) *new_ptr = ptr + 2; return '\t';
              case 'v': if (new_ptr) *new_ptr = ptr + 2; return '\v';
              case 'x':
                if (is_hex_digit(ptr[2]) && is_hex_digit(ptr[3]))
                {
                    if (new_ptr)
                        *new_ptr = ptr + 4;

                    return (hex_digit_to_number(ptr[2]) << 4) | hex_digit_to_number(ptr[3]);
                }
              [[fallthrough]];
              default:
                {
                    if (new_ptr)
                        *new_ptr = ptr + 1;
                    return '\\';
                }
            }
        }

        inline std::string add_quotes(const std::string &str)
        {
            std::size_t len = 2; // For 2 quotes.
            for (char ch : str)
                len += char_escape_len(ch);

            std::string ret;
            ret.reserve(len);
            ret.push_back('"');
            for (char ch : str)
            {
                if (char_escape_len(ch) == 1)
                    ret.push_back(ch);
                else
                    ret += char_escape_seq(ch);
            }
            ret.push_back('"');
            return ret;
        }

        inline std::string remove_quotes(const char *str, std::size_t *chars_consumed = 0)
        {
            if (*str != '"')
            {
                if (chars_consumed)
                    *chars_consumed = 0;
                return "";
            }
            str++;

            const char *begin = str;
            std::string ret;

            while ((*str != '"' || str[-1] == '\\') && *str != '\0')
                ret += char_from_escape(str, &str);

            if (*str != '"')
            {
                if (chars_consumed)
                    *chars_consumed = 0;
                return "";
            }

            if (chars_consumed)
                *chars_consumed = str - begin + 2;

            return ret;
        }

        // Increments `ptr` until a first non-whitespace char (or end of string) is reached.
        inline void skip_ws(const char *&ptr)
        {
            while (*ptr && (unsigned char)*ptr <= ' ')
                ptr++;
        }
        // Checks if `*ptr` is in the list, then increments `ptr` (if `*ptr != '\0'`).
        inline bool skip_one_of(const char *&ptr, const char *list)
        {
            if (!*ptr)
                return 0;
            return std::strchr(list, *ptr++);
        }
    }

    namespace Bytes
    {
        inline void fix_order(uint8_t *begin, uint8_t *end)
        {
            #ifdef REFLECTION_NONNATIVE_ENDIANNESS
            std::size_t half_len = (end - begin) / 2;
            for (std::size_t i = 0; i < half_len; i++)
                std::swap(*begin++, *--end);
            #else
            (void)begin;
            (void)end;
            #endif
        }
        template <typename T> void fix_order(T &value)
        {
            static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>, "T must be arithmetic or an enum.");
            fix_order((uint8_t *)&value, (uint8_t *)(&value + 1));
        }
    }

    namespace Cexpr
    {
        // Utils

        template <typename F, std::size_t ...Seq> inline void for_each(std::index_sequence<Seq...>, F &&f)
        {
            (f(std::integral_constant<std::size_t, Seq>{}) , ...);
        }
        template <typename F, std::size_t ...Seq> inline bool for_each_while(std::index_sequence<Seq...>, F &&f)
        {
            return (bool(f(std::integral_constant<std::size_t, Seq>{})) && ...);
        }

        template <typename T, typename = void> struct supports_reset_by_empty_list_assignment_impl : std::false_type {};
        template <typename T> struct supports_reset_by_empty_list_assignment_impl<T, std::void_t<decltype(std::declval<T &>() = {})>> : std::true_type {};
        template <typename T> inline constexpr bool supports_reset_by_empty_list_assignment = supports_reset_by_empty_list_assignment_impl<T>::value;

        template <typename T, typename = void> struct supports_reset_by_swap_impl : std::false_type {};
        template <typename T> struct supports_reset_by_swap_impl<T, std::void_t<decltype(std::swap(std::declval<T &>(), std::declval<T &>()))>> : std::true_type {};
        template <typename T> inline constexpr bool supports_reset_by_swap = supports_reset_by_swap_impl<T>::value;

        // Strings

        template <char ...C> struct str
        {
            static constexpr char value[] = {C..., '\0'};
        };

        template <typename F, typename ...P> struct str_cat_impl {using type = typename str_cat_impl<F, typename str_cat_impl<P...>::type>::type;};
        template <typename T> struct str_cat_impl<T> {using type = T;};
        template <char ...A, char ...B> struct str_cat_impl<str<A...>, str<B...>> {using type = str<A..., B...>;};
        template <typename F, typename ...P> using str_cat = typename str_cat_impl<F, P...>::type;

        template <char C> inline constexpr bool char_is_alphanum = (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '_';

        template <typename F, typename ...P> struct str_smart_cat_impl {using type = typename str_smart_cat_impl<F, typename str_smart_cat_impl<P...>::type>::type;};
        template <typename T> struct str_smart_cat_impl<T> {using type = T;};
        template <char ...A, char ...B> struct str_smart_cat_impl<str<A...>, str<B...>>
        {
            using type = std::conditional_t<sizeof...(A) == 0
                                             || sizeof...(B) == 0
                                             || !char_is_alphanum<str<A...>::value[(sizeof...(A)-1) * bool(sizeof...(A))]>
                                             || !char_is_alphanum<str<B...>::value[0]>,
                                            str_cat<str<A...>, str<B...>>,
                                            str_cat<str<A...>, str<' '>, str<B...>>>;
        };
        template <typename F, typename ...P> using str_smart_cat = typename str_smart_cat_impl<F, P...>::type;

        template <std::size_t A, std::size_t B> struct pow
        {
            static constexpr std::size_t value = A * pow<A, B-1>::value;
        };
        template <std::size_t A> struct pow<A, 0>
        {
            static constexpr std::size_t value = 1;
        };

        template <std::size_t N, std::size_t Pow, std::size_t ...Seq> constexpr auto num_to_str_impl(std::index_sequence<Seq...>)
        {
            if constexpr (N >= pow<10, Pow>::value)
                return num_to_str_impl<N, Pow+1>(std::make_index_sequence<Pow+1>{});
            else
                return str<(N / pow<10, Pow-1-Seq>::value % 10 + '0')...>{};
        }
        template <std::size_t N> using num_to_str = decltype(num_to_str_impl<N, 1>(std::make_index_sequence<1>{}));

        // Returns 1 only when the string consists of non-printable characters only.
        template <std::size_t N> inline constexpr bool string_is_empty(const char (&str)[N])
        {
            for (std::size_t i = 0; i < N; i++)
                if (str[i] > ' ')
                    return 0;
            return 1;
        }
    }

    namespace InterfaceDetails
    {
        // DO NOT mark your own `interface_*` functions as `noexcept`. It serves as an indicator that the function is not provided.

        template <typename...> struct type_list {};
        template <auto...> struct value_list {};

        struct sink
        {
            template <typename T> constexpr sink(T &&) {}
            template <typename T> constexpr sink operator=(T &&) {return *this;}
            template <typename T> operator T() const {return {};}
        };

        template <typename F, typename ...P> struct last_of_impl {using type = typename last_of_impl<P...>::type;};
        template <typename F> struct last_of_impl<F> {using type = F;};
        template <typename F, typename ...P> using last_of = typename last_of_impl<F, P...>::type;


        template <typename T> inline constexpr bool forced_primitive = std::is_same_v<std::string, T>;


        // Default interface functions

        inline constexpr std::size_t reflection_interface_field_count(/*unused*/ const void *) noexcept {return -1;}

        // This should return a reference to the field.
        template <std::size_t I> void reflection_interface_field(void *, index_const<I>) = delete;

        template <std::size_t I> constexpr bool reflection_interface_field_has_default_value(/*unused*/ const void *, index_const<I>) noexcept {return 0;}

        template <std::size_t I> constexpr const char *reflection_interface_field_name(/*unused*/ const void *, index_const<I>) noexcept {return "?";}

        // It makes all fields have numeric names, overriding `reflection_interface_field_name`.
        // `reflection_interface_field_has_default_value` is also overriden to 0.
        // When converting tuples to strings, field names aren't printed.
        inline constexpr bool reflection_interface_structure_is_tuple(/*unused*/ const void *) noexcept {return 0;}

        // Should be used for primitives only.
        inline std::string reflection_interface_primitive_to_string(const void *) noexcept {return "??";}
        inline std::size_t reflection_interface_primitive_from_string(void *, const char *) = delete; // Returns a number of chars consumed or 0 if the conversion has failed (in this case the object is left unchanged).

        inline std::size_t reflection_interface_primitive_byte_buffer_size(const void *) = delete; // Returns the amount of bytes needed to store an object.
        inline uint8_t *reflection_interface_primitive_to_bytes(const void *, uint8_t *buf) = delete; // Returns the pointer to the next free byte in the buffer.
        inline const uint8_t *reflection_interface_primitive_from_bytes(void *, const uint8_t *buf, const uint8_t *buf_end) = delete; // Returns a pointer to the next byte in the buffer or 0 on failure.

        // Should be used for composites only and return a short single-line summary of contents. Optional.
        inline std::string reflection_interface_composite_summary_string(const void *) noexcept {return "...";}

        inline std::size_t reflection_interface_container_size(const void *) noexcept {return 0;}
        // These should return corresponding iterators.
        inline void/*iterator*/ reflection_interface_container_begin(void *) noexcept {}
        inline void/*iterator*/ reflection_interface_container_end(void *) noexcept {}
        inline void/*iterator*/ reflection_interface_container_cbegin(const void *) noexcept {}
        inline void/*iterator*/ reflection_interface_container_cend(const void *) noexcept {}
        // These should return an iterator to the insterted element. For associative containers, this should do nothing if the element already exists.
        inline void/*iterator*/ reflection_interface_container_insert(void *, sink/*const const_iterator? &iterator*/, sink/*const decltype(*std::begin(T(...))) &value*/) noexcept;
        inline void/*iterator*/ reflection_interface_container_insert_move(void *, sink/*const const_iterator? &iterator*/, sink/*decltype(*std::begin(T(...))) &&value*/) noexcept;
        // This should return an iterator to the next valid element.
        inline void/*iterator*/ reflection_interface_container_erase(void *, sink/*const const_iterator? &begin_iterator*/, sink/*const const_iterator? &end_iterator*/) noexcept;

        template <typename T> std::enable_if_t<std::is_enum_v<T>, const std::unordered_map<std::string, T> &> reflection_interface_enum_string_value_map(const T *) {static std::unordered_map<std::string, T> ret; return ret;} // No noexcept here.
        template <typename T> std::enable_if_t<std::is_enum_v<T>, const std::unordered_map<T, std::string> &> reflection_interface_enum_value_string_map(const T *) {static std::unordered_map<T, std::string> ret; return ret;} // No noexcept here.


        // Interface function specializations

        // Strings
        inline std::string reflection_interface_primitive_to_string(const std::string *obj)
        {
            return Strings::add_quotes(*obj);
        }
        inline std::size_t reflection_interface_primitive_from_string(std::string *obj, const char *str)
        {
            std::size_t chars_consumed;
            std::string val = Strings::remove_quotes(str, &chars_consumed);
            if (chars_consumed == 0)
                return 0;
            *obj = val;
            return chars_consumed;
        }
        inline std::size_t reflection_interface_primitive_byte_buffer_size(const std::string *obj)
        {
            return 4 + obj->size();
        }
        inline uint8_t *reflection_interface_primitive_to_bytes(const std::string *obj, uint8_t *buf)
        {
            uint32_t len = obj->size();
            Bytes::fix_order(len);
            std::memcpy(buf, &len, sizeof len);
            buf += sizeof len;
            std::memcpy(buf, obj->data(), obj->size());
            buf += obj->size();
            return buf;
        }
        inline const uint8_t *reflection_interface_primitive_from_bytes(std::string *obj, const uint8_t *buf, const uint8_t *buf_end)
        {
            if (buf_end - buf < 4)
                return 0;
            uint32_t len;
            std::memcpy(&len, buf, sizeof len);
            Bytes::fix_order(len);
            buf += sizeof len;

            if (buf_end - buf < std::ptrdiff_t(len))
                return 0;
            obj->clear();
            obj->reserve(len);
            for (std::size_t i = 0; i < len; i++)
                obj->push_back(*buf++);
            return buf;
        }

        // Arithmetic types
        template <typename T> std::enable_if_t<std::is_arithmetic_v<T>, std::string> reflection_interface_primitive_to_string(const T *obj) {return Math::num_to_string<T>(*obj);}
        inline std::string reflection_interface_primitive_to_string(const bool *obj) {return (*obj ? "true" : "false");}

        template <typename T> std::enable_if_t<std::is_arithmetic_v<T>, std::size_t> reflection_interface_primitive_from_string(T *obj, const char *str)
        {
            if (std::isspace(*str))
                return 0;

            T ret;
            char *end;

            if constexpr (std::is_integral_v<T>)
            {
                if constexpr (sizeof (T) <= sizeof (long))
                {
                    if constexpr (std::is_signed_v<T>)
                        ret = std::strtol(str, &end, 0);
                    else
                        ret = std::strtoul(str, &end, 0);
                }
                else
                {
                    if constexpr (std::is_signed_v<T>)
                        ret = std::strtoll(str, &end, 0);
                    else
                        ret = std::strtoull(str, &end, 0);
                }
            }
            else if constexpr (sizeof (T) <= sizeof (float))
                ret = std::strtof(str, &end);
            else if constexpr (sizeof (T) <= sizeof (double))
                ret = std::strtod(str, &end);
            else
                ret = std::strtold(str, &end);

            if (end == str)
                return 0;

            *obj = ret;
            return end - str;
        }
        inline std::size_t reflection_interface_primitive_from_string(bool *obj, const char *str)
        {
            bool ok, value;
            switch (*str)
            {
              case 0:
              case 1:
                *obj = *str - '0';
                return 1;
              case 't':
                ok = !strncmp(str+1, "rue", 3);
                value = 1;
                break;
              case 'T':
                ok = !strncmp(str+1, "RUE", 3);
                value = 1;
                break;
              case 'f':
                ok = !strncmp(str+1, "alse", 4);
                value = 0;
                break;
              case 'F':
                ok = !strncmp(str+1, "ALSE", 4);
                value = 0;
                break;
            }

            if (ok)
            {
                *obj = value;
                return 4 + value;
            }
            else
                return 0;
        }

        // (these also work for enums)
        template <typename T> std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, std::size_t> reflection_interface_primitive_byte_buffer_size(const T *) {return sizeof(T);}
        template <typename T> std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, uint8_t *> reflection_interface_primitive_to_bytes(const T *obj, uint8_t *buf)
        {
            std::memcpy(buf, obj, sizeof(T));
            Bytes::fix_order(buf, buf + sizeof(T));
            return buf + sizeof(T);
        }
        template <typename T> std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, const uint8_t *> reflection_interface_primitive_from_bytes(T *obj, const uint8_t *buf, const uint8_t *buf_end)
        {
            if (buf_end - buf < std::ptrdiff_t(sizeof(T)))
                return 0;
            std::memcpy(obj, buf, sizeof(T));
            Bytes::fix_order(*obj);
            return buf + sizeof(T);
        }

        // Enums
        template <typename T> std::enable_if_t<std::is_enum_v<T>, std::string> reflection_interface_primitive_to_string(const T *obj)
        {
            const auto &map = reflection_interface_enum_value_string_map(obj);
            auto it = map.find(*obj);
            if (it == map.end())
            {
                std::underlying_type_t<T> val = (std::underlying_type_t<T>)*obj;
                return reflection_interface_primitive_to_string(&val);
            }
            else
                return it->second;
        }
        template <typename T> std::enable_if_t<std::is_enum_v<T>, std::size_t> reflection_interface_primitive_from_string(T *obj, const char *str)
        {
            if (Strings::is_digit(*str) || *str == '-' || *str == '+')
            {
                std::underlying_type_t<T> val;
                std::size_t status = reflection_interface_primitive_from_string(&val, str);
                if (!status) return 0;
                *obj = (T)val;
                return status;
            }
            std::size_t len = 0;
            const char *ptr = str;
            while (Strings::is_alphanum(*ptr))
            {
                ptr++;
                len++;
            }
            const auto &map = reflection_interface_enum_string_value_map(obj);
            auto it = map.find(std::string(str, len));
            if (it == map.end())
                return 0;
            *obj = it->second;
            return it->first.size();
        }
        // (byte-related functions from arithmetics are used)

        // Bit sets.
        template <std::size_t S> std::string reflection_interface_primitive_to_string(const std::bitset<S> *obj)
        {
            std::string ret;
            ret.reserve(S);
            for (std::size_t i = 0; i < S; i++)
                ret += '0' + obj->test(i);
            return ret;
        }
        template <std::size_t S> std::size_t reflection_interface_primitive_from_string(std::bitset<S> *obj, const char *str)
        {
            std::bitset<S> new_obj;
            for (std::size_t i = 0; i < S; i++)
            {
                if (str[i] != '0' && str[i] != '1')
                    return 0;
                new_obj.set(i, str[i] == '1');
            }
            *obj = new_obj;
            return S;
        }
        template <std::size_t S> std::size_t reflection_interface_primitive_byte_buffer_size(const std::bitset<S> *)
        {
            return S;
        }
        template <std::size_t S> uint8_t *reflection_interface_primitive_to_bytes(const std::bitset<S> *obj, uint8_t *buf)
        {
            for (std::size_t i = 0; i < S; i++)
                buf[i] = obj->test(i);
            return buf + S;
        }
        template <std::size_t S> const uint8_t *reflection_interface_primitive_from_bytes(std::bitset<S> *obj, const uint8_t *buf, const uint8_t *buf_end)
        {
            if (buf_end - buf < std::ptrdiff_t(S))
                return 0;
            for (std::size_t i = 0; i < S; i++)
            {
                if (buf[i] != 0 && buf[i] != 1)
                    return 0;
                obj->set(i, buf[i]);
            }
            return buf + S;
        }


        // Vectors/matrices
        template <typename T> constexpr std::enable_if_t<Math::type_category<T>::vec_or_mat, std::size_t> reflection_interface_field_count(const T *)
        {
            if constexpr (Math::type_category<T>::vec)
                return T::size;
            else
                return T::width;
        }
        template <typename T, std::size_t I> std::enable_if_t<Math::type_category<T>::vec_or_mat, decltype(T::x) &> reflection_interface_field(T *ptr, index_const<I>)
        {
                 if constexpr (I == 0) return ptr->x;
            else if constexpr (I == 1) return ptr->y;
            else if constexpr (I == 2) return ptr->z;
            else if constexpr (I == 3) return ptr->w;
            else                       return {};
        }
        template <typename T> constexpr std::enable_if_t<Math::type_category<T>::vec_or_mat, bool> reflection_interface_structure_is_tuple(const T *) {return 1;}
        template <typename T, typename = std::enable_if_t<Math::type_category<T>::vec_or_mat>> std::string reflection_interface_composite_summary_string(const T *ptr) {return ptr->to_string();}

        // Tuples, pairs and `std::array`s.
        template <typename T               > constexpr last_of<decltype(std::tuple_size<T>::value), std::size_t> reflection_interface_field_count       (const T *) {return std::tuple_size<T>::value;}
        template <typename T, std::size_t I> constexpr decltype(auto) reflection_interface_field(T *ptr, index_const<I>) -> last_of<decltype(std::tuple_size<T>::value), decltype(std::get<I>(*ptr))> {return std::get<I>(*ptr);}
        template <typename T               > constexpr last_of<decltype(std::tuple_size<T>::value), bool       > reflection_interface_structure_is_tuple(const T *) {return 1;}

        // Plain arrays.
        template <typename T               > constexpr std::enable_if_t<std::is_array_v<T>, std::size_t              > reflection_interface_field_count       (const T *                   ) {return std::extent_v<T>;}
        template <typename T, std::size_t I> constexpr std::enable_if_t<std::is_array_v<T>, std::remove_extent_t<T> &> reflection_interface_field             (      T *ptr, index_const<I>) {return (*ptr)[I];}
        template <typename T               > constexpr std::enable_if_t<std::is_array_v<T>, bool                     > reflection_interface_structure_is_tuple(const T *                   ) {return 1;}

        // Standard containers
        /*internal*/ template <typename T> using impl_std_cont_iter_t       = decltype(std::begin(std::declval<T &>()));
        /*internal*/ template <typename T> using impl_std_cont_const_iter_t = decltype(std::cbegin(std::declval<const T &>()));
        /*interanl*/ template <typename T> using impl_std_cont_value_t      = std::remove_const_t<std::remove_reference_t<decltype(*std::declval<impl_std_cont_const_iter_t<T>>())>>;
        template <typename T> std::enable_if_t<!forced_primitive<T>, std::size_t                                     > reflection_interface_container_size  (const T *ptr) {return std::size  (*ptr);}
        template <typename T> std::enable_if_t<!forced_primitive<T>, decltype(std::begin (std::declval<      T &>()))> reflection_interface_container_begin (      T *ptr) {return std::begin (*ptr);}
        template <typename T> std::enable_if_t<!forced_primitive<T>, decltype(std::end   (std::declval<      T &>()))> reflection_interface_container_end   (      T *ptr) {return std::end   (*ptr);}
        template <typename T> std::enable_if_t<!forced_primitive<T>, decltype(std::cbegin(std::declval<const T &>()))> reflection_interface_container_cbegin(const T *ptr) {return std::cbegin(*ptr);}
        template <typename T> std::enable_if_t<!forced_primitive<T>, decltype(std::cend  (std::declval<const T &>()))> reflection_interface_container_cend  (const T *ptr) {return std::cend  (*ptr);}
        template <typename T> std::enable_if_t<!forced_primitive<T>, decltype(std::declval<T &>().insert(std::cbegin(std::declval<const T &>()), std::declval<impl_std_cont_value_t<T>>()))>
                              reflection_interface_container_insert     (T *ptr, const impl_std_cont_const_iter_t<T> &iter, const impl_std_cont_value_t<T>  &value) {return ptr->insert(iter, value);}
        template <typename T> std::enable_if_t<!forced_primitive<T>, decltype(std::declval<T &>().insert(std::cbegin(std::declval<const T &>()), std::declval<impl_std_cont_value_t<T>>()))>
                              reflection_interface_container_insert_move(T *ptr, const impl_std_cont_const_iter_t<T> &iter,       impl_std_cont_value_t<T> &&value) {return ptr->insert(iter, (impl_std_cont_value_t<T> &&) value);}
        template <typename T> std::enable_if_t<!forced_primitive<T>, decltype(std::declval<T &>().erase(std::cbegin(std::declval<const T &>()), std::cbegin/*sic*/(std::declval<const T &>())))>
                              reflection_interface_container_erase      (T *ptr, const impl_std_cont_const_iter_t<T> &begin, const impl_std_cont_const_iter_t<T> &end) {return ptr->erase(begin, end);}

        class Interface
        {
            ~Interface() = delete;
          public:

            template <typename T> static constexpr bool is_structure() {return reflection_interface_field_count((const T *)0) != std::size_t(-1);}

            template <typename T> static constexpr std::size_t field_count() {return is_structure<T>() ? reflection_interface_field_count((const T *)0) : 0;}

            template <std::size_t I, typename T> static       auto &field(      T &obj) {return reflection_interface_field(&obj, index_const<I>{});}
            template <std::size_t I, typename T> static const auto &field(const T &obj) {return (const std::remove_reference_t<decltype(field<I>((T &)obj))> &)field<I>((T &)obj);}

            template <typename T, std::size_t I> using field_type = std::remove_reference_t<decltype(field<I>(std::declval<T &>()))>;

            template <typename T> static constexpr bool structure_is_tuple() {return reflection_interface_structure_is_tuple((const T *)0);} // If this is set to 1, `field_name` returns field indices.

            template <typename T, std::size_t I> static constexpr bool field_has_default_value()
            {
                if constexpr (structure_is_tuple<T>())
                    return 0;
                else
                    return reflection_interface_field_has_default_value((const T *)0, index_const<I>{});
            }

            template <typename T, std::size_t I> static constexpr const char *field_name()
            {
                if constexpr (structure_is_tuple<T>())
                    return Cexpr::num_to_str<I>::value;
                else
                    return reflection_interface_field_name((const T *)0, index_const<I>{});
            }

            // Note that for composites this returns a short summary of contents.
            template <typename T> static std::string to_string(const T &obj)
            {
                if constexpr (is_primitive<T>())
                    return reflection_interface_primitive_to_string(&obj);
                else
                    return reflection_interface_composite_summary_string(&obj);
            }
            template <typename T> static constexpr bool has_to_string()
            {
                if constexpr (is_primitive<T>())
                    return !noexcept(reflection_interface_primitive_to_string((const T *)0));
                else
                    return !noexcept(reflection_interface_composite_summary_string((const T *)0));
            }

            template <typename T> static std::size_t primitive_from_string(T &obj, const char *str) {return reflection_interface_primitive_from_string(&obj, str);}
            template <typename T> static constexpr bool primitive_has_from_string() {return !noexcept(reflection_interface_primitive_from_string((T *)0, (const char *)0));}

            template <typename T> static std::size_t primitive_byte_buffer_size(const T &obj) {return reflection_interface_primitive_byte_buffer_size(&obj);}
            template <typename T> static uint8_t *primitive_to_bytes(const T &obj, uint8_t *buf) {return reflection_interface_primitive_to_bytes(&obj, buf);} // Returns a next pointer.
            template <typename T> static const uint8_t *primitive_from_bytes(T &obj, const uint8_t *buf, const uint8_t *buf_end) {return reflection_interface_primitive_from_bytes(&obj, buf, buf_end);} // Returns a next pointer or 0 on failure.

            template <typename T> static const auto &enum_string_value_map() {return reflection_interface_enum_string_value_map((const T *)0);}
            template <typename T> static const auto &enum_value_string_map() {return reflection_interface_enum_value_string_map((const T *)0);}


            template <typename T> static constexpr bool is_container()
            {
                using type = std::remove_cv_t<T> *;
                return !is_structure<T>()
                    && !std::is_void_v<decltype(reflection_interface_container_size  (std::declval<type>()))>
                    && !std::is_void_v<decltype(reflection_interface_container_begin (std::declval<type>()))>
                    && !std::is_void_v<decltype(reflection_interface_container_end   (std::declval<type>()))>
                    && !std::is_void_v<decltype(reflection_interface_container_cbegin(std::declval<type>()))>
                    && !std::is_void_v<decltype(reflection_interface_container_cend  (std::declval<type>()))>;
            }

            template <typename T> using container_iterator_t       = decltype(reflection_interface_container_begin(std::declval<T *>()));
            template <typename T> using container_const_iterator_t = decltype(reflection_interface_container_cbegin(std::declval<const T *>()));

          private:
            template <typename T, typename = void> struct container_value_t_impl {using type = std::remove_const_t<std::remove_reference_t<decltype(*std::declval<container_const_iterator_t<T>>())>>;};
            template <typename T> struct container_value_t_impl<T, container_const_iterator_t<T>> {using type = void;};

            template <typename T, typename = void> struct container_make_value_type_mutable_impl {using type = std::remove_const_t<T>;};
            template <typename A, typename B> struct container_make_value_type_mutable_impl<std::pair<A,B>> {using type = std::pair<std::remove_const_t<A>, std::remove_const_t<B>>;};
          public:
            template <typename T> using container_value_t          = typename container_value_t_impl<T>::type;
            template <typename T> using container_mutable_value_t  = typename container_make_value_type_mutable_impl<container_value_t<T>>::type; // This is good for making mutable temporary objects to insert to a container.

            template <typename T> static std::size_t             container_size       (const T &obj) {return reflection_interface_container_size  (&obj);}
            template <typename T> static auto                    container_begin      (      T &obj) {return reflection_interface_container_begin (&obj);}
            template <typename T> static auto                    container_end        (      T &obj) {return reflection_interface_container_end   (&obj);}
            template <typename T> static auto                    container_cbegin     (const T &obj) {return reflection_interface_container_cbegin(&obj);}
            template <typename T> static auto                    container_cend       (const T &obj) {return reflection_interface_container_cend  (&obj);}
            template <typename T> static container_iterator_t<T> container_insert     (      T &obj, const container_const_iterator_t<T> &iter, const container_value_t<T>  &val) {return reflection_interface_container_insert     (&obj, iter, val);}
            template <typename T> static container_iterator_t<T> container_insert_move(      T &obj, const container_const_iterator_t<T> &iter,       container_value_t<T> &&val) {return reflection_interface_container_insert_move(&obj, iter, (container_value_t<T> &&) val);}
            template <typename T> static container_iterator_t<T> container_erase      (      T &obj, const container_const_iterator_t<T> &begin, const container_const_iterator_t<T> &end) {return reflection_interface_container_erase(&obj, begin, end);}


            template <typename T, typename F> static void container_for_each(      T &obj, F &&func) {std::for_each(container_begin(obj), container_end(obj), (F &&) func);}
            template <typename T, typename F> static void container_for_each(const T &obj, F &&func) {std::for_each(container_cbegin(obj), container_cend(obj), (F &&) func);}

            template <typename T> static constexpr bool is_primitive() {return !is_structure<T>() && !is_container<T>();}


            class Impl
            {
                ~Impl() = delete;
              public:

                // Counter

                template <typename T, typename Tag, std::size_t I, typename Unique, typename = void> struct counter_has_crumb
                {
                    static constexpr bool value = 0;
                };
                template <typename T, typename Tag, std::size_t I, typename Unique> struct counter_has_crumb<T,Tag,I,Unique,std::void_t<decltype(T::_reflection_internal_counter_crumb(type_list<Tag>{}, index_const<I>{}))>>
                {
                    static constexpr bool value = 1;
                };

                template <typename T, typename Tag, typename Unique, std::size_t I = 0> struct counter_value
                {
                    static constexpr bool has_crumb = counter_has_crumb<T,Tag,I,Unique>::value;
                    static constexpr std::size_t value = has_crumb + counter_value<T, Tag, Unique, has_crumb ? I+1 : -1>::value;
                };
                template <typename T, typename Tag, typename Unique> struct counter_value<T,Tag,Unique,-1>
                {
                    static constexpr std::size_t value = 0;
                };

                struct counter_tag_fields {};
            };
        };
    }

    using InterfaceDetails::Interface;

    template <typename T> std::string to_string(const T &object)
    {
        if constexpr (Interface::is_structure<T>())
        {
            std::string ret = (Interface::structure_is_tuple<T>() ? "(" : "{");

            auto lambda = [&](auto index)
            {
                constexpr bool no_names = !Interface::structure_is_tuple<T>();
                if constexpr (no_names)
                {
                    ret += Interface::field_name<T, index.value>();
                    ret += '=';
                }
                ret += to_string(Interface::field<index.value>(object));
                ret += ',';
            };

            Cexpr::for_each(std::make_index_sequence<Interface::field_count<T>()>{}, lambda);

            if (Interface::field_count<T>())
                ret[ret.size()-1] = (Interface::structure_is_tuple<T>() ? ')' : '}');

            return ret;
        }
        else if constexpr (Interface::is_container<T>())
        {
            std::string ret = "[";

            for (auto it = Interface::container_cbegin(object); it != Interface::container_cend(object); it++)
            {
                ret += to_string(*it);
                ret += ',';
            }

            if (Interface::container_size(object))
                ret[ret.size()-1] = ']';
            else
                ret.push_back(']');

            return ret;
        }
        else
        {
            static_assert(Interface::has_to_string<T>(), "This type doesn't support conversion to a string.");
            return Interface::to_string(object);
        }
    }

    template <typename T> std::string to_string_tree(const T &object, int depth = -1) // `depth == -1` means no limit.
    {
        [[maybe_unused]] auto Indent = [](std::string param, char symbol, bool no_leading_lf = 0) -> std::string
        {
            if (param.empty())
                return param;
            if (!no_leading_lf)
                param = '\n' + param;
            auto lf_c = std::count(param.begin(), param.end(), '\n');
            std::string indented;
            indented.reserve(param.size() + lf_c*2);

            for (char ch : param)
            {
                if (ch != '\n')
                    indented += ch;
                else
                {
                    indented += '\n';
                    indented += symbol;
                    indented += ' ';
                }
            }
            return indented;
        };


        if constexpr (Interface::is_structure<T>())
        {
            std::string ret;

            auto lambda = [&](auto index)
            {
                const auto &field = Interface::field<index.value>(object);
                using field_t = std::remove_reference_t<decltype(field)>;

                constexpr bool is_primitive = Interface::is_primitive<field_t>();
                constexpr bool structure_is_tuple = Interface::structure_is_tuple<T>();

                if (index.value != 0) ret += '\n';
                ret += (index.value != Interface::field_count<T>()-1 ? '|' : '`');
                ret += (depth == 0 && !is_primitive ? '*' : '-');
                if constexpr (!structure_is_tuple)
                    ret += Interface::field_name<T, index.value>();
                if constexpr (!is_primitive)
                {
                    if (depth == 0)
                    {
                        ret += (structure_is_tuple ? ':' : '=');
                        ret += Interface::to_string(field);
                    }
                    else
                    {
                        std::string str = Indent(to_string_tree(field, depth - 1), "| "[index.value == Interface::field_count<T>()-1], Interface::structure_is_tuple<T>());
                        if (structure_is_tuple && str.size() >= 1)
                            str[0] = '+';
                        ret += str;
                    }
                }
                else
                {
                    ret += (structure_is_tuple ? ':' : '=');
                    ret += Interface::to_string(field);
                }
            };

            Cexpr::for_each(std::make_index_sequence<Interface::field_count<T>()>{}, lambda);

            return ret;
        }
        else if constexpr (Interface::is_container<T>())
        {
            std::string ret;

            std::size_t pos = 0, size = Interface::container_size(object);

            using value_t = Interface::container_value_t<T>;

            constexpr bool primitive_value = Interface::is_primitive<value_t>();


            for (auto it = Interface::container_cbegin(object); it != Interface::container_cend(object); it++)
            {
                if (pos != 0) ret += '\n';
                ret += (pos != size-1 ? ':' : '`');
                ret += (depth == 0 && !primitive_value ? '*' : '-');
                ret += std::to_string(pos);
                if constexpr (!primitive_value)
                {
                    if (depth == 0)
                    {
                        ret += '=';
                        ret += Interface::to_string(*it);
                    }
                    else
                        ret += Indent(to_string_tree(*it, depth - 1), ": "[pos == size-1]);
                }
                else
                {
                    ret += '=';
                    ret += Interface::to_string(*it);
                }
                pos++;
            }

            return ret;
        }
        else /* is_primitive<T>() */ // This is not used recursively.
        {
            return Interface::to_string(object);
        }
    }


    enum FromStringMode {complete, maybe_incomplete};

    namespace from_string_impl
    {
        enum class Error
        {
            no_error,
            incomplete,
            bad_primitive,
            missing_start_of_tuple_structure,
            missing_start_of_structure,
            missing_start_of_container,
            missing_comma,
            missing_equality,
            invalid_field_name,
            duplicate_fields,
            missing_end_of_tuple_structure,
        };

        template <typename T> const char *from_string(T &object, const char *str, bool verbose_errors, Error &error, std::string &error_details, std::vector<const char *> &stack);

        template <typename T, std::size_t I> constexpr decltype(from_string<T>) *from_string_bound_to_field()
        {
            return [](T &object, const char *str, bool verbose_errors, Error &error, std::string &error_details, std::vector<const char *> &stack) -> const char *
            {
                return from_string(Interface::field<I>(object), str, verbose_errors, error, error_details, stack);
            };
        }

        template <typename T, std::size_t ...I> decltype(from_string<T>) *const (&from_string_func_ptr_array(std::index_sequence<I...>))[sizeof...(I)]
        {
            static decltype(from_string<T>) *const array[] {from_string_bound_to_field<T, I>()...};
            return array;
        }
        template <typename T> const char *from_string_field(std::size_t index, T &object, const char *str, bool verbose_errors, Error &error, std::string &error_details, std::vector<const char *> &stack)
        {
            (void)index; (void)object; (void)str; (void)verbose_errors; (void)error; (void)error_details; (void)stack;
            if constexpr (Interface::field_count<T>() > 0)
                return from_string_func_ptr_array<T>(std::make_index_sequence<Interface::field_count<T>()>{})[index](object, str, verbose_errors, error, error_details, stack);
            else
                return 0;
        }

        template <typename T, std::size_t ...I> const auto &field_name_to_index_map(std::index_sequence<I...>)
        {
            static const std::unordered_map<std::string, std::size_t> map{{Interface::field_name<T,I>(), I}...};
            return map;
        }
        template <typename T> std::size_t field_name_to_index(const std::string &name)
        {
            const auto &map = field_name_to_index_map<T>(std::make_index_sequence<Interface::field_count<T>()>{});
            auto it = map.find(name);
            if (it != map.end())
                return it->second;
            else
                return std::size_t(-1);
        }

        template <typename T, std::size_t ...I> const char *const (&field_index_to_name_array(std::index_sequence<I...>))[sizeof...(I)]
        {
            static constexpr const char *const array[]{Interface::field_name<T,I>()...};
            return array;
        }
        template <typename T> constexpr const char *field_index_to_name(std::size_t index)
        {
            (void)index;
            if constexpr (Interface::field_count<T>() > 0)
                return field_index_to_name_array<T>(std::make_index_sequence<Interface::field_count<T>()>{})[index];
            else
                return "";
        }

        // `stack` records field names. For container elements, it will have a null followed by (index + 1).
        template <typename T> const char *from_string(T &object, const char *str, bool verbose_errors, Error &error, std::string &error_details, std::vector<const char *> &stack)
        {
            (void)verbose_errors;

            if (!str)
                return 0;

            Strings::skip_ws(str);

            if constexpr (Interface::is_structure<T>())
            {
                if constexpr (Interface::structure_is_tuple<T>())
                {
                    if (!Strings::skip_one_of(str, "("))
                    {
                        error = Error::missing_start_of_tuple_structure;
                        return 0;
                    }

                    bool ok = Cexpr::for_each_while(std::make_index_sequence<Interface::field_count<T>()>{}, [&](auto index) -> bool
                    {
                        if constexpr (index.value != 0)
                        {
                            Strings::skip_ws(str);
                            if (!Strings::skip_one_of(str, ","))
                            {
                                error = Error::missing_comma;
                                return 0;
                            }
                        }

                        if (verbose_errors) stack.push_back(Interface::field_name<T, index.value>()); // Push field name.

                        str = from_string(Interface::field<index.value>(object), str, verbose_errors, error, error_details, stack);
                        if (!str) // If `from_string()` failed.
                            return 0; // `error` was already set by `from_string()`.

                        if (verbose_errors) stack.pop_back(); // Pop field name.

                        return 1;
                    });

                    if (!ok)
                        return 0; // Forward error from the lambda.

                    Strings::skip_ws(str);
                    if (!Strings::skip_one_of(str, "),"))
                    {
                        error = Error::missing_end_of_tuple_structure;
                        return 0;
                    }
                    if (str[-1] == ',')
                    {
                        Strings::skip_ws(str);
                        if (!Strings::skip_one_of(str, ")"))
                        {
                            error = Error::missing_end_of_tuple_structure;
                            return 0;
                        }
                    }
                }
                else // Structure, but not a tuple.
                {
                    if (!Strings::skip_one_of(str, "{"))
                    {
                        error = Error::missing_start_of_structure;
                        return 0;
                    }

                    bool assigned_fields[Interface::field_count<T>() > 0 ? Interface::field_count<T>() : 1] {};
                    bool first = 1;

                    while (1)
                    {
                        if (first)
                        {
                            first = 0;
                        }
                        else
                        {
                            Strings::skip_ws(str);
                            if (!Strings::skip_one_of(str, ","))
                            {
                                if (str[-1] == '}')
                                    break;

                                error = Error::missing_comma;
                                return 0;
                            }
                        }

                        Strings::skip_ws(str);

                        if (*str == '}')
                        {
                            str++;
                            break;
                        }

                        std::string field_name;

                        while (Strings::is_alphanum(*str))
                            field_name += *str++;

                        std::size_t field_index = field_name_to_index<T>(field_name);
                        if (field_index == std::size_t(-1))
                        {
                            error = Error::invalid_field_name;
                            if (verbose_errors)
                                error_details = (field_name.empty() ? " " : field_name);
                            return 0;
                        }

                        if (assigned_fields[field_index])
                        {
                            error = Error::duplicate_fields;
                            if (verbose_errors)
                                error_details = (field_name.empty() ? " " : field_name);
                            return 0;
                        }

                        if (verbose_errors) stack.push_back(field_index_to_name<T>(field_index)); // Push field name.

                        Strings::skip_ws(str);
                        if (!Strings::skip_one_of(str, "="))
                        {
                            error = Error::missing_equality;
                            return 0;
                        }

                        str = from_string_field(field_index, object, str, verbose_errors, error, error_details, stack);
                        if (!str) // If `from_string()` failed.
                            return 0; // `error` was already set by `from_string()`.

                        if (verbose_errors) stack.pop_back(); // Pop field name.
                    }

                    bool ok = Cexpr::for_each_while(std::make_index_sequence<Interface::field_count<T>()>{}, [&](auto index)
                    {
                        bool was_assigned = assigned_fields[index.value] || Interface::field_has_default_value<T,index.value>();
                        if (!was_assigned && verbose_errors)
                        {
                            if (error_details.size() > 0)
                                error_details += "`, `";
                            error_details += Interface::field_name<T, index.value>();
                        }
                        return was_assigned;
                    });
                    if (!ok)
                        error = Error::incomplete; // We don't stop on this error, the primary `from_string()` handles it.
                }
            }
            else if constexpr (Interface::is_container<T>())
            {
                if (!Strings::skip_one_of(str, "["))
                {
                    error = Error::missing_start_of_container;
                    return 0;
                }

                if (verbose_errors) stack.push_back(0); // This means that the next stack entry is an index rather than a field name.

                Interface::container_erase(object, Interface::container_cbegin(object), Interface::container_cend(object)); // Clear the container.

                std::size_t index = 0;

                while (1)
                {
                    if (index != 0)
                    {
                        Strings::skip_ws(str);
                        if (!Strings::skip_one_of(str, ","))
                        {
                            if (str[-1] == ']')
                                break;

                            error = Error::missing_comma;
                            return 0;
                        }
                    }

                    Strings::skip_ws(str);

                    if (*str == ']')
                    {
                        str++;
                        break;
                    }

                    Interface::container_mutable_value_t<T> tmp;

                    if (verbose_errors) stack.push_back((const char *)index); // Push field name.

                    str = from_string(tmp, str, verbose_errors, error, error_details, stack);
                    if (!str) // If `from_string()` failed.
                        return 0; // `error` was already set by `from_string()`.

                    Interface::container_insert_move(object, Interface::container_cend(object), std::move(tmp));

                    if (verbose_errors) stack.pop_back(); // Pop field name.

                    index++;
                }

                if (verbose_errors) stack.pop_back(); // Remove the 0.
            }
            else // a primitive type
            {
                std::size_t len = Reflection::Interface::primitive_from_string(object, str);
                if (len == 0)
                {
                    error = Error::bad_primitive;
                    return 0;
                }
                str += len;
            }

            return str;
        }
    }

    // Returns a pointer to the next character in the string.
    template <FromStringMode mode = complete, typename T> const char *from_string(T &object, const char *str, std::string *error_message = 0)
    {
        using Error = from_string_impl::Error;
        Error error = Error::no_error;
        std::string error_details;
        std::vector<const char *> stack;

        bool verbose_errors = (error_message != 0);

        str = from_string_impl::from_string(object, str, verbose_errors, error, error_details, stack);

        if (!str && verbose_errors && (mode == maybe_incomplete || error != Error::incomplete))
        {
            if (stack.size() > 0)
            {
                *error_message += "At `";
                for (std::size_t i = 0; i < stack.size(); i++)
                {
                    if (i != 0)
                        *error_message += '.';

                    if (stack[i] != 0)
                        *error_message += stack[i];
                    else
                        *error_message += to_string((std::size_t)stack[++i]); // Sic!
                }
                *error_message += "`: ";
            }

            switch (error)
            {
                case Error::no_error                        : *error_message += "No error"; break;
                case Error::incomplete                      : *error_message += "Missing fields"; break;
                case Error::bad_primitive                   : *error_message += "Primitive data type parsing failed"; break;
                case Error::missing_start_of_tuple_structure: *error_message += "Expected `(`"; break;
                case Error::missing_start_of_structure      : *error_message += "Expected `{`"; break;
                case Error::missing_start_of_container      : *error_message += "Expected `[`"; break;
                case Error::missing_comma                   : *error_message += "Expected `,`"; break;
                case Error::missing_equality                : *error_message += "Expected `=`"; break;
                case Error::invalid_field_name              : *error_message += "Invalid field name"; break;
                case Error::duplicate_fields                : *error_message += "Duplicate field"; break;
                case Error::missing_end_of_tuple_structure  : *error_message += "Expected `)`"; break;
            }

            if (error_details.size() > 0)
            {
                *error_message += ": `";
                *error_message += error_details;
                *error_message += '`';
            }
            *error_message += '.';

            return 0;
        }

        Strings::skip_ws(str);
        return str;
    }

    template <typename T> std::size_t byte_buffer_size(const T &object)
    {
        if constexpr (Interface::is_structure<T>())
        {
            std::size_t size = 0;

            auto lambda = [&](auto index)
            {
                size += byte_buffer_size(Interface::field<index.value>(object));
            };

            Cexpr::for_each(std::make_index_sequence<Interface::field_count<T>()>{}, lambda);

            return size;
        }
        else if constexpr (Interface::is_container<T>())
        {
            std::size_t size = sizeof(uint32_t);

            for (auto it = Interface::container_cbegin(object); it != Interface::container_cend(object); it++)
            {
                size += byte_buffer_size(*it);
            }

            return size;
        }
        else // primitive
        {
            return Interface::primitive_byte_buffer_size(object);
        }
    }

    // Returns a pointer to the next free byte in the buffer.
    template <typename T> uint8_t *to_bytes(const T &object, uint8_t *buf)
    {
        if constexpr (Interface::is_structure<T>())
        {
            auto lambda = [&](auto index)
            {
                buf = to_bytes(Interface::field<index.value>(object), buf);
            };

            Cexpr::for_each(std::make_index_sequence<Interface::field_count<T>()>{}, lambda);

            return buf;
        }
        else if constexpr (Interface::is_container<T>())
        {
            buf = to_bytes(uint32_t(Interface::container_size(object)), buf);

            for (auto it = Interface::container_cbegin(object); it != Interface::container_cend(object); it++)
            {
                buf = to_bytes(*it, buf);
            }

            return buf;
        }
        else // primitive
        {
            return Interface::primitive_to_bytes(object, buf);
        }
    }

    // Returns a pointer to the next byte in the buffer or 0 in failure.
    // WARNING: `object` may end up in an invalid state in case of a failure.
    template <typename T> const uint8_t *from_bytes(T &object, const uint8_t *buf, const uint8_t *buf_end)
    {
        if (!buf)
            return 0;

        if constexpr (Interface::is_structure<T>())
        {
            auto lambda = [&](auto index) -> bool
            {
                buf = from_bytes(Interface::field<index.value>(object), buf, buf_end);
                return buf;
            };

            Cexpr::for_each_while(std::make_index_sequence<Interface::field_count<T>()>{}, lambda);

            return buf;
        }
        else if constexpr (Interface::is_container<T>())
        {
            uint32_t size;
            buf = from_bytes(size, buf, buf_end);
            if (!buf)
                return 0;

            Interface::container_erase(object, Interface::container_cbegin(object), Interface::container_cend(object));

            for (uint32_t i = 0; i < size; i++)
            {
                Interface::container_mutable_value_t<T> tmp;
                buf = from_bytes(tmp, buf, buf_end);
                if (!buf)
                    return 0;
                Interface::container_insert_move(object, Interface::container_cend(object), std::move(tmp));
            }

            return buf;
        }
        else // primitive
        {
            return Interface::primitive_from_bytes(object, buf, buf_end);
        }
    }
}

/* Struct/class reflection.
 * Example struct definition:
 *
 *   struct A
 *   {
 *     Reflect(A)
 *     (
 *       (int)(a),
 *       (int[3])(b),
 *       (float)(c,d)
 *       (int)(e,f)(=42),
 *       (private:),
 *       (int)(g),
 *     )
 *   };
 *
 * or
 *
 *   ReflectStruct(name, (fields)) // Don't forget trailing comma in `fields`.
 *
 * If you want to reflect more fields separately, you can use ReflectMembers() macro with the same syntax (but without the class name enclosed in `()`).
 * Use `(expr)` to copy expressions literally.
 */

#define ReflectStruct(...) PP0_VA_CALL(ReflectStruct_A_, __VA_ARGS__)(__VA_ARGS__)
#define ReflectStruct_A_2(name_,         seq_) struct name_ { Reflect(name_       ) seq_ };
#define ReflectStruct_A_3(name_, bases_, seq_) struct name_ { Reflect(name_ bases_) seq_ };

#define Reflect(name_) \
    friend class ::Reflection::Interface; \
    using _reflection_internal_this_type = name_; \
    /* Interface: Get field count */\
    /* (Making it a template delays the expansion so we can get a proper counter value.) */\
    template <typename = void> friend constexpr ::std::size_t reflection_interface_field_count(const _reflection_internal_this_type *) {return ::Reflection::Interface::Impl::counter_value<_reflection_internal_this_type, ::Reflection::Interface::Impl::counter_tag_fields, void>::value;} \
    ReflectMembers

#define ReflectMembers(...) \
    PP0_SEQ_APPLY(PP0_VA_TO_SEQ_DISCARD_LAST(__VA_ARGS__), REFL0_Reflect_A, PP0_F_NULL, )

#define REFL0_Reflect_A(i, data, seq) \
    PP0_SEQ_CALL(REFL0_Reflect_B_, seq)(i, seq)

#define REFL0_Reflect_B_1(i, expr) \
    PP0_DEL_PARENS(expr)

#define REFL0_Reflect_B_2(i, seq) \
    PP0_SEQ_APPLY_A(PP0_VA_TO_SEQ(PP0_SEQ_AT(1,seq)), REFL0_Reflect_C, PP0_F_NULL, (i, (PP0_SEQ_FIRST(seq))/*type*/ , 0 , ))

#define REFL0_Reflect_B_3(i, seq) \
    PP0_SEQ_APPLY_A(PP0_VA_TO_SEQ(PP0_SEQ_AT(1,seq)), REFL0_Reflect_C, PP0_F_NULL, (i, (PP0_SEQ_FIRST(seq))/*type*/ , 1 , PP0_SEQ_AT(2,seq)/*init*/))

#define REFL0_Reflect_C(j, data, name) \
    PP0_CALL_B(REFL0_Reflect_D, name, j, PP0_DEL_PARENS(data))

#define REFL0_Reflect_D(name_, j_, i_, type_, has_init_, .../*init*/) \
    /* Define the field. */\
    std::enable_if_t<1, PP0_DEL_PARENS(type_)> name_ __VA_ARGS__; \
    /* Make sure there is no explicit empty init (because it would set `has_init_` to `1` without a good reason). */\
    static_assert(has_init_ == 0 || !::Reflection::Cexpr::string_is_empty(#__VA_ARGS__), "Empty default value."); \
    /* Field index. */\
    using _reflection_internal_field_index_type_##name_ = ::Reflection::index_const<::Reflection::Interface::Impl::counter_value<_reflection_internal_this_type, ::Reflection::Interface::Impl::counter_tag_fields, ::Reflection::InterfaceDetails::value_list<__LINE__, i_, j_>>::value>; \
    /* Increment field counter. */\
    static void _reflection_internal_counter_crumb(::Reflection::InterfaceDetails::type_list<::Reflection::Interface::Impl::counter_tag_fields>, _reflection_internal_field_index_type_##name_) {} \
    /* Inteface: Get the field by index. */\
    friend auto &reflection_interface_field(_reflection_internal_this_type *ptr, _reflection_internal_field_index_type_##name_) {return ptr->name_;} \
    /* Inteface: Whether the field has a default value or not. */\
    friend constexpr bool reflection_interface_field_has_default_value(const _reflection_internal_this_type *, _reflection_internal_field_index_type_##name_) {return has_init_;} \
    /* Inteface: Field name. */\
    friend constexpr const char *reflection_interface_field_name(const _reflection_internal_this_type *, _reflection_internal_field_index_type_##name_) {return #name_;}


/* Enum reflection.
 * Grammar:
 * [optional]
 * `placeholders`
 *
 *   Reflect[Member]Enum[Class]('name' [,'type'], 'list')
 *
 * Where `list` is a delimiter-less list of either
 *   ('name')
 * or
 *   ('name','value')
 */

// `...` is `[underlying_type,] seq`
#define ReflectEnum(name, ...)             PP0_VA_CALL(REFL0_ReflectEnum_A_, __VA_ARGS__)(name, inline,      , __VA_ARGS__)
#define ReflectEnumClass(name, ...)        PP0_VA_CALL(REFL0_ReflectEnum_A_, __VA_ARGS__)(name, inline, class, __VA_ARGS__)
#define ReflectMemberEnum(name, ...)       PP0_VA_CALL(REFL0_ReflectEnum_A_, __VA_ARGS__)(name, friend,      , __VA_ARGS__)
#define ReflectMemberEnumClass(name, ...)  PP0_VA_CALL(REFL0_ReflectEnum_A_, __VA_ARGS__)(name, friend, class, __VA_ARGS__)

#define REFL0_ReflectEnum_A_1(name, func_specifiers, pre, seq)                  REFL0_ReflectEnum_B(name, seq, func_specifiers, pre,)
#define REFL0_ReflectEnum_A_2(name, func_specifiers, pre, underlying_type, seq) REFL0_ReflectEnum_B(name, seq, func_specifiers, pre, : underlying_type)

#define REFL0_ReflectEnum_B(name, seq, func_specifiers, pre, post) enum pre name post { PP0_SEQ_APPLY(seq, REFL0_ReflectEnum_C, PP0_F_NULL,) }; ReflectExistingEnum(name, func_specifiers, seq)

#define REFL0_ReflectEnum_C(i, data, ...) PP0_VA_CALL(REFL0_ReflectEnum_D_, __VA_ARGS__)(__VA_ARGS__)

#define REFL0_ReflectEnum_D_1(name)        name,
#define REFL0_ReflectEnum_D_2(name, value) name = value,

// `func_specifiers_` depends on the scope. At namespace scope it must be `inline` (if the file is not a header, empty string may be used instead). At class scope it must be `friend`.
#define ReflectExistingEnum(type_, func_specifiers_, seq_) \
    /* Interface: String->value map. */\
    func_specifiers_ const auto &reflection_interface_enum_string_value_map(const type_ *) { static ::std::unordered_map<::std::string, type_> ret{ PP0_SEQ_APPLY(seq_, REFL0_ReflectEnum_Func_StringValueMap, PP0_F_COMMA, type_) }; return ret; } \
    /* Interface: Value->string map. */\
    func_specifiers_ const auto &reflection_interface_enum_value_string_map(const type_ *) { static ::std::unordered_map<type_, ::std::string> ret{ PP0_SEQ_APPLY(seq_, REFL0_ReflectEnum_Func_ValueStringMap, PP0_F_COMMA, type_) }; return ret; }

#define REFL0_ReflectEnum_Func_StringValueMap(i, type, ...) { PP0_STR(PP0_VA_AT(0, __VA_ARGS__)) , type::PP0_VA_AT(0, __VA_ARGS__) }
#define REFL0_ReflectEnum_Func_ValueStringMap(i, type, ...) { type::PP0_VA_AT(0, __VA_ARGS__) , PP0_STR(PP0_VA_AT(0, __VA_ARGS__)) }

#endif
