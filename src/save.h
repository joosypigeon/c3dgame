#ifndef SAVE_H
#define SAVE_H

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEP '\\'
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define PATH_SEP '/'
#endif

#define S_RESOURCES "resources"
#define S_HEIGHTMAPS "heightmaps"

#include <stdbool.h>

bool heightmap_exists(const char *filename);
void save_heightmap(const char *filename, float **heightmap, int rows, int cols);
float **load_matrix(const char *filename, int *out_rows, int *out_cols);
char *build_fullpath(const char *folder1, const char *folder2, const char *filename);

#endif // SAVE_H