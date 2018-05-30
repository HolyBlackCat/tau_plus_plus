#include "everything.h"

#define VERSION "1.2.7"

//#define FORCE_ACCUMULATOR // Use accumulator even if framebuffers are supported.
//#define FORCE_FRAMEBUFFER // Use framebuffers, halt if not supported.

#ifdef PACKED_ASSETS
extern unsigned char binary_bin_assets_texture_png_start;
extern unsigned char binary_bin_assets_texture_png_end;
extern unsigned char binary_bin_assets_Xolonium_Regular_ttf_start;
extern unsigned char binary_bin_assets_Xolonium_Regular_ttf_end;
#endif

#include <complex>
#include <fstream>
#include <iostream>
#include <list>
#include <map>

#include <quadmath.h>

Events::AutoErrorHandlers error_handlers;

Window win("[TAU++] Частотные характеристики v" VERSION, ivec2(800,600), Window::Settings{}.GlVersion(2,1).GlProfile(Window::any_profile).Resizable().MinSize(ivec2(800,600)));
Timing::TickStabilizer tick_stabilizer(60);

Renderers::Poly2D r;
Input::Mouse mouse;

Graphics::Texture tex_main(Graphics::Texture::linear);
Graphics::CharMap font_main;
Graphics::CharMap font_small;
Graphics::Font font_object_main;
Graphics::Font font_object_small;


constexpr int interface_rect_height = 128;
constexpr int max_poly_degree = 50;


namespace External
{
    extern "C"
    {
        // This is located in `rpoly.f`, written in fortran 77.
        void rpoly_(__float128 *coefs, int *inout_degree, __float128 *out_real, __float128 *out_imag, int *out_fail);
    }
}


namespace Draw
{
    inline namespace TextPresets
    {
        [[nodiscard]] auto WithCursor(int index = Input::TextCursorPos(), fvec3 color = fvec3(tick_stabilizer.ticks % 60 < 30)) // Returns a preset
        {
            return [=](Renderers::Poly2D::Text_t &obj)
            {
                obj.callback([=](Renderers::Poly2D::Text_t::CallbackParams params)
                {
                    constexpr int left = 1, right = 1;
                    if (params.render_pass && params.index == index)
                    {
                        r.Quad(params.pos - ivec2(left, params.obj.state().ch_map->Ascent()), ivec2(left + right, params.obj.state().ch_map->Height()))
                         .color(color).alpha(params.render[0].alpha).beta(params.render[0].beta);
                    }
                });
            };
        }
        [[nodiscard]] auto ColorAfterPos(int index, fvec3 color) // Returns a preset
        {
            return [=](Renderers::Poly2D::Text_t &obj)
            {
                obj.callback([=](Renderers::Poly2D::Text_t::CallbackParams params)
                {
                    if (params.render_pass && params.index >= index)
                        for (auto &it : params.render)
                                it.color = color;
                });
            };
        }
        [[nodiscard]] auto SupSub(Renderers::Poly2D::Text_t &ref) // A preset
        {
            ref.callback([sup = false, sub = false](const Renderers::Poly2D::Text_t::CallbackParams &params) mutable
            {
                if (params.ch == '{' || params.ch == '}')
                {
                    sup = (params.ch == '{');
                    params.glyph.advance = 0;
                    if (params.render_pass)
                        params.render.clear();
                }
                else if (params.ch == '[' || params.ch == ']')
                {
                    sub = (params.ch == '[');
                    params.glyph.advance = 0;
                    if (params.render_pass)
                        params.render.clear();
                }
                else if (sup || sub)
                {
                    constexpr float scale = 0.82;
                    params.glyph.advance = iround(params.glyph.advance * scale);
                    if (params.render_pass)
                    {
                        fmat3 m = fmat3(scale, 0, 0,
                                        0, scale, (sup - sub) * params.obj.state().ch_map->Height() / 3,
                                        0, 0, 1);
                        for (auto &it : params.render)
                            it.matrix = it.matrix /mul/ m;
                    }
                }
            });
        }
        [[nodiscard]] auto WithWhiteBackground(float alpha = 0.5) // Returns a preset
        {
            return [&, alpha](Renderers::Poly2D::Text_t &ref)
            {
                constexpr int up = 1, down = 1, sides = 3;
                ref.callback([&, alpha](const Renderers::Poly2D::Text_t::CallbackParams &params) mutable
                {
                    if (params.render_pass)
                    {
                        auto *ch_map = params.obj.state().ch_map;
                        bool first = params.index == 0,
                             last = params.index == int(u8strlen(params.obj.state().str)) - 1;
                        int kerning = params.obj.state().ch_map->Kerning(params.prev, params.ch);
                        r.Quad(params.pos - ivec2(sides * first + kerning, ch_map->Ascent()+up),
                               ivec2(params.glyph.advance + sides * (first + last) + kerning, ch_map->Height()+up+down)).color(fvec3(1)).alpha(alpha);
                    }
                });
            };
        }
    }

    namespace Accumulator
    {
        bool use_framebuffer = 0;
        Graphics::FrameBuffer framebuffer;
        Graphics::RenderBuffer framebuffer_rbuf;

        void Overwrite()
        {
            if (!use_framebuffer)
            {
                glAccum(GL_LOAD, 1);
            }
            else
            {
                framebuffer.Bind();
                glBlitFramebuffer(0, 0, win.Size().x, win.Size().y, 0, 0, win.Size().x, win.Size().y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                framebuffer.Unbind();
            }
        }
        void Return()
        {
            if (!use_framebuffer)
            {
                glAccum(GL_RETURN, 1);
            }
            else
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.GetHandle());
                glBlitFramebuffer(0, 0, win.Size().x, win.Size().y, 0, 0, win.Size().x, win.Size().y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            }
        }
    }

    ivec2 min, max;

    void HandleResize()
    {
        min = -win.Size() / 2;
        max = win.Size() + min;

        r.SetMatrix(fmat4::ortho(ivec2(min.x, max.y), ivec2(max.x, min.y), -1, 1));
        mouse.Transform(win.Size()/2, 1);

        if (Accumulator::use_framebuffer)
            Accumulator::framebuffer_rbuf.Storage(win.Size(), GL_RGBA8);
    }

    void Init()
    {
        Graphics::Blending::Enable();
        Graphics::Blending::FuncNormalPre();

        #ifndef PACKED_ASSETS
        Graphics::Image texture_image_main("assets/texture.png");
        font_object_main.Create("assets/Xolonium-Regular.ttf", 20);
        font_object_small.Create("assets/Xolonium-Regular.ttf", 11);
        #else
        Utils::MemoryFile font(&binary_bin_assets_Xolonium_Regular_ttf_start, &binary_bin_assets_Xolonium_Regular_ttf_end-&binary_bin_assets_Xolonium_Regular_ttf_start);
        font_object_main.Create(font, 20);
        font_object_small.Create(font, 11);

        Graphics::Image texture_image_main(Utils::MemoryFile(&binary_bin_assets_texture_png_start, &binary_bin_assets_texture_png_end-&binary_bin_assets_texture_png_start));
        #endif


        Graphics::Font::MakeAtlas(texture_image_main, ivec2(0,1024-128), ivec2(1024,128),
        {
            {font_object_main , font_main , Graphics::Font::normal, Strings::Encodings::cp1251()},
            {font_object_small, font_small, Graphics::Font::normal, Strings::Encodings::cp1251()},
        });

        tex_main.SetData(texture_image_main);

        r.Create(0x1000);
        r.SetTexture(tex_main);
        r.SetDefaultFont(font_main);
        r.BindShader();

        #if defined(FORCE_ACCUMULATOR) && defined(FORCE_FRAMEBUFFER)
        #  error You cant define both FORCE_ACCUMULATOR and FORCE_FRAMEBUFFER
        #endif

        #ifdef FORCE_ACCUMULATOR
        Accumulator::use_framebuffer = 0;
        #else
        Accumulator::use_framebuffer = bool(glBlitFramebuffer);
        #ifdef FORCE_FRAMEBUFFER
        if (!Accumulator::use_framebuffer)
            Program::Error("FORCE_FRAMEBUFFER was defined, but framebuffers are not supported on this machine.");
        #endif
        #endif
        if (Accumulator::use_framebuffer)
        {
            Accumulator::framebuffer_rbuf.Create();
            Accumulator::framebuffer_rbuf.Storage(win.Size(), GL_RGBA8);
            Accumulator::framebuffer.Create();
            Accumulator::framebuffer.Attach(Accumulator::framebuffer_rbuf);
        }

        Draw::HandleResize();
    }

    void Dot(int type, fvec2 pos, fvec3 color, float alpha = 1, float beta = 1)
    {
        int y = (type >= 2);
        if (type >= 2)
            type -= 2;
        r.Quad(pos, ivec2(13)).tex(ivec2(2+type*16,2+y*16)).center().color(color).mix(0).alpha(alpha).beta(beta);
    }

    void Overlay(int type = 0)
    {
        constexpr int tex_sz = 64;
        ivec2 size = (win.Size() + tex_sz - 1) / tex_sz;
        for (int y = 0; y < size.y; y++)
        for (int x = 0; x < size.x; x++)
            r.Quad(min + ivec2(x,y)*tex_sz, ivec2(tex_sz)).tex(ivec2(type*64, 128));
    }

    ivec2 RoughSize(const Graphics::CharMap &ch_map, std::string::const_iterator begin, std::string::const_iterator end)
    {
        ivec2 ret(0, ch_map.Height());
        int cur_w = 0;
        auto it = begin;
        uint16_t prev_ch = u8invalidchar;
        while (it != end)
        {
            if (u8isfirstbyte(it))
            {
                uint16_t ch = u8decode(it);

                if (ch == '\n')
                {
                    ret.y += ch_map.LineSkip();
                    if (cur_w > ret.x)
                        ret.x = cur_w;
                    cur_w = 0;
                    prev_ch = u8invalidchar;
                }

                auto glyph = ch_map.Get(ch);
                cur_w += glyph.advance;
                if (prev_ch != u8invalidchar)
                    cur_w += ch_map.Kerning(prev_ch, ch);

                prev_ch = ch;
            }
            it++;
        }
        ret.x = Math::max(ret.x, cur_w);
        return ret;
    }
}


