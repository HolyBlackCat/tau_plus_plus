cmake_minimum_required(VERSION 3.9)
project(__tau__)

add_compile_options(-pedantic-errors -Wextra -Wall -std=c++17 -D_GLIBCXX_DEBUG -g -I${CMAKE_SOURCE_DIR}/libs/include -I${CMAKE_SOURCE_DIR}/libs/win32/include)
set(CMAKE_EXE_LINKER_FLAGS "-L${CMAKE_SOURCE_DIR}/libs/win32 -static  -lmingw32 -lSDL2main -lSDL2 -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -lfreetype -lz" CACHE INTERNAL "" FORCE )
set( CMAKE_VERBOSE_MAKEFILE on )


add_executable(__tau__
        libs/glfl.cpp
        src/events.cpp
        src/graphics.cpp
        src/input.cpp
        src/main.cpp
        src/program.cpp
        src/strings.cpp
        src/ui.cpp
        src/window.cpp)
