#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

// ---------------------------- UPDATE THIS WHEN YOU CHANGE THE CODE
#define VERSION "2.5.8"
// ---------------------------- UPDATE THIS WHEN YOU CHANGE THE CODE

std::ofstream out_file("mat.h");

int indentation = 0;
bool new_line = 0;

template <typename ...P> const char *Jo(P &&... p)
{
    static constexpr int ret_buffers_c = 32;
    static std::string ret_buffers[ret_buffers_c];
    static int ret_pos = 0;
    static std::stringstream ss;
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

namespace Gen
{
    // Field names list
    static constexpr char field_names[][4]
    {
        {'x','y','z','w'},
        {'r','g','b','a'},
        {'s','t','p','q'},
    };
    static constexpr const char (&field_names_main)[4] = field_names[0];
    static constexpr int field_names_count = (sizeof field_names) / (sizeof field_names[0]);

    static constexpr const char *custom_op_list[]{"dot","cross","mul"};
    static constexpr char op_delim = '/'; // You shall use / or * or %, other ones will mess up operator precedence in existing code.

    void VectorPrototypes()
    {
        r R"(
inline namespace Vector // prototypes
{
template <unsigned int D, typename T> struct vec;
template <unsigned int W, unsigned int H, typename T> using mat = vec<W, vec<H, T>>;
}
)";
    }

    void Utility()
    {
    r R"(
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
)";
    }

    void Strings()
    {
        r R"(
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
@default:
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
@case '0':
@case '1':
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
)";
    }

    void Vector()
    {
        // Operator list
        static constexpr const char *ops_bin[]{"+","-","*","/","%","^","&","|","<<",">>","<",">","<=",">="},
                                    *ops_un[]{"~","+","-"},
                                    *ops_fix[]{"++","--"},
                                    *ops_bool_bin[]{"&&","||"},
                                    *ops_bool_un[]{"!"},
                                    *ops_as[]{"+=","-=","*=","/=","%=","^=","&=","|=","<<=",">>="};

        // .to_string_pretty() settings
        static constexpr int pretty_string_field_width = 12,
                             pretty_string_field_precision = 4;

        r R"(
inline namespace Vector
{
)";

        { // Declarations
            // Type list
            static constexpr struct {const char *tag, *name;} types[]
            {
                "b",   "bool",
                "c",   "char",
                "uc",  "unsigned char",
                "sc",  "signed char",
                "s",   "short",
                "us",  "unsigned short",
                "i",   "int",
                "u",   "unsigned int",
                "l",   "long",
                "ul",  "unsigned long",
                "ll",  "long long",
                "ull", "unsigned long long",
                "f",   "float",
                "d",   "double",
                "ld",  "long double",
                "i8",  "int8_t",
                "u8",  "uint8_t",
                "i16", "int16_t",
                "u16", "uint16_t",
                "i32", "int32_t",
                "u32", "uint32_t",
                "i64", "int64_t",
                "u64", "uint64_t",
            };

            // Type-generic usings
            for (int i = 2; i <= 4; i++)
            {
                l "template <typename T> using vec" << i << " = vec<" << i << ", T>;";
                l (i == 4 ? "\n" : " ");
            }
            for (int h = 2; h <= 4; h++)
            for (int w = 2; w <= 4; w++)
            {
                l "template <typename T> using mat" << w << 'x' << h << " = mat<" << w << ',' << h << ",T>;";
                l (w == 4 ? "\n" : " ");
            }
            for (int i = 2; i <= 4; i++)
            {
                l "template <typename T> using mat" << i << " = mat" << i << 'x' << i << "<T>;";
                l (i == 4 ? "\n" : " ");
            }
            l "\n";

            // Type-prefixed usings
            for (const auto &it : types)
            {
                // Size-generic
                l "template <unsigned int D> using " << it.tag << "vec = vec<D," << it.name << ">;\n" <<
                  "template <unsigned int W, unsigned int H> using " << it.tag << "mat = mat<W,H," << it.name << ">;\n";
                // Complete
                for (int i = 2; i <= 4; i++)
                {
                    l "using " << it.tag << "vec" << i << " = vec<" << i << ',' << it.name << ">;";
                    l (i == 4 ? "\n" : " ");
                }
                for (int h = 2; h <= 4; h++)
                for (int w = 2; w <= 4; w++)
                {
                    l "using " << it.tag << "mat" << w << 'x' << h << " = mat<" << w << ',' << h << ',' << it.name << ">;";
                    l (w == 4 ? "\n" : " ");
                }
                for (int i = 2; i <= 4; i++)
                {
                    l "using " << it.tag << "mat" << i << " = " << it.tag << "mat" << i << 'x' << i << ";";
                    l (i == 4 ? "\n" : " ");
                }
                l "\n";
            }
        }

        // Specializations
        auto CommonHeader = [&]
        {
            r R"(
static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "The vector base type must not have any cv-qualifiers.");
static_assert(!std::is_reference_v<T>, "Vectors of references are not allowed.");
using type = T;
)";
        };
        auto Fields = [&](int len, const char *type)
        {
            for (int i = 0; i < len; i++)
            {
                l "union {" << type << ' ';
                for (int j = 0; j < field_names_count; j++)
                {
                    if (j != 0)
                        l ", ";
                    l field_names[j][i];
                }
                l ";};\n";
            }
        };
        auto CommonMembers = [&](int sz)
        {
            // [] operator
            l "template <typename I> [[nodiscard]] T &operator[](I pos) {switch (pos) {";
            for (int i = 0; i < sz; i++)
                l "case " << i << ": return " << field_names_main[i] << "; ";
            l "default: static T ret; ret = {}; return ret;}}\n";

            // const [] operator
            l "template <typename I> [[nodiscard]] constexpr T operator[](I pos) const {switch (pos) {";
            for (int i = 0; i < sz; i++)
                l "case " << i << ": return " << field_names_main[i] << "; ";
            l "default: return {};}}\n";

            // Cast to bool
            l "[[nodiscard]] explicit constexpr operator bool() const {return ";
            for (int i = 0; i < sz; i++)
            {
                if (i != 0) l " || ";
                l "bool(" << field_names_main[i] << ")";
            }
            l "; static_assert(!std::is_same_v<base_type_t<vec>, bool>, \"Use .none(), .any(), or .all() for vectors/matrices of bool.\");}\n";

            // Constructors
            l "vec() = default;\n"; // Default
            l "explicit constexpr vec(T obj) : "; // Same initializer for each component.
            for (int i = 0; i < sz; i++)
            {
                if (i != 0) l ", ";
                l field_names_main[i] << "(obj)";
            }
            l " {}\n";
            l "constexpr vec("; // Piece-wise
            for (int i = 0; i < sz; i++)
            {
                if (i != 0) l ", ";
                l "decltype(x) p" << field_names_main[i]; // Sic!  decltype(x) is used instead of T on purpose.
            }
            l ") : ";
            for (int i = 0; i < sz; i++)
            {
                if (i != 0) l ", ";
                l field_names_main[i] << "(p" << field_names_main[i] << ')';
            }
            l " {}\n";
            l "template <typename TT> constexpr vec(const vec" << sz << "<TT> &obj) : "; // Casting from different type of vector.
            for (int i = 0; i < sz; i++)
            {
                if (i != 0) l ", ";
                l field_names_main[i] << "(obj." << field_names_main[i] << ')';
            }
            l " {}\n";

            // Member combinations
            l "[[nodiscard]] constexpr auto sum() const {return "; // Sum
            for (int i = 0; i < sz; i++)
            {
                if (i != 0) l " + ";
                l field_names_main[i];
            }
            l ";}\n";
            l "[[nodiscard]] constexpr auto product() const {return "; // Product
            for (int i = 0; i < sz; i++)
            {
                if (i != 0) l " * ";
                l field_names_main[i];
            }
            l ";}\n";

            // As array
            l "[[nodiscard]] constexpr T *as_array() {return (T *)this;}\n";
            l "[[nodiscard]] constexpr const T *as_array() const {return (const T *)this;}\n";

            // Temporary field changers
            for (int name = 0; name < field_names_count; name++)
            for (int i = 0; i < sz; i++)
            {
                // set
                l "[[nodiscard]] constexpr vec set_" << field_names[name][i] << "(T o) const {return {";
                for (int j = 0; j < sz; j++)
                {
                    if (j != 0) l ", ";
                    if (i == j) l 'o';
                    else l field_names_main[j];
                }
                l "};} ";

                // add
                l "[[nodiscard]] constexpr vec add_" << field_names[name][i] << "(T o) const {return {";
                for (int j = 0; j < sz; j++)
                {
                    if (j != 0) l ", ";
                    l field_names_main[j];
                    if (i == j) l "+o";
                }
                l "};} ";

                // sub
                l "[[nodiscard]] constexpr vec sub_" << field_names[name][i] << "(T o) const {return {";
                for (int j = 0; j < sz; j++)
                {
                    if (j != 0) l ", ";
                    l field_names_main[j];
                    if (i == j) l "-o";
                }
                l "};} ";

                // mul
                l "[[nodiscard]] constexpr vec mul_" << field_names[name][i] << "(T o) const {return {";
                for (int j = 0; j < sz; j++)
                {
                    if (j != 0) l ", ";
                    l field_names_main[j];
                    if (i == j) l "*o";
                }
                l "};} ";

                // div
                l "[[nodiscard]] constexpr vec div_" << field_names[name][i] << "(T o) const {return {";
                for (int j = 0; j < sz; j++)
                {
                    if (j != 0) l ", ";
                    l field_names_main[j];
                    if (i == j) l "/o";
                }
                l "};}\n";
            }
        };

        auto MatrixInverse = [&](int sz)
        {
            switch (sz)
            {
              case 4:
                r R"(
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
)";
                break;
              case 3:
                r R"(
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
)";
                break;
              case 2:
                r R"(
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
)";
                break;
            }
        };

        auto MatrixMul = [&](int x1y2, int x2, int y1)
        {
            auto Ma = [&](const char *t, int x, int y) -> const char *
            {
                if (x == 1 && y == 1) return t;
                if (x == 1) return Jo("vec", y, '<', t, '>');
                if (y == 1) return Jo("vec", x, '<', t, '>');
                return Jo("mat", x, 'x', y, '<', t, '>');
            };
            auto At = [&](int x, int y, int xx, int yy) -> const char *
            {
                if (xx == 1 && yy == 1) return "";
                if (xx == 1) return Jo(field_names_main[y]);
                if (yy == 1) return Jo(field_names_main[x]);
                return Jo(field_names_main[x], '.', field_names_main[y]);
            };

            l "template <typename TT> [[nodiscard]] constexpr " << Ma("larger_type_t<T,TT>", x2, y1) << " mul(const " << Ma("TT", x2, x1y2) << " &o) const {return {";
            for (int h = 0; h < y1; h++)
            {
                for (int w = 0; w < x2; w++)
                {
                    if (h != 0 || w != 0) l ", ";
                    for (int i = 0; i < x1y2; i++)
                    {
                        if (i != 0) l '+';
                        l At(i,h,x1y2,y1) << "*o." << At(w,i,x2,x1y2);
                    }
                }
            }
            l "};}\n";
        };

        { // Vectors
            for (int sz = 2; sz <= 4; sz++)
            {
                // Header
                l "template <typename T> struct vec<" << sz << ",T> // vec" << sz <<"\n{\n";
                CommonHeader();
                l "static constexpr int size = " << sz << ";\n";
                l "static constexpr bool is_floating_point = std::is_floating_point_v<T>;\n";


                // Fields
                Fields(sz, "T");


                // Members
                CommonMembers(sz);
                { // Change type
                    l "template <typename TT> [[nodiscard]] constexpr vec" << sz << "<TT> to() const {return {";
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0) l ",";
                        l "TT(" << field_names_main[i] << ")";
                    }
                    l "};}\n";
                }
                { // Length
                    l "[[nodiscard]] constexpr auto len_sqr() const {return "; // Squared
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0) l " + ";
                        l field_names_main[i] << '*' << field_names_main[i];
                    }
                    l ";}\n";
                    l "[[nodiscard]] constexpr auto len() const {return std::sqrt(len_sqr());}\n"; // Normal
                }
                { // Cross and dot products

                    // Dot
                    l "template <typename TT> [[nodiscard]] constexpr auto dot(const vec" << sz << "<TT> &o) const {return ";
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0) l " + ";
                        l field_names_main[i] << "*o." << field_names_main[i];
                    }
                    l ";}\n";

                    // Cross
                    switch (sz)
                    {
                      case 3:
                        l "template <typename TT> [[nodiscard]] constexpr auto cross(const vec3<TT> &o) const -> vec3<decltype(y * o.z - z * o.y)> {return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};}\n";
                        break;
                      case 2: // Pseudo cross product. Returns z only.
                        l "template <typename TT> [[nodiscard]] constexpr auto cross(const vec2<TT> &o) const -> decltype(x * o.y - y * o.x) {return x * o.y - y * o.x;}\n";
                        break;
                    }
                }
                if (sz == 2) // Ratio
                    l "[[nodiscard]] constexpr floating_point_t<T> ratio() const {return floating_point_t<T>(x) / floating_point_t<T>(y);}\n";
                // Normalize
                l "[[nodiscard]] constexpr auto norm() const -> vec" << sz << "<decltype(T{}/len())> {auto l = len(); if (l == 0) return vec" << sz << "<T>(0); else return *this / l;}\n";
                { // Apply
                    l "template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const vec" << sz << "<P> &... p) const -> vec" << sz << "<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {";
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0) l ", ";
                        l "func(" << field_names_main[i] << ", p." << field_names_main[i] << "...)";
                    }
                    l "};}\n";
                }
                { // Resizers
                    for (int i = 2; i <= 4; i++)
                    {
                        if (sz == i) continue;
                        l "[[nodiscard]] constexpr vec" << i << "<T> to_vec" << i << "(";
                        for (int j = sz; j < i; j++)
                        {
                            if (j != sz) l ", ";
                            l "T p" << field_names_main[j];
                        }
                        l ") const {return {";
                        for (int j = 0; j < i; j++)
                        {
                            if (j != 0) l ", ";
                            if (j >= sz) l "p";
                            l field_names_main[j];
                        }
                        l "};}\n";
                    }
                    for (int i = sz+1; i <= 4; i++)
                    {
                        l "[[nodiscard]] constexpr vec" << i << "<T> to_vec" << i << "() const {return to_vec" << i << "(";
                        for (int j = sz; j < i; j++)
                        {
                            if (j != sz) l ", ";
                            l "T{}";
                        }
                        l ");}\n";
                    }
                }
                { // Bool pack
                    auto BoolFunc = [&](const char *name, const char *bin, bool inverted)
                    {
                        l "[[nodiscard]] constexpr bool " << name << "() const {return ";
                        if (inverted) l "!(";
                        for (int i = 0; i < sz; i++)
                        {
                            if (i != 0) l ' ' << bin << ' ';
                            l field_names_main[i];
                        }
                        if (inverted) l ")";
                        l ";}\n";
                    };
                    BoolFunc("none", "||", 1);
                    BoolFunc("any", "||", 0);
                    BoolFunc("all", "&&", 0);
                }
                for (int i = 1; i <= 4; i++) // Multiplication
                    MatrixMul(sz, i, 1);
                { // Min and max
                    l "[[nodiscard]] constexpr T min() const {return std::min({";
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0) l ',';
                        l field_names_main[i];
                    }
                    l "});}\n";
                    l "[[nodiscard]] constexpr T max() const {return std::max({";
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0) l ',';
                        l field_names_main[i];
                    }
                    l "});}\n";
                }
                { // String operations
                    // To string
                    l "[[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start";
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0)
                            l " + sep";
                        l " + f(" << field_names_main[i] << ")";
                    }
                    l " + end;}\n";

                    l "[[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string(\"[\", \",\", \"]\", f);}\n";

                    l "[[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) "
                      "return to_string(" R"("[ "," "," ]")" ",num_to_string<T[" << pretty_string_field_width << "]," << pretty_string_field_width << "," << pretty_string_field_precision << ",'g','#'>); else "
                      "return to_string(" R"("[ "," "," ]")" ",num_to_string<T[" << pretty_string_field_width << "]," << pretty_string_field_width << ",-1>);}\n";

                    // From string
                    l R"([[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &end, int base = 0)
{
if (chars_consumed) *chars_consumed = 0;
if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
)";
                    for (int i = 0; i < sz; i++)
                    {
                        if (i != 0)
                            l "if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} "
                              "pos += sep.size();\n";
                        l "ret." << field_names_main[i] << " = num_from_string_mid<T>(src+pos, &offset, base); "
                          "if (!offset) {return {};} "
                          "pos += offset;\n";
                    }
                    l "if (strncmp(src+pos, end.c_str(), end.size())) {return {};} "
                      "pos += end.size();\n"
                      "if (chars_consumed) *chars_consumed = pos;\n"
                      "return ret;\n"
                      "}\n";

                    l "[[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, end, base); ch_con = src[ch_con] == '\\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}\n";
                    l "[[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, \"[\", \",\", \"]\", base);}\n";
                    l "[[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, \"[\", \",\", \"]\", base);}\n";
                }

                l "};\n";
            }
        }
        { // Matrices
            for (int h = 2; h <= 4; h++)
            for (int w = 2; w <= 4; w++)
            {
                // Header
                l "template <typename T> struct vec<" << w << ",vec<" << h << ",T>> // mat" << w << 'x' << h << "\n{\n";
                CommonHeader();
                l "static constexpr int width = " << w << ", height = " << h << ";\n";
                l "static constexpr bool is_floating_point = vec" << h << "<T>::is_floating_point;\n";


                // Fields
                Fields(w, Jo("vec",h,"<T>"));


                // Members
                CommonMembers(w);
                { // Additional ctors
                    l "constexpr vec("; // Full set of initializers
                    for (int hh = 0; hh < h; hh++)
                    {
                        for (int ww = 0; ww < w; ww++)
                        {
                            if (ww != 0 || hh != 0) l ", ";
                            l "T " << field_names_main[ww] << field_names_main[hh];
                        }
                    }
                    l ") : ";
                    for (int ww = 0; ww < w; ww++)
                    {
                        if (ww != 0) l ", ";
                        l field_names_main[ww] << '(';
                        for (int hh = 0; hh < h; hh++)
                        {
                            if (hh != 0) l ',';
                            l field_names_main[ww] << field_names_main[hh];
                        }
                        l ')';
                    }
                    l " {}\n";
                }
                { // Change type
                    l "template <typename TT> [[nodiscard]] constexpr mat" << w << "x" << h << "<TT> to() const {return {";
                    for (int hh = 0; hh < h; hh++)
                    for (int ww = 0; ww < w; ww++)
                    {
                        if (hh != 0 || ww != 0) l ",";
                        l "TT(" << field_names_main[ww] << "." << field_names_main[hh] << ")";
                    }
                    l "};}\n";
                }
                { // Transpose
                    l "[[nodiscard]] constexpr mat" << h << 'x' << w << "<T> transpose() const {return {";
                    for (int ww = 0; ww < w; ww++)
                    {
                        for (int hh = 0; hh < h; hh++)
                        {
                            if (ww != 0 || hh != 0) l ',';
                            l field_names_main[ww] << '.' << field_names_main[hh];
                        }
                    }
                    l "};}\n";
                }
                { // Factory methods
                    l "[[nodiscard]] static constexpr vec identity() {return {"; // Identity
                    for (int hh = 0; hh < h; hh++)
                    {
                        for (int ww = 0; ww < w; ww++)
                        {
                            if (ww != 0 || hh != 0) l ", ";
                            if (ww == hh) l '1';
                            else l '0';
                        }
                    }
                    l "};}\n";

                    auto MatrixFactoryMethod = [&](int minw, int minh, const char *name, const char *params, const char *sh_params, const char *body, bool float_only = 1)
                    {
                        if (w == minw && h == minh)
                        {
                            l "[[nodiscard]] static constexpr vec " << name << '(' << params << ")\n{\n";
                            if (float_only) l "static_assert(is_floating_point, \"This function only makes sense for floating-point matrices.\");\n";
                            l (1+body) << "}\n";
                        }
                        else if (w >= minw && h >= minh)
                            l "[[nodiscard]] static constexpr vec " << name << '(' << params << ") {return mat" << minw << 'x' << minh << "<T>::" << name << "(" << sh_params << ").to_mat" << w << 'x' << h << "();}\n";
                    };

                    MatrixFactoryMethod(2, 2, "scale", "const vec2<T> &v", "v", R"(
return {v.x, 0,
$       0, v.y};
)", 0);
                    MatrixFactoryMethod(3, 3, "scale", "const vec3<T> &v", "v", R"(
return {v.x, 0, 0,
$       0, v.y, 0,
$       0, 0, v.z};
)", 0);
                    MatrixFactoryMethod(4, 4, "scale", "const vec4<T> &v", "v", R"(
return {v.x, 0, 0, 0,
$       0, v.y, 0, 0,
$       0, 0, v.z, 0,
$       0, 0, 0, v.w};
)", 0);
                    MatrixFactoryMethod(3, 2, "ortho2D", "const vec2<T> &min, const vec2<T> &max", "min, max", R"(
return {2 / (max.x - min.x), 0, (min.x + max.x) / (min.x - max.x),
$       0, 2 / (max.y - min.y), (min.y + max.y) / (min.y - max.y)};
)");
                    MatrixFactoryMethod(4, 3, "ortho", "const vec2<T> &min, const vec2<T> &max, T near, T far", "min, max, near, far", R"(
return {2 / (max.x - min.x), 0, 0, (min.x + max.x) / (min.x - max.x),
$       0, 2 / (max.y - min.y), 0, (min.y + max.y) / (min.y - max.y),
$       0, 0, 2 / (near - far), (near + far) / (near - far)};
)");
                    MatrixFactoryMethod(4, 3, "look_at", "const vec3<T> &src, const vec3<T> &dst, const vec3<T> &local_up", "src, dst, local_up", R"(
vec3<T> v3 = (src-dst).norm();
vec3<T> v1 = local_up.cross(v3).norm();
vec3<T> v2 = v3.cross(v1);
return {v1.x, v1.y, v1.z, -src.x*v1.x - src.y*v1.y - src.z*v1.z,
$       v2.x, v2.y, v2.z, -src.x*v2.x - src.y*v2.y - src.z*v2.z,
$       v3.x, v3.y, v3.z, -src.x*v3.x - src.y*v3.y - src.z*v3.z};
)");
                    MatrixFactoryMethod(3, 2, "translate2D", "const vec2<T> &v", "v", R"(
return {1, 0, v.x,
$       0, 1, v.y};
)", 0);
                    MatrixFactoryMethod(4, 3, "translate", "const vec3<T> &v", "v", R"(
return {1, 0, 0, v.x,
$       0, 1, 0, v.y,
$       0, 0, 1, v.z};
)", 0);
                    MatrixFactoryMethod(2, 2, "rotate2D", "T angle", "angle", R"(
T c = std::cos(angle);
T s = std::sin(angle);
return {c, -s,
$       s, c};
)");
                    MatrixFactoryMethod(3, 3, "rotate_with_normalized_axis", "const vec3<T> &axis, T angle", "axis, angle", R"(
T c = std::cos(angle);
T s = std::sin(angle);
return {axis.x * axis.x * (1 - c) + c, axis.x * axis.y * (1 - c) - axis.z * s, axis.x * axis.z * (1 - c) + axis.y * s,
$       axis.y * axis.x * (1 - c) + axis.z * s, axis.y * axis.y * (1 - c) + c, axis.y * axis.z * (1 - c) - axis.x * s,
$       axis.x * axis.z * (1 - c) - axis.y * s, axis.y * axis.z * (1 - c) + axis.x * s, axis.z * axis.z * (1 - c) + c};
)");
                    MatrixFactoryMethod(3, 3, "rotate", "const vec3<T> &axis, T angle", "axis, angle", R"(
return rotate_with_normalized_axis(axis.norm(), angle);
)");
                    MatrixFactoryMethod(4, 4, "perspective", "T yfov, T wh_aspect, T near, T far", "yfov, wh_aspect, near, far", R"(
yfov = (T)1 / std::tan(yfov / 2);
return {yfov / wh_aspect , 0    , 0                           , 0                             ,
$       0                , yfov , 0                           , 0                             ,
$       0                , 0    , (near + far) / (near - far) , 2 * near * far / (near - far) ,
$       0                , 0    , -1                          , 0                             };
)");
                }
                { // 2D resizers
                    for (int hhh = 2; hhh <= 4; hhh++)
                    {
                        for (int www = 2; www <= 4; www++)
                        {
                            if (www == w && hhh == h) continue;
                            l "[[nodiscard]] constexpr mat" << www << 'x' << hhh << "<T> to_mat" << www << 'x' << hhh << "() const {return {";
                            for (int hh = 0; hh < hhh; hh++)
                            {
                                for (int ww = 0; ww < www; ww++)
                                {
                                    if (ww != 0 || hh != 0) l ',';
                                    if (ww < w && hh < h)
                                        l field_names_main[ww] << '.' << field_names_main[hh];
                                    else
                                    {
                                        if (ww == hh) l '1';
                                        else l '0';
                                    }
                                }
                            }
                            l "};}\n";
                            if (www == hhh)
                                l "[[nodiscard]] constexpr mat" << www << "<T> to_mat" << www << "() const {return to_mat" << www << 'x' << www << "();}\n";
                        }
                    }
                }
                { // Apply
                    l "template <typename F, typename ...P> [[nodiscard]] constexpr auto apply(F &&func, const mat" << w << "x" << h << "<P> &... p) const -> mat" << w << "x" << h << "<decltype(func(std::declval<T>(), std::declval<P>()...))> {return {";
                    for (int i = 0; i < w; i++)
                    {
                        if (i != 0) l ", ";
                        l field_names_main[i] << ".apply(func, p." << field_names_main[i] << "...)";
                    }
                    l "};}\n";
                }
                { // Bool pack
                    auto BoolFunc = [&](const char *name, const char *bin, bool inverted)
                    {
                        l "[[nodiscard]] constexpr bool " << name << "() const {return ";
                        if (inverted) l "!(";
                        for (int hh = 0; hh < h; hh++)
                        {
                            for (int ww = 0; ww < w; ww++)
                            {
                                if (ww != 0 || hh != 0) l ' ' << bin << ' ';
                                l field_names_main[ww] << '.' << field_names_main[hh];
                            }
                        }
                        if (inverted) l ")";
                        l ";}\n";
                    };
                    BoolFunc("none", "||", 1);
                    BoolFunc("any", "||", 0);
                    BoolFunc("all", "&&", 0);
                }
                { // Min and max
                    l "[[nodiscard]] constexpr T min() const {return std::min({";
                    for (int i = 0; i < w; i++)
                    {
                        for (int j = 0; j < h; j++)
                        {
                            if (i != 0 || j != 0) l ',';
                            l field_names_main[i] << '.' << field_names_main[j];
                        }
                    }
                    l "});}\n";
                    l "[[nodiscard]] constexpr T max() const {return std::max({";
                    for (int i = 0; i < w; i++)
                    {
                        for (int j = 0; j < h; j++)
                        {
                            if (i != 0 || j != 0) l ',';
                            l field_names_main[i] << '.' << field_names_main[j];
                        }
                    }
                    l "});}\n";
                }
                if (w == h) MatrixInverse(w); // Inverse
                for (int i = 1; i <= 4; i++) // Multiplication
                    MatrixMul(w, i, h);
                { // String operations
                    // To string
                    l "[[nodiscard]] std::string to_string(const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, std::string(*f)(T) = num_to_string<T>) const {return start";
                    for (int hh = 0; hh < h; hh++)
                    {
                        if (hh != 0)
                            l " + row_sep";
                        for (int ww = 0; ww < w; ww++)
                        {
                            if (ww != 0)
                                l " + sep";
                            l " + f(" << field_names_main[ww] << "." << field_names_main[hh] << ")";
                        }
                    }
                    l " + end;}\n";

                    l "[[nodiscard]] std::string to_string(std::string(*f)(T) = num_to_string<T>) const {return to_string(\"[\", \",\", \";\", \"]\", f);}\n";

                    l "[[nodiscard]] std::string to_string_pretty() const {if constexpr (is_floating_point) "
                      "return to_string(" R"("/ "," "," |\n| "," /")" ",num_to_string<T[" << pretty_string_field_width << "]," << pretty_string_field_width << "," << pretty_string_field_precision << ",'g','#'>); else "
                      "return to_string(" R"("/ "," "," |\n| "," /")" ",num_to_string<T[" << pretty_string_field_width << "]," << pretty_string_field_width << ",-1>);}\n";

                    // From string
                    l R"([[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0)
{
if (chars_consumed) *chars_consumed = 0;
if (strncmp(src, start.c_str(), start.size())) {return {};} std::size_t pos = start.size(); int offset; vec ret;
)";
                    for (int hh = 0; hh < h; hh++)
                    {
                        if (hh != 0)
                            l "if (strncmp(src+pos, row_sep.c_str(), row_sep.size())) {return {};} "
                              "pos += sep.size();\n";

                        for (int ww = 0; ww < w; ww++)
                        {
                            if (ww != 0)
                                l "if (strncmp(src+pos, sep.c_str(), sep.size())) {return {};} "
                                  "pos += sep.size();\n";
                            l "ret." << field_names_main[ww] << "." << field_names_main[hh] << " = num_from_string_mid<T>(src+pos, &offset, base); "
                              "if (!offset) {return {};} "
                              "pos += offset;\n";
                        }
                    }
                    l "if (strncmp(src+pos, end.c_str(), end.size())) {return {};} "
                      "pos += end.size();\n"
                      "if (chars_consumed) *chars_consumed = pos;\n"
                      "return ret;\n"
                      "}\n";

                    l "[[nodiscard]] static vec from_string(const char *src, bool *success, const std::string &start, const std::string &sep, const std::string &row_sep, const std::string &end, int base = 0) {int ch_con; vec ret = from_string_mid(src, &ch_con, start, sep, row_sep, end, base); ch_con = src[ch_con] == '\\0'; if (success) {*success = ch_con;} if (ch_con) {return ret;} else {return {};}}\n";
                    l "[[nodiscard]] static vec from_string_mid(const char *src, int *chars_consumed = 0, int base = 0) {return from_string_mid(src, chars_consumed, \"[\", \",\", \";\", \"]\", base);}\n";
                    l "[[nodiscard]] static vec from_string(const char *src, bool *success = 0, int base = 0) {return from_string(src, success, \"[\", \",\", \";\", \"]\", base);}\n";
                }

                l "};\n";
            }
        }
        l "\n";

        // Operators
        for (int sz = 2; sz <= 4; sz++)
        {
            for (const char *op : ops_bin)
            {
                // vec @ vec
                l "template <typename T1, typename T2> [[nodiscard]] constexpr vec" << sz << "<decltype(T1{}" << op << "T2{})> operator" << op << "(const vec" << sz << "<T1> &first, const vec" << sz << "<T2> &second) {return {";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l ',';
                    l "first." << field_names_main[i] << op << "second." << field_names_main[i];
                }
                l "};}\n";
                // vec @ other
                l "template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T2>::io_or_op>> [[nodiscard]] constexpr vec" << sz << "<decltype(T1{}" << op << "T2{})> operator" << op << "(const vec" << sz << "<T1> &first, const T2 &second) {return {";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l ',';
                    l "first." << field_names_main[i] << op << "second";
                }
                l "};}\n";
                // other @ vec
                l "template <typename T1, typename T2, typename = std::enable_if_t<!type_category<T1>::io_or_op>> [[nodiscard]] constexpr vec" << sz << "<decltype(T1{}" << op << "T2{})> operator" << op << "(const T1 &first, const vec" << sz << "<T2> &second) {return {";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l ',';
                    l "first" << op << "second." << field_names_main[i];
                }
                l "};}\n";
            }
            for (const char *op : ops_bool_bin)
            {
                // vec @ vec
                l "template <typename T1, typename T2> [[nodiscard]] constexpr bool operator" << op << "(const vec" << sz << "<T1> &first, const vec" << sz << "<T2> &second) {return bool(first) " << op << " bool(second);}\n";
                // vec @ other
                l "template <typename T1, typename T2> [[nodiscard]] constexpr bool operator" << op << "(const vec" << sz << "<T1> &first, const T2 &second) {return bool(first) " << op << " second;}\n";
                // other @ vec
                l "template <typename T1, typename T2> [[nodiscard]] constexpr bool operator" << op << "(const T1 &first, const vec" << sz << "<T2> &second) {return first " << op << " bool(second);}\n";
            }
            for (const char *op : ops_un)
            {
                l "template <typename T> [[nodiscard]] constexpr vec" << sz << "<decltype(" << op << "T{})> operator" << op << "(const vec" << sz << "<T> &object) {return {";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l ',';
                    l op << "object." << field_names_main[i];
                }
                l "};}\n";
            }
            for (const char *op : ops_bool_un)
            {
                l "template <typename T> [[nodiscard]] constexpr vec" << sz << "<decltype(" << op << "T{})> operator" << op << "(const vec" << sz << "<T> &object) {return " << op << "bool(object);}\n";
            }
            for (const char *op : ops_fix)
            {
                // Prefix
                l "template <typename T> constexpr vec" << sz << "<T> &operator" << op << "(vec" << sz << "<T> &object) {";
                for (int i = 0; i < sz; i++)
                {
                    l op << "object." << field_names_main[i] << "; ";
                }
                l "return object;}\n";
                // Postfix
                l "template <typename T> constexpr vec" << sz <<"<T> operator" << op << "(vec" << sz << "<T> &object, int) {return {";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l ',';
                    l "object." << field_names_main[i] << op;
                }
                l "};}\n";
            }

            for (const char *op : ops_as)
            {
                // vec @ vec
                l "template <typename T1, typename T2> constexpr vec" << sz << "<T1> &operator" << op << "(vec" << sz << "<T1> &first, const vec" << sz << "<T2> &second) {";
                for (int i = 0; i < sz; i++)
                    l "first." << field_names_main[i] << ' ' << op << ' ' << "second." << field_names_main[i] << "; ";
                l "return first;}\n";
                // vec @ other
                l "template <typename T1, typename T2> constexpr vec" << sz << "<T1> &operator" << op << "(vec" << sz << "<T1> &first, const T2 &second) {";
                for (int i = 0; i < sz; i++)
                    l "first." << field_names_main[i] << ' ' << op << ' ' << "second; ";
                l "return first;}\n";
            }
            l "\n";

            // == and !=
            const char *eq_ops[] {"==", "!="};
            const char *eq_op_delims[] {" && ", " || "};
            for (int i = 0; i < 2; i++)
            {
                const char *op = eq_ops[i];
                const char *delim = eq_op_delims[i];
                // vec @ vec
                l "template <typename T1, typename T2> [[nodiscard]] constexpr bool operator" << op << "(const vec" << sz << "<T1> &first, const vec" << sz << "<T2> &second) {return ";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l delim;
                    l "(first." << field_names_main[i] << ' ' << op << ' ' << "second." << field_names_main[i] << ')';
                }
                l ";}\n";
                // vec @ other
                l "template <typename T1, typename T2> [[nodiscard]] constexpr bool operator" << op << "(const vec" << sz << "<T1> &first, const T2 &second) {return ";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l delim;
                    l "(first." << field_names_main[i] << ' ' << op << ' ' << "second" << ')';
                }
                l ";}\n";
                // other @ vec
                l "template <typename T1, typename T2> [[nodiscard]] constexpr bool operator" << op << "(const T1 &first, const vec" << sz << "<T2> &second) {return ";
                for (int i = 0; i < sz; i++)
                {
                    if (i != 0) l delim;
                    l "(first" << ' ' << op << ' ' << "second." << field_names_main[i] << ')';
                }
                l ";}\n";
            }
        }
        l "\n";

        // Stream output
        for (int i = 2; i <= 4; i++)
        {
            l "template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const vec" << i << "<T> &vector) {stream << '['";
            for (int j = 0; j < i; j++)
            {
                if (j != 0)
                    l " << ','";
                l " << vector." << field_names_main[j];
            }
            l " << ']'; return stream;}\n";
        }
        for (int h = 2; h <= 4; h++)
        {
            for (int w = 2; w <= 4; w++)
            {
                l "template <typename X, typename Y, typename T> std::basic_ostream<X,Y> &operator<<(std::basic_ostream<X,Y> &stream, const mat" << w << "x" << h << "<T> &matrix) {stream << '['";
                for (int hh = 0; hh < h; hh++)
                {
                    if (hh != 0)
                        l " << ';'";
                    for (int ww = 0; ww < w; ww++)
                    {
                        if (ww != 0)
                            l " << ','";
                        l " << matrix." << field_names_main[ww] << "." << field_names_main[hh];
                    }
                }
                l " << ']'; return stream;}\n";
            }
        }
        // Stream input
        for (int i = 2; i <= 4; i++)
        {
            l "template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, vec" << i << "<T> &vector) {stream";
            for (int j = 0; j < i; j++)
                l " >> vector." << field_names_main[j];
            l "; return stream;}\n";
        }
        for (int h = 2; h <= 4; h++)
        {
            for (int w = 2; w <= 4; w++)
            {
                l "template <typename X, typename Y, typename T> std::basic_istream<X,Y> &operator>>(std::basic_istream<X,Y> &stream, mat" << w << "x" << h << "<T> &matrix) {stream";
                for (int hh = 0; hh < h; hh++)
                    for (int ww = 0; ww < w; ww++)
                        l " >> matrix." << field_names_main[ww] << "." << field_names_main[hh];
                l "; return stream;}\n";
            }
        }

        l "}\n";
    }

    void Quaternion()
    {
        r R"(
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
$            vec.w*o.vec.y - vec.x*o.vec.z + vec.y*o.vec.w + vec.z*o.vec.x,
$            vec.w*o.vec.z + vec.x*o.vec.y - vec.y*o.vec.x + vec.z*o.vec.w,
$            vec.w*o.vec.w - vec.x*o.vec.x - vec.y*o.vec.y - vec.z*o.vec.z});
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
$       neww*vec.y + newx*vec.z + newy*vec.w - newz*vec.x,
$       neww*vec.z - newx*vec.y + newy*vec.x + newz*vec.w};
}

