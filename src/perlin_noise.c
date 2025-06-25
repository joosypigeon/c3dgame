#include "perlin_noise.h"
#include <stdlib.h>
#include <math.h>

static unsigned char perm[512];

static float fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static float lerp(float t, float a, float b)
{
    return a + t * (b - a);
}

static float grad(int hash, float x, float y)
{
    switch (hash & 7)
    {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        default: return -y;
    }
}

void perlin_init(int seed)
{
    srand(seed);
    unsigned char p[256];
    for (int i = 0; i < 256; i++) p[i] = (unsigned char)i;
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        unsigned char tmp = p[i];
        p[i] = p[j];
        p[j] = tmp;
    }
    for (int i = 0; i < 512; i++) perm[i] = p[i & 255];
}

float perlin_noise2d(float x, float y)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    float xf = x - floorf(x);
    float yf = y - floorf(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = perm[xi    ] + yi;
    int ab = perm[xi    ] + yi + 1;
    int ba = perm[xi + 1] + yi;
    int bb = perm[xi + 1] + yi + 1;

    float x1 = lerp(u, grad(perm[aa], xf,     yf), grad(perm[ba], xf - 1, yf));
    float x2 = lerp(u, grad(perm[ab], xf, yf - 1), grad(perm[bb], xf - 1, yf - 1));

    return lerp(v, x1, x2);
}

float grad3D(int hash, float x, float y, float z) {
    int h = hash & 15;      // 16 possible values (0–15)
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin_noise3d(float x, float y, float z)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    // Nested permutation indexing
    int aaa = perm[perm[perm[xi    ] + yi    ] + zi    ];
    int aba = perm[perm[perm[xi    ] + yi + 1] + zi    ];
    int baa = perm[perm[perm[xi + 1] + yi    ] + zi    ];
    int bba = perm[perm[perm[xi + 1] + yi + 1] + zi    ];
    int aab = perm[perm[perm[xi    ] + yi    ] + zi + 1];
    int abb = perm[perm[perm[xi    ] + yi + 1] + zi + 1];
    int bab = perm[perm[perm[xi + 1] + yi    ] + zi + 1];
    int bbb = perm[perm[perm[xi + 1] + yi + 1] + zi + 1];

    float x1 = lerp(u, grad3D(aaa, xf,     yf,     zf),
                       grad3D(baa, xf - 1, yf,     zf));
    float x2 = lerp(u, grad3D(aba, xf,     yf - 1, zf),
                       grad3D(bba, xf - 1, yf - 1, zf));
    float y1 = lerp(v, x1, x2);

    float x3 = lerp(u, grad3D(aab, xf,     yf,     zf - 1),
                       grad3D(bab, xf - 1, yf,     zf - 1));
    float x4 = lerp(u, grad3D(abb, xf,     yf - 1, zf - 1),
                       grad3D(bbb, xf - 1, yf - 1, zf - 1));
    float y2 = lerp(v, x3, x4);

    return lerp(w, y1, y2);  // returns in [-1, 1]
}

float grad4D(int hash, float x, float y, float z, float w) {
    // There are 32 possible directions in 4D (we'll use hash & 31)
    int h = hash & 31;
    float a = h < 24 ? x : y;
    float b = h < 16 ? y : z;
    float c = h < 8  ? z : w;

    float u = (h & 1) ? -a : a;
    float v = (h & 2) ? -b : b;
    float t = (h & 4) ? -c : c;

    return u + v + t;
}

float perlin_noise4d(float x, float y, float z, float w)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;
    int wi = (int)floorf(w) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);
    float wf = w - floorf(w);

    float u = fade(xf);
    float v = fade(yf);
    float t = fade(zf);
    float s = fade(wf);

    // 16 gradients from the corners of a 4D hypercube
    float g0000 = grad4D(perm[perm[perm[perm[xi    ] + yi    ] + zi    ] + wi    ], xf,     yf,     zf,     wf);
    float g1000 = grad4D(perm[perm[perm[perm[xi + 1] + yi    ] + zi    ] + wi    ], xf - 1, yf,     zf,     wf);
    float g0100 = grad4D(perm[perm[perm[perm[xi    ] + yi + 1] + zi    ] + wi    ], xf,     yf - 1, zf,     wf);
    float g1100 = grad4D(perm[perm[perm[perm[xi + 1] + yi + 1] + zi    ] + wi    ], xf - 1, yf - 1, zf,     wf);

    float g0010 = grad4D(perm[perm[perm[perm[xi    ] + yi    ] + zi + 1] + wi    ], xf,     yf,     zf - 1, wf);
    float g1010 = grad4D(perm[perm[perm[perm[xi + 1] + yi    ] + zi + 1] + wi    ], xf - 1, yf,     zf - 1, wf);
    float g0110 = grad4D(perm[perm[perm[perm[xi    ] + yi + 1] + zi + 1] + wi    ], xf,     yf - 1, zf - 1, wf);
    float g1110 = grad4D(perm[perm[perm[perm[xi + 1] + yi + 1] + zi + 1] + wi    ], xf - 1, yf - 1, zf - 1, wf);

    float g0001 = grad4D(perm[perm[perm[perm[xi    ] + yi    ] + zi    ] + wi + 1], xf,     yf,     zf,     wf - 1);
    float g1001 = grad4D(perm[perm[perm[perm[xi + 1] + yi    ] + zi    ] + wi + 1], xf - 1, yf,     zf,     wf - 1);
    float g0101 = grad4D(perm[perm[perm[perm[xi    ] + yi + 1] + zi    ] + wi + 1], xf,     yf - 1, zf,     wf - 1);
    float g1101 = grad4D(perm[perm[perm[perm[xi + 1] + yi + 1] + zi    ] + wi + 1], xf - 1, yf - 1, zf,     wf - 1);

    float g0011 = grad4D(perm[perm[perm[perm[xi    ] + yi    ] + zi + 1] + wi + 1], xf,     yf,     zf - 1, wf - 1);
    float g1011 = grad4D(perm[perm[perm[perm[xi + 1] + yi    ] + zi + 1] + wi + 1], xf - 1, yf,     zf - 1, wf - 1);
    float g0111 = grad4D(perm[perm[perm[perm[xi    ] + yi + 1] + zi + 1] + wi + 1], xf,     yf - 1, zf - 1, wf - 1);
    float g1111 = grad4D(perm[perm[perm[perm[xi + 1] + yi + 1] + zi + 1] + wi + 1], xf - 1, yf - 1, zf - 1, wf - 1);

    // Interpolate along x
    float x00 = lerp(u, g0000, g1000);
    float x10 = lerp(u, g0100, g1100);
    float x01 = lerp(u, g0010, g1010);
    float x11 = lerp(u, g0110, g1110);
    float x02 = lerp(u, g0001, g1001);
    float x12 = lerp(u, g0101, g1101);
    float x03 = lerp(u, g0011, g1011);
    float x13 = lerp(u, g0111, g1111);

    // Interpolate along y
    float y0 = lerp(v, x00, x10);
    float y1 = lerp(v, x01, x11);
    float y2 = lerp(v, x02, x12);
    float y3 = lerp(v, x03, x13);

    // Interpolate along z
    float z0 = lerp(t, y0, y1);
    float z1 = lerp(t, y2, y3);

    // Interpolate along w and return final result
    return lerp(s, z0, z1);  // Result in [-1, 1]
}