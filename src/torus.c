#include "physics.h"

#include "torus.h"
#include "fbm_with_function_pointer.h"

#include <stdlib.h>

#include <stdio.h>

#include <float.h>

#include "save.h"

#include <assert.h>

typedef struct TorusCoords {
    float theta, phi;
    float cosTheta, sinTheta;
    float cosPhi, sinPhi;
} TorusCoords;

static TorusCoords torusCoords = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};

static float R = -1.0f;
static float r = -1.0f;

void SetTorusDimensions(float major, float minor) {
    R = major;
    r = minor;
}
#define WRAP_MOD(a, m) (((a) % (m) + (m)) % (m))

float **get_heightmap(const char *filename) {
    float **heightmap = NULL;
    if(heightmap_exists(filename)) {
        size_t rows = 0, cols = 0;
        char *fullpath = build_fullpath(S_RESOURCES, S_HEIGHTMAPS, filename);
        heightmap = load_matrix(fullpath, &rows, &cols);
        assert(heightmap != NULL && rows > 0 && cols > 0);
        assert(rows == MONITOR_HEIGHT && cols == MONITOR_WIDTH);
        printf("Heightmap loaded from %s\n", filename);
        return heightmap;
    } 
    printf("Heightmap does not exist at %s, generating new one.\n", filename);
    heightmap = malloc(MONITOR_HEIGHT * sizeof(float *));
    for (size_t i = 0; i < MONITOR_HEIGHT; i++) {
        heightmap[i] = malloc(MONITOR_WIDTH * sizeof(float));
    }

    perlin_init(42);  // consistent seed

    float scale = 0.005f;
    printf("Generating heightmap with scale: %f\n", scale);
    printf("MONITOR_WIDTH: %zu, MONITOR_HEIGHT: %zu\n", MONITOR_WIDTH, MONITOR_HEIGHT);
    printf("R: %f, r: %f\n", R, r);
    NoiseType noiseType = NOISE_PERLIN;  // Change this to switch noise types
    NoiseFunction4D fn = NULL;
    switch (noiseType) {
        case NOISE_VALUE:
            fn = noise4d;
            break;
        case NOISE_PERLIN:
            printf("Using Perlin noise with seed 42\n");
            perlin_init(42);  // Reinitialize Perlin noise with a consistent seed
            fn = perlin_noise4d;  // Use Perlin noise
            break;
        case NOISE_SIMPLEX:
            fn = simplex4d;  // Use Simplex noise
            break;
        default:
            fprintf(stderr, "Unknown noise type: %d\n", noiseType);
            exit(1);
    }

    #pragma omp parallel for schedule(static)
    for (size_t v = 0; v < MONITOR_HEIGHT; v++) {
        for (size_t u = 0; u < MONITOR_WIDTH; u++) {
            float nx = R * cos(u * 2.0f * PI / MONITOR_WIDTH) * scale;
            float ny = R * sin(u * 2.0f * PI / MONITOR_WIDTH) * scale;
            float nz = r * cos(v * 2.0f * PI / MONITOR_HEIGHT) * scale;
            float nw = r * sin(v * 2.0f * PI / MONITOR_HEIGHT) * scale;

            const float disp_offset = 0.1f;
            const float displacement_strength = 1.0f;

            float dx = fbm4d_fn(nx + disp_offset, ny, nz, nw, 6, 2.0f, 0.5f, fn);
            float dy = fbm4d_fn(nx, ny + disp_offset, nz, nw, 6, 2.0f, 0.5f, fn);
            float dz = fbm4d_fn(nx, ny, nz + disp_offset, nw, 6, 2.0f, 0.5f, fn);
            float dw = fbm4d_fn(nx, ny, nz, nw + disp_offset, 6, 2.0f, 0.5f, fn);

            float warped_noise = fbm4d_fn(
                nx + displacement_strength * dx,
                ny + displacement_strength * dy,
                nz + displacement_strength * dz,
                nw + displacement_strength * dw,
                6, 2.0f, 0.5f, fn
            );

            warped_noise = powf(warped_noise, 4.0f);  // boost height contrast
            assert(warped_noise >= 0.0f && warped_noise <= 1.0f); // Ensure noise is in [0, 1]
            heightmap[v][u] = warped_noise;
        }
    }
    printf("Heightmap generated with dimensions: %zu x %zu\n", MONITOR_WIDTH, MONITOR_HEIGHT);
    return heightmap;
}

