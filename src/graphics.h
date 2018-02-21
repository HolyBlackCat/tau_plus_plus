#ifndef GRAPHICS_H_INCLUDED
#define GRAPHICS_H_INCLUDED

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <ios>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>
#include <utility>

#include <GLFL/glfl.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_rect_pack.h>
#include <ft2build.h>
#include FT_FREETYPE_H // Ugh.

#include "exceptions.h"
#include "platform.h"
#include "program.h"
#include "reflection.h"
#include "template_utils.h"
#include "utils.h"

/* GLSL version chart:
    1.10.59		2.0		April 2004		#version 110
    1.20.8		2.1		September 2006	#version 120
    1.30.10		3.0		August 2008		#version 130
    1.40.08		3.1		March 2009		#version 140
    1.50.11		3.2		August 2009		#version 150
    3.30.6		3.3		February 2010	#version 330
    4.00.9		4.0		March 2010		#version 400
    4.10.6		4.1		July 2010		#version 410
    4.20.11		4.2		August 2011		#version 420
    4.30.8		4.3		August 2012		#version 430
    4.40		4.4		July 2013		#version 440
    4.50		4.5		August 2014		#version 450
    1.00        ES 2                    #version 100

    GLSL ES can be tested with `#ifdef GL_ES`.
    GLSL ES lacks default precision for float inside of fragment shaders.
*/

namespace Graphics
{
    inline namespace Exceptions
    {
        DefineExceptionBase(exception)

        DefineExceptionInline(cant_create_gl_resource, :exception, "Can't create an OpenGL resource.",
            (std::string,type,"Type")
        )
        DefineExceptionInline(gl_error, :exception, "OpenGL error.",
            (std::string,message,"Message")
        )
        DefineExceptionInline(no_free_texture_slots, :exception, "Out of free texture slots.",)

        DefineExceptionInline(cant_init_freetype, :exception, "Can't initialize Freetype.",)

        DefineExceptionInline(cant_parse_font, :exception, "Unable to parse a font.",
            (std::string,name,"Name")
        )
        DefineExceptionInline(bad_font_size, :exception, "Invalid font size.",
            (ivec2,size,"Specified size")
            (std::string,available,"Available sizes")
        )
        DefineExceptionInline(cant_render_glyph, :exception, "Unable to render a glyph.",
            (unsigned int,code,"Code")
            (std::string,reason,"Reason")
        )

        DefineExceptionInline(not_enough_texture_atlas_space, :exception, "Out of space for a font atlas.",
            (ivec2,pos,"Position")
            (ivec2,size,"Size")
        )

        DefineExceptionInline(cant_parse_image, :exception, "Unable to parse an image.",
            (std::string,name,"Name")
        )
        DefineExceptionInline(cant_save_image, :exception, "Unable to save an image.",
            (std::string,name,"Name")
        )

        DefineExceptionInline(shader_compilation_error, :exception, "Shader compilation error.",
            (std::string,name,"Name")
            (std::string,vertex_status,"Vertex status")
            (std::string,fragment_status,"Fragment status")
            (std::string,vertex_log,"Vertex log")
            (std::string,fragment_log,"Fragment log")
        )
        DefineExceptionInline(shader_linking_error, :exception, "Shader linking error.",
            (std::string,name,"Name")
            (std::string,fragment_log,"Log")
        )
    }

    inline void CheckErrors()
    {
        GLenum err = glGetError();
        if (err)
        {
            std::string msg;
            do
            {
                if (msg.size()) msg += ", ";
                switch (err)
                {
                    case GL_INVALID_ENUM:                  msg += "Invalid enum";                  break;
                    case GL_INVALID_VALUE:                 msg += "Invalid value";                 break;
                    case GL_INVALID_OPERATION:             msg += "Invalid operation";             break;
                    case GL_INVALID_FRAMEBUFFER_OPERATION: msg += "Invalid framebuffer operation"; break;
                    case GL_OUT_OF_MEMORY:                 msg += "Out of memory";                 break;
                    #ifdef GL_STACK_UNDERFLOW
                    case GL_STACK_UNDERFLOW:               msg += "Stack underflow";               break;
                    #endif
                    #ifdef GL_STACK_OVERFLOW
                    case GL_STACK_OVERFLOW:                msg += "Stack overflow";                break;
                    #endif
                    default:                               msg += "Unknown error";                 break;
                }
            }
            while ((err = glGetError()));
            throw gl_error(msg + '.');
        }
    }

    inline namespace Misc
    {
        enum ClearBits
        {
            color   = GL_COLOR_BUFFER_BIT,
            depth   = GL_DEPTH_BUFFER_BIT,
            stencil = GL_STENCIL_BUFFER_BIT,
        };
        inline ClearBits operator|(ClearBits a, ClearBits b) {return ClearBits(int(a) | int(b));}

        inline void Clear(ClearBits bits)
        {
            glClear(bits);
        }
        inline void ClearColor(fvec4 c)
        {
            glClearColor(c.r, c.g, c.b, c.a);
        }
        inline void ClearColor(fvec3 c)
        {
            ClearColor(c.to_vec4(1));
        }

        inline void Viewport(ivec2 pos, ivec2 size)
        {
            glViewport(pos.x, pos.y, size.x, size.y);
        }
        inline void Viewport(ivec2 size)
        {
            Viewport({0,0}, size);
        }

        namespace Blending
        {
            enum Factors
            {
                zero                 = GL_ZERO,
                one                  = GL_ONE,
                src                  = GL_SRC_COLOR,
                one_minus_src        = GL_ONE_MINUS_SRC_COLOR,
                dst                  = GL_DST_COLOR,
                one_minus_dst        = GL_ONE_MINUS_DST_COLOR,
                src_a                = GL_SRC_ALPHA,
                one_minus_src_a      = GL_ONE_MINUS_SRC_ALPHA,
                dst_a                = GL_DST_ALPHA,
                one_minus_dst_a      = GL_ONE_MINUS_DST_ALPHA,
                constant             = GL_CONSTANT_COLOR,
                one_minus_constant   = GL_ONE_MINUS_CONSTANT_COLOR,
                constant_a           = GL_CONSTANT_ALPHA,
                one_minus_constant_a = GL_ONE_MINUS_CONSTANT_ALPHA,
                src_a_saturate       = GL_SRC_ALPHA_SATURATE,
                OnPC
                (
                    src1             = GL_SRC1_COLOR,
                    one_minus_src1   = GL_ONE_MINUS_SRC1_COLOR,
                    src1_a           = GL_SRC1_ALPHA,
                    one_minus_src1_a = GL_ONE_MINUS_SRC1_ALPHA,
                )
            };
            enum Equations
            {
                eq_add              = GL_FUNC_ADD,
                eq_subtract         = GL_FUNC_SUBTRACT,
                eq_reverse_subtract = GL_FUNC_REVERSE_SUBTRACT,
                OnPC
                (
                    eq_min          = GL_MIN,
                    eq_max          = GL_MAX,
                )
            };

            // Func(a,b) and Equation(a) set same parameters for both color and alpha blending.
            // Func(a,b,c,d) and Equation(a,b) set same parameters for color and alpha blending separately.
            inline void Enable() {glEnable(GL_BLEND);}
            inline void Disable() {glDisable(GL_BLEND);}
            inline void Func(Factors src, Factors dst)                             {glBlendFunc(src, dst);}
            inline void Func(Factors src, Factors dst, Factors srca, Factors dsta) {glBlendFuncSeparate(src, dst, srca, dsta);}
            inline void Equation(Equations eq) {glBlendEquation(eq);}
            inline void Equation(Equations eq, Equations eqa) {glBlendEquationSeparate(eq, eqa);}

            inline void FuncOverwrite     () {Func(one, zero);}
            inline void FuncAdd           () {Func(one, one);}
            inline void FuncNormalSimple  () {Func(src_a, one_minus_src_a);} // Resulting alpha is incorrect.
            inline void FuncNormalRawToPre() {Func(src_a, one_minus_src_a, one, one_minus_src_a);} // Output is premultiplied.
            inline void FuncNormalPre     () {Func(one, one_minus_src_a);} // Source and and output are premultiplited
        }

        inline void Depth(bool enable)
        {
            (enable ? glEnable : glDisable)(GL_DEPTH_TEST);
        }
    }

