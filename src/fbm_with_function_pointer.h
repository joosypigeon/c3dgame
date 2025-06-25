#ifndef FBM_WITH_FUNCTION_POINTER_H
#define FBM_WITH_FUNCTION_POINTER_H

#include "noise3d4d.h"
#include "perlin_noise.h"
#include "simplex_noise.h"

typedef float (*NoiseFunction3D)(float, float, float);
typedef float (*NoiseFunction4D)(float, float, float, float);

float fbm3d_fn(float x, float y, float z, int octaves, float lacunarity, float gain, NoiseFunction3D noiseFunc);
float fbm4d_fn(float x, float y, float z, float w, int octaves, float lacunarity, float gain, NoiseFunction4D noiseFunc);
float fbm4d(float x, float y, float z, float w, int octaves, float lacunarity, float gain);
float fbm4dx(float x, float y, float z, float w, int octaves, float lacunarity, float gain);

typedef enum {
    NOISE_VALUE,
    NOISE_PERLIN,
    NOISE_SIMPLEX
} NoiseType;
#endif // FBM_WITH_FUNCTION_POINTER_H