// Generates a torus mesh with the specified number of rings and sides.
Mesh MyGenTorusMesh(size_t rings, size_t sides) {
    unsigned char **image = malloc(MONITOR_HEIGHT * sizeof(unsigned char *));
    if (!image) {
        perror("malloc failed");
        exit(1);
    }

    for (size_t y = 0; y < MONITOR_HEIGHT; y++) {
        image[y] = malloc(MONITOR_WIDTH * sizeof(unsigned char));
        if (!image[y]) {
            perror("malloc failed");
            exit(1);
        }
    }

    float **heightmap = get_heightmap("heightmap.bin");

    float min = FLT_MIN;
    float max = -FLT_MAX;
    for (size_t v = 0; v < MONITOR_HEIGHT; v++) {
        for (size_t u = 0; u < MONITOR_WIDTH; u++) {
            float height = heightmap[v][u];
            assert(height >= 0.0f && height <= 1.0f); // Ensure noise is in [0, 1]
            if (height < min) min = height;
            if (height > max) max = height;
            image[v][u] = (unsigned char)(height * 255.0f);
        }
    }
    printf("Heightmap min: %f, max: %f\n", min, max);

    FILE *f = fopen("heightmap_T.pgm", "wb");
    if (!f) {
        perror("Cannot write image");
        exit(1);
    }

    fprintf(f, "P5\n%zu %zu\n255\n", MONITOR_WIDTH, MONITOR_HEIGHT);  // P5 = binary greyscale
    for (size_t y = 0; y < MONITOR_HEIGHT; y++) {
        if (fwrite(image[y], sizeof(unsigned char), MONITOR_WIDTH, f) != MONITOR_WIDTH) {
            perror("Error writing image data");
            fclose(f);
            exit(1);
        }
    }
    fclose(f);

    printf("Heightmap written to heightmap_T.pgm\n");

    // Free the image memory
    for (size_t y = 0; y < MONITOR_HEIGHT; y++) {
        free(image[y]);
    }
    free(image);

    float upper_bound = 400.0f;
    float lower_bound = 0.0f;
    float gradient = (upper_bound - lower_bound) / (max - min);
    printf("Gradient: %f\n", gradient);


    // 1. Allocate vertex and normal grids
    Vector3 **vertexGrid = MemAlloc(rings * sizeof(Vector3 *));
    Vector3 **normalGrid = MemAlloc(rings * sizeof(Vector3 *));
    for (size_t i = 0; i < rings; i++) {
        vertexGrid[i] = MemAlloc(sides * sizeof(Vector3));
        normalGrid[i] = MemAlloc(sides * sizeof(Vector3));
        for (size_t j = 0; j < sides; j++) {
            normalGrid[i][j] = (Vector3){0.0f, 0.0f, 0.0f};
        }
    }

    // 2. Fill vertexGrid with positions (and optionally sample height)
    for (size_t i = 0; i < rings; i++) {
        float theta = (float)i / rings * 2.0f * PI;
        float cosTheta = cosf(theta);
        float sinTheta = sinf(theta);
        for (size_t j = 0; j < sides; j++) {
            float phi = ((float)j / sides) * 2.0f * PI;
            float cosPhi = cosf(phi);
            float sinPhi = sinf(phi);

            float x = (R + r * cosPhi) * cosTheta;
            float y = r * sinPhi;
            float z = (R + r * cosPhi) * sinTheta;

            float nx = cosPhi * cosTheta;
            float ny = sinPhi;
            float nz = cosPhi * sinTheta;

            Vector3 position = (Vector3){ x, y, z };
            Vector3 normal = (Vector3){ nx, ny, nz };

            int sx = WRAP_MOD((int)z, MONITOR_WIDTH);
            int sy = WRAP_MOD((int)(MONITOR_HEIGHT - x), MONITOR_HEIGHT);

            float height = heightmap[sy][sx];
            float adjusted_height = lower_bound + (height - min) * gradient;
            
            vertexGrid[i][j] = Vector3Add(position,Vector3Scale(normal, adjusted_height)); 
        }
    }

    char *filename = "heightmap.bin";
    if(!heightmap_exists(filename)) {
        save_heightmap(filename, heightmap, MONITOR_HEIGHT, MONITOR_WIDTH);
        printf("Heightmap saved to %s\n", filename);
    } else {
        printf("Heightmap already exists at %s, skipping save.\n", filename);
    }
    for (size_t i = 0; i < MONITOR_HEIGHT; i++) {
        free(heightmap[i]);
    }
    free(heightmap);


    for (size_t i = 0; i < rings; i++) {
        size_t i1 = (i + 1) % rings;
        for (size_t j = 0; j < sides; j++) {
            size_t j1 = (j + 1) % sides;

            Vector3 p00 = vertexGrid[i][j];
            Vector3 p01 = vertexGrid[i][j1];
            Vector3 p10 = vertexGrid[i1][j];
            Vector3 p11 = vertexGrid[i1][j1];

            // Triangle 1: p00, p01, p10
            Vector3 edge1 = Vector3Subtract(p01, p00);
            Vector3 edge2 = Vector3Subtract(p10, p00);
            Vector3 n1 = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

            normalGrid[i][j] = Vector3Add(normalGrid[i][j], n1);
            normalGrid[i][j1] = Vector3Add(normalGrid[i][j1], n1);
            normalGrid[i1][j] = Vector3Add(normalGrid[i1][j], n1);

            // Triangle 2: p10, p01, p11
            Vector3 edge3 = Vector3Subtract(p01, p10);
            Vector3 edge4 = Vector3Subtract(p11, p10);
            Vector3 n2 = Vector3Normalize(Vector3CrossProduct(edge3, edge4));

            normalGrid[i1][j] = Vector3Add(normalGrid[i1][j], n2);
            normalGrid[i][j1] = Vector3Add(normalGrid[i][j1], n2);
            normalGrid[i1][j1] = Vector3Add(normalGrid[i1][j1], n2);
        }
    }

    // 3. Generate indices (each quad = 2 triangles = 6 indices)
    int indexCount = rings * sides * 6;
    unsigned short *indices = MemAlloc(indexCount * sizeof(unsigned short)); // Use uint16 for Raylib
    size_t k = 0;
    for (size_t i = 0; i < rings; i++) {
        size_t i1 = (i + 1) % rings;
        for (size_t j = 0; j < sides; j++) {
            size_t j1 = (j + 1) % sides;

            int v00 = i * sides + j;
            int v01 = i * sides + j1;
            int v10 = i1 * sides + j;
            int v11 = i1 * sides + j1;

            // Triangle 1
            indices[k++] = v00;
            indices[k++] = v01;
            indices[k++] = v10;

            // Triangle 2
            indices[k++] = v10;
            indices[k++] = v01;
            indices[k++] = v11;
        }
    }

    int vertexCount = rings * sides;
    Vector3 *flatVertices = MemAlloc(vertexCount * sizeof(Vector3));
    Vector3 *flatNormals = MemAlloc(vertexCount * sizeof(Vector3));
    Vector2 *texcoords = MemAlloc(vertexCount * sizeof(Vector2));

    for (size_t i = 0; i < rings; i++) {
        for (size_t j = 0; j < sides; j++) {
            size_t idx = i * sides + j;
            flatVertices[idx] = vertexGrid[i][j];
            flatNormals[idx] = Vector3Normalize(normalGrid[i][j]);
            texcoords[idx] = (Vector2){ (float)j / sides, (float)i / rings };
        }
    }



    Mesh mesh = { 0 };
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = indexCount / 3;
    mesh.vertices = (float *)flatVertices;
    mesh.normals = (float *)flatNormals;
    mesh.texcoords = (float *)texcoords;
    mesh.indices = (unsigned short *)indices;

    UploadMesh(&mesh, false);
    return mesh;
}