    class Image
    {
        std::vector<u8vec4> data;
        int width = 0;
      public:
        enum Format {png, tga};

        Image() {}
        Image(ivec2 size, const uint8_t *ptr = 0)
        {
            FromMemory(size, ptr);
        }
        Image(Utils::MemoryFile file, bool flip_y = 0)
        {
            FromFile(file, flip_y);
        }
        ivec2 Size() const {return {width, int(data.size()) / width};}
        const u8vec4 *Data() const {return data.data();}
        u8vec4 At(ivec2 pos) const
        {
            if ((pos < 0).any() || (pos >= Size()).any())
                return {0,0,0,0};
            return data[pos.x + pos.y * width];
        }
        u8vec4 &At(ivec2 pos)
        {
            if ((pos < 0).any() || (pos >= Size()).any())
            {
                static u8vec4 ret;
                ret = {0,0,0,0};
                return ret;
            }
            return data[pos.x + pos.y * width];
        }
        void FastSet(ivec2 pos, u8vec4 color) // No bounds checking.
        {
            data[pos.x + pos.y * width] = color;
        }
        u8vec4 FastGet(ivec2 pos) const // No bounds checking.
        {
            return data[pos.x + pos.y * width];
        }
        Image &Fill(u8vec4 color)
        {
            for (auto &it : data)
                it = color;
            return *this;
        }

        void FromMemory(ivec2 size, const uint8_t *ptr = 0)
        {
            width = size.x;
            data.resize(size.product());
            if (ptr)
                std::copy(ptr, ptr + size.product() * sizeof(u8vec4), (uint8_t *)data.data());
        }
        void FromFile(Utils::MemoryFile file, bool flip_y = 0)
        {
            stbi_set_flip_vertically_on_load(flip_y); // This just sets an internal flag, shouldn't be slow.
            ivec2 size;
            [[maybe_unused]] int components;
            uint8_t *ptr = stbi_load_from_memory(file.Data(), file.Size(), &size.x, &size.y, &components, 4);
            if (!ptr)
            {
                width = 0;
                throw cant_parse_image(file.Name());
            }
            FromMemory(size, ptr);
            stbi_image_free(ptr);
        }
        void Destroy()
        {
            data = {};
            width = 0;
        }
        bool Exists() const
        {
            return data.size() != 0;
        }

        [[nodiscard]] static Image Memory(ivec2 size, const uint8_t *ptr = 0)
        {
            return Image(size, ptr);
        }
        [[nodiscard]] static Image File(Utils::MemoryFile file, bool flip_y = 0)
        {
            return Image(file, flip_y);
        }

        void SaveToFile(std::string fname, Format format = png)
        {
            int status = 0;
            switch (format)
            {
              case png:
                status = stbi_write_png(fname.c_str(), Size().x, Size().y, 4, data.data(), width * sizeof(u8vec4));
                return;
              case tga:
                status = stbi_write_tga(fname.c_str(), Size().x, Size().y, 4, data.data());
                return;
            }
            if (!status)
                throw cant_save_image(fname);
        }
    };

    class CharMap
    {
      public:
        struct Char
        {
            ivec2 tex_pos = {0,0}, size = {0,0}, offset = {0,0};
            int advance = 0;
        };
      private:
        static constexpr int pack_size = 256;
        static_assert(0x10000 % pack_size == 0);

        struct CharPack
        {
            std::bitset<pack_size> available; // Filled with zeroes by default.
            std::vector<Char> glyphs;
        };

        int height = 0, ascent = 0, line_skip = 0;
        bool enable_line_gap = 1;
        std::vector<CharPack> data{0x10000 / pack_size};

        using kerning_func_t = std::function<int(uint16_t, uint16_t)>;
        kerning_func_t kerning_func;
      public:
        CharMap() {Set(0xffff, {});}
        void Set(uint16_t index, const Char &glyph)
        {
            auto &sub_vec = data[index / pack_size];
            sub_vec.available[index % pack_size] = 1;
            if (sub_vec.glyphs.empty())
                sub_vec.glyphs.resize(pack_size);
            sub_vec.glyphs[index % pack_size] = glyph;
        }
        bool Available(uint16_t index) const
        {
            return data[index / pack_size].available[index % pack_size];
        }
        const Char &Get(uint16_t index) const // If no glyph with such index is found, returns 0xffff'th glyph.
        {
            if (!Available(index))
                return GetDefault();
            return data[index / pack_size].glyphs[index % pack_size];
        }
        const Char &GetDefault() const // Returns 0xffff'th glyph.
        {
            return Get(0xffff);
        }

        void SetMetrics(int new_height, int new_ascent, int new_line_skip)
        {
            height = new_height;
            ascent = new_ascent;
            line_skip = new_line_skip;
        }
        void EnableLineGap(bool new_enable_line_gap) // If enabled (default), LineSkip() returns height instead.
        {
            enable_line_gap = new_enable_line_gap;
        }
        int Height() const {return height;}
        int Ascent() const {return ascent;}
        int Descent() const {return height-ascent;}
        int LineSkip() const {return enable_line_gap ? line_skip : height;}
        int LineGap() const {return LineSkip() - height;}

        void SetKerning(kerning_func_t func)
        {
            kerning_func = func;
        }
        void ResetKerning()
        {
            kerning_func = 0;
        }
        int Kerning(uint16_t a, uint16_t b) const
        {
            if (!kerning_func)
                return 0;
            return kerning_func(a, b);
        }
    };

    class Font
    {
        struct FreetypeFontFuncs
        {
            template <typename> friend class ::Utils::Handle;
            static FT_Face Create(const std::string &/*display_name*/, const void *data, std::size_t data_size, int font_index)
            {
                static FT_Library lib = 0;
                if (!lib)
                {
                    if (FT_Init_FreeType(&lib))
                        throw cant_init_freetype();
                }

                FT_Open_Args args{};
                args.flags = FT_OPEN_MEMORY;
                args.memory_base = (decltype(args.memory_base))data;
                args.memory_size = data_size;

                FT_Face ret;
                if (FT_Open_Face(lib, &args, font_index, &ret))
                    return 0;

                return ret;
            }
            static void Destroy(FT_Face value)
            {
                FT_Done_Face(value);
            }
            static void Error(const std::string &display_name, const void *, std::size_t, int)
            {
                throw cant_parse_font(display_name);
            }
        };
        using FreetypeFont = Utils::Handle<FreetypeFontFuncs>;

        Utils::MemoryFile file;
        FreetypeFont ft_font;

      public:
        Font() {}
        Font(Utils::MemoryFile new_file, ivec2 size, int index = 0) // One of the components of `size` can be 0, which means 'same as other component'. (That's how freetype works.)
        {
            Create(new_file, size, index);
        }
        Font(Utils::MemoryFile new_file, int size, int index = 0)
        {
            Create(new_file, size, index);
        }
        void Create(Utils::MemoryFile new_file, ivec2 size, int index = 0)
        {
            FreetypeFont new_ft_font({new_file.Name(), new_file.Data(), new_file.Size(), index});
            if (FT_Set_Pixel_Sizes(*new_ft_font, size.x, size.y))
            {
                std::string available_sizes;
                if (new_ft_font.value()->num_fixed_sizes)
                {
                    for (int i = 0; i < new_ft_font.value()->num_fixed_sizes; i++)
                    {
                        if (i != 0)
                            available_sizes += ',';
                        available_sizes += Reflection::Interface::to_string(ivec2(new_ft_font.value()->available_sizes[i].width,
                                                                                  new_ft_font.value()->available_sizes[i].height));
                    }
                }
                else
                    available_sizes = "?";
                throw bad_font_size(size, available_sizes);
            }
            file    = std::move(new_file);
            ft_font = std::move(new_ft_font);
        }
        void Create(Utils::MemoryFile new_file, int size, int index = 0)
        {
            Create(new_file, {0, size}, index);
        }
        void Destroy()
        {
            file.Destroy();
            ft_font.destroy();
        }
        bool Exists() const
        {
            return bool(ft_font);
        }

