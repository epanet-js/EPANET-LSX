#pragma once

typedef char *(*FileIO_ReadFileFn)(const char *path);

char *FileIO_ReadText(const char *path);
void FileIO_SetReadFileFunction(FileIO_ReadFileFn fn);