inline namespace MathMisc
{
    using complex_t = std::complex<long double>;

    template <typename T> class BasicPolynominal
    {
        std::map<int, T> coefs;

        void RemoveUnused()
        {
            auto it = coefs.begin();
            while (it != coefs.end())
            {
                if (it->second == T(0))
                    it = coefs.erase(it);
                else
                    it++;
            }
        }
       public:
        BasicPolynominal() {}
        BasicPolynominal(T coef, int power = 0)
        {
            if (coef != T(0))
                coefs[power] = coef;
        }

        complex_t Eval(complex_t x) const
        {
            complex_t ret = 0;
            for (const auto &it : coefs)
                ret += it.second * std::pow(x, it.first);
            return ret;
        }

        int Degree() const
        {
            if (coefs.empty())
                return 0;
            return std::prev(coefs.end())->first;
        }
        T FirstCoef() const
        {
            if (coefs.empty())
                return 0;
            return std::prev(coefs.end())->second;
        }

        T GetCoef(int power) const
        {
            if (auto it = coefs.find(power); it != coefs.end())
                return it->second;
            else
                return 0;
        }
        void EraseCoef(int power)
        {
            if (auto it = coefs.find(power); it != coefs.end())
                coefs.erase(it);
        }

        bool CoefsOutOfRange() const
        {
            // Jenkins-Traub seems to not handle large values well.
            if (coefs.empty())
                return 0;
            using pair_t = typename std::map<int, T>::value_type;
            return std::max_element(coefs.begin(), coefs.end(), [](const pair_t &a, const pair_t &b){return abs(a.second) < abs(b.second);})->second > 1e300;
        }

        bool CoefsHaveDifferentSigns() const
        {
            if (coefs.size() < 1)
                return 0;
            auto it = coefs.begin();
            int s = sign((it++)->second);
            while (it != coefs.end())
                if (sign((it++)->second) != s)
                    return 1;
            return 0;
        }

        // If at least one root can't be found, returns an empty list.
        std::vector<ldvec2> Roots() const
        {
            static_assert(std::is_arithmetic_v<T>, "Whoops!");

            if (CoefsOutOfRange())
                return {};

            int deg = Degree(), saved_deg = deg;

            std::vector<__float128> coef_vec(deg+1);
            for (const auto &it : coefs)
                coef_vec[deg - it.first] = it.second;

            auto real = std::make_unique<__float128[]>(deg),
                 imag = std::make_unique<__float128[]>(deg);

            int fail = 0;

            External::rpoly_(coef_vec.data(), &deg, real.get(), imag.get(), &fail);

            if (fail || deg != saved_deg)
                return {};

            std::vector<ldvec2> ret;
            for (int i = 0; i < deg; i++)
                ret.push_back(ldvec2(real[i], imag[i]));

            return ret;
        }

        void LongDivision(BasicPolynominal b, BasicPolynominal &quo, BasicPolynominal &rem) const
        {
            BasicPolynominal &a = rem;
            a = *this;
            a.RemoveUnused();
            b.RemoveUnused();

            int a_deg = a.Degree();
            int b_deg = b.Degree();

            if (a_deg < b_deg)
            {
                quo = {};
                return;
            }

            T b_first = b.FirstCoef();

            quo = {};

            for (int power = a_deg - b_deg; power >= 0; power--)
            {
                T a_coef = a.GetCoef(power + b_deg);
                if (a_coef == 0)
                    continue;

                T c = a_coef / b_first;
                quo.coefs[power] = c;

                a -= b * BasicPolynominal(c, power);
                a.EraseCoef(power + b_deg);
            }
        }

        std::string ToString() const
        {
            auto is_neg = [](T x)
            {
                if constexpr (std::is_arithmetic_v<T>)
                    return x < 0;
                else
                    return (void(x), false);
            };
            auto signless = [](T x)
            {
                if constexpr (std::is_arithmetic_v<T>)
                    return abs(x);
                else
                    return x;
            };

            std::string ret;
            auto it = coefs.end();
            while (1)
            {
                if (it == coefs.begin())
                    break;

                it--;

                if (ret.size())
                    ret += (is_neg(it->second) ? " - " : " + ");
                else if (is_neg(it->second))
                    ret += '-';
                ret += Str(signless(it->second));
                if (it->first != 0)
                {
                    ret += "*x";
                    if (it->first != 1)
                        ret += "^" + Str(signless(it->first));
                }
            }
            return ret;
        }
        std::string ToStringDense() const
        {
            static_assert(std::is_arithmetic_v<T>, "Whoops!");

            if (coefs.empty())
                return "0";

            std::string ret;

            auto it = coefs.end();
            while (1)
            {
                if (it == coefs.begin())
                    break;

                it--;

                if (ret.size() && it->second >= 0)
                    ret += "+";
                char buf[64];
                std::snprintf(buf, sizeof buf, "%.8Lf", (long double)it->second);
                std::string sbuf = buf;
                while (sbuf.size() > 0 && (sbuf.back() == '0' || sbuf.back() == '.'))
                    sbuf.resize(sbuf.size() - 1);
                ret += sbuf;
                if (it->first != 0)
                {
                    ret += "s";
                    if (it->first != 1)
                        ret += std::to_string(it->first);
                }
            }

            return ret;
        }

        friend BasicPolynominal &operator+=(BasicPolynominal &a, const BasicPolynominal &b)
        {
            for (const auto &it : b.coefs)
                a.coefs[it.first] += it.second;
            a.RemoveUnused();
            return a;
        };
        friend BasicPolynominal &operator-=(BasicPolynominal &a, const BasicPolynominal &b)
        {
            for (const auto &it : b.coefs)
                a.coefs[it.first] -= it.second;
            a.RemoveUnused();
            return a;
        };

        [[nodiscard]] friend BasicPolynominal operator+(const BasicPolynominal &a, const BasicPolynominal &b)
        {
            BasicPolynominal ret = a;
            ret += b;
            return ret;
        }
        [[nodiscard]] friend BasicPolynominal operator-(const BasicPolynominal &a, const BasicPolynominal &b)
        {
            BasicPolynominal ret = a;
            ret -= b;
            return ret;
        }

        friend BasicPolynominal operator*(const BasicPolynominal &a, const BasicPolynominal &b)
        {
            BasicPolynominal ret;
            for (const auto &x : a.coefs)
            for (const auto &y : b.coefs)
                ret.coefs[x.first + y.first] += x.second * y.second;
            ret.RemoveUnused();
            return ret;
        }
        friend BasicPolynominal &operator*=(BasicPolynominal &a, const BasicPolynominal &b)
        {
            a = a * b;
            return a;
        }

        [[nodiscard]] bool operator==(const BasicPolynominal &other) const
        {
            return coefs == other.coefs;
        }
        [[nodiscard]] bool operator!=(const BasicPolynominal &other) const
        {
            return coefs != other.coefs;
        }
    };

    using Polynominal = BasicPolynominal<long double>;
    using PolynominalC = BasicPolynominal<complex_t>;

    template <typename T> class BasicPolyFraction
    {
        using Polynominal = BasicPolynominal<T>;
        Polynominal num, den;
      public:

        BasicPolyFraction(const Polynominal &num = {0}, const Polynominal &den = {1}) : num(num), den(den) {}

        complex_t Eval(complex_t x) const
        {
            return num.Eval(x) / den.Eval(x);
        }

        int NumDegree() const
        {
            return num.Degree();
        }
        int DenDegree() const
        {
            return den.Degree();
        }
        int Degree() const
        {
            return max(num.Degree(), den.Degree());
        }

        long double NumFirstCoef() const
        {
            return num.FirstCoef();
        }
        long double DenFirstCoef() const
        {
            return den.FirstCoef();
        }

        bool CoefsHaveDifferentSigns() const
        {
            return num.CoefsHaveDifferentSigns() || den.CoefsHaveDifferentSigns();
        }

        // If at least one root can't be found, returns an empty list.
        std::vector<ldvec2> NumRoots() const
        {
            return num.Roots();
        }
        std::vector<ldvec2> DenRoots() const
        {
            return den.Roots();
        }

        const Polynominal &Num() const {return num;}
        const Polynominal &Den() const {return den;}

        std::string ToString() const
        {
            return "(" + num.ToString() + ")/(" + den.ToString() + ")";
        }

        [[nodiscard]] friend BasicPolyFraction operator+(const BasicPolyFraction &a, const BasicPolyFraction &b)
        {
            if (a.den == b.den)
                return BasicPolyFraction(a.num + b.num, a.den);
            return BasicPolyFraction(a.num * b.den + b.num * a.den, a.den * b.den);
        }
        [[nodiscard]] friend BasicPolyFraction operator-(const BasicPolyFraction &a, const BasicPolyFraction &b)
        {
            if (a.den == b.den)
                return BasicPolyFraction(a.num - b.num, a.den);
            return BasicPolyFraction(a.num * b.den - b.num * a.den, a.den * b.den);
        }
        [[nodiscard]] friend BasicPolyFraction operator*(const BasicPolyFraction &a, const BasicPolyFraction &b)
        {
            return BasicPolyFraction(a.num * b.num, a.den * b.den);
        }
        [[nodiscard]] friend BasicPolyFraction operator/(const BasicPolyFraction &a, const BasicPolyFraction &b)
        {
            return BasicPolyFraction(a.num * b.den, a.den * b.num);
        }

        friend BasicPolyFraction &operator+=(BasicPolyFraction &a, const BasicPolyFraction &b) {a = a + b; return a;}
        friend BasicPolyFraction &operator-=(BasicPolyFraction &a, const BasicPolyFraction &b) {a = a - b; return a;}
        friend BasicPolyFraction &operator*=(BasicPolyFraction &a, const BasicPolyFraction &b) {a = a * b; return a;}
        friend BasicPolyFraction &operator/=(BasicPolyFraction &a, const BasicPolyFraction &b) {a = a / b; return a;}

        [[nodiscard]] BasicPolyFraction Pow(int p)
        {
            if (p == 0)
                return BasicPolyFraction(1);

            BasicPolyFraction ret = *this;

            bool flip = 0;
            if (p < 0)
            {
                p = -p;
                flip = 1;
            }

            while (--p > 0)
                ret *= *this;

            if (flip)
                std::swap(ret.num, ret.den);

            return ret;
        }
    };

    using PolyFraction = BasicPolyFraction<long double>;
    using PolyFractionC = BasicPolyFraction<complex_t>;

    template <typename T> std::vector<T> SolveLinearSystem(int n, std::vector<T> a)
    {
        const int s = n+1;

        // First pass
        for (int i = 0; i < n; i++)
        {
            // Find max row
            auto max = abs(a[i + i*s]);
            int max_index = i;
            for (int j = i+1; j < n; j++)
            {
                auto cur = abs(a[i + j*s]);
                if (cur > max)
                {
                    max = cur;
                    max_index = j;
                }
            }

            // Swap max row with current
            if (max_index != i)
                std::swap_ranges(&a[i*s], &a[i*s] + s, &a[max_index*s]);

            // Subtract current row from ones below
            for (int j = i+1; j<n; j++)
            {
                T c = -a[i + j*s] / a[i + i*s];

                a[i + j*s] = 0;

                for (int k = i+1; k < n+1; k++)
                    a[k + j*s] += c * a[k + i*s];
            }
        }

        std::vector<T> x(n);

        // Second pass
        for (int i = n-1; i >= 0; i--)
        {
            T cur_x = x[i] = a[n + i*s] / a[i + i*s];

            for (int j = 0; j < i; j++)
                a[n + j*s] -= a[i + j*s] * cur_x;
        }

        if (std::all_of(x.begin(), x.end(), [](T x){return std::isfinite(x.real()) && std::isfinite(x.imag());}))
            return x;
        else
            return {};
    }
}


class Expression
{
  public:
    struct Exception : std::exception
    {
        std::string message;
        int pos;

        Exception() {}
        Exception(std::string message, int pos) : message(message), pos(pos) {}

        const char *what() const noexcept override
        {
            return message.c_str();
        }
    };

  private:
    struct Token
    {
        enum Type {num, lparen, rparen, op, var};
        enum Operator {plus, minus, mul, fake_mul, div, pow, left_paren}; // `fake_mul` is used for unary minus: `-a` -> `-1*a`

        Type type;

        union
        {
            Operator op_type;
            struct
            {
                long double value;
                bool is_int;
            }
            n;
        };

        int starts_at;

        std::string ToString() const
        {
            switch (type)
            {
              case num:
                return std::to_string(n.value);
              case lparen:
                return "(";
              case rparen:
                return ")";
              case var:
                return "x";
              case op:
                switch (op_type)
                {
                    case plus:       return "+";
                    case minus:      return "-";
                    case mul:        return "*";
                    case fake_mul:   return "* (fake)";
                    case div:        return "/";
                    case pow:        return "^";
                    case left_paren: return "(";
                }
            }
            return "?";
        }
    };

    static int Precedence(Token::Operator op)
    {
        switch (op)
        {
            case Token::plus:       return 1;
            case Token::minus:      return 1;
            case Token::mul:        return 2;
            case Token::div:        return 2;
            case Token::pow:        return 3;
            case Token::fake_mul:   return 3;
            case Token::left_paren: return -1;
        }
        return -1;
    }
    static bool IsRightAssociative(Token::Operator op)
    {
        return op == Token::pow || op == Token::fake_mul;
    }

    using OpFunc = complex_t (*)(const complex_t &, const complex_t &);

    static OpFunc OperatorFunc(Token::Operator op)
    {
        switch (op)
        {
          case Token::plus:
            return [](const complex_t &a, const complex_t &b){return a + b;};
          case Token::minus:
            return [](const complex_t &a, const complex_t &b){return a - b;};
          case Token::mul:
          case Token::fake_mul:
            return [](const complex_t &a, const complex_t &b){return a * b;};
          case Token::div:
            return [](const complex_t &a, const complex_t &b){return a / b;};
          case Token::pow:
            return [](const complex_t &a, const complex_t &b){return std::pow(a, b);};
          case Token::left_paren:
            return 0;
        }
        return 0;
    }


    static bool Tokenize(std::string_view str, char var_name, std::list<Token> *list, int *error_pos, std::string *error_msg)
    {
        std::list<Token> ret;
        Token token;

        int index = 0;
        while (1)
        {
            token.starts_at = index;
            switch (char ch = str[index])
            {
              case '\0':
                *list = ret;
                return 1;
              case '(':
                token.type = Token::lparen;
                ret.push_back(token);
                index++;
                break;
              case ')':
                token.type = Token::rparen;
                ret.push_back(token);
                index++;
                break;
              case '+':
                token.type = Token::op;
                token.op_type = Token::plus;
                ret.push_back(token);
                index++;
                break;
              case '-':
                token.type = Token::op;
                token.op_type = Token::minus;
                ret.push_back(token);
                index++;
                break;
              case '*':
                token.type = Token::op;
                token.op_type = Token::mul;
                ret.push_back(token);
                index++;
                break;
              case '/':
                token.type = Token::op;
                token.op_type = Token::div;
                ret.push_back(token);
                index++;
                break;
              case '^':
                token.type = Token::op;
                token.op_type = Token::pow;
                ret.push_back(token);
                index++;
                break;
              default:
                if ((unsigned char)ch <= ' ')
                {
                    index++;
                    break;
                }
                if ((ch >= '0' && ch <= '9') || ch == '.' || ch == ',')
                {
                    bool is_int = 1;
                    std::string num;
                    while (1)
                    {
                        ch = str[index];
                        if (ch < '0' || ch > '9')
                            break;
                        num += ch;
                        index++;
                    }
                    ch = str[index];
                    if (ch == '.' || ch == ',')
                    {
                        is_int = 0;
                        index++;
                        num += '.';
                        while (1)
                        {
                            ch = str[index];
                            if (ch < '0' || ch > '9')
                            {
                                if (ch == '.' || ch == ',')
                                {
                                    *error_pos = index;
                                    *error_msg = Str("Больше одной точки в записи числа.");
                                    return 0;
                                }
                                break;
                            }
                            num += ch;
                            index++;
                        }
                    }

                    if (num == ".")
                    {
                        *error_pos = token.starts_at;
                        *error_msg = Str("Ожидались цифры до и/или после точки.");
                        return 0;
                    }

                    long double value = 0;
                    if (auto end = Reflection::from_string(value, num.c_str()); end == num.c_str() + num.size())
                    {
                        token.type = Token::num;
                        token.n.value = value;
                        token.n.is_int = is_int;
                        ret.push_back(token);
                        break;
                    }
                }
                if (ch == var_name)
                {
                    token.type = Token::var;
                    ret.push_back(token);
                    index++;
                    break;
                }

                *error_pos = index;
                *error_msg = Str("Ожидалось число, переменная `", var_name, "`, скобка или операция.");
                return 0;
            }
        }
    }

    static bool FinalizeTokenList(std::list<Token> &list, int *error_pos, std::string *error_msg)
    {
        std::vector<int> paren_stack;

        auto it = list.begin(), prev = it;

        while (it != list.end())
        {
            bool increment_iter = 1;
            switch (it->type)
            {
              case Token::lparen:
                paren_stack.push_back(it->starts_at);
                [[fallthrough]];
              case Token::num:
              case Token::var:
                if (it != prev)
                {
                    if (it != prev && prev->type != Token::lparen && prev->type != Token::op)
                    {
                        if (it->type == Token::lparen)
                            paren_stack.pop_back();
                        Token new_token;
                        new_token.type = Token::op;
                        new_token.op_type = (it->type != Token::num ? Token::mul : Token::pow);
                        new_token.starts_at = it->starts_at;
                        it = list.insert(it, new_token);
                        /*
                        *error_pos = it->starts_at;
                        *error_msg = "Пропущена операция.";
                        return 0;
                        */
                    }
                }
                break;
              case Token::rparen:
                if (paren_stack.empty())
                {
                    *error_pos = it->starts_at;
                    *error_msg = "Лишняя закрывающая скобка.";
                    return 0;
                }
                paren_stack.pop_back();
                if (it != prev && (prev->type == Token::lparen || prev->type == Token::op))
                {
                    *error_pos = it->starts_at;
                    if (prev->type == Token::op && prev->op_type == Token::pow)
                        *error_msg = "Пропущено целое число.";
                    else
                        *error_msg = "Пропущено число или переменная.";
                    return 0;
                }
                break;
              case Token::op:
                if ((it == prev || prev->type == Token::lparen || prev->type == Token::op) && (it->op_type == Token::plus || it->op_type == Token::minus))
                {
                    if (it->op_type == Token::minus)
                    {
                        it->op_type = Token::fake_mul;
                        Token new_token;
                        new_token.starts_at = it->starts_at;
                        new_token.type = Token::num;
                        new_token.n.value = -1;
                        it = list.insert(it, new_token);
                        break;
                    }
                    else if (std::next(it) != list.end())
                    {
                        it = list.erase(it);
                        increment_iter = 0;
                        break;
                    }
                }
                if (it == prev || prev->type == Token::op || prev->type == Token::lparen)
                {
                    *error_pos = it->starts_at;
                    if (prev->type == Token::op && prev->op_type == Token::pow)
                        *error_msg = "Пропущено целое число.";
                    else
                        *error_msg = "Пропущено число или переменная.";
                    return 0;
                }
                break;
            }

            prev = it;
            if (increment_iter)
                it++;
        }

        if (list.empty())
        {
            *error_pos = 0;
            *error_msg = "Пустое выражение.";
            return 0;
        }

        if (list.back().type == Token::op)
        {
            *error_pos = list.back().starts_at;
            *error_msg = "Пропущено число или переменная.";
            return 0;
        }

        if (paren_stack.size() > 0)
        {
            *error_pos = paren_stack.back();
            *error_msg = "Скобка не закрыта.";
            return 0;
        }

        return 1;
    }


