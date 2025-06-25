#include "fbm_with_function_pointer.h"

float fbm3d_fn(float x, float y, float z, int octaves, float lacunarity, float gain, NoiseFunction3D noise) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }

    return (total / maxValue + 1.0f) / 2.0f;  // Normalise to [0, 1]
}

float fbm4d_fn(float x, float y, float z, float w, int octaves, float lacunarity, float gain, NoiseFunction4D noise) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency, z * frequency, w * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }

    return (total / maxValue + 1.0f) / 2.0f;  // Normalise to [0, 1]
}