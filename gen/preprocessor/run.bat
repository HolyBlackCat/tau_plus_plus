g++ preprocessor.cpp -o preprocessor.exe -std=c++17 -Wall -Wextra -pedantic-errors
pause
preprocessor.exe
del /F /Q preprocessor.exe
g++ preprocessor.h -std=c++17 -Wall -Wextra -pedantic-errors -fsyntax-only
pause
move /Y preprocessor.h ../../src