    struct Element
    {
        enum Type {num, var, op};

        int position;
        Type type;

        union
        {
            struct
            {
                Token::Operator type;
                OpFunc func;
            }
            o;
            struct
            {
                long double value;
                bool is_int;
            }
            n;
        };
    };

    std::vector<Element> elements;

    struct FractionData
    {
        bool cant_find_num_roots = 0, cant_find_den_roots = 0;
        bool coefs_have_different_signs = 0; // Not a good thing(tm). This is tested separately for numerator and denominator, then the result is ||'ed.
        bool has_negative_first_fac_ratio = 0; // Also not a good thing.
        long double num_first_fac = 1, den_first_fac = 1;
        std::vector<complex_t> num_roots, den_roots;

        PolyFraction fraction;
    };
    FractionData frac;

    static bool ParseExpression(const std::list<Token> &tokens, std::vector<Element> *elems)
    {
        // Shunting-yard algorithm

        struct OperatorStackElement
        {
            Token::Operator op;
            int pos;
        };
        std::vector<OperatorStackElement> op_stack;

        for (const auto &token : tokens)
        {
            switch (token.type)
            {
              case Token::num:
                {
                    Element el;
                    el.position = token.starts_at;
                    el.type = Element::num;
                    el.n.value = token.n.value;
                    el.n.is_int = token.n.is_int;
                    elems->push_back(el);
                }
                break;
              case Token::var:
                {
                    Element el;
                    el.position = token.starts_at;
                    el.type = Element::var;
                    elems->push_back(el);
                }
                break;
              case Token::op:
                {
                    int this_prec = Precedence(token.op_type);
                    Element el;
                    el.type = Element::op;
                    while (op_stack.size() > 0 && op_stack.back().op != Token::left_paren && (this_prec < Precedence(op_stack.back().op) || (this_prec == Precedence(op_stack.back().op) && !IsRightAssociative(token.op_type))))
                    {
                        el.o.type = op_stack.back().op;
                        el.o.func = OperatorFunc(op_stack.back().op);
                        el.position = op_stack.back().pos;
                        elems->push_back(el);
                        op_stack.pop_back();
                    }
                    op_stack.push_back({token.op_type, token.starts_at});
                }
                break;
              case Token::lparen:
                op_stack.push_back({Token::left_paren, token.starts_at});
                break;
              case Token::rparen:
                while (1)
                {
                    if (op_stack.empty())
                        return 0;

                    bool lparen_found = (op_stack.back().op == Token::left_paren);
                    if (!lparen_found)
                    {
                        Element el;
                        el.type = Element::op;
                        el.position = op_stack.back().pos;
                        el.o.type = op_stack.back().op;
                        el.o.func = OperatorFunc(op_stack.back().op);
                        elems->push_back(el);
                    }
                    op_stack.pop_back();

                    if (lparen_found)
                        break;
                }
                break;
            }
        }

        while (op_stack.size() > 0)
        {
            if (op_stack.back().op == Token::left_paren)
                return 0;
            Element el;
            el.type = Element::op;
            el.position = op_stack.back().pos;
            el.o.type = op_stack.back().op;
            el.o.func = OperatorFunc(op_stack.back().op);
            elems->push_back(el);
            op_stack.pop_back();
        }

        return 1;
    }

    static PolyFraction MakePolyFraction(const std::vector<Element> &elems)
    {
        struct StackElem
        {
            bool is_int_lit;
            int int_lit_value;
            PolyFraction frac;
        };

        std::vector<StackElem> stack;

        for (const auto &elem : elems)
        {
            switch (elem.type)
            {
              case Element::num:
                {
                    StackElem el;
                    if (elem.n.is_int && abs(elem.n.value) <= max_poly_degree + 0.5)
                    {
                        el.is_int_lit = 1;
                        el.int_lit_value = iround(elem.n.value);
                    }
                    else
                    {
                        el.is_int_lit = 0;
                    }
                    el.frac = PolyFraction(Polynominal(elem.n.value));
                    stack.push_back(el);
                }
                break;
              case Element::var:
                {
                    StackElem el;
                    el.is_int_lit = 0;
                    el.frac = PolyFraction(Polynominal(1, 1));
                    stack.push_back(el);
                }
                break;
              case Element::op:
                {
                    if (stack.size() < 2)
                        throw Exception("Ошибка при вычислении.", 0);
                    StackElem &p1 = stack[stack.size() - 2], &p2 = stack.back(), result;
                    result.is_int_lit = 0;
                    switch (elem.o.type)
                    {
                      case Token::plus:
                        result.frac = p1.frac + p2.frac;
                        break;
                      case Token::minus:
                        result.frac = p1.frac - p2.frac;
                        break;
                      case Token::fake_mul:
                        if (p2.is_int_lit)
                        {
                            result.is_int_lit = 1;
                            result.int_lit_value = -p2.int_lit_value;
                        }
                        [[fallthrough]];
                      case Token::mul:
                        result.frac = p1.frac * p2.frac;
                        break;
                      case Token::div:
                        result.frac = p1.frac / p2.frac;
                        break;
                      case Token::pow:
                        if (!p2.is_int_lit || abs(p2.int_lit_value) > max_poly_degree)
                            throw Exception(Str("Показатель степени должен быть целочисленной константой не больше ", max_poly_degree, "."), elem.position);
                        result.frac = p1.frac.Pow(p2.int_lit_value);
                        break;
                      case Token::left_paren:
                        // This shouldn't happen.
                        break;
                    }
                    if (result.frac.Degree() > max_poly_degree)
                        throw Exception(Str("Операция приводит к образованию многочлена слишком большой степени (больше ", max_poly_degree, ")."), elem.position);
                    stack.pop_back();
                    stack.pop_back(); // Sic! We pop twice.
                    stack.push_back(result);
                }
                break;
            }
        }

        if (stack.size() != 1)
            throw Exception("Ошибка при вычислении.", 0);

        return stack[0].frac;
    }

    static void ExtractFractionData(const PolyFraction &frac, FractionData *data, bool find_roots = 1)
    {
        data->num_first_fac = frac.NumFirstCoef();
        data->den_first_fac = frac.DenFirstCoef();

        data->has_negative_first_fac_ratio = (data->num_first_fac * data->den_first_fac < 0);

        auto num_roots = frac.NumRoots(),
             den_roots = frac.DenRoots();

        if (find_roots)
        {
            data->cant_find_num_roots = frac.NumDegree() && num_roots.empty();
            data->cant_find_den_roots = frac.DenDegree() && den_roots.empty();
        }

        data->coefs_have_different_signs = frac.CoefsHaveDifferentSigns();

        if (data->cant_find_num_roots || data->cant_find_den_roots)
            return;

        if (find_roots)
        {
            for (const auto &root : num_roots)
                data->num_roots.push_back({root.x, root.y});
            for (const auto &root : den_roots)
                data->den_roots.push_back({root.x, root.y});
        }
    }

    struct StepResponseData
    {
        bool dirty = 1;

        struct Element
        {
            // value = a * t^p * exp(t*b) * (t >= 0)
            complex_t a, b;
            int p;
        };
        std::vector<Element> elems; // Sum values for all elements to obtain the final value.
    };
    StepResponseData step_response;

  public:
    Expression() {}
    Expression(std::string str, char var = 's')
    {
        std::list<Expression::Token> tokens;
        int err_pos;
        std::string err_msg;

        if (Tokenize(str, var, &tokens, &err_pos, &err_msg) &&
            FinalizeTokenList(tokens, &err_pos, &err_msg))
        {
            if (!ParseExpression(tokens, &elements))
                throw Exception("Недопустимое выражение.", 0);
            frac.fraction = MakePolyFraction(elements);
            ExtractFractionData(frac.fraction, &frac);
        }
        else
        {
            throw Exception(err_msg, err_pos);
        }
    }
    // Those roots must have -1 imag part if they're real, or 0+ imag part if they're complex.
    Expression(long double factor, const std::vector<complex_t> &num_roots, const std::vector<complex_t> &den_roots, std::string *text = 0)
    {
        Polynominal num(factor), den(1);
        for (const auto &it : num_roots)
        {
            if (it.imag() < -0.5)
            {
                num *= Polynominal(1,1) - it.real();
                frac.num_roots.push_back({it.real(), 0});
            }
            else
            {
                num *= Polynominal(1,2) + ipow(it.real(),2) + ipow(it.imag(),2) - 2 * it.real() * Polynominal(1,1);
                frac.num_roots.push_back({it.real(), it.imag()});
                frac.num_roots.push_back({it.real(), -it.imag()});
            }
        }
        for (const auto &it : den_roots)
        {
            if (it.imag() < -0.5)
            {
                den *= Polynominal(1,1) - it.real();
                frac.den_roots.push_back({it.real(), 0});
            }
            else
            {
                den *= Polynominal(1,2) + ipow(it.real(),2) + ipow(it.imag(),2) - 2 * it.real() * Polynominal(1,1);
                frac.den_roots.push_back({it.real(), it.imag()});
                frac.den_roots.push_back({it.real(), -it.imag()});
            }
        }

        std::string str = "(" + num.ToStringDense() + ") / (" + den.ToStringDense() + ")";
        if (text)
            *text = str;

        std::list<Expression::Token> tokens;
        int err_pos;
        std::string err_msg;

        if (Tokenize(str, 's', &tokens, &err_pos, &err_msg) &&
            FinalizeTokenList(tokens, &err_pos, &err_msg))
        {
            if (!ParseExpression(tokens, &elements))
                throw Exception("Недопустимое выражение.", 0);
            frac.fraction = PolyFraction(num, den);
            ExtractFractionData(frac.fraction, &frac, 0);
        }
        else
        {
            throw Exception(err_msg, err_pos);
        }
    }

    explicit operator bool() const
    {
        return elements.size() > 0;
    }

    complex_t Eval(complex_t variable) const
    {
        std::vector<complex_t> stack;
        for (const auto &elem : elements)
        {
            switch (elem.type)
            {
              case Element::num:
                stack.push_back({elem.n.value, 0});
                break;
              case Element::var:
                stack.push_back(variable);
                break;
              case Element::op:
                {
                    if (stack.size() < 2)
                        throw std::runtime_error("Ошибка при вычислении.");
                    complex_t result = elem.o.func(stack[stack.size()-2], stack.back());
                    stack.pop_back();
                    stack.pop_back(); // Sic! We pop twice.
                    stack.push_back(result);
                }
                break;
            }
        }

        if (stack.size() != 1)
            throw std::runtime_error("Ошибка при вычислении.");

        return stack.front();
    }
    ldvec2 EvalVec(complex_t variable) const
    {
        auto val = Eval(variable);
        return {val.real(), val.imag()};
    }

    long double EvalAmplitude(complex_t variable) const
    {
        /*
        static bool b = 0;
        b = !b;
        if (b)
            return std::abs(Eval(variable));
        //*/

        if (!frac.coefs_have_different_signs || CantFindRoots())
            return std::abs(Eval(variable)) * (frac.has_negative_first_fac_ratio ? -1 : 1);

        long double ampl = frac.num_first_fac / frac.den_first_fac;
        for (const auto &root : frac.num_roots)
            ampl *= std::abs(variable - root);
        for (const auto &root : frac.den_roots)
            ampl /= std::abs(variable - root);
        return ampl;
    }
    long double EvalPhase(complex_t variable) const
    {
        /*
        static bool b = 0;
        b = !b;
        if (b)
            return std::arg(Eval(variable));
        //*/

        if (CantFindRoots())
            return std::arg(Eval(variable));

        long double phase = 0;
        for (const auto &root : frac.num_roots)
            phase += std::arg(variable - root);
        for (const auto &root : frac.den_roots)
            phase -= std::arg(variable - root);
        return phase;
    }
    long double EvalStepResponse(long double t)
    {
        ComputeStepResponse();

        if (t < 0)
            return 0;

        complex_t ret = 0;

        for (const auto &it : step_response.elems)
            ret += it.a * std::pow(t, it.p) * std::exp(t * it.b);

        return ret.real();
    }

    bool CantFindRoots() const
    {
        return frac.cant_find_num_roots || frac.cant_find_den_roots;
    }