Mesh MyGenFlatTorusMesh(size_t rings, size_t sides) {
    unsigned char **image = malloc(MONITOR_HEIGHT * sizeof(unsigned char *));
    if (!image) {
        perror("malloc failed");
        exit(1);
    }

    for (size_t y = 0; y < MONITOR_HEIGHT; y++) {
        image[y] = malloc(MONITOR_WIDTH * sizeof(unsigned char));
        if (!image[y]) {
            perror("malloc failed");
            exit(1);
        }
    }

    float **heightmap = get_heightmap("heightmap.bin");

    float min = FLT_MIN;
    float max = -FLT_MAX;
    for (size_t v = 0; v < MONITOR_HEIGHT; v++) {
        for (size_t u = 0; u < MONITOR_WIDTH; u++) {
            float height = heightmap[v][u];
            assert(height >= 0.0f && height <= 1.0f); // Ensure noise is in [0, 1]
            if (height < min) min = height;
            if (height > max) max = height;
            image[v][u] = (unsigned char)(height * 255.0f);
        }
    }
    printf("Heightmap min: %f, max: %f\n", min, max);

    FILE *f = fopen("heightmap.pgm", "wb");
    if (!f) {
        perror("Cannot write image");
        exit(1);
    }

    fprintf(f, "P5\n%zu %zu\n255\n", MONITOR_WIDTH, MONITOR_HEIGHT);  // P5 = binary greyscale
    for (size_t y = 0; y < MONITOR_HEIGHT; y++) {
        if (fwrite(image[y], sizeof(unsigned char), MONITOR_WIDTH, f) != MONITOR_WIDTH) {
            perror("Error writing image data");
            fclose(f);
            exit(1);
        }
    }
    fclose(f);

    printf("Heightmap written to heightmap_T.pgm\n");

    // Free the image memory
    for (size_t y = 0; y < MONITOR_HEIGHT; y++) {
        free(image[y]);
    }
    free(image);

    // 1. Allocate vertex and normal grids
    Vector3 **vertexGrid = MemAlloc(rings * sizeof(Vector3 *));
    Vector3 **normalGrid = MemAlloc(rings * sizeof(Vector3 *));
    for (size_t i = 0; i < rings; i++) {
        vertexGrid[i] = MemAlloc(sides * sizeof(Vector3));
        normalGrid[i] = MemAlloc(sides * sizeof(Vector3));
        for (size_t j = 0; j < sides; j++) {
            normalGrid[i][j] = (Vector3){0.0f, 0.0f, 0.0f};
        }
    }

    // 2. Fill vertexGrid with positions (and optionally sample height)
    float upper_bound = 100.0f;
    float lower_bound = 0.0f;
    float gradient = (upper_bound - lower_bound) / (max - min);
    printf("Gradient: %f\n", gradient);
    for (size_t i = 0; i < rings; i++) {
        float theta = (float)i / rings * 2.0f * PI;
        for (size_t j = 0; j < sides; j++) {
            float phi = (float)j / sides * 2.0f * PI;

            float x = HALF_MONITOR_HEIGHT - phi * r;
            float z = R * theta - HALF_MONITOR_WIDTH;

            int sx = WRAP_MOD((int)(z + HALF_MONITOR_WIDTH), MONITOR_WIDTH);
            int sy = WRAP_MOD((int)(HALF_MONITOR_HEIGHT - x), MONITOR_HEIGHT);

            float height = heightmap[sy][sx];
            float adjusted_height = lower_bound + (height - min) * gradient;

            vertexGrid[i][j] = (Vector3){ x, adjusted_height, z };
        }
    }

    char *filename = "heightmap.bin";
    if(!heightmap_exists(filename)) {
        save_heightmap(filename, heightmap, MONITOR_HEIGHT, MONITOR_WIDTH);
        printf("Heightmap saved to %s\n", filename);
    } else {
        printf("Heightmap already exists at %s, skipping save.\n", filename);
    }
    for (size_t i = 0; i < MONITOR_HEIGHT; i++) {
        free(heightmap[i]);
    }
    free(heightmap);

    for (size_t i = 0; i < rings; i++) {
        size_t i1 = (i + 1) % rings;
        for (size_t j = 0; j < sides; j++) {
            size_t j1 = (j + 1) % sides;

            Vector3 p00 = vertexGrid[i][j];
            Vector3 p01 = vertexGrid[i][j1];
            Vector3 p10 = vertexGrid[i1][j];
            Vector3 p11 = vertexGrid[i1][j1];

            // Triangle 1: p00, p01, p10
            Vector3 edge1 = Vector3Subtract(p01, p00);
            Vector3 edge2 = Vector3Subtract(p10, p00);
            Vector3 n1 = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

            normalGrid[i][j] = Vector3Add(normalGrid[i][j], n1);
            normalGrid[i][j1] = Vector3Add(normalGrid[i][j1], n1);
            normalGrid[i1][j] = Vector3Add(normalGrid[i1][j], n1);

            // Triangle 2: p10, p01, p11
            Vector3 edge3 = Vector3Subtract(p01, p10);
            Vector3 edge4 = Vector3Subtract(p11, p10);
            Vector3 n2 = Vector3Normalize(Vector3CrossProduct(edge3, edge4));

            normalGrid[i1][j] = Vector3Add(normalGrid[i1][j], n2);
            normalGrid[i][j1] = Vector3Add(normalGrid[i][j1], n2);
            normalGrid[i1][j1] = Vector3Add(normalGrid[i1][j1], n2);
        }
    }

    // 3. Generate indices (each quad = 2 triangles = 6 indices)
    int indexCount = rings * sides * 6;
    unsigned short *indices = MemAlloc(indexCount * sizeof(unsigned short)); // Use uint16 for Raylib
    size_t k = 0;
    for (size_t i = 0; i < rings - 1; i++) {
        size_t i1 = i + 1;
        for (size_t j = 0; j < sides - 1; j++) {
            size_t j1 = j + 1;

            int v00 = i * sides + j;
            int v01 = i * sides + j1;
            int v10 = i1 * sides + j;
            int v11 = i1 * sides + j1;

            // Triangle 1
            indices[k++] = v00;
            indices[k++] = v01;
            indices[k++] = v10;

            // Triangle 2
            indices[k++] = v10;
            indices[k++] = v01;
            indices[k++] = v11;
        }
    }

    int vertexCount = rings * sides;
    Vector3 *flatVertices = MemAlloc(vertexCount * sizeof(Vector3));
    Vector3 *flatNormals = MemAlloc(vertexCount * sizeof(Vector3));
    Vector2 *texcoords = MemAlloc(vertexCount * sizeof(Vector2));

    for (size_t i = 0; i < rings; i++) {
        for (size_t j = 0; j < sides; j++) {
            size_t idx = i * sides + j;
            flatVertices[idx] = vertexGrid[i][j];
            flatNormals[idx] = Vector3Normalize(normalGrid[i][j]);
            texcoords[idx] = (Vector2){ (float)j / sides, (float)i / rings };
        }
    }

    Mesh mesh = { 0 };
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = indexCount / 3;
    mesh.vertices = (float *)flatVertices;
    mesh.normals = (float *)flatNormals;
    mesh.texcoords = (float *)texcoords;
    mesh.indices = (unsigned short *)indices;

    UploadMesh(&mesh, false);
    return mesh;
}