        int Ascent() const
        {
            return (*ft_font)->size->metrics.ascender >> 6; // It's stored in fixed point format (supposed to be already rounded) and we truncate it.
        }
        int Descent() const
        {
            return -((*ft_font)->size->metrics.descender >> 6); // Sic, we negate the result.
        }
        int Height() const
        {
            return Ascent() + Descent();
        }
        int LineSkip() const
        {
            return (*ft_font)->size->metrics.height >> 6; // Ha, freetype calls it 'height'.
        }
        int LineGap() const
        {
            return LineSkip() - Height();
        }
        bool HasKerning() const
        {
            return FT_HAS_KERNING(*ft_font);
        }
        int Kerning(uint16_t a, uint16_t b) const
        {
            if (!HasKerning())
                return 0;
            FT_Vector vec;
            if (FT_Get_Kerning(*ft_font, FT_Get_Char_Index(*ft_font, a), FT_Get_Char_Index(*ft_font, b), FT_KERNING_DEFAULT, &vec))
                return 0;
            return (vec.x + (1 << 5)) >> 6;
        }
        std::function<int(uint16_t, uint16_t)> KerningFunc() const // Returns 0 if there is no kerning for this font. Freetype font pointer is copied into the function, you should keep corresponding object alive.
        {
            if (!HasKerning())
                return 0;
            return [fnt = *ft_font](uint16_t a, uint16_t b) -> int
            {
                FT_Vector vec;
                if (FT_Get_Kerning(fnt, FT_Get_Char_Index(fnt, a), FT_Get_Char_Index(fnt, b), FT_KERNING_DEFAULT, &vec))
                    return 0;
                return (vec.x + (1 << 5)) >> 6;
            };
        }
        bool HasChar(uint16_t ch) const
        {
            return (bool)FT_Get_Char_Index(*ft_font, ch);
        }

        enum RenderMode
        {
            normal = FT_LOAD_TARGET_NORMAL,
            light  = FT_LOAD_TARGET_LIGHT,
            mono   = FT_LOAD_TARGET_MONO,
        };

        struct CharData
        {
            std::vector<unsigned char> data;
            ivec2 size, offset;
            int advance;

            void CopyImage(Image &img, ivec2 pos, u8vec3 color = {255,255,255})
            {
                for (int y = 0; y < size.y; y++)
                for (int x = 0; x < size.x; x++)
                    ((u8vec4 *)img.Data())[img.Size().x * (y + pos.y) + x + pos.x] = color.to_vec4(data[size.x * y + x]);
            }
        };

        CharData GetChar(uint16_t ch, RenderMode mode) // Freetype caches the last rendered glyph for each font. After you render another one, the returned image reference is no longer valid.
        {
            if (FT_Load_Char(*ft_font, ch, FT_LOAD_RENDER | mode))
                throw cant_render_glyph(ch, "Unknown.");
            auto glyph = (*ft_font)->glyph;
            auto bitmap = glyph->bitmap;
            if (mode == mono ? bitmap.pixel_mode != FT_PIXEL_MODE_MONO : bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || bitmap.num_grays != 256)
                throw cant_render_glyph(ch, "Unexpected pixel format.");
            CharData ret;
            ret.size = ivec2(bitmap.width, bitmap.rows);
            ret.offset = ivec2(glyph->bitmap_left, -glyph->bitmap_top);
            ret.advance = (glyph->advance.x + (1 << 5)) >> 6; // The last 6 bits of `bitmap.advance` represent fractional part, here we perform a poor man's rounding.
            ret.data.resize(ret.size.product());
            if (mode == mono)
            {
                for (int y = 0; y < ret.size.y; y++)
                {
                    unsigned char byte = 0, *byte_ptr = bitmap.buffer + bitmap.pitch * y;
                    for (int x = 0; x < ret.size.x; x++)
                    {
                        if (x % 8 == 0)
                            byte = *byte_ptr++;
                        ret.data[ret.size.x * y + x] = (byte & 128 ? 255 : 0);
                        byte <<= 1;
                    }
                }
            }
            else
            {
                for (int y = 0; y < ret.size.y; y++)
                    std::copy(bitmap.buffer + bitmap.pitch * y, bitmap.buffer + bitmap.pitch * y + ret.size.x, ret.data.data() + ret.size.x * y);
            }
            return ret;
        }

        struct AtlasEntry
        {
            Font &font;
            CharMap &map;
            RenderMode mode;
            std::vector<uint16_t> chars;
            bool kerning; // If this is enabled, the font object must remain alive as long as kerning is used.

            AtlasEntry(Font &font, CharMap &map, RenderMode mode, Utils::ViewRange<uint16_t> char_range, bool kerning = 1)
                : font(font), map(map), mode(mode), chars(char_range.begin(), char_range.end()), kerning(kerning) {}
        };

        static void MakeAtlas(Image &img, ivec2 pos, ivec2 size, Utils::ViewRange<AtlasEntry> entries_range)
        {
            DebugAssert("A rectange specified for a font atlas doesn't fit into the image.", (pos >= 0).all() && (pos + size <= img.Size()).all());
            std::vector<AtlasEntry> entries(entries_range.begin(), entries_range.end());
            int ch_count = 0;
            for (const auto &it : entries)
                ch_count += it.chars.size();

            stbrp_context packer_context;
            std::vector<stbrp_node> packer_buffer(ch_count);
            stbrp_init_target(&packer_context, size.x-1, size.y-1, packer_buffer.data(), packer_buffer.size()); // -1 is for 1 pixel margin. No cleanup is necessary, as well as no error checking.
            std::vector<CharData> chars;
            chars.reserve(ch_count);
            std::vector<stbrp_rect> char_rects;
            char_rects.reserve(ch_count);
            for (auto &entry : entries)
            {
                auto new_end = std::remove_if(entry.chars.begin(), entry.chars.end(), [&](uint16_t ch){return ch == 0xffff || !entry.font.HasChar(ch);});
                if (new_end != entry.chars.end())
                {
                    entry.chars.erase(new_end, entry.chars.end());
                    entry.chars.push_back(0xffff);
                }

                for (const auto &ch : entry.chars)
                {
                    auto font_ch = entry.font.GetChar(ch, entry.mode);
                    stbrp_rect rect;
                    rect.w = font_ch.size.x + 1; // 1 pixel margin
                    rect.h = font_ch.size.y + 1;
                    char_rects.push_back(rect);
                    chars.push_back(std::move(font_ch));
                }
            }
            if (!stbrp_pack_rects(&packer_context, char_rects.data(), char_rects.size()))
                throw not_enough_texture_atlas_space(pos, size);
            int i = 0;
            for (const auto &entry : entries)
            {
                entry.map.SetMetrics(entry.font.Height(), entry.font.Ascent(), entry.font.LineSkip());
                entry.map.SetKerning(entry.font.KerningFunc());
                for (const auto &ch : entry.chars)
                {
                    ivec2 dst_pos = pos + ivec2(char_rects[i].x, char_rects[i].y) + 1;
                    chars[i].CopyImage(img, dst_pos);
                    for (int x = -1; x < chars[i].size.x+1; x++)
                    {
                        img.FastSet(dst_pos + ivec2(x, -1             ), {0,0,0,0});
                        img.FastSet(dst_pos + ivec2(x, chars[i].size.y), {0,0,0,0});
                    }
                    for (int y = 0; y < chars[i].size.y; y++) // Sic! The range for x is different.
                    {
                        img.FastSet(dst_pos + ivec2(-1             , y), {0,0,0,0});
                        img.FastSet(dst_pos + ivec2(chars[i].size.x, y), {0,0,0,0});
                    }
                    entry.map.Set(ch, {dst_pos, chars[i].size, chars[i].offset, chars[i].advance});
                    i++;
                }
            }
        }
    };