    void ComputeStepResponse()
    {
        const long double root_epsilon = 0.00001; // Roots closer to each other than this value

        if (!step_response.dirty)
            return;
        step_response.dirty = 0;

        struct Root
        {
            complex_t value = 0;
            int count = 0;

            Root() {}
            Root(complex_t value, int count) : value(value), count(count) {}
        };
        std::vector<Root> roots;
        int root_count = 0;

        { // Find out what roots we have and how many times they are repeated
            if (frac.cant_find_den_roots)
                return;
            auto roots_raw = frac.den_roots;
            roots_raw.push_back(0); // That's `*= 1/s`.
            std::vector<std::vector<int>> root_reach_list(roots_raw.size());

            // For each root, find equal or almost equal roots
            for (size_t i = 0; i < roots_raw.size(); i++)
            {
                for (size_t j = 0; j < roots_raw.size(); j++)
                {
                    if (std::abs(roots_raw[i] - roots_raw[j]) < root_epsilon)
                        root_reach_list[i].push_back(j);
                }
            }

            int remaining_roots = roots_raw.size();

            while (remaining_roots > 0)
            {
                auto max = std::max_element(root_reach_list.begin(), root_reach_list.end(), [](const std::vector<int> &a, const std::vector<int> &b){return a.size() < b.size();});
                std::vector<int> elems = *max;
                for (auto &vec : root_reach_list)
                    vec.erase(std::remove_if(vec.begin(), vec.end(), [&](int x){return std::find(elems.begin(), elems.end(), x) != elems.end();}), vec.end());
                int count = elems.size();
                complex_t average = 0;
                for (int el : elems)
                    average += roots_raw[el];
                average /= count;
                roots.push_back(Root(average, count));
                remaining_roots -= count;
            }

            root_count = roots_raw.size();
        }

        std::vector<complex_t> system_matrix(root_count * (root_count + 1));
        int stride = root_count + 1;
        Polynominal num, unused;
        frac.fraction.Num().LongDivision(frac.fraction.Den() * Polynominal(1,1), unused, num);
        (void)unused;

        // Compute the right hand side of the equations
        for (int i = 0; i < root_count; i++)
            system_matrix[root_count + stride*i] = num.GetCoef(i) / frac.den_first_fac;

        struct PartialFrac
        {
            int root, power;
            PartialFrac() {}
            PartialFrac(int root, int power) : root(root), power(power) {}
        };
        std::vector<PartialFrac> partial_fracs;

        for (size_t i = 0; i < roots.size(); i++)
        for (int j = 1; j <= roots[i].count; j++)
        {
            partial_fracs.push_back(PartialFrac(i, j));
        }

        for (int i = 0; i < root_count; i++)
        {
            PolynominalC poly(1);
            int this_root = partial_fracs[i].root;
            int this_power = partial_fracs[i].power;
            for (int j = 0; j < int(roots.size()); j++)
            {
                if (j == this_root)
                    continue;
                for (int k = 1; k <= roots[j].count; k++)
                    poly *= PolynominalC(1,1) - roots[j].value;
            }
            for (int j = 1; j <= roots[this_root].count - this_power; j++)
                poly *= PolynominalC(1,1) - roots[this_root].value;

            for (int j = 0; j < root_count; j++)
                system_matrix[i + stride*j] = poly.GetCoef(j);
        }

        auto values = SolveLinearSystem(root_count, system_matrix);
        if (values.empty())
            return;

        step_response.elems = {};
        for (int i = 0; i < root_count; i++)
        {
            auto &pf = partial_fracs[i];
            StepResponseData::Element new_elem;
            new_elem.a = values[i] / complex_t(std::tgamma((long double)pf.power));
            //std::cout << "... " << values[i] << '\n';
            new_elem.b = roots[pf.root].value;
            new_elem.p = pf.power - 1;
            step_response.elems.push_back(new_elem);
            /*/
            PolynominalC poly(1);
            for (int j = 0; j < pf.power; j++)
                poly *= PolynominalC(1,1) - roots[pf.root].value;
            std::cout << PolyFractionC(values[i], poly).ToString() << '\n';
            //*/
        }

        /*/
        std::cout << "value = sigma: a * t^p * exp(t*b)\n";
        for (auto it : step_response.elems)
        {
            std::cout << "a = " << it.a << "\n";
            std::cout << "b = " << it.b << "\n";
            std::cout << "p = " << it.p << "\n";
        }
        //*/
    }

    const auto &GetFracData() const
    {
        return frac;
    }
};


class Plot
{
  public:
    enum Flags
    {
        lock_scale_ratio = 1,
        vertical_pi      = 2,
        horizontal_log10 = 4,
        vertical_20log10 = 8,
        has_horizontal_func  = 16,
    };

    using func_t = std::function<ldvec2(long double)>;
    constexpr static long double default_scale_factor = 100,
                                 default_min = 0,
                                 default_max = 8;

    struct Func
    {
        func_t func;
        fvec3 color;
        std::string name;
    };

    static ivec2 ViewportPos()
    {
        return ivec2(0, interface_rect_height);
    }
    static ivec2 ViewportSize()
    {
        return win.Size().sub_y(interface_rect_height);
    }

  private:
    static constexpr float window_margin = 0.4;
    static constexpr int window_smallest_pix_margin = 32;

    static constexpr int bounding_box_segment_count = 512,
                         grid_max_number_precision = 4;
    static constexpr float bounding_box_discarded_edges = 0.03; // Must be less than 0.5

    static constexpr ivec2 min_grid_cell_pixel_size = ivec2(48),
                           grid_number_offset_h     = ivec2(8,0),
                           grid_number_offset_v     = ivec2(3,-8);

    static constexpr fvec3 grid_text_color = fvec3(0),
                           grid_light_text_color = fvec3(0.4);
    static constexpr float grid_zero_line_color  = 0.2,
                           grid_large_line_color = 0.55,
                           grid_mid_line_color   = 0.8,
                           grid_small_line_color = 0.85;

    using distr_t = std::uniform_real_distribution<long double>;

    std::vector<Func> funcs;
    int flags;

    ldvec2 offset = ivec2(0);
    ldvec2 scale = ivec2(default_scale_factor);
    ldvec2 default_offset = ivec2(0);
    ldvec2 default_scale = ldvec2(default_scale_factor);
    long double range_start = default_min, range_len = default_max - default_min;
    long double range_start_real = default_min, range_len_real = default_max - default_min;
    long double current_value_offset = 0.5; // NOTE: These default values must be synced with those in `ResetAccumulator()`.
    unsigned long long current_value_index = 0, current_value_max_index = 1; // ^

    ldvec2 grid_scale_step_factor = ldvec2(10);
    ivec2 grid_cell_segments = ivec2(10);
    ivec2 grid_cell_highlight_step = ivec2(5);

    bool grabbed = 0;
    ivec2 grab_offset = ivec2(0);

    bool scale_changed_this_tick = 0,
         scale_changed_prev_tick = 1;


    struct PointData
    {
        bool valid;
        ldvec2 pos;
    };

    PointData Point(long double freq, int func_index) const
    {
        PointData ret;
        ret.pos = funcs[func_index].func(freq);
        if (flags & vertical_pi)
            ret.pos.y /= ld_pi;
        if (flags & horizontal_log10)
            ret.pos.x = std::log10(ret.pos.x);
        if (flags & vertical_20log10)
            ret.pos.y = std::log10(std::abs(ret.pos.y));
        ret.pos.y = -ret.pos.y;
        ret.valid = std::isfinite(ret.pos.x) && std::isfinite(ret.pos.y);
        return ret;
    }

    ldvec2 MinScale()
    {
        return std::pow(0.1, grid_max_number_precision - 2 + 10) * ldvec2(min_grid_cell_pixel_size * 2 + 2);
    }
    ldvec2 MaxScale()
    {
        return std::pow(10, grid_max_number_precision - 2 + 10) * ldvec2(min_grid_cell_pixel_size * 2 - 2);
    }

    void AddPoint(int type, long double value)
    {
        for (int i = 0; i < int(funcs.size()); i++)
        {
            auto point = Point(value, i);
            if (!point.valid)
                continue;
            ldvec2 pos = (point.pos + offset) * scale;
            if ((abs(pos) > win.Size()/2).any())
                continue;
            Draw::Dot(type, pos, funcs[i].color);
        }
    }

  public:
    bool draw_area = 0;
    long double area_a = 0, area_mid = 0, area_b = 0;

    Plot(int flags = 0) : flags(flags)
    {
        default_offset += ViewportPos() / default_scale / 2;
        offset = default_offset;
    }
    Plot(const std::vector<Func> &funcs, long double a, long double b, int flags) : funcs(funcs), flags(flags), range_start(a), range_len(b - a)
    {
        RecalculateDefaultOffsetAndScale();
        offset = default_offset;
        scale = default_scale;
    }

    explicit operator bool() const
    {
        return funcs.size() > 0;
    }

    void Tick(int count)
    {
        // Move if needed
        if (grabbed)
        {
            if (mouse.left.up())
                grabbed = 0;

            offset = (mouse.pos() - grab_offset) / scale;

            ResetAccumulator();
        }

        if (funcs.size() > 0)
        {
            // Draw points
            while (count-- > 0)
            {
                long double value = range_start_real + (current_value_offset + current_value_index++ / double(current_value_max_index)) * range_len_real;

                if (flags & horizontal_log10)
                    value = std::pow(10.0l, value);

                if (current_value_index >= current_value_max_index)
                {
                    if (current_value_max_index == 1)
                    {
                        AddPoint(3, range_start);
                        AddPoint(2, range_start+range_len);
                    }
                    current_value_index = 0;
                    current_value_max_index <<= 1;
                    current_value_offset /= 2;
                }

                AddPoint(grabbed || scale_changed_this_tick, value);
            }
            r.Finish();
        }

        if (!scale_changed_this_tick && scale_changed_prev_tick)
            ResetAccumulator();
        scale_changed_prev_tick = scale_changed_this_tick;
        scale_changed_this_tick = 0;
    }

    void RecalculateDefaultOffsetAndScale()
    {
        if (!bool(*this))
            return;

        ldvec2 box_min(0), box_max(0);
        std::vector<long double> values_x, values_y;
        for (auto *it : {&values_x, &values_y})
            it->reserve(bounding_box_segment_count + 1);

        for (int i = 0; i < int(funcs.size()); i++)
        {
            for (auto *it : {&values_x, &values_y})
                it->clear();

            for (const auto &point : {Point(range_start, i), Point(range_start + range_len, i)})
            {
                if (point.valid)
                {
                    for (auto mem : {&ldvec2::x, &ldvec2::y})
                    {
                        if (point.pos.*mem < box_min.*mem)
                            box_min.*mem = point.pos.*mem;
                        if (point.pos.*mem > box_max.*mem)
                            box_max.*mem = point.pos.*mem;
                    }
                }
            }

            for (int j = 0; j <= bounding_box_segment_count; j++)
            {
                auto point = Point(j / double(bounding_box_segment_count) * range_len + range_start, i);
                if (!point.valid)
                    continue;

                values_x.push_back(point.pos.x);
                values_y.push_back(point.pos.y);
            }

            if (values_x.size() > 0)
            {
                for (auto *it : {&values_x, &values_y})
                    std::sort(it->begin(), it->end());

                int index_min = iround(values_x.size() * bounding_box_discarded_edges),
                    index_max = iround(values_x.size() * (1-bounding_box_discarded_edges));
                ldvec2 a(values_x[index_min], values_y[index_min]),
                       b(values_x[index_max], values_y[index_max]);
                for (auto mem : {&ldvec2::x, &ldvec2::y})
                {
                    if (a.*mem < box_min.*mem)
                        box_min.*mem = a.*mem;
                    if (b.*mem > box_max.*mem)
                        box_max.*mem = b.*mem;
                }
            }
        }

        for (auto mem : {&ldvec2::x, &ldvec2::y})
        {
            default_offset.*mem = (box_min.*mem + box_max.*mem) / -2;
            if (box_min.*mem == box_max.*mem)
                continue;
            default_scale.*mem = (ldvec2(ViewportSize()).*mem * (1-window_margin)) / (box_max.*mem - box_min.*mem);
        }

        if (flags & lock_scale_ratio)
            default_scale = ldvec2(default_scale.min());

        clamp_assign(default_scale, MinScale(), MaxScale());

        default_offset += ViewportPos() / default_scale / 2;

        ldvec2 viewport_min = (Draw::min + ViewportPos() + window_smallest_pix_margin) / default_scale,
               viewport_max = viewport_min + (ViewportSize() - window_smallest_pix_margin*2) / default_scale;
        clamp_assign(default_offset, viewport_min, viewport_max);
    }

