#include "everything.h"

#include <iostream>

Events::AutoErrorHandlers error_handlers;

Window win("TAU++", ivec2(800,600), Window::Settings{}.GlVersion(2,1).GlProfile(Window::any_profile));
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
    }

    void Init()
    {
        Graphics::ClearColor(fvec3(1));

        Graphics::Blending::Enable();
        Graphics::Blending::FuncNormalPre();

        Graphics::Image texture_image_main("assets/texture.png");

        font_object_main.Create("assets/FreeSerif.ttf", 20);

        Graphics::Font::MakeAtlas(texture_image_main, ivec2(0,1024-64), ivec2(1024,64),
        {
            {font_object_main, font_main, Graphics::Font::normal, Strings::Encodings::cp1251()},
        });

        tex_main.SetData(texture_image_main);

        r.Create(0x1000);
        r.SetTexture(tex_main);
        r.SetMatrix(fmat4::ortho2D(win.Size() / ivec2(-2,2), win.Size() / ivec2(2,-2)));
        r.SetDefaultFont(font_main);
        r.BindShader();

        mouse.Transform(win.Size()/2, 1);


        Graphics::Clear(Graphics::color);
        Draw::Accumulator::Overwrite();
    }
}

int main(int, char **)
{
    Draw::Init();

    float angle = 0;
    int time = 1;

    auto Tick = [&]
    {
        Draw::Accumulator::Return();
        for (int i = 0; i < 10; i++)
        {
            float radius = std::pow(time++, 0.75);
            angle += 0.25 / radius;
            r.Quad(fmat2::rotate2D(angle) /mul/ fvec2(radius,0), ivec2(14)).tex(ivec2(1,1)).center().color(fvec3(0.9,0.4,0)).mix(0);
            r.Quad(fmat2::rotate2D(angle+f_pi*2/3) /mul/ fvec2(radius,0), ivec2(14)).tex(ivec2(1,1)).center().color(fvec3(0,0.9,0.4)).mix(0);
            r.Quad(fmat2::rotate2D(angle-f_pi*2/3) /mul/ fvec2(radius,0), ivec2(14)).tex(ivec2(1,1)).center().color(fvec3(0.4,0,0.9)).mix(0);
        }
        r.Finish();
        Draw::Accumulator::Overwrite();
    };
    auto Render = [&]
    {
        Draw::Accumulator::Return();
        //r.Text(mouse.pos(), "Hello, world!").color(fvec3(0)).scale(1);
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
            }
            Tick();
        }

        Render();
        r.Finish();

        win.Swap();
    }
}
