#ifndef RENDERERS2D_H_INCLUDED
#define RENDERERS2D_H_INCLUDED

#include <cstddef>
#include <functional>
#include <vector>
#include <utility>

#include "graphics.h"
#include "mat.h"
#include "reflection.h"
#include "strings.h"
#include "template_utils.h"

namespace Renderers
{
    namespace Poly2D_impl
    {
        ReflectStruct(Attributes, (
            (fvec2)(pos),
            (fvec4)(color),
            (fvec2)(texture_pos),
            (fvec3)(factors),
        ))

        ReflectStruct(Uniforms, (
            (Graphics::Shader::Uniform_v<fmat4>)(matrix),
            (Graphics::Shader::Uniform_v<fvec2>)(texture_size),
            (Graphics::Shader::Uniform_f<Graphics::Texture>)(texture),
            (Graphics::Shader::Uniform_f<fmat4>)(color_matrix),
        ))
    }

    class Poly2D
    {
        Graphics::Shader shader;
        Graphics::RenderQueue<Poly2D_impl::Attributes, Graphics::triangles> queue;
        Poly2D_impl::Uniforms uni;

        const Graphics::CharMap *ch_map = 0;

      public:
        class Quad_t
        {
            using ref = Quad_t &&;

            // The constructor sets those:
            TemplateUtils::ResetOnMove<decltype(Poly2D::queue) *> queue;
            fvec2 m_pos, m_size;

            bool has_texture = 0;
            fvec2 m_tex_pos = fvec2(0), m_tex_size = fvec2(0);

            bool has_center = 0;
            fvec2 m_center = fvec2(0);
            bool m_center_pos_tex = 0;

            bool has_matrix = 0;
            fmat3 m_matrix = fmat3::identity();

            bool has_color = 0;
            fvec3 m_colors[4] {};

            bool has_tex_color_fac = 0;
            float m_tex_color_factors[4] = {1,1,1,1};

            float m_alpha[4] = {1,1,1,1};
            float m_beta[4] = {1,1,1,1};

            bool m_abs_pos = 0;
            bool m_abs_tex_pos = 0;

            bool m_flip_x = 0, m_flip_y = 0;

          public:
            Quad_t(decltype(Poly2D::queue) *queue, fvec2 pos, fvec2 size) : queue(queue), m_pos(pos), m_size(size) {}

            Quad_t(const Quad_t &) = delete;
            Quad_t &operator=(const Quad_t &) = delete;

            Quad_t(Quad_t &&) = default;
            Quad_t &operator=(Quad_t &&) = default;

            ~Quad_t()
            {
               if (!queue)
                    return;

                DebugAssert("2D poly renderer: Quad with no texture nor color specified.", has_texture || has_color);
                DebugAssert("2D poly renderer: Quad with absolute corner coodinates with a center specified.", m_abs_pos + has_center < 2);
                DebugAssert("2D poly renderer: Quad with absolute texture coordinates mode but no texture coordinates specified.", m_abs_tex_pos <= has_texture);
                DebugAssert("2D poly renderer: Quad with texture and color, but without a mixing factor.", (has_texture && has_color) == has_tex_color_fac);
                DebugAssert("2D poly renderer: Quad with a matrix but without a center specified.", has_matrix <= has_center);

                if (m_abs_pos)
                    m_size -= m_pos;
                if (m_abs_tex_pos)
                    m_tex_size -= m_tex_pos;

                Poly2D_impl::Attributes out[4];

                if (has_texture)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        out[i].color = m_colors[i].to_vec4(0);
                        out[i].factors.x = m_tex_color_factors[i];
                        out[i].factors.y = m_alpha[i];
                    }

                    if (m_center_pos_tex)
                    {
                        if (m_tex_size.x)
                            m_center.x *= m_size.x / m_tex_size.x;
                        if (m_tex_size.y)
                            m_center.y *= m_size.y / m_tex_size.y;
                    }
                }
                else
                {
                    for (int i = 0; i < 4; i++)
                    {
                        out[i].color = m_colors[i].to_vec4(m_alpha[i]);
                        out[i].factors.x = out[i].factors.y = 0;
                    }
                }

                for (int i = 0; i < 4; i++)
                    out[i].factors.z = m_beta[i];

