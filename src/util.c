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

