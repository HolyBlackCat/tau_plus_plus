g++ mat_h.cpp -o mat_h.exe -std=c++17 -Wall -Wextra -pedantic-errors
pause
mat_h.exe
del /F /Q mat_h.exe
g++ mat.h -std=c++17 -Wall -Wextra -pedantic-errors -fsyntax-only
pause
move /Y mat.h ../../src