#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

void perlin_init(int seed);
float perlin_noise2d(float x, float y);
float perlin_noise3d(float x, float y, float z);
float perlin_noise4d(float x, float y, float z, float w);
#endif // PERLIN_NOISE_H
