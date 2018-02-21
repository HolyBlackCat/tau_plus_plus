#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "events.h"


namespace Input
{
    class Key
    {
      public:
        using DeviceType = Events::Input::DeviceType;
        using DeviceID   = Events::Input::DeviceID;
        using Index      = Events::Input::Index;

      private:
        using Action     = Events::Input::Action;

        struct Object
        {
            DeviceType device_type = DeviceType::no_device;
            DeviceID device_id = 0;
            Index index = 0;
        };

        std::vector<Object> objects;
        mutable bool state_pressed = 0, state_released = 0, state_repeated = 0, state_down = 0;

        mutable Events::Time_t last_updated = 0;
        void update() const;

      public:
        Key() {}
        Key(DeviceType device_type, DeviceID device_id, Index index) : objects({{device_type, device_id, index}}) {}

        [[nodiscard]] Key operator|(const Key &o) const;

        [[nodiscard]] bool pressed () const {update(); return state_pressed;}
        [[nodiscard]] bool released() const {update(); return state_released;}
        [[nodiscard]] bool repeated() const {update(); return state_repeated;}
        [[nodiscard]] bool down    () const {update(); return state_down;}
        [[nodiscard]] bool up      () const {update(); return !state_down;}
    };

    class Mouse
    {
      public:
        using DeviceType = Events::Input::DeviceType;
        using DeviceID   = Events::Input::DeviceID;

      private:
        DeviceType device_type;
        DeviceID device_id;

        ivec2 offset = ivec2(0);
        float scale = 1;

      public:
        Mouse(DeviceType device_type, DeviceID device_id)
          : device_type(device_type), device_id(device_id),
            left      {device_type, 0, 1},
            middle    {device_type, 0, 2},
            right     {device_type, 0, 3},
            x1        {device_type, 0, 4},
            x2        {device_type, 0, 5},
            any_button{device_type, 0, 0}
        {}
        Mouse() : Mouse(DeviceType::mouse, 0) {}

        Key left, middle, right, x1, x2, any_button;
        [[nodiscard]] Key button(int index) const;

        [[nodiscard]] ivec2 pos() const;
        [[nodiscard]] ivec2 shift() const;
        [[nodiscard]] ivec2 wheel() const;

        [[nodiscard]] ivec2 raw_pos() const; // Like `pos()`, but in unmapped coordinates.
        [[nodiscard]] ivec2 rel_pos() const; // Like `shift()`, but in unmapped coordinates. Should be used in relative mode instead of `shift()`.

        void Transform(ivec2 new_offset, float new_scale);

        static void Show(bool s);
        static void Relative(bool r);
    };

    std::string RawText(); // This checks if the input is valid utf8 and can be represented using 16 bits per char.

    // It remembers the address of the last passed string and resets internal cursor position if it changes. Null can be used to reset too.
    // Line-feeds will never be added to the `str`.
    void Text(std::string *str, int len_cap = -1, const std::string &allowed_chars = "");

    int TextCursorPos(); // The cursor should be drawn before or under the characther with returned index.
    int TextCursorBytePos();

    namespace Keys
    {
        inline const Key any = {Key::DeviceType::keyboard, 0, 0};

        #define KEY(name_, sdl_name_) inline const Key name_ = {Key::DeviceType::keyboard, 0, SDL_SCANCODE_##sdl_name_};
        KEY(_0, 0) KEY(_1, 1) KEY(_2, 2) KEY(_3, 3) KEY(_4, 4)
        KEY(_5, 5) KEY(_6, 6) KEY(_7, 7) KEY(_8, 8) KEY(_9, 9)

        KEY(a, A) KEY(b, B) KEY(c, C) KEY(d, D) KEY(e, E)
        KEY(f, F) KEY(g, G) KEY(h, H) KEY(i, I) KEY(j, J)
        KEY(k, K) KEY(l, L) KEY(m, M) KEY(n, N) KEY(o, O)
        KEY(p, P) KEY(q, Q) KEY(r, R) KEY(s, S) KEY(t, T)
        KEY(u, U) KEY(v, V) KEY(w, W) KEY(x, X) KEY(y, Y)
        KEY(z, Z)

        KEY(apostrophe   , APOSTROPHE  )
        KEY(comma        , COMMA       )
        KEY(period       , PERIOD      )
        KEY(minus        , MINUS       )
        KEY(equals       , EQUALS      )
        KEY(semicolon    , SEMICOLON   )
        KEY(slash        , SLASH       )
        KEY(l_bracket    , LEFTBRACKET )
        KEY(r_bracket    , RIGHTBRACKET)
        KEY(backslash    , BACKSLASH   )
        KEY(grave        , GRAVE       )

        KEY(num_0, KP_0) KEY(num_1, KP_1) KEY(num_2, KP_2) KEY(num_3, KP_3) KEY(num_4, KP_4)
        KEY(num_5, KP_5) KEY(num_6, KP_6) KEY(num_7, KP_7) KEY(num_8, KP_8) KEY(num_9, KP_9)

        KEY(num_period   , KP_PERIOD   )
        KEY(num_equals   , KP_EQUALS   )
        KEY(num_enter    , KP_ENTER    )
        KEY(num_plus     , KP_PLUS     )
        KEY(num_minus    , KP_MINUS    )
        KEY(num_multiply , KP_MULTIPLY )
        KEY(num_divide   , KP_DIVIDE   )

        KEY(f1 , F1 ) KEY(f2 , F2 ) KEY(f3 , F3 ) KEY(f4 , F4 ) KEY(f5 , F5 ) KEY(f6 , F6 )
        KEY(f7 , F7 ) KEY(f8 , F8 ) KEY(f9 , F9 ) KEY(f10, F10) KEY(f11, F11) KEY(f12, F12)
        KEY(f13, F13) KEY(f14, F14) KEY(f15, F15) KEY(f16, F16) KEY(f17, F17) KEY(f18, F18)
        KEY(f19, F19) KEY(f20, F20) KEY(f21, F21) KEY(f22, F22) KEY(f23, F23) KEY(f24, F24)

        KEY(menu         , MENU        )
        KEY(escape       , ESCAPE      )
        KEY(enter        , RETURN      )
        KEY(tab          , TAB         )
        KEY(backspace    , BACKSPACE   )
        KEY(insert       , INSERT      )
        KEY(del          , DELETE      )
        KEY(up           , UP          )
        KEY(down         , DOWN        )
        KEY(left         , LEFT        )
        KEY(right        , RIGHT       )
        KEY(page_up      , PAGEUP      )
        KEY(page_down    , PAGEDOWN    )
        KEY(home         , HOME        )
        KEY(end          , END         )
        KEY(caps_lock    , CAPSLOCK    )
        KEY(scroll_lock  , SCROLLLOCK  )
        KEY(num_lock     , NUMLOCKCLEAR)
        KEY(print_screen , PRINTSCREEN )
        KEY(pause        , PAUSE       )
        KEY(space        , SPACE       )

        KEY(l_shift      , LSHIFT      )
        KEY(r_shift      , RSHIFT      )
        KEY(l_ctrl       , LCTRL       )
        KEY(r_ctrl       , RCTRL       )
        KEY(l_alt        , LALT        )
        KEY(r_alt        , RALT        )
        KEY(l_gui        , LGUI        )
        KEY(r_gui        , RGUI        )
        #undef KEY
    }
}

namespace Keys = Input::Keys;

#endif