                if (m_flip_x)
                {
                    m_tex_pos.x += m_tex_size.x;
                    m_tex_size.x = -m_tex_size.x;
                    if (has_center)
                        m_center.x = m_size.x - m_center.x;
                }
                if (m_flip_y)
                {
                    m_tex_pos.y += m_tex_size.y;
                    m_tex_size.y = -m_tex_size.y;
                    if (has_center)
                        m_center.y = m_size.y - m_center.y;
                }

                out[0].pos = -m_center;
                out[2].pos = m_size - m_center;
                out[1].pos = {out[2].pos.x, out[0].pos.y};
                out[3].pos = {out[0].pos.x, out[2].pos.y};

                if (has_matrix)
                {
                    for (auto &it : out)
                        it.pos = m_pos + (m_matrix /mul/ it.pos.to_vec3(1)).to_vec2();
                }
                else
                {
                    for (auto &it : out)
                        it.pos += m_pos;
                }

                out[0].texture_pos = m_tex_pos;
                out[2].texture_pos = m_tex_pos + m_tex_size;
                out[1].texture_pos = {out[2].texture_pos.x, out[0].texture_pos.y};
                out[3].texture_pos = {out[0].texture_pos.x, out[2].texture_pos.y};

                queue->Quad(out[0], out[1], out[2], out[3]);
            }