    class Texture
    {
        class HandleFuncs
        {
            template <typename> friend class ::Utils::Handle;
            static GLuint Create() {GLuint value; glGenTextures(1, &value); return value;}
            static void Destroy(GLuint value) {glDeleteTextures(1, &value);}
            static void Error() {throw cant_create_gl_resource("Texture");}
        };
        using Handle_t = Utils::Handle<HandleFuncs>;

        using SlotAllocator = Utils::ResourceAllocator<int, int>;
        inline static SlotAllocator slot_allocator{64};

        Handle_t handle;
        int slot = SlotAllocator::not_allocated;

        ivec2 size{};

      public:
        static void SetActiveSlot(int slot) // You don't usually need to call this manually.
        {
            static int old_slot = 0;
            if (slot == old_slot)
                return;
            old_slot = slot;
            glActiveTexture(GL_TEXTURE0 + slot);
        }

        enum InterpMode
        {
            nearest,
            linear,
            min_nearest_mag_linear,
            min_linear_mag_nearest,
        };
        enum WrapMode
        {
            clamp  = GL_CLAMP_TO_EDGE,
            mirror = GL_MIRRORED_REPEAT,
            repeat = GL_REPEAT,
            fill   = OnPC(GL_CLAMP_TO_BORDER) OnMobile(GL_CLAMP_TO_EDGE),
        };

        Texture() {}
        Texture(decltype(nullptr)) : handle(Handle_t::params_t{}) {}
        Texture(Texture &&) = default;
        Texture &operator=(Texture &&) = default;

        Texture(InterpMode mode) // Creates and attaches the texture.
        {
            Create();
            Interpolation(mode);
        }
        Texture(InterpMode mode, ivec2 img_size, const u8vec4 *data = 0) // Creates and attaches the texture.
        {
            Create();
            SetData(img_size, data);
            Interpolation(mode);
        }
        Texture(InterpMode mode, GLint internal_format, GLint format, GLint type, ivec2 img_size, const unsigned char *data = 0) // Creates and attaches the texture.
        {
            Create();
            SetData(internal_format, format, type, img_size, data);
            Interpolation(mode);
        }
        Texture(const Image &img, InterpMode mode) // Creates and attaches the texture.
        {
            Create();
            SetData(img);
            Interpolation(mode);
        }

        void Create()
        {
            handle.create({});
        }
        void Destroy()
        {
            Detach();
            handle.destroy();
        }
        bool Exists() const
        {
            return bool(handle);
        }

        void Attach() // The texture must be created first. Consecutive calls just activate a slot to which the texture was attached.
        {
            DebugAssert("Attempt to attach a null texture.", *handle);
            if (slot != SlotAllocator::not_allocated) // Already attached.
            {
                SetActiveSlot(slot);
                return;
            }
            slot = slot_allocator.alloc();
            if (slot == SlotAllocator::not_allocated)
                throw no_free_texture_slots();
            SetActiveSlot(slot);
            glBindTexture(GL_TEXTURE_2D, *handle);
        }
        void Detach()
        {
            if (slot == SlotAllocator::not_allocated)
                return; // Not attached.
            SetActiveSlot(slot);
            glBindTexture(GL_TEXTURE_2D, 0);
            slot_allocator.free(slot);
            slot = SlotAllocator::not_allocated;
        }
        bool Attached() const
        {
            return slot != SlotAllocator::not_allocated;
        }

        int Slot() const
        {
            return slot;
        }
        GLuint Handle() const
        {
            return *handle;
        }

        void SetData(ivec2 img_size, const u8vec4 *data = 0) // Attaches the texture.
        {
            Attach();
            size = img_size;
            glTexImage2D(GL_TEXTURE_2D, 0, OnPC(GL_RGBA8) OnMobile(GL_RGBA), size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        void SetData(GLint internal_format, GLint format, GLint type, ivec2 img_size, const unsigned char *data = 0) // Attaches the texture.
        {
            Attach();
            size = img_size;
            glTexImage2D(GL_TEXTURE_2D, 0, internal_format, size.x, size.y, 0, format, type, data);
        }
        void SetData(const Image &img) // Attaches the texture.
        {
            SetData(img.Size(), img.Data());
        }

        void SetDataPart(ivec2 pos, ivec2 size, const u8vec4 *data) // Attaches the texture.
        {
            Attach();
            glTexSubImage2D(GL_TEXTURE_2D, 0, pos.x, pos.y, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        void SetDataPart(ivec2 pos, const Image &img) // Attaches the texture.
        {
            SetDataPart(pos, img.Size(), img.Data());
        }

        void Interpolation(InterpMode mode) // Attaches the texture.
        {
            Attach();
            GLenum min_mode, mag_mode;
            if (mode == nearest || mode == min_nearest_mag_linear)
                min_mode = GL_NEAREST;
            else
                min_mode = GL_LINEAR;
            if (mode == nearest || mode == min_linear_mag_nearest)
                mag_mode = GL_NEAREST;
            else
                mag_mode = GL_LINEAR;

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_mode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_mode);
        }

        void WrapX(WrapMode mode) // Attaches the texture.
        {
            Attach();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLuint)mode);
        }
        void WrapY(WrapMode mode) // Attaches the texture.
        {
            Attach();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLuint)mode);
        }
        void Wrap(WrapMode mode) // Attaches the texture.
        {
            WrapX(mode);
            WrapY(mode);
        }

        ivec2 Size() const
        {
            return size;
        }

        static void SetMaxTextureCount(int count) // If you have any attached textures when calling this, it does nothing.
        {
            if (slot_allocator.current_size() != 0)
                return;
            slot_allocator.resize(count);
        }

