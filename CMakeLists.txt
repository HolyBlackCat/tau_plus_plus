cmake_minimum_required(VERSION 3.9)
project(__tau__)


add_compile_options(-pedantic-errors -Wextra -Wall -std=c++17 -I${CMAKE_SOURCE_DIR}/libs/include -I${CMAKE_SOURCE_DIR}/libs/win32/include)
link_directories(libs/win32)


add_executable(__tau__
        libs/glfl.cpp
        src/events.cpp
        src/graphics.cpp
        src/input.cpp
        src/main.cpp
        src/program.cpp
        src/strings.cpp
        src/ui.cpp
        src/window.cpp
        libs/icon.rc
        )

target_link_libraries(__tau__
        mingw32
        SDL2main
        SDL2
        m
        dinput8
        dxguid
        dxerr8
        user32
        gdi32
        winmm
        imm32
        ole32
        oleaut32
        shell32
        version
        uuid
        freetype
        z)