    void ResetAccumulator()
    {
        Graphics::Clear(Graphics::color);

        { // Calculate clamped range
            if (flags & has_horizontal_func)
            {
                range_start_real = range_start;
                range_len_real = range_len;
            }
            else
            {
                long double from = range_start, to = range_start + range_len;
                if (flags & horizontal_log10)
                {
                    from = std::log10(from);
                    to = std::log10(to);
                }
                range_start_real = max(from, -ViewportSize().x/2 / scale.x - offset.x);
                range_len_real = min(to, ViewportSize().x/2 / scale.x - offset.x) - range_start_real;
            }
        }

        { // Draw grid
            ldvec2 visible_size = win.Size() / scale;
            ldvec2 corner = -offset - visible_size/2;

            ldvec2 min_cell_size = min_grid_cell_pixel_size / scale;
            ldvec2 cell_size;

            for (auto mem : {&ldvec2::x, &ldvec2::y})
                cell_size.*mem = std::pow(grid_scale_step_factor.*mem, ceil(std::log(min_cell_size.*mem) / std::log(grid_scale_step_factor.*mem)));

            ivec2 line_count = iround(ceil(visible_size / cell_size)) + 1;

            ldvec2 first_cell_pos = floor(corner / cell_size) * cell_size;

            auto lambda = [&](bool text, long double ldvec2::*ld_a, long double ldvec2::*ld_b, int ivec2::*int_a, int ivec2::*int_b)
            {
                (void)ld_b;

                bool vertical = ld_a == &ldvec2::x;
                bool text_on_mid_lines = cell_size.*ld_a > min_cell_size.*ld_a * 2;

                for (int i = 0; i < line_count.*int_a * grid_cell_segments.*int_a; i++)
                {
                    long double value = first_cell_pos.*ld_a + i * cell_size.*ld_a / grid_cell_segments.*int_a;

                    bool mid_line = i % grid_cell_highlight_step.*int_a == 0;
                    bool large_line = i % grid_cell_segments.*int_a == 0;
                    bool zero_line = large_line && abs(value) < cell_size.*ld_a / grid_cell_segments.*int_a / 2;

                    if (!text)
                    { // Line

                        ldvec2 pixel_pos;
                        pixel_pos.*ld_a = (value + offset.*ld_a) * scale.*ld_a;
                        pixel_pos.*ld_b = 0;

                        ivec2 pixel_size = ivec2(5, win.Size().*int_b + 5);

                        float color;
                        if (zero_line)
                            color = grid_zero_line_color;
                        else if (large_line)
                            color = grid_large_line_color;
                        else if (mid_line)
                            color = grid_mid_line_color;
                        else
                            color = grid_small_line_color;

                        auto quad = r.Quad(pixel_pos, pixel_size).tex(ivec2(34 + 8 * !mid_line, 2), ivec2(5)).center().color(fvec3(1)).mix(1-color);

                        if (!vertical)
                            quad.matrix(fmat2(0,1,-1,0));
                    }
                    else
                    { // Number
                        if (!(large_line || (mid_line && text_on_mid_lines)))
                            continue;

                        ivec2 pixel_pos;
                        pixel_pos.*int_a = iround((value + offset.*ld_a) * scale.*ld_a);
                        pixel_pos.*int_b = (vertical ? Draw::max.*int_b : Draw::min.*int_b);
                        pixel_pos += (vertical ? grid_number_offset_v : grid_number_offset_h);

                        if (!vertical)
                        {
                            value = -value;

                            if (flags & vertical_20log10)
                                value *= 20;
                        }

                        char string_buf[64] = "0";
                        if (!zero_line)
                            std::snprintf(string_buf, sizeof string_buf, "%.*Lg", grid_max_number_precision, value);
                        std::string str = string_buf;

                        if (auto pos = str.find_last_of("+-"); pos != str.npos && pos != 0)
                        {
                            std::string new_str(str, 0, pos-1);
                            new_str += "·10[";
                            if (str[pos] == '-')
                                new_str += "-";
                            pos++;
                            while (str[pos] == '0')
                                pos++;
                            while (str[pos])
                                new_str += str[pos++];
                            new_str += ']';
                            str = new_str;
                        }

                        if (!vertical)
                        {
                            if (flags & vertical_pi)
                                str += " п";
                            if (flags & vertical_20log10)
                                str += " дБ";
                        }
                        else
                        {
                            if (flags & horizontal_log10)
                                str = "{10 }" + str;
                        }

                        r.Text(pixel_pos, str).align(ivec2(-1,1)).font(font_small).color(large_line ? grid_text_color : grid_light_text_color).preset(Draw::SupSub);
                    }
                }
            };

            Graphics::Blending::FuncAdd();
            Graphics::Blending::Equation(Graphics::Blending::eq_min);
            lambda(0, &ldvec2::x, &ldvec2::y, &ivec2::x, &ivec2::y);
            lambda(0, &ldvec2::y, &ldvec2::x, &ivec2::y, &ivec2::x);
            if (flags & lock_scale_ratio)
            {
                for (float angle : {f_pi/4, f_pi*3/4})
                {
                    ldvec2 n = ldvec2(cos(angle), sin(angle)),
                           n2 = ldvec2(n.y, -n.x);

                    ldvec2 pos = offset * scale;
                    pos -= pos /dot/ n2 * n2;

                    r.Quad(pos, ivec2(5,win.Size().max()*2)).tex(ivec2(34, 2), ivec2(5)).center().color(fvec3(1)).mix(1-grid_large_line_color).matrix(fmat2(n.x,-n.y,n.y,n.x));
                }
            }
            r.Finish();

            Graphics::Blending::FuncNormalPre();
            Graphics::Blending::Equation(Graphics::Blending::eq_add);
            lambda(1, &ldvec2::x, &ldvec2::y, &ivec2::x, &ivec2::y);
            lambda(1, &ldvec2::y, &ldvec2::x, &ivec2::y, &ivec2::x);
            r.Finish();
        }

        // Draw colored area
        if (draw_area)
        {
            constexpr long double width = 3;
            r.Quad(ldvec2(Draw::min.x, (-area_a + offset.y) * scale.y), ldvec2(Draw::max.x, (-area_b + offset.y) * scale.y)).absolute().color(fvec3(0.3,1,0.2)).alpha(0.7);
            r.Quad(ldvec2(Draw::min.x, (-area_mid + offset.y) * scale.y - width/2), ldvec2(Draw::max.x, (-area_mid + offset.y) * scale.y + width/2)).absolute().color(fvec3(0.3,1,0.2)*0.6).alpha(1);
        }

        { // Curve names
            constexpr int gap = 3, gap_y = 2;
            int y = ViewportPos().y + 16;
            for (const auto &func : funcs)
            {
                ivec2 pos(Draw::max.x - gap, Draw::min.y + y);
                r.Text(pos, func.name).color(func.color).align(ivec2(1,-1)).preset(Draw::SupSub).preset(Draw::WithWhiteBackground());
                y += gap_y + font_main.Height();
            }
            r.Finish();
        }

        Draw::Accumulator::Overwrite();

        current_value_offset = 0.5;
        current_value_index = 0;
        current_value_max_index = 1;
    }

    void Grab()
    {
        grabbed = 1;
        grab_offset = mouse.pos() - offset * scale;
    }

    void Scale(ldvec2 scale_factor)
    {
        ldvec2 old_mid_offset = ViewportPos() / 2 / scale - offset;

        scale *= scale_factor;
        clamp_assign(scale, MinScale(), MaxScale());

        if (flags & lock_scale_ratio)
            scale.x = scale.y;

        ldvec2 new_mid_offset = ViewportPos() / 2 / scale - offset;
        offset += new_mid_offset - old_mid_offset;

        scale_changed_this_tick = 1;

        ResetAccumulator();
    }

    void ResetOffsetAndScale()
    {
        offset = default_offset;
        scale = default_scale;

        scale_changed_this_tick = 1;

        ResetAccumulator();
    }
};


class Button
{
    inline static std::string current_tooltip;

    ivec2 location = ivec2(0);
    ivec2 pos = ivec2(0);
    int icon_index = 0;
    bool visible = 0;
    bool holdable = 0;
    std::string tooltip;
    std::function<void(void)> func;

    bool pressed = 0, hovered = 0;

    ivec2 ScreenPos() const
    {
        return win.Size() * location / 2 + pos;
    }

  public:
    Button() {}
    Button(ivec2 location, ivec2 pos, int icon_index, bool holdable, std::string tooltip, std::function<void(void)> func)
        : location(location), pos(pos), icon_index(icon_index), visible(1), holdable(holdable), tooltip(tooltip), func(std::move(func)) {}

    void Tick(bool &button_pressed, bool ignore_hover)
    {
        constexpr int half_extent = 24;

        if (!visible)
        {
            pressed = 0;
            hovered = 0;
            return;
        }

        ivec2 screen_pos = ScreenPos();
        hovered = !ignore_hover && (mouse.pos() >= screen_pos - half_extent).all() && (mouse.pos() < screen_pos + half_extent).all();

        if (hovered)
            current_tooltip = tooltip;

        if (hovered && button_pressed)
        {
            button_pressed = 0;
            pressed = 1;
            if (!holdable)
                func();
        }

        if (pressed && holdable)
            func();

        if (!hovered || !mouse.left.down())
            pressed = 0;
    }
    void Render() const
    {
        constexpr int pressed_offset = 2;

        if (!visible)
            return;

        ivec2 screen_pos = ScreenPos();

        // Button itself
        r.Quad(screen_pos.add_y(pressed * pressed_offset), ivec2(48)).tex(ivec2(48*hovered, 32)).center();
        // Icon
        r.Quad(screen_pos.add_y(pressed * pressed_offset), ivec2(48)).tex(ivec2(48*icon_index, 80)).center();
    }

    static const std::string &CurrentTooltip()
    {
        return current_tooltip;
    }
    static void ResetTooltip()
    {
        current_tooltip = "";
    }
};


class TextField
{
    static constexpr int height = 24, text_offset_x = 6, fancy_margin_r = 128;

    static constexpr fvec3 title_color = fvec3(0),
                           invalid_color = fvec3(0.9,0.2,0);

    using tick_func_t = std::function<void(TextField &, bool updated)>;
    using render_func_t = std::function<void(const TextField &)>;

    inline static unsigned int id_counter = 0, active_id = -1;
    inline static uint32_t last_keypress_time = 0;

    unsigned int id = -2;
  public:

    ivec2 location = ivec2(0);
    ivec2 pos = ivec2(0);
    int width = 128;
    int max_chars = 64;
    std::string title;
    std::string allowed_chars = "";
    bool visible = 0;

    bool cross_hovered = 0;

    bool with_cross_button = 0;
    bool fancy_input = 0;

    tick_func_t tick_func;
    render_func_t render_func;

    std::string value;

    TextField() {}
    TextField(ivec2 location, ivec2 pos, int width, int max_chars, std::string title, std::string allowed_chars = "", tick_func_t tick_func = 0, render_func_t render_func = 0)
        : id(id_counter++), location(location), pos(pos), width(width), max_chars(max_chars), title(title), allowed_chars(allowed_chars), visible(1), tick_func(tick_func), render_func(render_func) {}

    bool invalid = 0;
    int invalid_pos = 0;
    std::string invalid_text;

    ivec2 ScreenPos() const
    {
        return win.Size() * location / 2 + pos;
    }
    ivec2 ScreenSize() const
    {
        return ivec2(width, height);
    }

    void Activate()
    {
        active_id = id;
        Input::Text(0);
    }
    static void Deactivate()
    {
        active_id = -1;
        Input::Text(0);
    }
    bool Active() const
    {
        return id == active_id;
    }

    void Tick(bool &button_pressed)
    {
        if (!visible)
            return;

        ivec2 screen_pos = ScreenPos(),
              screen_size = ScreenSize();

        std::string saved_value = value;

        cross_hovered = with_cross_button && (abs(mouse.pos() - screen_pos - ivec2(screen_size.x - 12, screen_size.y/2)) <= 8).all();

        if ((mouse.pos() >= screen_pos).all() && (mouse.pos() < screen_pos + screen_size).all() && button_pressed)
        {
            button_pressed = 0;
            Activate();
            if (cross_hovered)
                value = "";
        }

        if (id == active_id)
        {
            Input::Text(&value, max_chars, allowed_chars);
            if (Keys::l_ctrl.down() || Keys::r_ctrl.down())
            {
                if (Keys::c.pressed())
                    SDL_SetClipboardText(value.c_str());
                if (Keys::v.pressed())
                {
                    auto ptr = SDL_GetClipboardText();
                    if (ptr)
                    {
                        value = ptr;
                        SDL_free(ptr);
                        value.erase(std::remove_if(value.begin(), value.end(), [&](char ch){return allowed_chars.find_first_of(ch) == allowed_chars.npos;}), value.end());
                        Input::SetTextCursorPos(value.size()); // Yeah, this is not safe for non-ASCII characters. Good we have none here.
                    }
                }
            }
        }

        if (Keys::any.repeated())
            last_keypress_time = Events::Time();

        if (tick_func)
            tick_func(*this, value != saved_value);
    }

    void Render(bool inv = 0) const
    {
        constexpr int fancy_shade_width = 32; // This appears on edjes if text is too long.
        constexpr fvec3 back_color(1);

        if (!visible)
            return;

        ivec2 screen_pos = ScreenPos(),
              screen_size = ScreenSize();

        bool selected = id == active_id;

        // Box
        if (selected)
            r.Quad(screen_pos-1, screen_size+2).color(fvec3(0)).alpha(0.5);
        r.Quad(screen_pos, screen_size).color(fvec3(0));
        r.Quad(screen_pos+1, screen_size-2).color(!inv ? back_color : back_color*0.7f);

        // Title
        r.Text(screen_pos, title).color(!inv ? title_color : 1-title_color).font(font_small).align(ivec2(-1,1));

        // Invalid text
        r.Text(screen_pos.add_y(height), invalid_text).color(title_color).font(font_small).align(ivec2(-1,-1)).color(invalid_color);

        { // Text
            const int text_offset_right = text_offset_x + 20*with_cross_button;

            int cursor_pos = 0;
            if (id == active_id)
                cursor_pos = Input::TextCursorPos();

            ivec2 pos;
            std::string str;
            int cursor_pos_offset;
            bool start_shade = 0, end_shade = 0;
            if (!fancy_input)
            {
                pos = screen_pos + ivec2(text_offset_x, height/2);
                str = value;
                cursor_pos_offset = 0;
            }
            else
            {
                pos = screen_pos + ivec2(text_offset_x, height/2);
                std::string::const_iterator cur_iter = value.begin() + cursor_pos, start_iter = cur_iter, end_iter = cur_iter;
                int last_delta = 0, last_width = 0;

                while (1)
                {
                    if (start_iter == value.begin())
                    {
                        last_delta = 0;
                        break;
                    }
                    auto test_iter = std::prev(start_iter);
                    int w = Draw::RoughSize(font_small, test_iter, cur_iter).x;
                    int delta = (width - text_offset_x - fancy_margin_r) - w;
                    if (delta < 0)
                        break;
                    last_delta = delta;
                    last_width = w;
                    start_iter = test_iter;
                }
                pos.x += last_delta;
                last_width += last_delta;
                while (1)
                {
                    if (end_iter == value.end())
                        break;
                    auto test_iter = std::next(end_iter);
                    if ((width - last_width - text_offset_x - text_offset_right) < Draw::RoughSize(font_small, cur_iter, test_iter).x)
                        break;
                    end_iter = test_iter;
                }

                str = std::string(start_iter, end_iter);
                cursor_pos_offset = start_iter - value.begin();

                start_shade = start_iter != value.begin();
                end_shade = end_iter != value.end();
            }

            {
                auto text = r.Text(pos, str).align_h(-1).font(font_small).color(fvec3(0));
                if (invalid)
                    text.preset(Draw::ColorAfterPos(invalid_pos - cursor_pos_offset, invalid_color));
                if (selected && (Events::Time() - last_keypress_time) % 60 < 30)
                    text.preset(Draw::WithCursor(cursor_pos - cursor_pos_offset, fvec3(0)));
            }

            if (fancy_input)
            {
                if (start_shade)
                    r.Quad(screen_pos+1, ivec2(fancy_shade_width, screen_size.y-2)).color(back_color).alpha(1,0,0,1);
                if (end_shade)
                    r.Quad(screen_pos + ivec2(screen_size.x-1-fancy_shade_width-text_offset_right+text_offset_x, 1), ivec2(fancy_shade_width, screen_size.y-2)).color(back_color).alpha(0,1,1,0);
            }
        }

        // Cross button
        if (with_cross_button)
        {
            ivec2 pos = screen_pos + ivec2(screen_size.x - 12, screen_size.y/2);
            r.Quad(pos, ivec2(16)).center().color(back_color);
            auto q = r.Quad(pos, ivec2(16)).center().tex(ivec2(64,0));
            if (cross_hovered)
                q.color(invalid_color).mix(0);
        }

        if (render_func)
            render_func(*this);
    }
};


