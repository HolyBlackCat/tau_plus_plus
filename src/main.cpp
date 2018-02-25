#include "everything.h"

#include <complex>
#include <iostream>
#include <list>

Events::AutoErrorHandlers error_handlers;

Window win("TAU++", ivec2(800,600), Window::Settings{}.GlVersion(2,1).GlProfile(Window::any_profile).Resizable().MinSize(ivec2(640,480)));
Timing::TickStabilizer tick_stabilizer(60);

Renderers::Poly2D r;
Input::Mouse mouse;

Graphics::Texture tex_main(Graphics::Texture::linear);
Graphics::CharMap font_main;
Graphics::Font font_object_main;


namespace Draw
{
    namespace Accumulator
    {
        void Clear()
        {
            glClear(GL_ACCUM_BUFFER_BIT);
        }
        void Overwrite()
        {
            glAccum(GL_LOAD, 1);
        }
        void Add()
        {
            glAccum(GL_ACCUM, 1);
        }
        void Return()
        {
            glAccum(GL_RETURN, 1);
        }

        void ModifyAdd(float value)
        {
            glAccum(GL_ADD, value);
        }
        void ModifyMul(float value)
        {
            glAccum(GL_MULT, value);
        }
    }

    ivec2 min, max;

    void HandleResize()
    {
        min = -win.Size() / 2;
        max = win.Size() + min;

        r.SetMatrix(fmat4::ortho(ivec2(min.x, max.y), ivec2(max.x, min.y), -1, 1));
        mouse.Transform(win.Size()/2, 1);
    }

    void Init()
    {
        Graphics::Blending::Enable();
        Graphics::Blending::FuncNormalPre();

        Graphics::Image texture_image_main("assets/texture.png");

        font_object_main.Create("assets/Xolonium-Regular.ttf", 20);

        Graphics::Font::MakeAtlas(texture_image_main, ivec2(0,1024-64), ivec2(1024,64),
        {
            {font_object_main, font_main, Graphics::Font::normal, Strings::Encodings::cp1251()},
        });

        tex_main.SetData(texture_image_main);

        r.Create(0x1000);
        r.SetTexture(tex_main);
        r.SetDefaultFont(font_main);
        Draw::HandleResize();
        r.BindShader();
    }

    void Dot(int type, fvec2 pos, fvec3 color, float alpha = 1, float beta = 1)
    {
        r.Quad(pos, ivec2(14)).tex(ivec2(1 + 16 * type,1)).center().color(color).mix(0).alpha(alpha).beta(beta);
    }
    void Line(int type, fvec2 a, fvec2 b, fvec3 color, float alpha = 1, float beta = 1)
    {
        int len = iround((b - a).len());

        for (int i = 0; i <= len; i++)
            Dot(type, a + (b - a) * i / len, color, alpha, beta);
    }
}


class Expression
{
  public:
    using complex_t = std::complex<long double>;

  private:
    struct Token
    {
        enum Type {num, lparen, rparen, op, var, imag_unit};
        enum Operator {plus, minus, mul, fake_mul, div, pow, left_paren};

        Type type;

        int starts_at;

        union
        {
            Operator op_type;
            long double num_value;
        };