            ref tex(fvec2 pos, fvec2 size)
            {
                DebugAssert("2D poly renderer: Quad_t texture specified twice.", !has_texture);
                has_texture = 1;

                m_tex_pos = pos;
                m_tex_size = size;
                return (ref)*this;
            }
            ref tex(fvec2 pos)
            {
                tex(pos, m_size);
                return (ref)*this;
            }
            ref center(fvec2 c)
            {
                DebugAssert("2D poly renderer: Quad_t center specified twice.", !has_center);
                has_center = 1;

                m_center = c;
                m_center_pos_tex = 1;
                return (ref)*this;
            }
            ref pixel_center(fvec2 c) // Same as `center()`, but the coordinates are always measured in pixels instead of texels even if a texture is specified.
            {
                DebugAssert("2D poly renderer: Quad_t center specified twice.", !has_center);
                has_center = 1;

                m_center = c;
                m_center_pos_tex = 0;
                return (ref)*this;
            }
            ref center()
            {
                pixel_center(m_size / 2);
                return (ref)*this;
            }
            ref matrix(fmat3 m) // This can be called multiple times, resulting in multiplying matrices in the order they were passed.
            {
                if (has_matrix)
                    m_matrix = m_matrix /mul/ m;
                else
                {
                    has_matrix = 1;
                    m_matrix = m;
                }
                return (ref)*this;
            }
            ref matrix(fmat2 m)
            {
                matrix(m.to_mat3());
                return (ref)*this;
            }
            ref rotate(float a) // Uses `matrix()`.
            {
                matrix(fmat3::rotate2D(a));
                return (ref)*this;
            }
            ref translate(fvec2 v) // Uses a matrix.
            {
                matrix(fmat3::translate2D(v));
                return (ref)*this;
            }
            ref scale(fvec2 s) // Uses a matrix.
            {
                matrix(fmat3::scale(s));
                return (ref)*this;
            }
            ref scale(float s) // Uses a matrix.
            {
                scale(fvec2(s));
                return (ref)*this;
            }
            ref color(fvec3 c)
            {
                DebugAssert("2D poly renderer: Quad_t color specified twice.", !has_color);
                has_color = 1;

                for (auto &it : m_colors)
                    it = c;
                return (ref)*this;
            }
            ref color(fvec3 a, fvec3 b, fvec3 c, fvec3 d)
            {
                DebugAssert("2D poly renderer: Quad_t color specified twice.", !has_color);
                has_color = 1;

                m_colors[0] = a;
                m_colors[1] = b;
                m_colors[2] = c;
                m_colors[3] = d;
                return (ref)*this;
            }
            ref mix(float x) // 0 - fill with color, 1 - use texture
            {
                DebugAssert("2D poly renderer: Quad_t texture/color factor specified twice.", !has_tex_color_fac);
                has_tex_color_fac = 1;

                for (auto &it : m_tex_color_factors)
                    it = x;
                return (ref)*this;
            }
            ref mix(float a, float b, float c, float d)
            {
                DebugAssert("2D poly renderer: Quad_t texture/color factor specified twice.", !has_tex_color_fac);
                has_tex_color_fac = 1;

                m_tex_color_factors[0] = a;
                m_tex_color_factors[1] = b;
                m_tex_color_factors[2] = c;
                m_tex_color_factors[3] = d;
                return (ref)*this;
            }
            ref alpha(float a)
            {
                for (auto &it : m_alpha)
                    it = a;
                return (ref)*this;
            }
            ref alpha(float a, float b, float c, float d)
            {
                m_alpha[0] = a;
                m_alpha[1] = b;
                m_alpha[2] = c;
                m_alpha[3] = d;
                return (ref)*this;
            }
            ref beta(float a) // 1 - normal blending, 0 - additive blending
            {
                for (auto &it : m_beta)
                    it = a;
                return (ref)*this;
            }
            ref beta(float a, float b, float c, float d)
            {
                m_beta[0] = a;
                m_beta[1] = b;
                m_beta[2] = c;
                m_beta[3] = d;
                return (ref)*this;
            }
            ref absolute(bool x = 1) // Interpret size as a position of the second corner
            {
                m_abs_pos = x;
                return (ref)*this;
            }
            ref absolute_tex(bool x = 1) // Interpret texture size as a position of the second corner
            {
                m_abs_tex_pos = x;
                return (ref)*this;
            }
            ref flip_x(bool f = 1) // Flips texture horizontally if it was specified. Updates the center accordingly if it was specified.
            {
                m_flip_x = f;
                return (ref)*this;
            }
            ref flip_y(bool f = 1) // Flips texture vertically if it was specified. Updates the center accordingly if it was specified.
            {
                m_flip_y = f;
                return (ref)*this;
            }
        };
        class Triangle_t
        {
            using ref = Triangle_t &&;

            // The constructor sets those:
            TemplateUtils::ResetOnMove<decltype(Poly2D::queue) *> queue;
            fvec2 m_pos, m_vectices[3];

            bool has_texture = 0;
            fvec2 m_tex_pos[3] = {};

            bool has_matrix = 0;
            fmat3 m_matrix = fmat3::identity();

            bool has_color = 0;
            fvec3 m_colors[3] {};

            bool has_tex_color_fac = 0;
            float m_tex_color_factors[3] = {1,1,1};

            float m_alpha[3] = {1,1,1};
            float m_beta[3] = {1,1,1};

          public:
            Triangle_t(decltype(Poly2D::queue) *queue, fvec2 pos, fvec2 a, fvec2 b, fvec2 c) : queue(queue), m_pos(pos), m_vectices{a, b, c} {}

            Triangle_t(const Triangle_t &) = delete;
            Triangle_t &operator=(const Triangle_t &) = delete;

            Triangle_t(Triangle_t &&) = default;
            Triangle_t &operator=(Triangle_t &&) = default;

            ~Triangle_t()
            {
                if (!queue)
                    return;

                DebugAssert("2D poly renderer: Triangle with no texture nor color specified.", has_texture || has_color);
                DebugAssert("2D poly renderer: Triangle with texture and color, but without a mixing factor.", (has_texture && has_color) == has_tex_color_fac);

                Poly2D_impl::Attributes out[3];

                if (has_texture)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        out[i].color = m_colors[i].to_vec4(0);
                        out[i].factors.x = m_tex_color_factors[i];
                        out[i].factors.y = m_alpha[i];
                    }
                }
                else
                {
                    for (int i = 0; i < 3; i++)
                    {
                        out[i].color = m_colors[i].to_vec4(m_alpha[i]);
                        out[i].factors.x = out[i].factors.y = 0;
                    }
                }

                for (int i = 0; i < 3; i++)
                {
                    out[i].factors.z = m_beta[i];
                    out[i].texture_pos = m_tex_pos[i];
                }

                if (has_matrix)
                {
                    for (int i = 0; i < 3; i++)
                        out[i].pos = m_pos + (m_matrix /mul/ m_vectices[i].to_vec3(1)).to_vec2();
                }
                else
                {
                    for (int i = 0; i < 3; i++)
                        out[i].pos = m_pos + m_vectices[i];
                }

                queue->Triangle(out[0], out[1], out[2]);
            }

            ref tex(ivec2 pos)
            {
                tex_f(pos);
                return (ref)*this;
            }
            ref tex(ivec2 a, ivec2 b, ivec2 c)
            {
                tex_f(a, b, c);
                return (ref)*this;
            }
            ref tex_f(fvec2 pos)
            {
                tex_f(pos, pos, pos);
                return (ref)*this;
            }
            ref tex_f(fvec2 a, fvec2 b, fvec2 c)
            {
                DebugAssert("2D poly renderer: Triangle texture specified twice.", !has_texture);
                has_texture = 1;

                m_tex_pos[0] = a;
                m_tex_pos[1] = b;
                m_tex_pos[2] = c;
                return (ref)*this;
            }
            ref matrix(fmat3 m) // This can be called multiple times, resulting in multiplying matrices in the order they were passed.
            {
                if (has_matrix)
                    m_matrix = m_matrix /mul/ m;
                else
                {
                    has_matrix = 1;
                    m_matrix = m;
                }
                return (ref)*this;
            }
            ref matrix(fmat2 m)
            {
                matrix(m.to_mat3());
                return (ref)*this;
            }
            ref rotate(float a) // Uses `matrix()`.
            {
                matrix(fmat3::rotate2D(a));
                return (ref)*this;
            }
            ref translate(fvec2 v) // Uses a matrix.
            {
                matrix(fmat3::translate2D(v));
                return (ref)*this;
            }
            ref scale(fvec2 s) // Uses a matrix.
            {
                matrix(fmat3::scale(s));
                return (ref)*this;
            }
            ref scale(float s) // Uses a matrix.
            {
                scale(fvec2(s));
                return (ref)*this;
            }
            ref color(fvec3 c)
            {
                DebugAssert("2D poly renderer: Triangle color specified twice.", !has_color);
                has_color = 1;

                for (auto &it : m_colors)
                    it = c;
                return (ref)*this;
            }
            ref color(fvec3 a, fvec3 b, fvec3 c)
            {
                DebugAssert("2D poly renderer: Triangle color specified twice.", !has_color);
                has_color = 1;

                m_colors[0] = a;
                m_colors[1] = b;
                m_colors[2] = c;
                return (ref)*this;
            }
            ref mix(float x) // 0 - fill with color, 1 - use texture
            {
                DebugAssert("2D poly renderer: Triangle texture/color factor specified twice.", !has_tex_color_fac);
                has_tex_color_fac = 1;

                for (auto &it : m_tex_color_factors)
                    it = x;
                return (ref)*this;
            }
            ref mix(float a, float b, float c)
            {
                DebugAssert("2D poly renderer: Triangle texture/color factor specified twice.", !has_tex_color_fac);
                has_tex_color_fac = 1;

                m_tex_color_factors[0] = a;
                m_tex_color_factors[1] = b;
                m_tex_color_factors[2] = c;
                return (ref)*this;
            }
            ref alpha(float a)
            {
                for (auto &it : m_alpha)
                    it = a;
                return (ref)*this;
            }
            ref alpha(float a, float b, float c)
            {
                m_alpha[0] = a;
                m_alpha[1] = b;
                m_alpha[2] = c;
                return (ref)*this;
            }
            ref beta(float a) // 1 - normal blending, 0 - additive blending
            {
                for (auto &it : m_beta)
                    it = a;
                return (ref)*this;
            }
            ref beta(float a, float b, float c)
            {
                m_beta[0] = a;
                m_beta[1] = b;
                m_beta[2] = c;
                return (ref)*this;
            }
        };
        class Text_t
        {
          public:
            struct RenderData
            {
                fvec3 color = {1,1,1};
                float alpha = 1, beta = 1;
                fmat3 matrix = fmat3::identity();
            };

            // Copyable callback parameters:
            struct CallbackParams
            {
                bool render_pass;
                int index;
                uint16_t ch, prev;
                Text_t &obj;
                Graphics::CharMap::Char &glyph;
                std::vector<RenderData> &render;
                ivec2 pos;
            };

            using callback_type = std::function<void(const CallbackParams &)>;

            struct State
            {
                // The constructor sets those:
                fvec2 pos; // This is dynamically adjusted during drawing.
                std::string_view str;
                const Graphics::CharMap *ch_map;

                ivec2 alignment = ivec2(0);
                fmat3 matrix = fmat3::identity();
                fvec3 color = {1,1,1};
                float alpha = 1, beta = 1;
                int spacing = 0, line_gap = 0;
                int tab_width = 4; // Measured in spaces
                bool kerning = 1;
                std::vector<callback_type> callbacks;
            };

          private:

            using ref = Text_t &&;

            TemplateUtils::ResetOnMove<decltype(Poly2D::queue) *> queue; // The constructor sets it.

            State obj_state; // State

            uint16_t prev_ch = 0xffff; // This is there because we reset it when changing fonts via callbacks.

            void Render()
            {
                DebugAssert("2D poly renderer: Text with no font specified.", obj_state.ch_map != 0);

                if (!queue)
                    return;

                // Those are copied to prevent callbacks from messing them up.
                decltype(Poly2D::queue) *saved_queue = queue;
                queue.value() = 0;
                ivec2 saved_alignment = obj_state.alignment = sign(obj_state.alignment);

                struct Line
                {
                    int width, ascent;
                };

                ivec2 pos(0);
                std::vector<Line> lines;
                std::size_t line_number = 0;

                auto Loop = [&](bool do_render)
                {
                    int line_ascent  = obj_state.ch_map->Ascent(),
                        line_descent = obj_state.ch_map->Descent(),
                        line_gap     = obj_state.ch_map->LineGap(),
                        line_gap_st  = obj_state.line_gap;
                    int last_spacing = 0, last_gap = 0;
                    int line_offset_x = 0;

                    auto StartLine = [&]
                    {
                        if (!do_render)
                            return;
                        if (saved_alignment.x != -1)
                        {
                            pos.x = -lines[line_number].width;
                            if (saved_alignment.x == 0)
                                pos.x /= 2;
                        }
                        pos.y += lines[line_number].ascent;

                        line_offset_x = pos.x;
                    };
                    auto EndLine = [&]
                    {
                        pos.x -= last_spacing;
                        last_spacing = 0;

                        if (!do_render)
                            lines.push_back({pos.x, line_ascent});

                        pos.y += line_descent + line_gap + line_gap_st;
                        if (!do_render)
                            pos.y += line_ascent; // Otherwise it's handle in StartLine().

                        last_gap = line_gap + line_gap_st;

                        line_ascent  = obj_state.ch_map->Ascent();
                        line_descent = obj_state.ch_map->Descent();
                        line_gap     = obj_state.ch_map->LineGap();
                        line_gap_st  = obj_state.line_gap;

                        pos.x = 0;

                        if (do_render)
                            line_number++;
                    };

                    int index = 0;

                    auto CallCallbacks = [&](uint16_t ch, Graphics::CharMap::Char &info, std::vector<RenderData> &render)
                    {
                        if (obj_state.callbacks.size())
                        {
                            const Graphics::CharMap *ch_map_copy = obj_state.ch_map;

                            for (const auto &callback : obj_state.callbacks)
                                callback({do_render, index, ch, prev_ch, *this, info, render, obj_state.pos + pos});

                            if (ch_map_copy != obj_state.ch_map)
                            {
                                if (line_ascent < obj_state.ch_map->Ascent())
                                    line_ascent = obj_state.ch_map->Ascent();
                                if (line_descent < obj_state.ch_map->Descent())
                                    line_descent = obj_state.ch_map->Descent();
                                if (line_gap < obj_state.ch_map->LineGap())
                                    line_gap = obj_state.ch_map->LineGap();
                            }
                            if (line_gap_st > obj_state.line_gap)
                                line_gap_st = obj_state.line_gap;
                        }
                    };

                    StartLine();
                    auto it = obj_state.str.begin();
                    while (it != obj_state.str.end())
                    {
                        if (u8isfirstbyte(it))
                        {
                            switch (*it)
                            {
                              default:
                                {
                                    uint16_t ch = u8decode(it);

                                    Graphics::CharMap::Char info = obj_state.ch_map->Get(ch);

                                    if (obj_state.kerning)
                                        pos.x += obj_state.ch_map->Kerning(prev_ch, ch);

                                    std::vector<RenderData> render{{obj_state.color, obj_state.alpha, obj_state.beta, obj_state.matrix /mul/ fmat3::translate2D(pos + info.offset)}};

                                    CallCallbacks(ch, info, render);

                                    if (do_render)
                                    {
                                        for (const auto &it : render)
                                        {
                                            Quad_t(saved_queue, obj_state.pos, info.size)
                                                .tex(info.tex_pos)
                                                .alpha(it.alpha).beta(it.beta).color(it.color).mix(0)
                                                .center(ivec2(0)).matrix(it.matrix);
                                        }
                                    }

                                    pos.x += info.advance + (last_spacing = obj_state.spacing);

                                    prev_ch = ch;
                                }
                                break;
                              case '\n':
                                {
                                    EndLine();
                                    prev_ch = 0xffff;
                                    StartLine();
                                }
                                break;
                              case '\t':
                                {
                                    int tab_pixels = obj_state.tab_width * obj_state.ch_map->Get(' ').advance;
                                    pos.x = (pos.x - line_offset_x + tab_pixels - 1) / tab_pixels * tab_pixels;
                                    prev_ch = '\t';
                                }
                                break;
                            }
                            index++;
                        }
                        it++;
                    }

                    std::vector<RenderData> tmp_render{{obj_state.color, obj_state.alpha, obj_state.beta, fmat3::identity()}};
                    auto tmp_info = obj_state.ch_map->Get('\0');
                    CallCallbacks('\0', tmp_info, tmp_render);

                    EndLine();
                    pos.y -= last_gap;
                };

                State saved_state = obj_state;
                Loop(0);
                obj_state = std::move(saved_state);
                if (saved_alignment.y != -1)
                {
                    pos.y = -pos.y;
                    if (saved_alignment.y == 0)
                        pos.y /= 2;
                }
                else
                    pos.y = 0;

                Loop(1);
            }

          public:
            Text_t(decltype(Poly2D::queue) *queue, const Graphics::CharMap *ch_map, fvec2 pos, std::string_view str) : queue(queue)
            {
                obj_state.pos = pos;
                obj_state.str = str;
                obj_state.ch_map = ch_map;
            }

            Text_t(const Text_t &) = delete;
            Text_t &operator=(const Text_t &) = delete;

            Text_t(Text_t &&) = default;
            Text_t &operator=(Text_t &&) = default;

            ~Text_t()
            {
                Render();
            }

            ref callback(callback_type c) // These can be chained. Note that if alignment is required, the callback will be called twice for each symbol, first time when calculating the alignment.
            {
                obj_state.callbacks.emplace_back(std::move(c));
                return (ref)*this;
            }
            const State &state() // For use from inside callbacks.
            {
                return obj_state;
            }

            ref font(Graphics::CharMap &&) = delete;
            ref font(const Graphics::CharMap &map)
            {
                obj_state.ch_map = &map;
                prev_ch = 0xffff; // This resets the kerning.
                return (ref)*this;
            }

            ref align_h(int a)
            {
                obj_state.alignment.x = a;
                return (ref)*this;
            }
            ref align_v(int a)
            {
                obj_state.alignment.y = a;
                return (ref)*this;
            }
            ref align(ivec2 a)
            {
                obj_state.alignment = a;
                return (ref)*this;
            }
            ref matrix(fmat3 m) // Multiplies the matrix by `m`.
            {
                obj_state.matrix = obj_state.matrix /mul/ m;
                return (ref)*this;
            }
            ref back_matrix(fmat3 m) // Multiplies `m` by the matrix.
            {
                obj_state.matrix = m /mul/ obj_state.matrix;
                return (ref)*this;
            }
            ref matrix(fmat2 m) // Multiplies the matrix by `m`.
            {
                matrix(m.to_mat3());
                return (ref)*this;
            }
            ref back_matrix(fmat2 m) // Multiplies `m` by the matrix.
            {
                back_matrix(m.to_mat3());
                return (ref)*this;
            }
            ref rotate(float a) // Uses `matrix()`.
            {
                matrix(fmat3::rotate2D(a));
                return (ref)*this;
            }
            ref scale(fvec2 s) // Uses `matrix()`.
            {
                matrix(fmat3::scale(s));
                return (ref)*this;
            }
            ref scale(float s) // Uses `matrix()`.
            {
                scale(fvec2(s));
                return (ref)*this;
            }
            ref color(fvec3 c)
            {
                obj_state.color = c;
                return (ref)*this;
            }
            ref alpha(float a)
            {
                obj_state.alpha = a;
                return (ref)*this;
            }
            ref beta(float b)
            {
                obj_state.beta = b;
                return (ref)*this;
            }
            ref spacing(int s)
            {
                obj_state.spacing += s;
                return (ref)*this;
            }
            ref line_gap(int g)
            {
                obj_state.line_gap += g;
                return (ref)*this;
            }
            ref tab_width(int w)
            {
                obj_state.tab_width = w;
                return (ref)*this;
            }
            ref kerning(bool k) // Enabled by default
            {
                obj_state.kerning = k;
                return (ref)*this;
            }

            template <typename F> ref preset(F &&func) // void func(Text_t &ref)
            {
                func(*this);
                return (ref)*this;
            }
        };

        Poly2D() {}
        Poly2D(int size, const Graphics::Shader::Config &cfg = {})
        {
            Create(size, cfg);
        }
        Poly2D(int size, const std::string &v_src, const std::string &f_src, const Graphics::Shader::Config &cfg = {})
        {
            Create(size, v_src, f_src, cfg);
        }
        void Create(int size, const Graphics::Shader::Config &cfg = {})
        {
            constexpr const char *v = R"(
VARYING( vec4 , color       )
VARYING( vec2 , texture_pos )
VARYING( vec3 , factors     )
void main()
{
    gl_Position = u_matrix * vec4(a_pos, 0, 1);
    v_color       = a_color;
    v_texture_pos = a_texture_pos / u_texture_size;
    v_factors     = a_factors;
})";
            constexpr const char *f = R"(
