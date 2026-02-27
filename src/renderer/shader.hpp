#pragma once

#include "../common.h"
#include <stdlib.h>
#include "glad/glad.h"
#include <string>

std::string loadFile(std::string &filename);

u32 loadShader(std::string filename, u32 type);

u32 createProgram(u32 vert, u32 frag, int del);

