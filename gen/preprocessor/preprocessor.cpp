#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

// ---------------------------- UPDATE THIS WHEN YOU CHANGE THE CODE
#define VERSION "0.0.8"
// ---------------------------- UPDATE THIS WHEN YOU CHANGE THE CODE

std::ofstream out_file("preprocessor.h");

int indentation = 0;
bool new_line = 0;

std::stringstream ss;

template <typename ...P> const char *Jo(P &&... p)
{
    static constexpr int ret_buffers_c = 32;
    static std::string ret_buffers[ret_buffers_c];
    static int ret_pos = 0;
    ss.clear();
    ss.str("");
    int dummy[] {(ss << p, 0)...};
    (void)dummy;
    ret_buffers[ret_pos] = ss.str();
    const char *ret = ret_buffers[ret_pos].c_str();
    ret_pos = (ret_pos + 1) % ret_buffers_c;
    return ret;
}

void Print(const char *ptr)
{
    auto Indent = [&](int off = 0)
    {
        if (new_line)
        {
            new_line = 0;
            for (int i = 0; i < indentation * 4 + off; i++) out_file << ' ';
        }
    };

    while (*ptr)
    {
        switch (*ptr)
        {
          case '\n':
            new_line = 1;
            break;
          case '{':
            Indent();
            indentation++;
            break;
          case '}':
            indentation--;
            Indent();
            break;
          case '@':
            Indent(-2);
            break;
          case '$':
            Indent(-4);
            break;
          default:
            Indent();
            break;
        }
        if (*ptr == '$')
            out_file << ' ';
        else if (*ptr != '@')
            out_file << *ptr;

        ptr++;
    }
}

struct
{
    template <typename T> auto operator<<(T && obj)
    {
        Print(Jo(obj));
        return *this;
    }
} o;

#define r o << 1+
#define l o <<

constexpr int pp_args = 32;

const std::string small_suffixes[] {"", "_A", "_B"},
                  large_suffixes[] {"", "_A"};