        std::string ToString() const
        {
            switch (type)
            {
              case num:
                return std::to_string(num_value);
              case lparen:
                return "(";
              case rparen:
                return ")";
              case var:
                return "x";
              case imag_unit:
                return "j";
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
    static complex_t (*OperatorFunc(Token::Operator op))(complex_t, complex_t)
    {
        switch (op)
        {
          case Token::plus:
            return [](complex_t a, complex_t b){return a + b;};
          case Token::minus:
            return [](complex_t a, complex_t b){return a - b;};
          case Token::mul:
          case Token::fake_mul:
            return [](complex_t a, complex_t b){return a * b;};
          case Token::div:
            return [](complex_t a, complex_t b){return a / b;};
          case Token::pow:
            return [](complex_t a, complex_t b){return std::pow(a, b);};
          case Token::left_paren:
            return 0;
        }
        return 0;
    }

    static bool Tokenize(std::string_view str, char var_name, std::list<Token> *list, int *error_pos, std::string *error_msg)
    {
        DebugAssert("`j` is reserved for the imaginary unit.", var_name != 'j');

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
                if (ch >= '0' && ch <= '9')
                {
                    std::string num;
                    num += ch;
                    while (1)
                    {
                        ch = str[++index];
                        if ((ch >= '0' && ch <= '9') || ch == '.' || ch == ',')
                            num += (ch == ',' ? '.' : ch);
                        else
                            break;
                    }

                    long double value;
                    if (auto end = Reflection::from_string(value, num.c_str()); end == num.c_str() + num.size())
                    {
                        token.type = Token::num;
                        token.num_value = value;
                        ret.push_back(token);
                        break;
                    }
                    else
                    {
                        index = token.starts_at;
                    }
                }
                if (ch == var_name && (str[index+1] < 'a' || str[index+1] > 'z') && (str[index+1] < 'A' || str[index+1] > 'Z'))
                {
                    token.type = Token::var;
                    ret.push_back(token);
                    index++;
                    break;
                }
                if (ch == 'j' && (str[index+1] < 'a' || str[index+1] > 'z') && (str[index+1] < 'A' || str[index+1] > 'Z'))
                {
                    token.type = Token::imag_unit;
                    ret.push_back(token);
                    index++;
                    break;
                }

                *error_pos = index;
                *error_msg = Str("В выражении могут встречаться только числа, мнимая единица `j`, переменная `", var_name, "`, скобки и операции.");
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
            switch (it->type)
            {
              case Token::lparen:
                paren_stack.push_back(it->starts_at);
                [[fallthrough]];
              case Token::num:
              case Token::var:
              case Token::imag_unit:
                if (it != prev)
                {
                    if (prev->type != Token::lparen && prev->type != Token::op)
                    {
                        *error_pos = it->starts_at;
                        *error_msg = "Пропущена операция.";
                        return 0;
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
                break;
              case Token::op:
                if ((it == prev || prev->type == Token::lparen || prev->type == Token::op) && (it->op_type == Token::plus || it->op_type == Token::minus))
                {
                    if (it->op_type == Token::plus)
                    {
                        it = list.erase(it);
                    }
                    else
                    {
                        Token new_token;
                        new_token.starts_at = it->starts_at;
                        new_token.op_type = Token::fake_mul;
                        it = list.insert(it, new_token);
                        new_token.type = Token::num;
                        new_token.num_value = -1;
                        it = list.insert(it, new_token);
                    }
                    break;
                }
                if (it == prev || prev->type == Token::op || prev->type == Token::lparen)
                {
                    *error_pos = it->starts_at;
                    *error_msg = "Пропущено число или переменная.";
                    return 0;
                }
                break;
            }
            prev = it;
            it++;
        }

        if (list.size() && list.back().type == Token::op)
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

        Type type;

        union
        {
            complex_t (*op_func)(complex_t, complex_t);
            struct
            {
                long double real, imag;
            }
            num_value;
        };
    };

    std::vector<Element> elements;


    static bool ParseExpression(const std::list<Token> &tokens, std::vector<Element> *elems)
    {
        // Shunting-yard algorithm

        std::vector<Token::Operator> op_stack;

        for (const auto &token : tokens)
        {
            switch (token.type)
            {
              case Token::num:
                {
                    Element el;
                    el.type = Element::num;
                    el.num_value.real = token.num_value;
                    el.num_value.imag = 0;
                    elems->push_back(el);
                }
                break;
              case Token::imag_unit:
                {
                    Element el;
                    el.type = Element::num;
                    el.num_value.real = 0;
                    el.num_value.imag = 1;
                    elems->push_back(el);
                }
                break;
              case Token::var:
                {
                    Element el;
                    el.type = Element::var;
                    elems->push_back(el);
                }
                break;
              case Token::op:
                {
                    int this_prec = Precedence(token.op_type);
                    Element el;
                    el.type = Element::op;
                    while (op_stack.size() > 0 && op_stack.back() != Token::left_paren && (this_prec < Precedence(op_stack.back()) || (this_prec == Precedence(op_stack.back()) && !IsRightAssociative(token.op_type))))
                    {
                        el.op_func = OperatorFunc(op_stack.back());
                        elems->push_back(el);
                        op_stack.pop_back();
                    }
                    op_stack.push_back(token.op_type);
                }
                break;
              case Token::lparen:
                op_stack.push_back(Token::left_paren);
                break;
              case Token::rparen:
                while (1)
                {
                    if (op_stack.empty())
                        return 0;

                    bool lparen_found = (op_stack.back() == Token::left_paren);
                    if (!lparen_found)
                    {
                        Element el;
                        el.type = Element::op;
                        el.op_func = OperatorFunc(op_stack.back());
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
            if (op_stack.back() == Token::left_paren)
                return 0;
            Element el;
            el.type = Element::op;
            el.op_func = OperatorFunc(op_stack.back());
            elems->push_back(el);
            op_stack.pop_back();
        }

        return 1;
    }

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
                throw std::runtime_error("Неправильное выражение.");
        }
        else
        {
            throw std::runtime_error(Str(std::string(err_pos, ' '), "^ ", err_msg));
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
                stack.push_back({elem.num_value.real, elem.num_value.imag});
                break;
              case Element::var:
                stack.push_back(variable);
                break;
              case Element::op:
                {
                    if (stack.size() < 2)
                        throw std::runtime_error("Ошибка при вычислении.");
                    complex_t result = elem.op_func(stack[stack.size()-2], stack.back());
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
};


class Plot
{
    static constexpr float window_margin = 0.4;

    using func_t = std::function<ldvec2(long double)>;
    using distr_t = std::exponential_distribution<long double>;

    func_t func;
    int flags;

    ldvec2 offset = ivec2(0);
    ldvec2 scale = ivec2(1);
    ldvec2 default_offset = ivec2(0);
    ldvec2 default_scale = ldvec2(1);
    mutable distr_t distr{1};

    bool grabbed = 0;
    ivec2 grab_offset = ivec2(0);

    bool scale_changed_this_tick = 0,
         scale_changed_prev_tick = 1;


    struct PointData
    {
        bool valid;
        ldvec2 pos;
    };

    PointData Point(long double freq) const
    {
        PointData ret;
        ret.pos = func(freq);
        ret.valid = std::isfinite(ret.pos.x) && std::isfinite(ret.pos.y);
        ret.pos.y = -ret.pos.y;
        return ret;
    }

  public:
    enum Flags {lock_scale_ratio = 1};

    Plot() {}
    Plot(func_t func, int flags = 0) : func(std::move(func)), flags(flags)
    {
        RecalculateDefaultOffsetAndScale();
        offset = default_offset;
        scale = default_scale;
    }

    void SetMeanFreq(long double freq)
    {
        distr = distr_t(1.l / freq);
    }

    explicit operator bool() const
    {
        return bool(func);
    }

    void Tick(int count)
    {
        // Move if needed
        if (grabbed)
        {
            Graphics::Clear(Graphics::color);
            Draw::Accumulator::Overwrite();

            offset = (mouse.pos() - grab_offset) / scale;

            if (mouse.left.up())
                grabbed = 0;
        }

        // Draw points
        while (count-- > 0)
        {
            long double freq = distr(Rand::Generator());
            auto point = Point(freq);
            if (!point.valid)
                continue;
            ldvec2 pos = (point.pos + offset) * scale;
            if ((abs(pos) > win.Size()).any())
                continue;
            Draw::Dot(grabbed || scale_changed_this_tick, pos, fvec3(0,0.4,0.9));
        }
        r.Finish();

        if (!scale_changed_this_tick && scale_changed_prev_tick)
            ResetAccumulator();
        scale_changed_prev_tick = scale_changed_this_tick;
        scale_changed_this_tick = 0;
    }

    void RecalculateDefaultOffsetAndScale()
    {
        ldvec2 box_min(0), box_max(0);

        for (int i = 0; i <= 64; i++)
        {
            long double freq = i / 4.l;
            auto point = Point(freq);
            if (!point.valid)
                continue;

            for (auto mem : {&ldvec2::x, &ldvec2::y})
            {
                if (point.pos.*mem < box_min.*mem)
                    box_min.*mem = point.pos.*mem;
                else if (point.pos.*mem > box_max.*mem)
                    box_max.*mem = point.pos.*mem;
            }
        }

        for (auto mem : {&ldvec2::x, &ldvec2::y})
        {
            default_offset.*mem = (box_min.*mem + box_max.*mem) / -2;
            if (box_min.*mem == box_max.*mem)
                continue;
            default_scale.*mem = (ldvec2(win.Size()).*mem * (1-window_margin)) / (box_max.*mem - box_min.*mem);
        }

        if (flags & lock_scale_ratio)
            default_scale = ldvec2(default_scale.min());
    }

    void ResetAccumulator()
    {
        Graphics::Clear(Graphics::color);
        Draw::Accumulator::Overwrite();
    }

    void Grab()
    {
        grabbed = 1;
        grab_offset = mouse.pos() - offset * scale;
    }

    void Scale(ldvec2 scale_factor)
    {
        scale *= scale_factor;
        if (flags & lock_scale_ratio)
            scale.x = scale.y;

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

    void Tick(bool &button_pressed)
    {
        constexpr int half_extent = 24;

        if (!visible)
        {
            pressed = 0;
            hovered = 0;
            return;
        }

        ivec2 screen_pos = ScreenPos();
        hovered = (mouse.pos() >= screen_pos - half_extent).all() && (mouse.pos() < screen_pos + half_extent).all();

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


int main(int, char **)
{
    Draw::Init();

    Graphics::ClearColor(fvec3(1));
    Graphics::Clear(Graphics::color);
//    for (imat2 m : {fmat2(1,0,0,1), fmat2(0,1,-1,0)})
//        for (int i = -4; i <= 4; i++)
//            Draw::Line(m /mul/ fvec2(i,4.25) * 60, m /mul/ fvec2(i,-4.25) * 60, fvec3(0.85));
//    r.Finish();
    Draw::Accumulator::Overwrite();

    Expression e("(1+s+s^3*0.5) / (0.2 * s^4 - 0.7 * s + 1)");
    Plot plot([e](long double t){return e.EvalVec({0,t});});

    std::vector<Button> buttons;

    enum class ButtonCategory {scale};

    auto AddButtonCategory = [&](ButtonCategory category)
    {
        constexpr float scale_factor = 1.015;

        switch (category)
        {
          case ButtonCategory::scale:
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
        }
    };
    AddButtonCategory(ButtonCategory::scale);

    auto Tick = [&]
    {
        Draw::Accumulator::Return();

        bool button_pressed = mouse.left.pressed();

        Button::ResetTooltip();
        for (auto &button : buttons)
            button.Tick(button_pressed);

        if (button_pressed)
            plot.Grab();

        plot.Tick(128);

        Draw::Accumulator::Overwrite();
    };
    auto Render = [&]
    {
        Draw::Accumulator::Return();

        for (const auto &button : buttons)
            button.Render();

        { // Button tooltips
            if (Button::CurrentTooltip().size() > 0)
            {
                constexpr int tooltip_box_height = 32;
                r.Quad(ivec2(Draw::min.x, Draw::max.y - tooltip_box_height), ivec2(win.Size().x, tooltip_box_height)).color(fvec3(0.95));
                r.Quad(ivec2(Draw::min.x, Draw::max.y - tooltip_box_height), ivec2(win.Size().x, 1)).color(fvec3(0.3));
                r.Text(ivec2(0, Draw::max.y - tooltip_box_height/2), Button::CurrentTooltip()).color(fvec3(0));
            }
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
                Graphics::Clear(Graphics::color);
                Draw::Accumulator::Overwrite();
                Draw::HandleResize();
                plot.RecalculateDefaultOffsetAndScale();
            }
            Tick();
        }

        Render();
        r.Finish();

        win.Swap();
    }
}
