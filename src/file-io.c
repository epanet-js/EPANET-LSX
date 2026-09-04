#include <stdio.h>
#include <stdlib.h>

#include "file-io.h"

static char *readTextFromDisk(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) return NULL;

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }
  long size = ftell(file);
  if (size < 0) {
    fclose(file);
    return NULL;
  }
  rewind(file);

  char *buffer = malloc((size_t)size + 1);
  if (buffer == NULL) {
    fclose(file);
    return NULL;
  }

  size_t read = fread(buffer, 1, (size_t)size, file);
  fclose(file);
  if (read != (size_t)size) {
    free(buffer);
    return NULL;
  }

  buffer[size] = '\0';
  return buffer;
}

static FileIO_ReadFileFn readFile = readTextFromDisk;

char *FileIO_ReadText(const char *path) {
  return readFile(path);
}

void FileIO_SetReadFileFunction(FileIO_ReadFileFn fn) {
  readFile = (fn == NULL) ? readTextFromDisk : fn;
}
