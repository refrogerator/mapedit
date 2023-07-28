all:
	clang++ -std=c++20 -o vulkan -lSDL2 -lEGL -lGL -lOpenGL -lm src/main.cpp -I3rdparty -ggdb -Werror=return-type -I3rdparty/imgui -I3rdparty/imgui/backends 3rdparty/imgui/backends/imgui_impl_sdl2.o 3rdparty/imgui/backends/imgui_impl_opengl3.o 3rdparty/imgui/imgui*.o

run: all
	./vulkan

win:
	x86_64-w64-mingw32-g++ -std=c++20 -o vulkan.exe -L/usr/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows -lSDL2 -lm src/main.cpp -I3rdparty -ggdb -Werror=return-type -Wall -lstdc++

