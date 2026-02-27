#define GLM_ENABLE_EXPERIMENTAL

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <flecs.h>

#include "cgltf.h"
#include <algorithm>
#include <vector>
#include <filesystem>
#include <print>
#include <format>
#include <dirent.h>
#include <sys/types.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
