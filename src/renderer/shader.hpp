#pragma once

#include "../common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "glad/glad.h"

char *loadFile(char *filename);

u32 loadShader(char *filename, u32 type);

u32 createProgram(u32 vert, u32 frag, int del);