template <typename TT> [[nodiscard]] constexpr vec4<larger_type_t<T,TT>> mul(const vec4<TT> &in) const // Leaves in.w unchanged. Complexity: 24x`*` 17x`+-`
{
    return mul(in.to_vec3).to_vec4(in.w);
}

[[nodiscard]] constexpr mat3<T> make_mat3() const // The quaternion must be normalized! Complexity: 18x`*` 12x`+-` (+ multiplication 9x`*` 6x`+-`)
{
return {1 - 2*vec.y*vec.y - 2*vec.z*vec.z, 2*vec.x*vec.y - 2*vec.z*vec.w, 2*vec.x*vec.z + 2*vec.y*vec.w,
$       2*vec.x*vec.y + 2*vec.z*vec.w, 1 - 2*vec.x*vec.x - 2*vec.z*vec.z, 2*vec.y*vec.z - 2*vec.x*vec.w,
$       2*vec.x*vec.z - 2*vec.y*vec.w, 2*vec.y*vec.z + 2*vec.x*vec.w, 1 - 2*vec.x*vec.x - 2*vec.y*vec.y};
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
)";
    }

    void CustomOperators()
    {
        l "inline namespace Operators\n{\n";
        for (const char *name : custom_op_list)
        {
            l "template <typename T> struct " << name << "_operator_impl_expression_t : private custom_operator_base_t\n";
            l "{\n";
            l "T first_arg;\n";
            l "template <typename TT> constexpr " << name << "_operator_impl_expression_t(TT &&obj) : first_arg((TT &&) obj) {}\n";
            l "template <typename TT> [[nodiscard]] constexpr auto operator" << op_delim << "(TT &&obj) const {return first_arg." << name << "((TT &&) obj);}\n";
            l "};\n";
        }
        l '\n';
        for (const char *name : custom_op_list)
        {
            l "struct " << name << "_operator_impl_t : private custom_operator_base_t\n";
            l "{\n";
            l "constexpr " << name << "_operator_impl_t(){}\n";
            l "};\n";
            l "constexpr " << name << "_operator_impl_t " << name << ";\n";
            l "template <typename T> [[nodiscard]] constexpr auto operator" << op_delim << "(T &&obj, decltype(" << name << ")) {return " << name << "_operator_impl_expression_t<T>((T &&) obj);}\n";
        }
        l "}\n";
    }

    void Misc()
    {
        r R"(
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
)";
        struct
        {
            const char *name, *logic;
        }
        min_max[] {{"min","a > b ? b : a"},
                   {"max","b > a ? b : a"}};

        for (const auto &it : min_max)
        {
            l '\n';
            l "template <typename T1, typename T2> [[nodiscard]] constexpr larger_type_t<T1,T2> " << it.name << "(T1 a, T2 b) {return (" << it.logic << ");}\n";
            l "template <typename T1, typename T2, unsigned int D1> [[nodiscard]] constexpr larger_type_t<vec<D1,T1>,T2> " << it.name << "(const vec<D1,T1> &a, T2 b) {return a.apply(" << it.name << "<base_type_t<vec<D1,T1>>, T2>, change_base_type_t<vec<D1,T1>,T2>(b));}\n";
            l "template <typename T1, typename T2, unsigned int D2> [[nodiscard]] constexpr larger_type_t<T1,vec<D2,T2>> " << it.name << "(T1 a, const vec<D2,T2> &b) {return b.apply(" << it.name << "<T1, base_type_t<vec<D2,T2>>>, change_base_type_t<vec<D2,T2>,T1>(a));}\n";
            l "template <typename T1, typename T2, unsigned int D1, unsigned int D2> [[nodiscard]] constexpr larger_type_t<vec<D1,T1>,vec<D2,T2>> " << it.name << "(const vec<D1,T1> &a, const vec<D2,T2> &b) {return a.apply(" << it.name << "<base_type_t<vec<D1,T1>>, base_type_t<vec<D2,T2>>>, b);}\n";
        }
        r R"(
}
)";
    }

    void Export()
    {
        r R"(
namespace Export
{
using namespace Vector;
using namespace Quaternion;
using namespace Operators;
using namespace Misc;

using Strings::num_to_string;
using Strings::num_from_string_mid;
}
)";
    }

    void StdSpecializations()
    {
        r R"(
namespace std
{
)";
        // std::less
        for (int i = 2; i <= 4; i++)
        {
            r R"(
template <typename T> struct less<Math::vec)" << i << R"(<T>>
{
using result_type = bool;
using first_argument_type = Math::vec)" << i << R"(<T>;
using second_argument_type = Math::vec)" << i << R"(<T>;
constexpr bool operator()(const Math::vec)" << i << R"(<T> &a, const Math::vec)" << i << R"(<T> &b) const
{
)";
            for (int j = 0; j < i; j++)
                l "if (a." << field_names_main[j] << " < b." << field_names_main[j] << ") return 1;\n" <<
                  "if (a." << field_names_main[j] << " > b." << field_names_main[j] << ") return 0;\n";
            r R"(
return 0;
}
};
)";
        }
        l '\n';

        // std::hash
        for (int i = 2; i <= 4; i++)
        {
            r R"(
template <typename T> struct hash<Math::vec)" << i << R"(<T>>
{
using result_type = std::size_t;
using argument_type = Math::vec)" << i << R"(<T>;
std::size_t operator()(const Math::vec)" << i << R"(<T> &v) const
{
std::size_t ret = std::hash<decltype(v.x)>{}(v.x);
)";
            for (int j = 1; j < i; j++)
                l "Math::hash_combine(ret, std::hash<decltype(v.x)>{}(v." << field_names_main[j] << "));\n";
            r R"(
return ret;
}
};
)";
        }

        r R"(
}
)";
    }
}

int main()
{
    r R"(
#ifndef MAT_H_INCLUDED
#define MAT_H_INCLUDED

// Version )" VERSION R"( by HolyBlackCat

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
)";
    Gen::VectorPrototypes();
    l "\n";
    Gen::Utility();
    l "\n";
    Gen::Strings();
    l "\n";
    Gen::Vector();
    l "\n";
    Gen::Quaternion();
    l "\n";
    Gen::CustomOperators();
    l "\n";
    Gen::Misc();
    l "\n";
    Gen::Export();
    r R"(
}

)";
    Gen::StdSpecializations();
    r R"(

using namespace Math::Export;

#endif
)";
}
