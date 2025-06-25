/*
 * noise3d4d.c
 *
 * This file implements 3D and 4D **value noise**, a type of coherent noise
 * used in procedural texture generation, terrain, and volumetric effects.
 *
 * Key characteristics of this implementation:
 * - Each lattice point (integer grid coordinate) is assigned a pseudorandom **scalar value**
 *   using a hash function (`hash3`, `hash4`).
 * - Smooth interpolation (via a Hermite polynomial) is used between lattice values.
 * - No gradient vectors or dot products are used — this distinguishes it from gradient noise
 *   such as **Perlin noise** or **Simplex noise**.
 *
 * The output is smooth and continuous, but lacks directional features typically seen in
 * gradient-based noise, making it suitable for isotropic patterns.
 *
 * Functions provided:
 * - float noise3d(float x, float y, float z): returns scalar value noise in 3D space
 * - float noise4d(float x, float y, float z, float w): returns scalar value noise in 4D space
 */

 #include "noise3d4d.h"
#include <math.h>

// --- Helper Functions ---
static inline float fract(float x) { return x - floorf(x); }
static inline float lerp(float a, float b, float t) { return a * (1.0f - t) + b * t; }
static inline float smooth_interp(float t) { return t * t * (3.0f - 2.0f * t); }

// --- Hash Functions ---
float hash3(float x, float y, float z) {
    return fract(sinf(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453f);
}

float hash4(float x, float y, float z, float w) {
    return fract(sinf(x * 127.1f + y * 311.7f + z * 74.7f + w * 269.5f) * 43758.5453f);
}

// --- 3D Gradient Noise ---
float noise3d(float x, float y, float z) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    int iz = (int)floorf(z);
    float fx = fract(x);
    float fy = fract(y);
    float fz = fract(z);

    float u = smooth_interp(fx);
    float v = smooth_interp(fy);
    float w = smooth_interp(fz);

    float n000 = hash3(ix + 0, iy + 0, iz + 0);
    float n100 = hash3(ix + 1, iy + 0, iz + 0);
    float n010 = hash3(ix + 0, iy + 1, iz + 0);
    float n110 = hash3(ix + 1, iy + 1, iz + 0);
    float n001 = hash3(ix + 0, iy + 0, iz + 1);
    float n101 = hash3(ix + 1, iy + 0, iz + 1);
    float n011 = hash3(ix + 0, iy + 1, iz + 1);
    float n111 = hash3(ix + 1, iy + 1, iz + 1);

    float nx00 = lerp(n000, n100, u);
    float nx10 = lerp(n010, n110, u);
    float nx01 = lerp(n001, n101, u);
    float nx11 = lerp(n011, n111, u);

    float nxy0 = lerp(nx00, nx10, v);
    float nxy1 = lerp(nx01, nx11, v);

    return lerp(nxy0, nxy1, w);
}

// --- 4D Gradient Noise ---
float noise4d(float x, float y, float z, float w_) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    int iz = (int)floorf(z);
    int iw = (int)floorf(w_);
    float fx = fract(x);
    float fy = fract(y);
    float fz = fract(z);
    float fw = fract(w_);

    float u = smooth_interp(fx);
    float v = smooth_interp(fy);
    float s = smooth_interp(fz);
    float t = smooth_interp(fw);

    float n0000 = hash4(ix+0, iy+0, iz+0, iw+0);
    float n1000 = hash4(ix+1, iy+0, iz+0, iw+0);
    float n0100 = hash4(ix+0, iy+1, iz+0, iw+0);
    float n1100 = hash4(ix+1, iy+1, iz+0, iw+0);
    float n0010 = hash4(ix+0, iy+0, iz+1, iw+0);
    float n1010 = hash4(ix+1, iy+0, iz+1, iw+0);
    float n0110 = hash4(ix+0, iy+1, iz+1, iw+0);
    float n1110 = hash4(ix+1, iy+1, iz+1, iw+0);

    float n0001 = hash4(ix+0, iy+0, iz+0, iw+1);
    float n1001 = hash4(ix+1, iy+0, iz+0, iw+1);
    float n0101 = hash4(ix+0, iy+1, iz+0, iw+1);
    float n1101 = hash4(ix+1, iy+1, iz+0, iw+1);
    float n0011 = hash4(ix+0, iy+0, iz+1, iw+1);
    float n1011 = hash4(ix+1, iy+0, iz+1, iw+1);
    float n0111 = hash4(ix+0, iy+1, iz+1, iw+1);
    float n1111 = hash4(ix+1, iy+1, iz+1, iw+1);

    float nx000 = lerp(n0000, n1000, u);
    float nx100 = lerp(n0100, n1100, u);
    float nx010 = lerp(n0010, n1010, u);
    float nx110 = lerp(n0110, n1110, u);
    float nx001 = lerp(n0001, n1001, u);
    float nx101 = lerp(n0101, n1101, u);
    float nx011 = lerp(n0011, n1011, u);
    float nx111 = lerp(n0111, n1111, u);

    float nxy00 = lerp(nx000, nx100, v);
    float nxy10 = lerp(nx010, nx110, v);
    float nxy01 = lerp(nx001, nx101, v);
    float nxy11 = lerp(nx011, nx111, v);

    float nxyz0 = lerp(nxy00, nxy10, s);
    float nxyz1 = lerp(nxy01, nxy11, s);

    return lerp(nxyz0, nxyz1, t);
}