VARYING( vec4 , color       )
VARYING( vec2 , texture_pos )
VARYING( vec3 , factors     )
void main()
{
    vec4 tex_color = texture2D(u_texture, v_texture_pos);
    gl_FragColor = vec4(v_color.rgb * (1. - v_factors.x) + tex_color.rgb * v_factors.x,
                        v_color.a   * (1. - v_factors.y) + tex_color.a   * v_factors.y);
    vec4 result = u_color_matrix * vec4(gl_FragColor.rgb, 1);
    gl_FragColor.a *= result.a;
    gl_FragColor.rgb = result.rgb * gl_FragColor.a;
    gl_FragColor.a *= v_factors.z;
})";
            Create(size, v, f, cfg);
        }
        void Create(int size, const std::string &v_src, const std::string &f_src, const Graphics::Shader::Config &cfg = {}) // With custom shader.
        {
            decltype(shader) new_shader;
            new_shader.Create<Poly2D_impl::Attributes>("2D renderer", v_src, f_src, &uni, cfg);
            decltype(queue) new_queue(size);
            shader = std::move(new_shader);
            queue  = std::move(new_queue);

            SetMatrix(fmat4::identity());
            ResetColorMatrix();
        }
        void Destroy()
        {
            shader.Destroy();
            queue.Destroy();
        }

        void Finish() // Binds the shader.
        {
            shader.Bind();
            queue.Draw();
        }

        void BindShader() const
        {
            shader.Bind();
        }

        void SetMatrix(fmat4 m) // Binds the shader, flushes the queue.
        {
            Finish();
            uni.matrix = m;
        }

        // final_color = (color_matrix * vec4(color.rgb,1)) * vec4(1,1,1,color.a)
        void SetColorMatrix(fmat4 m) // Binds the shader, flushes the queue.
        {
            Finish();
            uni.color_matrix = m;
        }
        void ResetColorMatrix() // Binds the shader, flushes the queue.
        {
            Finish();
            uni.color_matrix = fmat4::identity();
        }

        void SetTexture(const Graphics::Texture &texture) // Binds the shader, flushes the queue.
        {
            Finish();
            uni.texture = texture;
            uni.texture_size = texture.Size();
        }
        void SetTexture(Graphics::Texture &&) = delete;

        void SetDefaultFont(const Graphics::CharMap &map)
        {
            ch_map = &map;
        }
        void SetDefaultFont(Graphics::CharMap &&) = delete;

        Quad_t Quad(fvec2 pos, fvec2 size)
        {
            return {&queue, pos, size};
        }
        Triangle_t Triangle(fvec2 pos, fvec2 a, fvec2 b, fvec2 c)
        {
            return {&queue, pos, a, b, c};
        }
        Text_t Text(fvec2 pos, std::string_view str)
        {
            return {&queue, ch_map, pos, str};
        }
    };
}

#endif