float get_theta(float u) {
        return 2 * PI * u / MONITOR_WIDTH;
}

float get_phi(float v) {
        return 2 * PI * v / MONITOR_HEIGHT;
}

Vector3 get_torus_normal(float u, float v) {
    float theta = get_theta(u);
    float phi = get_phi(v);

    float nx = cosf(phi) * cosf(theta);
    float ny = sinf(phi);
    float nz = cosf(phi) * sinf(theta);

    return (Vector3){ nx, ny, nz };
}

Vector3 get_phi_tangent(float u, float v) {
    float theta = get_theta(u);
    float phi = get_phi(v);

    float tx = -sinf(phi) * cosf(theta);
    float ty = cosf(phi);
    float tz = -sinf(phi) * sinf(theta);

    return (Vector3){ tx, ty, tz };
}

Vector3 get_theta_tangent(float u, float v) {
    (void)v;  // Silences unused parameter warning
    float theta = get_theta(u);

    float tx = -sinf(theta);
    float ty = 0.0f;
    float tz =  cosf(theta);

    return (Vector3){ tx, ty, tz };
}

Vector3 get_torus_position(float u, float v) {
    (void)v;  // Silences unused parameter warning
    float theta = get_theta(u);
    float phi = get_phi(v);

    float x = (R + r * cosf(phi)) * cosf(theta);
    float y = r * sinf(phi);
    float z = (R + r * cosf(phi)) * sinf(theta);

    return (Vector3){ x, y, z };
}

