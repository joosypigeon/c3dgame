#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "save.h"

char *build_fullpath(const char *folder1, const char *folder2, const char *filename) {
    const size_t length = strlen(folder1) + 1 + strlen(folder2) + 1 + strlen(filename) + 1; // 2 slashes + null terminator
    char *full_path = malloc(length);
    if (!full_path) {
        perror("malloc failed");
        exit(1);
    }
    snprintf(full_path, length, "%s%c%s%c%s", folder1, PATH_SEP, folder2, PATH_SEP, filename);
    return full_path;
}

bool file_exists(const char *filename) {
    printf("Checking if file exists: %s\n", filename);
    FILE *f = fopen(filename, "r");
    if (f) {
        printf("File exists: %s\n", filename);
        fclose(f);
        return true;  // File exists
    }
    printf("File does not exist: %s\n", filename);
    return false;      // File does not exist
}

bool heightmap_exists(const char *filename) {
    const char *folder1 = S_RESOURCES;
    const char *folder2 = S_HEIGHTMAPS;

    const size_t length = strlen(folder1) + strlen(folder2) + strlen(filename) + 3; // 2 slashes + null terminator
    
    char full_path[length];
    snprintf(full_path, length, "%s%c%s%c%s", folder1, PATH_SEP, folder2, PATH_SEP, filename);
    
    return file_exists(full_path);
}   

void save_matrix(const char *filename, float **matrix, int rows, int cols) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Cannot open file for writing");
        return;
    }

    // Write dimensions
    fwrite(&rows, sizeof(int), 1, f);
    fwrite(&cols, sizeof(int), 1, f);

    // Write matrix data row by row
    for (int i = 0; i < rows; i++) {
        fwrite(matrix[i], sizeof(float), cols, f);
    }

    fclose(f);
}

void save_heightmap(const char *filename, float **heightmap, int rows, int cols) {
    const char *folder1 = S_RESOURCES;
    const char *folder2 = S_HEIGHTMAPS;

    const size_t length = strlen(folder1) + 1 + strlen(folder2) + 1 + strlen(filename) + 1; // 2 slashes + null terminator
    
    char full_path[length];
    snprintf(full_path, length, "%s%c%s%c%s", folder1, PATH_SEP, folder2, PATH_SEP, filename);
    printf("Full path: %s\n", full_path);
    
    if (!file_exists(full_path)) {
        mkdir(folder1, 0755);
        const size_t folder2_length = strlen(folder1) + 1 + strlen(folder2) + 1; // 1 for PATH_SEP + 1 for null terminator
        char folder2_path[folder2_length];
        snprintf(folder2_path, folder2_length, "%s%c%s", folder1, PATH_SEP, folder2);
        mkdir(folder2_path, 0755);
    }

    save_matrix(full_path, heightmap, rows, cols);
    printf("Heightmap saved to %s\n", full_path);
}

// Function to load a matrix from a binary file
float **load_matrix(const char *filename, int *out_rows, int *out_cols) {
    printf("load_matrix: Loading matrix from file: %s\n", filename);
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Failed to open file");
        return NULL;
    }

    int rows, cols;

    if (fread(&rows, sizeof(int), 1, f) != 1) {
        perror("Failed to read number of rows");
        fclose(f);
        return NULL;
    }

    if (fread(&cols, sizeof(int), 1, f) != 1) {
        perror("Failed to read number of columns");
        fclose(f);
        return NULL;
    }

    float **matrix = malloc(rows * sizeof(float *));
    if (!matrix) {
        perror("Failed to allocate row pointers");
        fclose(f);
        return NULL;
    }

    for (int i = 0; i < rows; i++) {
        matrix[i] = malloc(cols * sizeof(float));
        if (!matrix[i]) {
            perror("Failed to allocate row data");
            // Free already allocated rows
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            fclose(f);
            return NULL;
        }

        if (fread(matrix[i], sizeof(float), cols, f) != (size_t)cols) {
            perror("Failed to read row data");
            // Clean up
            for (int j = 0; j <= i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            fclose(f);
            return NULL;
        }
    }

    fclose(f);
    *out_rows = rows;
    *out_cols = cols;
    return matrix;
}
