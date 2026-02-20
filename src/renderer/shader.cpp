#include "shader.hpp"

char *loadFile(char *filename) {
  FILE *file = fopen(filename, "rb");
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  rewind(file);
  char *content = (char*)malloc(len + 1);
  fread(content, 1, len, file);
  content[len] = 0;
  return content;
}

u32 loadShader(char *filename, u32 type) {
  char *full;
  int len = snprintf(0, 0, "res/shaders/%s", filename);
  full = (char*)malloc(len + 1);
  snprintf(full, len + 1, "res/shaders/%s", filename);
  char *fortnite = loadFile(full);
  free(full);

  u32 shader = glCreateShader(type);
  glShaderSource(shader, 1, &fortnite, 0);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    printf("Shader %s failed to compile:\n%s\n", filename, infoLog);
    exit(1);
  }
  return shader;
}

u32 createProgram(u32 vert, u32 frag, int del) { //impure
  u32 shader = glCreateProgram();
  glAttachShader(shader, vert);
  glAttachShader(shader, frag);
  glLinkProgram(shader);
  int success;
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shader, 512, NULL, infoLog);
    printf("Program failed to link:\n%s\n", infoLog);
  }
  if (del) {
    glDeleteShader(vert);
    glDeleteShader(frag);
  }
  return shader;
}