        ~Texture()
        {
            if (slot != SlotAllocator::not_allocated)
                slot_allocator.free(slot); // OpenGL will unbind the deleted texture automatically.
        }
    };

    template <typename T> const char *GlslTypeName()
    {
        using namespace TemplateUtils::CexprStr;
             if constexpr (std::is_same_v<T, bool        >) return "bool";
        else if constexpr (std::is_same_v<T, float       >) return "float";
        else if constexpr (std::is_same_v<T, double      >) return "double";
        else if constexpr (std::is_same_v<T, int         >) return "int";
        else if constexpr (std::is_same_v<T, unsigned int>) return "uint";
        else if constexpr (Math::type_category<T>::vec_or_mat)
        {
            using Base = typename T::type;
            using Prefix = std::conditional_t<std::is_same_v<Base, float       >, str_lit<>,
                           std::conditional_t<std::is_same_v<Base, double      >, str_lit<'d'>,
                           std::conditional_t<std::is_same_v<Base, bool        >, str_lit<'b'>,
                           std::conditional_t<std::is_same_v<Base, int         >, str_lit<'i'>,
                           std::conditional_t<std::is_same_v<Base, unsigned int>, str_lit<'u'>,
                           void
                           >>>>>;
            static_assert(!std::is_void_v<Prefix>, "No name for vectors of this base type.");
            static_assert(!Math::type_category<T>::mat || std::is_same_v<Base, float> || std::is_same_v<Base, double>, "Matrices aren't allowed to have this base type.");
            using Body = std::conditional_t<Math::type_category<T>::vec, str_lit<'v','e','c'>, str_lit<'m','a','t'>>;
            if constexpr (Math::type_category<T>::vec)
                return str_lit_cat<Prefix, Body, str_lit<T::size + '0'>>::value;
            else if constexpr (T::width == T::height)
                return str_lit_cat<Prefix, Body, str_lit<T::width + '0'>>::value;
            else
                return str_lit_cat<Prefix, Body, str_lit<T::width + '0', 'x', T::height + '0'>>::value;
        }
        else if constexpr (std::is_same_v<T, Texture>)
        {
            return "sampler2D";
        }
        else
        {
            static_assert(std::is_void_v<T>, "No name for this type.");
            return "void";
        }
    }

    class Buffer
    {
        template <typename> friend class ::Utils::Handle;
        static GLuint Create() {GLuint value; glGenBuffers(1, &value); return value;}
        static void Destroy(GLuint value) {glDeleteBuffers(1, &value);} \
        static void Error() {throw cant_create_gl_resource("Buffer");}
        using Handle = Utils::Handle<Buffer>;
        Handle handle;

      public:
        Buffer(decltype(nullptr)) : handle(Handle::params_t{}) {}
        Buffer() {}
        void create() {handle.create({});}
        void destroy() {handle.destroy();}
        GLuint operator*() const {return *handle;}
    };

    enum Usage
    {
        static_draw  = GL_STATIC_DRAW,
        dynamic_draw = GL_DYNAMIC_DRAW,
        stream_draw  = GL_STREAM_DRAW,
    };

    enum Primitive
    {
        points    = GL_POINTS,
        lines     = GL_LINES,
        triangles = GL_TRIANGLES,
    };

    class BufferCommon
    {
        template <typename T> friend class VertexBuffer;
        template <typename T> friend class IndexBuffer;
        inline static GLuint vertex_binding = 0;
        inline static GLuint vertex_draw_binding = 0;
        inline static int active_attribute_count = 0;
        inline static GLuint index_binding = 0;
    };

    template <typename T> class VertexBuffer
    {
        static_assert(Reflection::Interface::field_count<T>() || std::is_void_v<T>, "T must be reflected or be `void`.");

        Buffer buffer;
        int size = 0;

      public:
        VertexBuffer() {}
        VertexBuffer(VertexBuffer &&) = default;
        VertexBuffer &operator=(VertexBuffer &&) = default;

        VertexBuffer(decltype(nullptr)) : buffer(nullptr) {}
        VertexBuffer(int count, const T *data = 0, Usage usage = static_draw) : buffer(nullptr)
        {
            if (count)
                SetData(count, data, usage);
        }

        void Create()
        {
            buffer.create();
            size = 0;
        }
        void Destroy()
        {
            if (*buffer)
            {
                if (BufferCommon::vertex_binding == *buffer)
                    BufferCommon::vertex_binding = 0; // GL unbinds a buffer when it's deleted.
                buffer.destroy();
            }
        }
        bool Exists() const
        {
            return *buffer != 0;
        }

        int Size() const {return size;}
        int ByteSize() const {return size * sizeof(T);}

        void BindStorage() const // Removes draw binding.
        {
            DebugAssert("Attempt to bind a null buffer.", *buffer);
            if (BufferCommon::vertex_binding == *buffer)
                return;
            BufferCommon::vertex_binding = *buffer;
            glBindBuffer(GL_ARRAY_BUFFER, BufferCommon::vertex_binding);
            BufferCommon::vertex_draw_binding = 0;
        }
        static void UnbindStorage() // Removes draw binding.
        {
            // I don't want to check old binding here.
            BufferCommon::vertex_binding = 0;
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            BufferCommon::vertex_draw_binding = 0;
        }
        void BindDraw() const // Also does storage binding.
        {
            DebugAssert("Attempt to bind a null buffer.", *buffer);
            if (BufferCommon::vertex_draw_binding == *buffer)
                return;
            BindStorage();
            BufferCommon::vertex_draw_binding = *buffer;
            SetActiveAttributes(Reflection::Interface::field_count<T>());
            int offset = 0, pos = 0;
            TemplateUtils::for_each(std::make_index_sequence<Reflection::Interface::field_count<T>()>{}, [&](auto index)
            {
                using CurType = Reflection::Interface::field_type<T, index.value>;
                int components;
                if constexpr (Math::type_category<CurType>::vec)
                    components = CurType::size;
                else
                    components = 1;
                glVertexAttribPointer(pos++, components, GL_FLOAT, 0, sizeof(T), (void *)offset);
                offset += sizeof(CurType);
            });
        }
        static void UnbindDraw()
        {
            BufferCommon::vertex_draw_binding = 0;
        }

        void Draw(Primitive p, int from, int count) // Binds for drawing.
        {
            BindDraw();
            glDrawArrays(p, from, count);
        }
        void Draw(Primitive p, int count) // Binds for drawing.
        {
            Draw(p, 0, count);
        }
        void Draw(Primitive p) // Binds for drawing.
        {
            Draw(p, 0, Size());
        }

        void SetData(int count, const T *data = 0, Usage usage = static_draw) // Binds storage.
        {
            BindStorage();
            size = count;
            glBufferData(GL_ARRAY_BUFFER, count * max(1u, sizeof(T)), data, usage);
        }
        void SetDataPart(int obj_offset, int count, const T *data) // Binds storage.
        {
            SetDataPartBytes(obj_offset * sizeof(T), count * max(1u, sizeof(T)), (const char *)data);
        }
        void SetDataPartBytes(int offset, int bytes, const char *data) // Binds storage.
        {
            BindStorage();
            glBufferSubData(GL_ARRAY_BUFFER, offset, bytes, data);
        }

        static void SetActiveAttributes(int count) // Makes sure attributes 0..count-1 are active.
        {
            if (count == BufferCommon::active_attribute_count)
                return;
            if (BufferCommon::active_attribute_count < count)
                do glEnableVertexAttribArray(BufferCommon::active_attribute_count++); while (BufferCommon::active_attribute_count < count);
            else if (BufferCommon::active_attribute_count > count)
                do glDisableVertexAttribArray(--BufferCommon::active_attribute_count); while (BufferCommon::active_attribute_count > count);
        }

        ~VertexBuffer()
        {
            Destroy(); // We need to call this to unbind if necessary.
        }
    };

    template <typename T> class IndexBuffer
    {
        static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || (std::is_same_v<T, uint32_t> && !IsOnMobile), "Invalid type.");
        inline static constexpr GLint type_enum = (std::is_same_v<T, uint8_t>  ? GL_UNSIGNED_BYTE  :
                                                   std::is_same_v<T, uint16_t> ? GL_UNSIGNED_SHORT :
                                                                                       GL_UNSIGNED_INT);

        Buffer buffer;
        int size = 0;

      public:
        IndexBuffer() {}
        IndexBuffer(IndexBuffer &&) = default;
        IndexBuffer &operator=(IndexBuffer &&) = default;

        IndexBuffer(decltype(nullptr)) : buffer(nullptr) {}
        IndexBuffer(int count, const T *data = 0, Usage usage = static_draw) : buffer(nullptr)
        {
            if (count)
                SetData(count, data, usage);
        }

        void Create()
        {
            buffer.create();
            size = 0;
        }
        void Destroy()
        {
            if (*buffer)
            {
                if (BufferCommon::index_binding == *buffer)
                    BufferCommon::index_binding = 0; // GL unbinds a buffer when it's deleted.
                buffer.destroy();
            }
        }
        bool Exists() const
        {
            return *buffer != 0;
        }

        int Size() const {return size;}
        int ByteSize() const {return size * sizeof(T);}

        void Bind() const
        {
            DebugAssert("Attempt to bind a null buffer.", *buffer);
            if (BufferCommon::index_binding == *buffer)
                return;
            BufferCommon::index_binding = *buffer;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferCommon::index_binding);
        }
        static void Unbind()
        {
            // I don't want to check old binding here.
            BufferCommon::index_binding = 0;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        bool Bound() const
        {
            return BufferCommon::index_binding == *buffer;
        }

        void Draw(Primitive p, int from, int count) // Binds for drawing.
        {
            Bind();
            glDrawElements(p, count, type_enum, from * sizeof(T));
        }
        void Draw(Primitive p, int count) // Binds for drawing.
        {
            Draw(p, 0, count);
        }
        void Draw(Primitive p) // Binds for drawing.
        {
            Draw(p, 0, Size());
        }


        void SetData(int count, const T *data = 0, Usage usage = static_draw) // Binds storage.
        {
            Bind();
            size = count;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * max(1u, sizeof(T)), data, usage);
        }
        void SetDataPart(int obj_offset, int count, const T *data) // Binds storage.
        {
            SetDataPartBytes(obj_offset * sizeof(T), count * max(1u, sizeof(T)), (const char *)data);
        }
        void SetDataPartBytes(int offset, int bytes, const char *data) // Binds storage.
        {
            Bind();
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, bytes, data);
        }
    };

    template <typename V, typename I> class VertexIndexBuffer
    {
      public:
        VertexBuffer<V> vertices;
        IndexBuffer<I> indices;

        VertexIndexBuffer() {}
        VertexIndexBuffer(VertexIndexBuffer &&) = default;
        VertexIndexBuffer &operator=(VertexIndexBuffer &&) = default;

        VertexIndexBuffer(decltype(nullptr)) : vertices(nullptr), indices(nullptr) {}
        VertexIndexBuffer(VertexBuffer<V> &&v, IndexBuffer<I> &&i) : vertices(std::move(v)), indices(std::move(i)) {}

        void DrawVertices(Primitive p, int from, int count)
        {
            vertices.Draw(p, from, count);
        }
        void DrawVertices(Primitive p, int count)
        {
            vertices.Draw(p, count);
        }
        void DrawVertices(Primitive p)
        {
            vertices.Draw(p);
        }

        void Draw(Primitive p, int from, int count)
        {
            vertices.BindDraw();
            indices.Draw(p, from, count);
        }
        void Draw(Primitive p, int count)
        {
            vertices.BindDraw();
            indices.Draw(p, count);
        }
        void Draw(Primitive p)
        {
            vertices.BindDraw();
            indices.Draw(p);
        }
    };

    enum OverflowPolicy {flush, expand};

    template <typename T, Primitive P, OverflowPolicy Policy = flush> class RenderQueue
    {
        static_assert(Reflection::Interface::field_count<T>(), "T must be reflected.");
        std::vector<T> data;
        int pos = 0;
        VertexBuffer<T> buffer;

        void Overflow()
        {
            if constexpr (Policy == flush)
                Draw();
            else // expand
            {
                int new_size = data.size();
                     if constexpr (P == lines) new_size /= 2;
                else if constexpr (P == triangles) new_size /= 3;
                new_size = new_size * 3 / 2;
                     if constexpr (P == lines) new_size *= 2;
                else if constexpr (P == triangles) new_size *= 3;
                data.resize(new_size);
                buffer.SetData(new_size, 0, stream_draw);
            }
        }
      public:
        RenderQueue() {}
        RenderQueue(int prim_count)
        {
            Create(prim_count);
        }
        void Create(int prim_count)
        {
            if (prim_count < 1)
                prim_count = 1;
            if constexpr (P == lines)
                prim_count *= 2;
            else if constexpr (P == triangles)
                prim_count *= 3;
            std::vector<T> new_data(prim_count); // Extra exception safety.
            buffer.Create();
            buffer.SetData(prim_count, 0, stream_draw);
            data = std::move(new_data);
            pos = 0;
        }
        void Destroy()
        {
            data = {};
            buffer.Destroy();
        }
        explicit operator bool() const
        {
            return bool(buffer);
        }

        void Reset()
        {
            pos = 0;
        }
        void DrawNoReset()
        {
            DebugAssert("Attempt to flush a null render queue.", buffer.Exists());
            buffer.SetDataPart(0, pos, data.data());
            buffer.Draw(P, pos);
        }
        void Draw()
        {
            DrawNoReset();
            Reset();
        }
        void Point(const T &a)
        {
            static_assert(P == points, "This function for point queues only.");
            if (pos + 1 > int(data.size()))
                Overflow();
            data[pos++] = a;
        }
        void Line(const T &a, const T &b)
        {
            static_assert(P == lines, "This function for line queues only.");
            if (pos + 2 > int(data.size()))
                Overflow();
            data[pos++] = a;
            data[pos++] = b;
        }
        void Triangle(const T &a, const T &b, const T &c)
        {
            static_assert(P == triangles, "This function for triangle queues only.");
            if (pos + 3 > int(data.size()))
                Overflow();
            data[pos++] = a;
            data[pos++] = b;
            data[pos++] = c;
        }
        void Quad(const T &a, const T &b, const T &c, const T &d) // Not really a quad, but rather two triangles.
        {
            Triangle(a, b, d);
            Triangle(d, b, c);
        }
    };

    class Shader
    {
        enum class ShaderType
        {
            vertex   = GL_VERTEX_SHADER,
            fragment = GL_FRAGMENT_SHADER,
        };

        class SeparateShader
        {
            template <typename> friend class ::Utils::Handle;
            static GLuint Create(ShaderType type) {return glCreateShader((GLenum)type);}
            static void Destroy(GLuint value) {glDeleteShader(value);}
            static void Error(ShaderType) {throw cant_create_gl_resource("Shader");}
            using Handle = Utils::Handle<SeparateShader>;
            Handle object;
          public:
            SeparateShader() {}
            SeparateShader(ShaderType type) : object({type}) {}
            void create(ShaderType type) {object.create({type});}
            void destroy() {object.destroy();}
            GLuint handle() const {return *object;}
            SeparateShader &&move() {return (SeparateShader&&)*this;}
            bool compile(const std::string &src) const
            {
                const char *ptr = src.c_str();
                glShaderSource(*object, 1, &ptr, 0);
                glCompileShader(*object);
                GLint status;
                glGetShaderiv(*object, GL_COMPILE_STATUS, &status);
                return status;
            }
            std::string get_log() const
            {
                GLint len;
                glGetShaderiv(*object, GL_INFO_LOG_LENGTH, &len);
                if (len == 0)
                    return "";
                std::string ret;
                ret.resize(len-1); // std::string adds a null-terminator automatically
                glGetShaderInfoLog(*object, len, 0, ret.data());
                return Strings::Escape(Strings::Trim(ret));
            }
        };

        class Program
        {
            template <typename> friend class ::Utils::Handle;
            static GLuint Create() {return glCreateProgram();}
            static void Destroy(GLuint value) {glDeleteProgram(value);}
            static void Error() {throw cant_create_gl_resource("Shader program");}
            using Handle = Utils::Handle<Program>;
            Handle object;
          public:
            Program() {}
            Program(decltype(nullptr)) : object(Handle::params_t{}) {}
            void destroy() {object.destroy();}
            GLuint handle() const {return *object;}
            Program &&move() {return (Program&&)*this;}
            void attach(const SeparateShader &sh) const
            {
                glAttachShader(*object, sh.handle());
            }
            void set_attribute_location(int loc, const std::string &name)
            {
                glBindAttribLocation(*object, loc, name.c_str());
            }
            bool link()
            {
                glLinkProgram(*object);
                GLint status;
                glGetProgramiv(*object, GL_LINK_STATUS, &status);
                return status;
            }
            std::string get_log()
            {
                GLint len;
                glGetProgramiv(*object, GL_INFO_LOG_LENGTH, &len);
                if (len == 0)
                    return "";
                std::string ret;
                ret.resize(len-1); // std::string adds a null-terminator automatically
                glGetProgramInfoLog(*object, len, 0, ret.data());
                return Strings::Escape(Strings::Trim(ret));
            }
        };

        Program program;
        SeparateShader vertex, fragment;
        inline static GLuint binding = 0;
      public:
        Shader() {}
        Shader(Shader &&) = default;
        Shader &operator=(Shader &&) = default;

        struct Config
        {
            std::string version = "330 compatibility";
            std::string vertex_header, fragment_header;
            std::string uniform = "uniform";
            std::string uniform_prefix = "u_";
            std::string attribute = "attribute";
            std::string attribute_prefix = "a_";
            std::string varying_vertex = "varying";
            std::string varying_fragment = "varying";
            std::string varying_prefix = "v_";
        };
        struct Attribute
        {
            int location;
            std::string name;
        };
        void CreateRaw(const std::string &name, const std::string &v_src, const std::string &f_src, Utils::Range<const Attribute> attributes = {})
        {
            Program p(nullptr);
            SeparateShader v(ShaderType::vertex), f(ShaderType::fragment);
            p.attach(v);
            p.attach(f);
            bool v_status = v.compile(v_src);
            bool f_status = f.compile(f_src);
            if (!v_status || !f_status)
                throw shader_compilation_error(name,
                                               v_status ? "OK" : "NOT OK",
                                               f_status ? "OK" : "NOT OK",
                                               '\n' + v.get_log(),
                                               '\n' + f.get_log());
            for (const auto &it : attributes)
                p.set_attribute_location(it.location, it.name);
            if (!p.link())
                throw shader_linking_error(name, '\n' + p.get_log());
            vertex   = v.move();
            fragment = f.move();
            program  = p.move();
        }

        template <typename T> class Uniform
        {
            GLuint sh = 0;
            int loc = -1;
          public:
            using raw_type = T;
            using type = std::remove_extent_t<T>; // Array member type
            inline static constexpr bool is_array   = std::is_array_v<raw_type>,
                                         is_texture = std::is_same_v<type, Texture>,
                                         is_bool    = std::is_same_v<Math::base_type_t<type>, bool>;
            inline static constexpr int len = std::extent_v<std::conditional_t<is_array, raw_type, raw_type[1]>>;
            using effective_type = std::conditional_t<is_texture || is_bool, int, type>; // Textures and bools become ints here
            using base_type = Math::base_type_t<effective_type>; // Vectors and matrices become scalars here.
            Uniform() {}
            Uniform(GLuint sh, int loc) : sh(sh), loc(loc) {}
            const type &operator=(const type &object) const // Binds the shader.
            {
                static_assert(!is_array, "Use .set() to set arrays.");

                DebugAssert("Attempt to bind a null shader.", sh);
                if (sh != binding)
                {
                    binding = sh;
                    glUseProgram(sh);
                }

                if constexpr (is_texture) glUniform1i(loc, object.Slot());
                else if constexpr (std::is_same_v<effective_type, float       >) glUniform1f (loc, object);
                else if constexpr (std::is_same_v<effective_type, fvec2       >) glUniform2f (loc, object.x, object.y);
                else if constexpr (std::is_same_v<effective_type, fvec3       >) glUniform3f (loc, object.x, object.y, object.z);
                else if constexpr (std::is_same_v<effective_type, fvec4       >) glUniform4f (loc, object.x, object.y, object.z, object.w);
                else if constexpr (std::is_same_v<effective_type, int         >) glUniform1i (loc, object);
                else if constexpr (std::is_same_v<effective_type, ivec2       >) glUniform2i (loc, object.x, object.y);
                else if constexpr (std::is_same_v<effective_type, ivec3       >) glUniform3i (loc, object.x, object.y, object.z);
                else if constexpr (std::is_same_v<effective_type, ivec4       >) glUniform4i (loc, object.x, object.y, object.z, object.w);
                else if constexpr (std::is_same_v<effective_type, unsigned int>) glUniform1ui(loc, object);
                else if constexpr (std::is_same_v<effective_type, uvec2       >) glUniform2ui(loc, object.x, object.y);
                else if constexpr (std::is_same_v<effective_type, uvec3       >) glUniform3ui(loc, object.x, object.y, object.z);
                else if constexpr (std::is_same_v<effective_type, uvec4       >) glUniform4ui(loc, object.x, object.y, object.z, object.w);
                else set_no_bind(&object, 1);
                return object;
            }
            void set(const effective_type *ptr, int count, int offset = 0) const
            {
                DebugAssert("Attempt to bind a null shader.", sh);
                if (sh != binding)
                {
                    binding = sh;
                    glUseProgram(sh);
                }
                set_no_bind(ptr, count, offset);
            }
          private:
            void set_no_bind(const effective_type *ptr, int count, int offset = 0) const
            {
                DebugAssert("Uniform index is out of range.", offset + count <= len);
                int l = loc + offset;
                     if constexpr (std::is_same_v<effective_type, float       >) glUniform1fv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fvec2       >) glUniform2fv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fvec3       >) glUniform3fv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fvec4       >) glUniform4fv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, int         >) glUniform1iv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, ivec2       >) glUniform2iv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, ivec3       >) glUniform3iv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, ivec4       >) glUniform4iv (l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, unsigned int>) glUniform1uiv(l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, uvec2       >) glUniform2uiv(l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, uvec3       >) glUniform3uiv(l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, uvec4       >) glUniform4uiv(l, count, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat2       >) glUniformMatrix2fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat3       >) glUniformMatrix3fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat4       >) glUniformMatrix4fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat3x2     >) glUniformMatrix3x2fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat4x2     >) glUniformMatrix4x2fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat2x3     >) glUniformMatrix2x3fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat4x3     >) glUniformMatrix4x3fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat2x4     >) glUniformMatrix2x4fv(l, count, 0, (base_type *)ptr);
                else if constexpr (std::is_same_v<effective_type, fmat3x4     >) glUniformMatrix3x4fv(l, count, 0, (base_type *)ptr);
                else static_assert(std::is_void_v<effective_type>, "Uniforms of this type are not supported.");
            }
        };
        template <typename T> class Uniform_v : public Uniform<T> {public: using Uniform<T>::Uniform; using Uniform<T>::operator=;};
        template <typename T> class Uniform_f : public Uniform<T> {public: using Uniform<T>::Uniform; using Uniform<T>::operator=;};

        template <typename ReflAttributes = void, // Has to be reflected. Regardless of reflected types, shader will get them as floats.
                  typename ReflUniforms   = void> // Has to be reflected and contain only Uniform[_v|_f] structs.
        void Create(const std::string &name, const std::string &v_src, const std::string &f_src, ReflUniforms *uniforms = 0, const Config &cfg = {})
        {
            (void)uniforms;
            std::string v, f;
            v = "#version " + cfg.version + '\n' + cfg.vertex_header + '\n';
            f = "#version " + cfg.version + '\n' + cfg.fragment_header + '\n';
            std::vector<Attribute> attribute_vector;
            v += "#define VARYING(type,name) " + cfg.varying_vertex   + " type " + (cfg.varying_prefix.size() ? cfg.varying_prefix + "##name;\n" : "name;\n");
            f += "#define VARYING(type,name) " + cfg.varying_fragment + " type " + (cfg.varying_prefix.size() ? cfg.varying_prefix + "##name;\n" : "name;\n");
            if constexpr (!std::is_void_v<ReflUniforms>)
            {
                constexpr int field_count = Reflection::Interface::field_count<ReflUniforms>();
                static_assert(field_count > 0, "Unable to reflect common uniforms. Pass `void` as ReflUniforms if you have none.");
                TemplateUtils::for_each(std::make_index_sequence<field_count>{}, [&](auto index)
                {
                    const char *field_name = Reflection::Interface::field_name<ReflUniforms, index.value>();
                    using field_type_raw = Reflection::Interface::field_type<ReflUniforms, index.value>;
                    using field_type_with_extent = typename field_type_raw::raw_type;
                    using field_type = std::remove_extent_t<field_type_with_extent>;
                    std::string s = cfg.uniform + ' ' + GlslTypeName<field_type>() + ' ' + cfg.uniform_prefix + field_name;
                    if (std::is_array_v<field_type_with_extent>)
                        s += Str('[', std::extent_v<field_type_with_extent>, ']');
                    s += ";\n";
                    if constexpr (!std::is_same_v<Uniform_v<field_type>, field_type_raw>)
                        f += s;
                    if constexpr (!std::is_same_v<Uniform_f<field_type>, field_type_raw>)
                        v += s;
                });
            }
            if constexpr (!std::is_void_v<ReflAttributes>)
            {
                constexpr int field_count = Reflection::Interface::field_count<ReflAttributes>();
                static_assert(field_count > 0, "Unable to reflect attributes. Pass `void` as ReflAttributes if you have none.");
                attribute_vector.reserve(field_count);
                TemplateUtils::for_each(std::make_index_sequence<field_count>{}, [&](auto index)
                {
                    const char *field_name = Reflection::Interface::field_name<ReflAttributes, index.value>();
                    using field_type = Math::change_base_type_t<Reflection::Interface::field_type<ReflAttributes, index.value>, float>;
                    v += cfg.attribute + ' ' + GlslTypeName<field_type>() + ' ' + cfg.attribute_prefix + field_name + ";\n";
                    attribute_vector.push_back({int(attribute_vector.size()), cfg.attribute_prefix + field_name});
                });
            }
            CreateRaw(name, v + v_src, f + f_src, attribute_vector);
            if constexpr (!std::is_void_v<ReflUniforms>)
            {
                if (uniforms)
                {
                    constexpr int field_count = Reflection::Interface::field_count<ReflUniforms>();
                    static_assert(field_count > 0, "Unable to reflect common uniforms. Pass `void` as ReflUniforms if you have none.");
                    TemplateUtils::for_each(std::make_index_sequence<field_count>{}, [&](auto index)
                    {
                        const char *field_name = Reflection::Interface::field_name<ReflUniforms, index.value>();
                        auto &ref = Reflection::Interface::field<index.value>(*uniforms);
                        ref = std::remove_reference_t<decltype(ref)>(program.handle(), GetUniformLocation(cfg.uniform_prefix + field_name));
                    });
                }
            }
        }
        void Destroy()
        {
            if (program.handle() && binding == program.handle())
                Unbind(); // GL doesn't delete shaders if they're bound.
            program.destroy();
            vertex.destroy();
            fragment.destroy();
        }
        bool Exists() const
        {
            return program.handle() != 0;
        }

        template <typename ReflAttributes = void,
                  typename ReflUniforms   = void>
        static Shader Make(const std::string &name, const std::string &v_src, const std::string &f_src, ReflUniforms *uniforms = 0, const Config &cfg = {})
        {
            Shader sh;
            sh.Create<ReflAttributes, ReflUniforms>(name, v_src, f_src, uniforms, cfg);
            return sh;
        }

        void Bind() const
        {
            DebugAssert("Attempt to bind a null shader.", program.handle());
            if (program.handle() == binding)
                return;
            binding = program.handle();
            glUseProgram(program.handle());
        }
        static void Unbind()
        {
            // I don't want to check old binding here.
            binding = 0;
            glUseProgram(0);
        }
        bool Bound() const
        {
            DebugAssert("Attempt to use a null shader.", program.handle());
            return binding == program.handle();
        }

        GLint GetUniformLocation(const std::string &name) const
        {
            DebugAssert("Attempt to use a null shader.", program.handle());
            return glGetUniformLocation(program.handle(), name.c_str());
        }
        ~Shader()
        {
            Destroy(); // We need to call this to unbind if necessary.
        }
    };

    class RenderBuffer
    {
        struct HandleFuncs
        {
            template <typename> friend class ::Utils::Handle;
            static GLuint Create() {GLuint value; glGenRenderbuffers(1, &value); return value;}
            static void Destroy(GLuint value) {glDeleteRenderbuffers(1, &value);}
            static void Error() {throw cant_create_gl_resource("Renderbuffer");}
        };
        using Handle_t = Utils::Handle<HandleFuncs>;

        Handle_t handle;

        inline static GLuint binding = 0;

      public:
        RenderBuffer() {}
        RenderBuffer(decltype(nullptr)) : handle(Handle_t::params_t{}) {}
        RenderBuffer(RenderBuffer &&) = default;
        RenderBuffer &operator=(RenderBuffer &&) = default;

        void Create()
        {
            handle.create({});
        }
        void Destroy()
        {
            handle.destroy();
        }
        bool Exists() const
        {
            return bool(handle);
        }

        GLuint Handle() const
        {
            return *handle;
        }

        void Bind() const
        {
            DebugAssert("Attempt to bind a null renderbuffer.", *handle);
            if (binding == *handle)
                return;
            binding = *handle;
            glBindRenderbuffer(GL_RENDERBUFFER, *handle);
        }
        static void Unbind()
        {
            if (binding == 0)
                return;
            binding = 0;
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }

    };

    class FrameBuffer
    {
        struct HandleFuncs
        {
            template <typename> friend class ::Utils::Handle;
            static GLuint Create() {GLuint value; glGenFramebuffers(1, &value); return value;}
            static void Destroy(GLuint value) {glDeleteFramebuffers(1, &value);}
            static void Error() {throw cant_create_gl_resource("Framebuffer");}
        };
        using Handle = Utils::Handle<HandleFuncs>;

        Handle handle;

        inline static GLuint binding = 0;

        struct Attachment
        {
            GLuint type, handle;

            Attachment(const Texture &tex)
            {
                handle = tex.Handle();
                type = GL_TEXTURE_2D;
            }
            Attachment(const RenderBuffer &rb)
            {
                handle = rb.Handle();
                type = GL_RENDERBUFFER;
            }
        };

      public:
        FrameBuffer() {}
        FrameBuffer(decltype(nullptr)) : handle(Handle::params_t{}) {}
        FrameBuffer(FrameBuffer &&) = default;
        FrameBuffer &operator=(FrameBuffer &&) = default;

        void Create()
        {
            handle.create({});
        }
        void Destroy()
        {
            handle.destroy();
        }
        bool Exists() const
        {
            return bool(handle);
        }

        void Bind() const
        {
            DebugAssert("Attempt to bind a null framebuffer.", *handle);
            if (binding == *handle)
                return;
            binding = *handle;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *handle);
        }
        static void Unbind()
        {
            if (binding == 0)
                return;
            binding = 0;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }
        bool Bound() const
        {
            return binding == *handle;
        }

        void Attach(Attachment att) // Binds the framebuffer. All previous color attachements are removed.
        {
            Bind();
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, att.type, att.handle, 0);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
        }
        void Attach(Utils::ViewRange<Attachment> att) // Binds the framebuffer. All previous color attachements are removed.
        {
            Bind();
            int pos = 0;
            for (const auto &it : att)
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + pos++, it.type, it.handle, 0);
            std::vector<GLenum> att_enums(pos);
            std::iota(att_enums.begin(), att_enums.end(), GL_COLOR_ATTACHMENT0);
            glDrawBuffers(pos, att_enums.data());
        }
        void AttachDepth(Attachment att) // Binds the framebuffer.
        {
            Bind();
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, att.type, att.handle, 0);
        }
    };
}

#endif
