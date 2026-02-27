#include "shader.hpp"
#include <fstream>
#include <sstream>
#include <print>

std::string loadFile(std::string &filename) {
    std::ifstream input(filename);
    std::stringstream sstr;
    input >> sstr.rdbuf();
    return sstr.str();
}

u32 loadShader(std::string filename, u32 type) {
  std::string full = std::format("res/shaders/{}", filename);
  std::string fortnite = loadFile(full);
  char *fortnite2 = fortnite.data();

  u32 shader = glCreateShader(type);
  glShaderSource(shader, 1, &fortnite2, 0);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::println("Shader {} failed to compile:\n{}", filename, infoLog);
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