int main(int, char **)
{
    const std::string default_expression = "";
    constexpr fvec3 plot_color = fvec3(0.9,0,0.66);
    constexpr ivec2 table_gui_rect_size(400,300), table_gui_offset(64,80);
    constexpr int table_gui_button_h = 48;
    constexpr int message_timer_start = 150, message_alpha_time = 60;
    constexpr ivec2 misc_button_size = ivec2(460,64);


    Draw::Init();

    Graphics::ClearColor(fvec3(1));
    Graphics::Clear(Graphics::color);
    Draw::Accumulator::Overwrite();

    enum class State {main, real_imag, amplitude, phase, amplitude_log10, phase_log10, step};
    State cur_state = State::main;

    long double freq_min = Plot::default_min, freq_max = Plot::default_max;

    constexpr long double time_min_def = -5, time_max_def = 20;
    long double time_min = time_min_def, time_max = time_max_def;

    bool show_table_gui = 0;

    std::string message_text;
    float message_timer = 0;
    auto ShowMessage = [&](std::string text)
    {
        message_text = text;
        message_timer = message_timer_start;
    };

    bool show_about = 0;
    float about_screen_alpha = 0;
    constexpr float about_screen_alpha_step = 0.1;

    bool show_misc = 0;
    bool show_root_editor = 0;

    Expression e;
    try
    {
        e = Expression(default_expression);
    }
    catch (...) {}

    std::vector<complex_t> e_num_roots, e_den_roots;
    long double e_main_factor = 1;

    // If you change those, don't forget to also change them in lambda MakeTable() below.
    auto func_main  = [&e](long double t){return e.EvalVec({0,t});};
    auto func_real  = [&e](long double t){return ldvec2(t,e.EvalVec({0,t}).x);};
    auto func_imag  = [&e](long double t){return ldvec2(t,e.EvalVec({0,t}).y);};
    auto func_ampl  = [&e](long double t){return ldvec2(t,e.EvalAmplitude({0,t}));};
    auto func_phase = [&e](long double t){return ldvec2(t,e.EvalPhase({0,t}));};
    auto func_step  = [&e](long double t){return ldvec2(t,e.EvalStepResponse(t));};

    Plot plot;

    static auto PlotFlags = [&]() -> int
    {
        switch (cur_state)
        {
          case State::main:
            return Plot::lock_scale_ratio | Plot::has_horizontal_func;
          case State::phase:
            return Plot::vertical_pi;
          case State::amplitude_log10:
            return Plot::vertical_20log10 | Plot::horizontal_log10;
          case State::phase_log10:
            return Plot::vertical_pi | Plot::horizontal_log10;
          default:
            return 0;
        }
        return 0;
    };

    std::list<Button> buttons;
    std::list<TextField> text_fields, root_ed_text_fields;

    enum class InterfaceObj {func_input, range_input, scale_xy, scale, mode, export_data, misc, about, root_ed};

    bool need_interface_reset = 0;
    TextField *func_input = 0, *range_input_min = 0, *range_input_max = 0, *time_input_min = 0, *time_input_max = 0,
              *root_input_re = 0, *root_input_im = 0, *root_input_fac = 0;

    static auto SwapFreqLimitsIfNeeded = [&]
    {
        if (freq_min > freq_max)
        {
            std::swap(freq_min, freq_max);
            std::swap(range_input_min->value, range_input_max->value);
        }
    };

    constexpr float scale_factor = 1.012;

    auto RegenerateExprFromRoots = [&]
    {
        plot.ResetAccumulator();
        try
        {
            e = Expression(e_main_factor, e_num_roots, e_den_roots, &func_input->value);
            func_input->invalid = 0;
            func_input->invalid_pos = 0;
            func_input->invalid_text = "";
        }
        catch (Expression::Exception &exc)
        {
            e = {};
            func_input->invalid = 1;
            func_input->invalid_pos = exc.pos;
            func_input->invalid_text = exc.message;
        }
        need_interface_reset = 1;
    };

    long double root_ed_inp_re = 0, root_ed_inp_im = 0;

    static auto AddInterface = [&](InterfaceObj category)
    {
        constexpr int range_input_w = 64, input_gap_w = 32;

        switch (category)
        {
          case InterfaceObj::func_input:
            text_fields.push_back(TextField(ivec2(-1,-1), ivec2(24+range_input_w*2 + input_gap_w*2, 80), 9000, 1000, "Передаточная функция W(s)", "01234567890.,+-*/^()sj ", [&](TextField &ref, bool upd)
            {
                ref.width = win.Size().x - ref.pos.x - 24;
                if (upd)
                {
                    plot.ResetAccumulator();
                    try
                    {
                        e = Expression(ref.value);
                        e_num_roots = e.GetFracData().num_roots;
                        e_den_roots = e.GetFracData().den_roots;
                        e_main_factor = e.GetFracData().num_first_fac / e.GetFracData().den_first_fac;
                        if (root_input_fac)
                        {
                            char buffer[64];
                            std::snprintf(buffer, sizeof buffer, "%.6Lf", e_main_factor);
                            std::string sbuf = buffer;
                            while (sbuf.size() > 0 && (sbuf.back() == '0' || sbuf.back() == '.'))
                                sbuf.resize(sbuf.size() - 1);
                            root_input_fac->value = sbuf;
                        }
                        for (auto *ptr : {&e_num_roots, &e_den_roots})
                        {
                            auto it = ptr->begin();
                            while (it != ptr->end())
                            {
                                if (it->imag() < 0)
                                {
                                    it = ptr->erase(it);
                                    continue;
                                }
                                if (it->imag() == 0)
                                {
                                    *it = complex_t(it->real(), -1);
                                }
                                it++;
                            }
                        }
                        ref.invalid = 0;
                        ref.invalid_pos = 0;
                        ref.invalid_text = "";
                    }
                    catch (Expression::Exception &exc)
                    {
                        e = {};
                        e_main_factor = 1;
                        if (root_input_fac)
                            root_input_fac->value = "1";
                        e_num_roots = {};
                        e_den_roots = {};
                        ref.invalid = 1;
                        ref.invalid_pos = exc.pos;
                        ref.invalid_text = exc.message;
                    }
                    need_interface_reset = 1;
                }
            }));
            func_input = &text_fields.back();
            func_input->value = default_expression;
            func_input->with_cross_button = 1;
            func_input->fancy_input = 1;
            break;
          case InterfaceObj::range_input:
            {
                auto Lambda = [&, last_valid = std::string(), last_valid_cur = int()](TextField &ref, long double &param, bool upd) mutable
                {
                    if (!upd && ref.Active())
                    {
                        last_valid = ref.value;
                        last_valid_cur = Input::TextCursorPos();
                        return;
                    }
                    if (upd)
                    {
                        std::string value_copy = ref.value;
                        std::replace(value_copy.begin(), value_copy.end(), ',', '.');
                        bool ok = 1;
                        if (auto pos = value_copy.find_last_of('-'); pos != value_copy.npos && pos != 0)
                            ok = 0;
                        if (std::count(value_copy.begin(), value_copy.end(), '.') > 1)
                            ok = 0;
                        if (!ok)
                        {
                            ref.value = last_valid;
                            Input::SetTextCursorPos(last_valid_cur);
                        }
                        else
                        {
                            last_valid = ref.value;
                            last_valid_cur = Input::TextCursorPos();
                        }
                        param = 0;
                        Reflection::from_string(param, value_copy.c_str());
                        need_interface_reset = 1;
                    }
                };
                range_input_min = &text_fields.emplace_back(TextField(ivec2(-1,-1), ivec2(24, 80), 64, 6, "Мин. частота", "0123456789.,-", [&, Lambda](TextField &ref, bool upd) mutable
                {
                    Lambda(ref, freq_min, upd);
                }));
                text_fields.back().value = Reflection::to_string(Plot::default_min);
                range_input_max = &text_fields.emplace_back(TextField(ivec2(-1,-1), ivec2(24+range_input_w+input_gap_w, 80), 64, 6, "Макс. частота", "0123456789.,-", [&, Lambda](TextField &ref, bool upd) mutable
                {
                    Lambda(ref, freq_max, upd);
                }));
                text_fields.back().value = Reflection::to_string(Plot::default_max);

                time_input_min = &text_fields.emplace_back(TextField(ivec2(-1,-1), ivec2(24, 80), 64, 6, "Мин. время", "0123456789.,-", [&, Lambda](TextField &ref, bool upd) mutable
                {
                    Lambda(ref, time_min, upd);
                }));
                text_fields.back().value = Reflection::to_string(time_min_def);
                text_fields.back().visible = 0;
                time_input_max = &text_fields.emplace_back(TextField(ivec2(-1,-1), ivec2(24+range_input_w+input_gap_w, 80), 64, 6, "Макс. время", "0123456789.,-", [&, Lambda](TextField &ref, bool upd) mutable
                {
                    Lambda(ref, time_max, upd);
                }));
                text_fields.back().value = Reflection::to_string(time_max_def);
                text_fields.back().visible = 0;
            }
            break;
          case InterfaceObj::scale:
            {
                int x = -32;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 0, 0, "Сбросить расположение графика", [&]{plot.ResetOffsetAndScale();})); // Reset offset and scale
                x -= 48+8;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 2, 1, "Уменьшить масштаб", [&]{plot.Scale(ldvec2(1,1/scale_factor));})); // Decrease scale
                x -= 48;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 1, 1, "Увеличить масштаб", [&]{plot.Scale(ldvec2(1,scale_factor));})); // Increase scale
            }
            break;
          case InterfaceObj::scale_xy:
            {
                int x = -32;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 0, 0, "Сбросить расположение графика", [&]{plot.ResetOffsetAndScale();})); // Reset offset and scale
                x -= 48+8;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 6, 1, "Уменьшить масштаб по оси Y", [&]{plot.Scale(ldvec2(1,1/scale_factor));})); // Decrease Y scale
                x -= 48;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 5, 1, "Увеличить масштаб по оси Y", [&]{plot.Scale(ldvec2(1,scale_factor));})); // Increase Y scale
                x -= 48+8;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 4, 1, "Уменьшить масштаб по оси X", [&]{plot.Scale(ldvec2(1/scale_factor,1));})); // Decrease X scale
                x -= 48;
                buttons.push_back(Button(ivec2(1,-1), ivec2(x,32), 3, 1, "Увеличить масштаб по оси X", [&]{plot.Scale(ldvec2(scale_factor,1));})); // Increase X scale
            }
            break;
          case InterfaceObj::mode:
            {
                int x = 32;
                // Primary mode
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 7, 0, "Годограф амплитудно-фазовой характеристики", [&]{
                    cur_state = State::main;
                    need_interface_reset = 1;
                }));
                x += 48;
                // Real&imaginary mode
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 8, 0, "Действительная и мнимая части", [&]{
                    cur_state = State::real_imag;
                    need_interface_reset = 1;
                }));
                x += 48;
                // Amplitude mode
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 9, 0, "Амплитуда", [&]{
                    cur_state = State::amplitude;
                    need_interface_reset = 1;
                }));
                x += 48;
                // Phase mode
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 10, 0, "Фаза", [&]{
                    cur_state = State::phase;
                    need_interface_reset = 1;
                }));
                x += 48;
                // Amplitude log10 mode
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 11, 0, "Амплитуда (в децибелах) от логарифма частоты", [&]{
                    cur_state = State::amplitude_log10;
                    need_interface_reset = 1;
                }));
                x += 48;
                // Phase log10 mode
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 12, 0, "Фаза от логарифма частоты", [&]{
                    cur_state = State::phase_log10;
                    need_interface_reset = 1;
                }));
            }
            break;
          case InterfaceObj::export_data:
            {
                // Save values to table
                int x = 320+8;
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 13, 0, "Сохранить таблицу значений", [&]{
                    if (plot)
                        show_table_gui = 1;
                }));
                x += 48;
                buttons.push_back(Button(ivec2(-1,-1), ivec2(x,32), 14, 0, "Сохранить изображение", [&]{
                    constexpr fvec3 text_color(0);
                    constexpr ivec2 text_offset(6,4);
                    constexpr float text_bg_alpha = 0.8;

                    SwapFreqLimitsIfNeeded();

                    if (!plot)
                        return;
                    std::string file_name = "";
                    switch (cur_state)
                    {
                        case State::main:            file_name = "plot_main.png";            break;
                        case State::real_imag:       file_name = "plot_real_imag.png";       break;
                        case State::amplitude:       file_name = "plot_amplitude.png";       break;
                        case State::phase:           file_name = "plot_phase.png";           break;
                        case State::amplitude_log10: file_name = "plot_amplitude_log10.png"; break;
                        case State::phase_log10:     file_name = "plot_phase_log10.png";     break;
                        case State::step:            file_name = "plot_step_response.png";     break;
                    }

                    std::string text_func = func_input->value, text_min = range_input_min->value, text_max = range_input_max->value;
                    for (auto *it : {&text_func, &text_min, &text_max})
                        it->erase(std::remove(it->begin(), it->end(), ' '), it->end());
                    r.Text(Draw::min + plot.ViewportPos() + text_offset, "W(s) = " + text_func)
                     .color(text_color).font(font_small).align(ivec2(-1)).preset(Draw::WithWhiteBackground(text_bg_alpha));
                    r.Text(Draw::min + plot.ViewportPos() + text_offset.add_y(2+font_small.Height()), "Частоты от " + text_min + " до " + text_max + " рад/c")
                     .color(text_color).font(font_small).align(ivec2(-1)).preset(Draw::WithWhiteBackground(text_bg_alpha));
                    r.Finish();

                    Graphics::Image image(plot.ViewportSize());
                    glReadPixels(plot.ViewportPos().x, win.Size().y - plot.ViewportSize().y - plot.ViewportPos().y,
                                 plot.ViewportSize().x, plot.ViewportSize().y,
                                 GL_RGBA, GL_UNSIGNED_BYTE, (void *)image.Data());

                    Draw::Accumulator::Return();

                    for (int i = 0; i < image.Size().y/2; i++)
                    {
                        std::swap_ranges((u8vec4 *)image.Data() + image.Size().x * i,
                                         (u8vec4 *)image.Data() + image.Size().x * (i+1),
                                         (u8vec4 *)image.Data() + image.Size().x * (image.Size().y-1-i));
                    }

                    try
                    {
                        image.SaveToFile(file_name);
                        ShowMessage("Изображение сохранено в файл\n" + file_name);
                    }
                    catch (...)
                    {
                        ShowMessage("Не могу сохранить изображение в файл\n" + file_name);
                    }
                }));
            }
            break;
          case InterfaceObj::misc:
            buttons.push_back(Button(ivec2(-1,-1), ivec2(416+8*2,32), 15, 0, "Прочее", [&]{
                show_misc = 1;
            }));
            break;
          case InterfaceObj::about:
            buttons.push_back(Button(ivec2(-1,-1), ivec2(416+8*3+48,32), 16, 0, "О программе", [&]{
                show_about = 1;
            }));
            break;
          case InterfaceObj::root_ed:
            auto Lambda = [&, last_valid = std::string(), last_valid_cur = int()](TextField &ref, long double *param, bool upd) mutable
            {
                if (!upd && ref.Active())
                {
                    last_valid = ref.value;
                    last_valid_cur = Input::TextCursorPos();
                    return;
                }
                if (upd)
                {
                    std::string value_copy = ref.value;
                    std::replace(value_copy.begin(), value_copy.end(), ',', '.');
                    bool ok = 1;
                    if (auto pos = value_copy.find_last_of('-'); pos != value_copy.npos && pos != 0)
                        ok = 0;
                    if (std::count(value_copy.begin(), value_copy.end(), '.') > 1)
                        ok = 0;
                    if (!ok)
                    {
                        ref.value = last_valid;
                        Input::SetTextCursorPos(last_valid_cur);
                    }
                    else
                    {
                        last_valid = ref.value;
                        last_valid_cur = Input::TextCursorPos();
                    }
                    if (param)
                    {
                        *param = 0;
                        Reflection::from_string(*param, value_copy.c_str());
                    }
                    need_interface_reset = 1;
                }
            };
            root_input_re = &root_ed_text_fields.emplace_back(TextField(ivec2(1,1), ivec2(-280, -100), 120, 20, "Действ. часть", "0123456789.,-", [&, Lambda](TextField &ref, bool upd) mutable
            {
                Lambda(ref, &root_ed_inp_re, upd);
            }));
            root_ed_text_fields.back().value = "0";
            root_input_im = &root_ed_text_fields.emplace_back(TextField(ivec2(1,1), ivec2(-140, -100), 120, 20, "Мнимая часть", "0123456789.,-", [&, Lambda](TextField &ref, bool upd) mutable
            {
                Lambda(ref, &root_ed_inp_im, upd);
            }));
            root_ed_text_fields.back().value = "0";
            root_input_fac = &root_ed_text_fields.emplace_back(TextField(ivec2(-1,1), ivec2(30, -50), 100, 20, "Коэф. усиления", "0123456789.,-", [&, Lambda](TextField &ref, bool upd) mutable
            {
                Lambda(ref, 0, upd);

                if (upd)
                {
                    try
                    {
                        e_main_factor = std::stold(ref.value);
                        ref.invalid = 0;
                    }
                    catch (...)
                    {
                        e_main_factor = 1;
                        ref.invalid = 1;
                    }
                    RegenerateExprFromRoots();
                }
            }));
            root_ed_text_fields.back().value = "1";
            break;
        }
    };

    auto ResetInterface = [&]()
    {
        if (!bool(e))
            plot = Plot(PlotFlags());
        else
        {
            switch (cur_state)
            {
              case State::main:
                plot = Plot({{func_main, plot_color, "W(j·w)"}}, freq_min, freq_max, PlotFlags());
                break;
              case State::real_imag:
                plot = Plot({{func_real, fvec3(0.9,0.2,0), "P(w)"},{func_imag, fvec3(0.2,0.7,0), "Q(w)"}}, freq_min, freq_max, PlotFlags());
                break;
              case State::amplitude:
                plot = Plot({{func_ampl, fvec3(0.9,0.4,0), "A(w)"}}, freq_min, freq_max, PlotFlags());
                break;
              case State::phase:
                plot = Plot({{func_phase, fvec3(0,0.7,0.9), "ф(w)"}}, freq_min, freq_max, PlotFlags());
                break;
              case State::amplitude_log10:
                plot = Plot({{func_ampl, fvec3(0.9,0.4,0), "20·log{10} A(w)"}}, freq_min, freq_max, PlotFlags());
                break;
              case State::phase_log10:
                plot = Plot({{func_phase, fvec3(0,0.7,0.9), "ф(w)"}}, freq_min, freq_max, PlotFlags());
                break;
              case State::step:
                plot = Plot({{func_step, fvec3(0,0,0), "h(t)"}}, time_min, time_max, PlotFlags());
                {
                    long double dst = e.EvalVec({0,0}).x;

                    bool stable = 1;
                    const auto &frac = e.GetFracData();
                    if (frac.cant_find_den_roots)
                    {
                        stable = 0;
                    }
                    else
                    {
                        for (const auto &it : frac.den_roots)
                        {
                            if (it.real() >= 0)
                            {
                                stable = 0;
                                break;
                            }
                        }
                    }

                    //std::cout << dst << '\n';
                    if (stable && std::isfinite(dst))
                    {
                        plot.draw_area = 1;
                        plot.area_mid = dst;
                        plot.area_a = dst*(1-0.05);
                        plot.area_b = dst*(1+0.05);
                        plot.ResetAccumulator();
                    }
                }
                break;
            }
        }

        buttons = {};

        AddInterface(cur_state == State::main ? InterfaceObj::scale : InterfaceObj::scale_xy);
        AddInterface(InterfaceObj::mode);
        AddInterface(InterfaceObj::export_data);
        AddInterface(InterfaceObj::misc);
        AddInterface(InterfaceObj::about);

        range_input_min->visible = (cur_state != State::step);
        range_input_max->visible = (cur_state != State::step);
        time_input_min->visible = (cur_state == State::step);
        time_input_max->visible = (cur_state == State::step);
    };
    AddInterface(InterfaceObj::func_input);
    AddInterface(InterfaceObj::range_input);
    AddInterface(InterfaceObj::root_ed);
    ResetInterface();

    int table_len_input_value = 101;
    bool table_gui_button_hovered_l = 0,
         table_gui_button_hovered_r = 0;
    TextField table_len_input(ivec2(0,0), -table_gui_rect_size/2 + table_gui_offset + ivec2(0, 56), 64, 4, "Количество точек", "0123456789", [&](TextField &ref, bool upd)
    {
        if (upd)
        {
            if (!ref.value.size())
            {
                table_len_input_value = 0;
                ref.invalid = 1;
                ref.invalid_text = "Выберите количество точек";
                return;
            }
            Reflection::from_string(table_len_input_value, ref.value.c_str());
            if (table_len_input_value < 2)
            {
                ref.invalid = 1;
                ref.invalid_text = "Число должно быть больше единицы";
                return;
            }
            ref.invalid = 0;
            ref.invalid_text = "";
        }
    });
    table_len_input.value = Reflection::to_string(table_len_input_value);

    auto MakeTable = [&]
    {
        constexpr int column_w = 15, precision = 6;

        SwapFreqLimitsIfNeeded();

        std::string file_name = "table.txt";

        std::ofstream out(file_name);
        if (!out)
        {
            ShowMessage("Не могу записать таблицу в файл\n" + file_name);
            return;
        }

        std::string text_func = func_input->value, text_min = range_input_min->value, text_max = range_input_max->value;
        for (auto *it : {&text_func, &text_min, &text_max})
            it->erase(std::remove(it->begin(), it->end(), ' '), it->end());
        out << "W(s) = " + text_func << '\n'
            << "Частоты от " + text_min + " до " + text_max + " рад/c\n"
            << "Количество точек: " << table_len_input_value << "\n\n";

        out << std::setw(column_w) << "w"
            << std::setw(column_w) << "P(w)"
            << std::setw(column_w) << "Q(w)"
            << std::setw(column_w) << "A(w)"
            << std::setw(column_w+1) << "ф(w)" // `+1` because cyrillic `ф` is two bytes.
            << std::setw(column_w) << "log10(w)"
            << std::setw(column_w) << "20*log10(A)" << "\n\n";

        for (int i = 0; i < table_len_input_value; i++)
        {
            long double freq = i / (long double)(table_len_input_value-1) * (freq_max - freq_min) + freq_min;
            ldvec2 vec = func_main(freq);
            long double ampl = func_ampl(freq).y, phase = func_phase(freq).y / ld_pi;
            out << ' ' << std::setw(column_w-1) << std::setprecision(precision) << freq
                << ' ' << std::setw(column_w-1) << std::setprecision(precision) << vec.x
                << ' ' << std::setw(column_w-1) << std::setprecision(precision) << vec.y
                << ' ' << std::setw(column_w-1) << std::setprecision(precision) << ampl
                << ' ' << std::setw(column_w-2) << std::setprecision(precision) << phase << "п"
                << ' ' << std::setw(column_w-1) << std::setprecision(precision) << std::log10(freq)
                << ' ' << std::setw(column_w-1) << std::setprecision(precision) << 20*std::log10(ampl) << '\n';
        }

        out << "\n\n"
            << "h(t)\n"
            << "Время от " + time_input_min->value + " до " + time_input_max->value << " c\n"
            << "Количество точек: " << table_len_input_value << "\n\n";
        out << std::setw(column_w) << "t"
            << std::setw(column_w) << "h(t)" << "\n\n";

        for (int i = 0; i < table_len_input_value; i++)
        {
            long double time = i / (long double)(table_len_input_value-1) * (time_max - time_min) + time_min;
            out << ' ' << std::setw(column_w-1) << std::setprecision(precision) << time
                << ' ' << std::setw(column_w-1) << std::setprecision(precision) << func_step(time).y << '\n';
        }

        if (!out)
        {
            ShowMessage("Не могу записать таблицу в файл\n" + file_name);
            return;
        }

        ShowMessage("Таблица сохранена в файл\n" + file_name);
    };

    auto RenderAbout = [&]
    {
        constexpr ivec2 box_size(416,154);
        constexpr fvec3 text_color(0);
        r.Quad(ivec2(0), box_size).tex(ivec2(0,192)).center().alpha(about_screen_alpha);
        r.Text(-box_size/2 + 16, "[TAU++] Частотные характеристики\n"
                                 "\n"
                                 "Версия " VERSION "\n"
                                 "Билд от " __DATE__ ", " __TIME__ "\n"
                                 "\n"
                                 "Код: Егор Михайлов\n"
                                 "Тестирование: Евгений Холупко, Дмитрий Бутаков\n"
                                 "\n"
                                 "https://github.com/HolyBlackCat/tau_plus_plus").alpha(about_screen_alpha).color(text_color).align(ivec2(-1)).font(font_small);
    };

    struct MiscButton
    {
        std::string text;
        std::function<void()> func;
    };
    std::vector<MiscButton> misc_buttons
    {
        { "Построить переходную функцию h(t)", [&]
            {
                show_misc = 0;
                cur_state = State::step;
                need_interface_reset = 1;
            }
        },
        { "Нули и полюса", [&]
            {
                show_misc = 0;
                show_root_editor = 1;
            }
        },
    };

    auto MiscMenuTick = [&](bool &button_pressed)
    {
        if (Keys::escape.pressed())
        {
            show_misc = 0;
            return;
        }
        if ((abs(mouse.pos() - ivec2(Draw::max.x - 24, Draw::min.y + 24)) <= 24).all() && button_pressed)
        {
            show_misc = 0;
            button_pressed = 0;
            return;
        }

        int y_pos = -int(misc_buttons.size()) * misc_button_size.y/2;
        for (int i = 0; i < int(misc_buttons.size()); i++)
        {
            if ((abs(mouse.pos() - ivec2(0, y_pos + (i+0.5) * misc_button_size.y)) < misc_button_size/2).all() && button_pressed)
            {
                button_pressed = 0;
                misc_buttons[i].func();
            }
        }

        button_pressed = 0;
    };
    auto MiscMenuRender = [&]
    {
        Draw::Overlay(1);
        if ((abs(mouse.pos() - ivec2(Draw::max.x - 24, Draw::min.y + 24)) <= 24).all())
            r.Quad(ivec2(Draw::max.x - 24, Draw::min.y + 24), ivec2(44)).color(fvec3(1)).alpha(0.15).center();
        r.Quad(ivec2(Draw::max.x - 24, Draw::min.y + 24), ivec2(48)).tex(ivec2(96,32)).center();

        int y_pos = -int(misc_buttons.size()) * misc_button_size.y/2;
        for (int i = 0; i < int(misc_buttons.size()); i++)
        {
            bool hovered = (abs(mouse.pos() - ivec2(0, y_pos + (i+0.5) * misc_button_size.y)) < misc_button_size/2).all();
            r.Quad(ivec2(0, y_pos + (i+0.5) * misc_button_size.y), misc_button_size-16).color(fvec3(0)).alpha(0.5).center();
            if (hovered)
                r.Quad(ivec2(0, y_pos + (i+0.5) * misc_button_size.y), misc_button_size-16).color(fvec3(1)).alpha(0.1).center();

            r.Text(ivec2(0, y_pos + (i+0.5) * misc_button_size.y), misc_buttons[i].text).color(fvec3(1));
        }
    };

    int root_ed_num_sel = -1, root_ed_den_sel = -1;
    int root_ed_add_sel = -1;

    auto RootEditorTick = [&](bool &button_pressed)
    {
        // Text fields
        for (auto &text_field : root_ed_text_fields)
            text_field.Tick(button_pressed);

        if (Keys::escape.pressed())
        {
            show_root_editor = 0;
            return;
        }
        if ((abs(mouse.pos() - ivec2(Draw::max.x - 24, Draw::min.y + 24)) <= 24).all() && button_pressed)
        {
            show_root_editor = 0;
            button_pressed = 0;
            return;
        }

        root_ed_num_sel = -1;
        for (int i = 0; i < int(e_num_roots.size()); i++)
        {
            if ((abs(mouse.pos() - ivec2(Draw::min.x/2, Draw::min.y + 32 + i*16)) <= ivec2(200,16)/2).all())
            {
                root_ed_num_sel = i;
                break;
            }
        }

        root_ed_den_sel = -1;
        for (int i = 0; i < int(e_den_roots.size()); i++)
        {
            if ((abs(mouse.pos() - ivec2(Draw::max.x/2, Draw::min.y + 32 + i*16)) <= ivec2(200,16)/2).all())
            {
                root_ed_den_sel = i;
                break;
            }
        }

        root_ed_add_sel = -1;
        if ((abs(mouse.pos() - ivec2(Draw::max.x - 220, Draw::max.y - 40)) <= ivec2(50,20)).all())
            root_ed_add_sel = 0;
        else if ((abs(mouse.pos() - ivec2(Draw::max.x - 80, Draw::max.y - 40)) <= ivec2(50,20)).all())
            root_ed_add_sel = 1;

        if ((root_ed_num_sel != -1 || root_ed_den_sel != -1) && button_pressed)
        {
            button_pressed = 0;

            if (root_ed_num_sel != -1)
                e_num_roots.erase(e_num_roots.begin() + root_ed_num_sel);
            else
                e_den_roots.erase(e_den_roots.begin() + root_ed_den_sel);

            RegenerateExprFromRoots();
        }

        if (root_ed_add_sel != -1 && button_pressed)
        {
            button_pressed = 0;

            std::vector<complex_t> &dst = (root_ed_add_sel == 0 ? e_num_roots : e_den_roots);

            if (dst.size() < 25)
            {
                if (root_ed_inp_im == 0)
                    dst.push_back({root_ed_inp_re, -1});
                else
                    dst.push_back({root_ed_inp_re, abs(root_ed_inp_im)});
            }

            RegenerateExprFromRoots();
        }

        button_pressed = 0;
    };
    auto RootEditorRender = [&]
    {
        Draw::Overlay(1);
        if ((abs(mouse.pos() - ivec2(Draw::max.x - 24, Draw::min.y + 24)) <= 24).all())
            r.Quad(ivec2(Draw::max.x - 24, Draw::min.y + 24), ivec2(44)).color(fvec3(1)).alpha(0.15).center();
        r.Quad(ivec2(Draw::max.x - 24, Draw::min.y + 24), ivec2(48)).tex(ivec2(96,32)).center();

        r.Text(ivec2(Draw::min.x/2, Draw::min.y+16), "Нули").color(fvec3(1)).font(font_small);
        r.Text(ivec2(Draw::max.x/2, Draw::min.y+16), "Полюса").color(fvec3(1)).font(font_small);


        for (size_t i = 0; i < e_num_roots.size(); i++)
        {
            char buffer[64];
            if (e_num_roots[i].imag() < -0.5)
                std::snprintf(buffer, sizeof buffer, "%.6Lg", e_num_roots[i].real());
            else
                std::snprintf(buffer, sizeof buffer, "%.6Lg +-j %.6Lg", e_num_roots[i].real(), e_num_roots[i].imag());
            r.Text(ivec2(Draw::min.x/2 - 96, Draw::min.y + 32 + i*16), buffer).color(int(i) == root_ed_num_sel ? fvec3(1,0,0) : fvec3(1)).font(font_small).align_h(-1);
        }
        for (size_t i = 0; i < e_den_roots.size(); i++)
        {
            char buffer[64];
            if (e_den_roots[i].imag() < -0.5)
                std::snprintf(buffer, sizeof buffer, "%.6Lg", e_den_roots[i].real());
            else
                std::snprintf(buffer, sizeof buffer, "%.6Lg +-j %.6Lg", e_den_roots[i].real(), e_den_roots[i].imag());
            r.Text(ivec2(Draw::max.x/2 - 96, Draw::min.y + 32 + i*16), buffer).color(int(i) == root_ed_den_sel ? fvec3(1,0,0) : fvec3(1)).font(font_small).align_h(-1);
        }

        r.Text(ivec2(Draw::max.x - 220, Draw::max.y - 40), "Добавить\nнуль").color(fvec3(1)).font(font_small);
        r.Text(ivec2(Draw::max.x - 80, Draw::max.y - 40), "Добавить\nполюс").color(fvec3(1)).font(font_small);
        if (root_ed_add_sel == 0)
            r.Quad(ivec2(Draw::max.x - 220, Draw::max.y - 40), ivec2(100,40)).color(fvec3(1)).alpha(0.2).center();
        else if (root_ed_add_sel == 1)
            r.Quad(ivec2(Draw::max.x - 80, Draw::max.y - 40), ivec2(100,40)).color(fvec3(1)).alpha(0.2).center();

        if (root_ed_num_sel != -1)
            r.Text(ivec2(0, Draw::max.y-80), "Нажмите чтобы удалить нуль").color(fvec3(1,0,0));
        else if (root_ed_den_sel != -1)
            r.Text(ivec2(0, Draw::max.y-80), "Нажмите чтобы удалить полюс").color(fvec3(1,0,0));

        if (e.CantFindRoots())
            r.Text(ivec2(0, Draw::max.y-20), "Нули и/или полюса не могут быть определены").color(fvec3(1,0,0)).font(font_small);

        // Text fields
        for (auto &text_field : root_ed_text_fields)
            text_field.Render(1);
    };

    auto Tick = [&]
    {
        Draw::Accumulator::Return();

        bool button_pressed = mouse.left.pressed();

        // Disable text fields on click
        if (button_pressed)
            TextField::Deactivate();

        // About
        if (show_about)
        {
            about_screen_alpha += about_screen_alpha_step;
            if (about_screen_alpha > 1)
                about_screen_alpha = 1;

            if (button_pressed || Keys::enter.pressed() || Keys::escape.pressed())
            {
                show_about = 0;
                button_pressed = 0;
            }
        }
        else
        {
            about_screen_alpha -= about_screen_alpha_step;
            if (about_screen_alpha < 0)
                about_screen_alpha = 0;
        }

        // Table creation GUI
        if (show_table_gui)
        {
            table_len_input.Tick(button_pressed);

            table_gui_button_hovered_l = abs(mouse.pos().x + table_gui_rect_size.x/4) <= table_gui_rect_size.x/4 && abs(mouse.pos().y - table_gui_rect_size.y/2 + table_gui_button_h/2) <= table_gui_button_h/2;
            table_gui_button_hovered_r = abs(mouse.pos().x - table_gui_rect_size.x/4) <= table_gui_rect_size.x/4 && abs(mouse.pos().y - table_gui_rect_size.y/2 + table_gui_button_h/2) <= table_gui_button_h/2;

            if (((button_pressed && table_gui_button_hovered_l) || Keys::enter.pressed()) && !table_len_input.invalid)
            {
                MakeTable();
                show_table_gui = 0;
            }
            if ((button_pressed && table_gui_button_hovered_r) || Keys::escape.pressed())
            {
                show_table_gui = 0;
            }

            button_pressed = 0;
        }

        // Misc menu
        if (show_misc)
            MiscMenuTick(button_pressed);

        // Root editor
        if (show_root_editor)
            RootEditorTick(button_pressed);


        // Text fields
        for (auto &text_field : text_fields)
            text_field.Tick(button_pressed);

        // Buttons
        Button::ResetTooltip();
        for (auto &button : buttons)
            button.Tick(button_pressed, show_table_gui || show_misc || show_root_editor);

        // Interface reset if needed
        if (need_interface_reset)
        {
            need_interface_reset = 0;
            ResetInterface();
        }

        // Swap min and max frequency if they are in the wrong order
        if (!range_input_min->Active() && !range_input_max->Active())
            SwapFreqLimitsIfNeeded();

        // Ignore clicks on interface rectangle
        if (mouse.pos().y + win.Size().y/2 < interface_rect_height)
            button_pressed = 0;

        // Move plot
        if (button_pressed)
            plot.Grab();

        // Plot tick
        plot.Tick(127); // This should be `2^n - 1` for plot to look pretty while moving.

        // Message tick
        if (message_timer > 0)
            message_timer--;

        Draw::Accumulator::Overwrite();
    };
    auto Render = [&]
    {
        constexpr float interface_rect_alpha = 0.95,
                        interface_rect_alpha2 = 0.75;

        Draw::Accumulator::Return();

        // Interface background
        r.Quad(Draw::min, ivec2(win.Size().x, interface_rect_height)).color(fvec3(0.94, 0.94, 0.90)).alpha(interface_rect_alpha, interface_rect_alpha, interface_rect_alpha2, interface_rect_alpha2);
        r.Quad(Draw::min.add_y(interface_rect_height), ivec2(win.Size().x, 1)).color(fvec3(0));

        // Text fields
        for (auto &text_field : text_fields)
            text_field.Render();

        // Mode indicator
        if (cur_state != State::step)
            r.Quad(-win.Size()/2 + ivec2(32).add_x(48 * int(cur_state)), ivec2(50)).color(fvec3(0)).center();

        // Warnings
        if ((cur_state == State::phase || cur_state == State::phase_log10) && e.CantFindRoots())
        {
            ivec2 pos(0,win.Size().y/2 * 3/4);
            r.Quad(pos, ivec2(win.Size().x*0.82, 32)).color(fvec3(1)).alpha(0.8).center();
            r.Text(pos, "Нули и/или полюса передаточной функции не могут быть определены. Фаза вычисляется по модулю 2п.").font(font_small).color(fvec3(0.9,0.2,0));
        }

        // Buttons
        for (const auto &button : buttons)
            button.Render();

        { // Button tooltips
            constexpr int tooltip_box_height = 32,
                          tooltip_box_gap = 32;
            constexpr fvec3 tooltip_box_line_color = fvec3(0.3),
                            tooltip_box_fill_color = fvec3(0.95),
                            tooltip_box_text_color = fvec3(0);

            if (Button::CurrentTooltip().size() > 0)
            {
                r.Quad(ivec2(Draw::min.x, Draw::max.y - tooltip_box_height - tooltip_box_gap), ivec2(win.Size().x, tooltip_box_height)).color(tooltip_box_fill_color);
                r.Quad(ivec2(Draw::min.x, Draw::max.y - tooltip_box_height - tooltip_box_gap), ivec2(win.Size().x, 1)).color(tooltip_box_line_color);
                r.Quad(ivec2(Draw::min.x, Draw::max.y - tooltip_box_height), ivec2(win.Size().x, 1)).color(tooltip_box_line_color);
                r.Text(ivec2(0, Draw::max.y - tooltip_box_height/2 - tooltip_box_gap), Button::CurrentTooltip()).color(tooltip_box_text_color);
            }
        }

        // Table creation GUI
        if (show_table_gui)
        {
            constexpr fvec3 rect_color(0.95), rect_color2(0.875), rect_frame_color(0), title_color(0), text_color(0),
                            button_color(0), button_color_inact(0.5), button_color_back(1), button_color_frame(0.75);
            constexpr int button_margin = 2, rect_frame_width = 8;
            Draw::Overlay();
            r.Quad(ivec2(0), table_gui_rect_size+rect_frame_width*2).color(rect_frame_color).center().alpha(1/3.);
            r.Quad(ivec2(0), table_gui_rect_size+2).color(rect_frame_color).center();
            r.Quad(ivec2(0), table_gui_rect_size).color(rect_color, rect_color, rect_color2, rect_color2).center();
            r.Text(ivec2(0,-table_gui_rect_size.y/2), "Создание таблицы").color(title_color).align_v(-1);
            r.Quad(ivec2(0,-table_gui_rect_size.y/2+font_main.Height()), ivec2(table_gui_rect_size.x+2,1)).color(rect_frame_color).center();
            table_len_input.Render();
            r.Text(-table_gui_rect_size/2 + table_gui_offset, Str("Начальная частота:\t", freq_min, "\n"
                                                                  "Конечная частота: \t", freq_max)).font(font_small).color(text_color).align(ivec2(-1));
            r.Text(-table_gui_rect_size/2 + table_gui_offset + ivec2(0,108), Str("Шаг частоты: \t\t\t", table_len_input.invalid ? "?" : Str((freq_max - freq_min)/(table_len_input_value-1)) )).font(font_small).color(text_color).align(ivec2(-1));

            if (table_gui_button_hovered_l)
            {
                r.Quad(ivec2(-table_gui_rect_size.x/4, table_gui_rect_size.y/2-table_gui_button_h/2), ivec2(table_gui_rect_size.x/2,table_gui_button_h)-button_margin*2 + 2).color(button_color_frame).center();
                r.Quad(ivec2(-table_gui_rect_size.x/4, table_gui_rect_size.y/2-table_gui_button_h/2), ivec2(table_gui_rect_size.x/2,table_gui_button_h)-button_margin*2)
                 .color(button_color_back,button_color_back,button_color_frame,button_color_frame).center();
            }
            if (table_gui_button_hovered_r)
            {
                r.Quad(ivec2(table_gui_rect_size.x/4, table_gui_rect_size.y/2-table_gui_button_h/2), ivec2(table_gui_rect_size.x/2,table_gui_button_h)-button_margin*2 + 2).color(button_color_frame).center();
                r.Quad(ivec2(table_gui_rect_size.x/4, table_gui_rect_size.y/2-table_gui_button_h/2), ivec2(table_gui_rect_size.x/2,table_gui_button_h)-button_margin*2)
                 .color(button_color_back,button_color_back,button_color_frame,button_color_frame).center();
            }

            r.Text((table_gui_rect_size/2).mul_x(-1) + ivec2(table_gui_rect_size.x/4, -table_gui_button_h/2), "Создать").color(table_len_input.invalid ? button_color_inact : button_color);
            r.Text(table_gui_rect_size/2 - ivec2(table_gui_rect_size.x/4, table_gui_button_h/2), "Отмена").color(button_color);
        }

        // Misc menu
        if (show_misc)
            MiscMenuRender();

        // Root editor
        if (show_root_editor)
            RootEditorRender();


        // About
        if (about_screen_alpha > 0)
            RenderAbout();

        // Message
        if (message_timer > 0)
        {
            constexpr fvec3 frame_color(0), rect_color(0.92), text_color(0);
            constexpr ivec2 rect_size(480,80);
            constexpr int frame_w = 8;
            float alpha = min(message_timer / float(message_alpha_time), 1);
            r.Quad(ivec2(0), rect_size + 2*(frame_w)).color(frame_color).center().alpha(pow(alpha, 6) * 0.5);
            r.Quad(ivec2(0), rect_size + 2).color(frame_color).center().alpha(pow(alpha, 3));
            r.Quad(ivec2(0), rect_size).color(rect_color).center().alpha(alpha);
            r.Text(ivec2(0), message_text).color(text_color).alpha(alpha);
        }
    };

    uint64_t frame_start = Timing::Clock();

    while (1)
    {
        uint64_t time = Timing::Clock(), frame_delta = time - frame_start;
        frame_start = time;

        while (tick_stabilizer.Tick(frame_delta))
        {
            Events::Process();
            if (win.size_changed)
            {
                win.size_changed = 0;
                Graphics::Viewport(win.Size());
                Draw::HandleResize();
                plot.RecalculateDefaultOffsetAndScale();
                plot.ResetAccumulator();
            }
            Tick();
        }

        Graphics::CheckErrors();

        Render();
        r.Finish();

        win.Swap();
    }
}