int main()
{
    r R"(
#ifndef PREPROCESSOR_H_INCLUDED
#define PREPROCESSOR_H_INCLUDED

// Version )" VERSION R"( by HolyBlackCat

#include <cstddef>
#include <type_traits>
#include <utility>


)";
    // Utils
    l "#define PP0_F_NULL(...)\n";
    l "#define PP0_F_COMMA(...) ,\n";
    l '\n';
    l "#define PP0_E(...) __VA_ARGS__\n";
    l '\n';
    l "#define PP0_VA_FIRST(...) PP0_VA_FIRST_IMPL_(__VA_ARGS__,)\n";
    l "#define PP0_VA_FIRST_IMPL_(x, ...) x\n";
    l '\n';
    l "#define PP0_PARENS(...) (__VA_ARGS__)\n";
    l "#define PP0_DEL_PARENS(...) PP0_E __VA_ARGS__\n";
    l "#define PP0_2_PARENS(...) ((__VA_ARGS__))\n";
    l "#define PP0_PARENS_COMMA(...) (__VA_ARGS__),\n";
    l '\n';
    l "#define PP0_CC(a, b) PP0_CC_IMPL_(a,b)\n";
    l "#define PP0_CC_IMPL_(a, b) a##b\n";
    l '\n';
    for (std::string suffix : small_suffixes)
        l "#define PP0_CALL" << suffix << "(macro, ...) macro(__VA_ARGS__)\n";
    l '\n';

    // Variadic size
    l "#define PP0_VA_SIZE(...) PP0_VA_SIZE_IMPL_(__VA_ARGS__";
    for (int i = pp_args; i >= 0; i--)
        l "," << i;
    l ")\n";
    l "#define PP0_VA_SIZE_NOT_1(...) PP0_VA_SIZE_IMPL_(__VA_ARGS__";
    for (int i = pp_args; i >= 0; i--)
        l "," << (i > 1);
    l ")\n";
    l "#define PP0_VA_SIZE_IMPL_(";
    for (int i = 0; i < pp_args; i++)
        l "i" << i+1 << ",";
    l "size,...) size\n";
    l '\n';

    // To string
    l "#define PP0_STR(...) PP0_STR_IMPL_(__VA_ARGS__)\n";
    l "#define PP0_STR_IMPL_(...) #__VA_ARGS__\n";
    l '\n';

    // Overload
    l "#define PP0_VA_CALL(name, ...) PP0_CC(name, PP0_VA_SIZE(__VA_ARGS__))\n";
    l '\n';

    // Seq overload
    l "#define PP0_SEQ_CALL(name, seq) PP0_CC(name, PP0_SEQ_SIZE(seq))\n";
    l '\n';

    // Seq first
    l "#define PP0_SEQ_FIRST(seq) PP0_DEL_PARENS(PP0_VA_FIRST(PP0_PARENS_COMMA seq,))\n";
    l '\n';

    // Seq empty
    l "#define PP0_SEQ_EMPTY(seq) PP0_VA_SIZE_NOT_1(PP0_CC(PP0_SEQ_EMPTY_IMPL_, PP0_SEQ_SIZE(seq)))\n";
    l "#define PP0_SEQ_EMPTY_IMPL_0 ,\n";
    l '\n';

    // Va to seq
    l "#define PP0_VA_TO_SEQ(...) PP0_VA_CALL(PP0_VA_TO_SEQ_, __VA_ARGS__,)(__VA_ARGS__,)\n";
    l "#define PP0_VA_TO_SEQ_DISCARD_LAST(...) PP0_VA_CALL(PP0_VA_TO_SEQ_, __VA_ARGS__)(__VA_ARGS__)\n";
    l "#define PP0_VA_TO_SEQ_NULL\n";
    l "#define PP0_VA_TO_SEQ_1(x) PP0_VA_TO_SEQ_##x##NULL // If a error message points to this line, you probably forgot a trailing comma somewhere.\n";
    for (int i = 1; i <= pp_args; i++)
    {
        // Note that 1 is added to all the indices in this block. This was done to simplify VA_TO_SEQ_DISCARD_LAST
        l "#define PP0_VA_TO_SEQ_" << i+1 << "(x, ...) (x)PP0_VA_TO_SEQ_" << i << "(__VA_ARGS__)\n";
    }
    l '\n';

    // Seq size
    l "#define PP0_SEQ_SIZE(seq) PP0_CC(PP0_SEQ_SIZE_0 seq, _VAL)\n";
    for (int i = 0; i <= pp_args; i++)
        l "#define PP0_SEQ_SIZE_" << i << "(...) PP0_SEQ_SIZE_" << i+1 << '\n';
    for (int i = 0; i <= pp_args; i++)
        l "#define PP0_SEQ_SIZE_" << i << "_VAL " << i << "\n";
    l '\n';

    // Va at
    l "#define PP0_VA_AT(index, ...) PP0_CC(PP0_VA_AT_, index)(__VA_ARGS__,)\n";
    l "#define PP0_VA_AT_0(ret, ...) ret\n";
    for (int i = 1; i <= pp_args; i++)
        l "#define PP0_VA_AT_" << i << "(_, ...) PP0_VA_AT_" << i-1 << "(__VA_ARGS__)\n";
    l '\n';

    // Seq at
    l "#define PP0_SEQ_AT(index, seq) PP0_SEQ_FIRST( PP0_CC(PP0_SEQ_AT_EAT_, index)(seq) )\n";
    l "#define PP0_SEQ_AT_EAT_0(seq) seq\n";
    for (int i = 1; i <= pp_args; i++)
        l "#define PP0_SEQ_AT_EAT_" << i << "(seq) PP0_SEQ_AT_EAT_" << i-1 << "(PP0_F_NULL seq)\n";

    // Seq apply
    l "// macro(i, data, element...)\n";
    for (std::string suffix : large_suffixes)
    {
        l "#define PP0_SEQ_APPLY" << suffix << "(seq, macro, macro_sep_f, data) PP0_SEQ_CALL(PP0_SEQ_APPLY" << suffix << "_, seq)(macro, macro_sep_f, data, seq, )\n";
        l "#define PP0_SEQ_APPLY" << suffix << "_0(macro, macro_sep_f, data, seq, seq_)\n";
        l "#define PP0_SEQ_APPLY" << suffix << "_1(macro, macro_sep_f, data, seq, seq_) PP0_CALL" << suffix << "(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq))\n";
        for (int i = 2; i <= pp_args; i++)
            l "#define PP0_SEQ_APPLY" << suffix << "_" << i << "(macro, macro_sep_f, data, seq, seq_) PP0_CALL" << suffix << "(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY" << suffix << "_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)\n";
    }
    l '\n';



    l "\n#endif\n";
}