void set_torus_coords(float u, float v) {
    torusCoords.theta = get_theta(u);
    torusCoords.phi = get_phi(v);
    torusCoords.cosTheta = cosf(torusCoords.theta);
    torusCoords.sinTheta = sinf(torusCoords.theta);
    torusCoords.cosPhi = cosf(torusCoords.phi);
    torusCoords.sinPhi = sinf(torusCoords.phi);
}

Vector3 get_torus_position_fast() {
    return (Vector3){ (R + r * torusCoords.cosPhi) * torusCoords.cosTheta,
                      r * torusCoords.sinPhi,
                      (R + r * torusCoords.cosPhi) * torusCoords.sinTheta };
}

Vector3 get_torus_normal_fast() {
    return (Vector3){ torusCoords.cosPhi * torusCoords.cosTheta,
                      torusCoords.sinPhi,
                      torusCoords.cosPhi * torusCoords.sinTheta };
}
Vector3 get_theta_tangent_fast() {
    return (Vector3){ -torusCoords.sinTheta, 0.0f, torusCoords.cosTheta };
}
Vector3 get_phi_tangent_fast() {
    return (Vector3){ -torusCoords.sinPhi * torusCoords.cosTheta,
                      torusCoords.cosPhi,
                      -torusCoords.sinPhi * torusCoords.sinTheta };
}