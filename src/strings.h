#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <string>
#include <string_view>
#include <sstream>

namespace Strings
{
    namespace impl
    {
        inline std::stringstream ss;
        inline const std::stringstream::fmtflags stdfmt = ss.flags();
    }

    // Str() resets stringstream flags first.
    // Str_() does not.

    template <typename ...P> [[nodiscard]] std::string Str(P &&... params)
    {
        impl::ss.clear();
        impl::ss.str("");
        impl::ss.flags(impl::stdfmt);
        (impl::ss << ... << params);
        return impl::ss.str();
    }
    template <typename ...P> [[nodiscard]] std::string Str_(P &&... params)
    {
        impl::ss.clear();
        impl::ss.str("");
        (impl::ss << ... << params);
        return impl::ss.str();
    }

    [[nodiscard]] inline std::string_view Trim(std::string_view str)
    {
        static constexpr char chars_to_remove[] = " \n\r\t\0";
        str.remove_prefix(std::min(str.size(), str.find_first_not_of(chars_to_remove)));
        if (str.size())
            str.remove_suffix(str.size() - str.find_last_not_of(chars_to_remove) - 1);
        return str;
    }

    [[nodiscard]] inline std::string Escape(std::string_view str)
    {
        auto EscapeSeq = [](char ch) -> const char *
        {
            switch (ch)
            {
                // Line feed is not escaped.
                case '\a': return "\\a";
                case '\b': return "\\b";
                case '\f': return "\\f";
                case '\r': return "\\r";
                case '\t': return "\\t";
                case '\v': return "\\v";
                default: return 0;
            }
        };
        (void)Str(std::hex);
        std::string ret;
        for (char it : str)
        {
            if ((it >= '\0' && it < ' ' && it != '\n') || it == 127)
            {
                auto seq = EscapeSeq(it);
                if (seq)
                    ret += seq;
                else
                    ret += Str_("\\x", int((unsigned char)it / 16), int((unsigned char)it % 16));
            }
            else
                ret += it;
        }
        return ret;
    }

    inline namespace UTF8
    {
        constexpr uint16_t u8invalidchar = 0xffff;

        [[nodiscard]] inline bool u8isfirstbyte(char ch) // Check if a pointed byte is a first byte of a symbol.
        {
            return (ch & 0b1100'0000) != 0b1000'0000;
        }
        template <typename Iter> [[nodiscard]] bool u8isfirstbyte(Iter it)
        {
            return u8isfirstbyte(*it);
        }

        [[nodiscard]] inline std::size_t u8strlen(std::string_view str)
        {
            std::size_t ret = 0;
            for (char it : str)
                if (u8isfirstbyte(it))
                    ret++;
            return ret;
        }

        [[nodiscard]] inline std::size_t u8charlen(char ch) // If `ch` is not a first byte of a sequence, returns 0.
        {
            if ((ch & 0b1000'0000) == 0)
                return 1;
            if (!u8isfirstbyte(ch))
                return 0;
            for (int i = 1; i < 8; i++)
                if ((((ch & (0xff00 >> (i+1))) ^ (0xff00 >> i)) & 0xff) == 0)
                    return i;
            return 8;
        }
        template <typename Iter> [[nodiscard]] std::size_t u8charlen(Iter it)
        {
            return u8charlen(*it);
        }

        template <typename Iter> [[nodiscard]] uint16_t u8decode(Iter str) // Decodes the first character only. Returns `u8invalidchar` on failure.
        {
            switch (u8charlen(*str))
            {
              default:
                return u8invalidchar;
              case 1:
                return *str;
              case 2:
                return ((*str & 0b0001'1111) << 6) | (str[1] & 0b0011'1111);
              case 3:
                return ((*str & 0b0000'1111) << 12) | ((str[1] & 0b0011'1111) << 6) | (str[2] & 0b0011'1111);
            }
        }

        [[nodiscard]] inline bool u8valid16(std::string_view str)
        {
            std::size_t len = 0;
            for (char it : str)
            {
                std::size_t cur_len = u8charlen(it);
                if (cur_len > 3)
                    return 0;
                if (bool(cur_len) == bool(len))
                    return 0;
                if (cur_len)
                    len = cur_len;
                len--;
            }
            return 1;
        }
    }

    inline namespace Encodings
    {
        const uint16_t (&cp1251())[256];
    }
}

using Strings::Str;
using Strings::Str_;
using namespace Strings::UTF8;

#endif
