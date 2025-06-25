// simplex_noise.c
// 3D and 4D Simplex Noise (public domain implementation based on Stefan Gustavson)

#include <math.h>
#include <stdint.h>

// Skewing and unskewing factors for 3D
#define F3 0.3333333f
#define G3 0.1666667f

// Skewing and unskewing factors for 4D
#define F4 0.309017f   // (sqrt(5)-1)/4
#define G4 0.1381966f  // (5-sqrt(5))/20

// Gradient directions for 3D
static const int grad3[12][3] = {
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
};

// Gradient directions for 4D
static const int grad4[32][4] = {
    {0,1,1,1}, {0,1,1,-1}, {0,1,-1,1}, {0,1,-1,-1},
    {0,-1,1,1}, {0,-1,1,-1}, {0,-1,-1,1}, {0,-1,-1,-1},
    {1,0,1,1}, {1,0,1,-1}, {1,0,-1,1}, {1,0,-1,-1},
    {-1,0,1,1}, {-1,0,1,-1}, {-1,0,-1,1}, {-1,0,-1,-1},
    {1,1,0,1}, {1,1,0,-1}, {1,-1,0,1}, {1,-1,0,-1},
    {-1,1,0,1}, {-1,1,0,-1}, {-1,-1,0,1}, {-1,-1,0,-1},
    {1,1,1,0}, {1,1,-1,0}, {1,-1,1,0}, {1,-1,-1,0},
    {-1,1,1,0}, {-1,1,-1,0}, {-1,-1,1,0}, {-1,-1,-1,0}
};

// Permutation table (same as in Perlin's original implementation)
static const uint8_t perm[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

    // Repeat
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

// Dot product helpers
static inline float dot3(const int* g, float x, float y, float z) {
    return g[0]*x + g[1]*y + g[2]*z;
}

static inline float dot4(const int* g, float x, float y, float z, float w) {
    return g[0]*x + g[1]*y + g[2]*z + g[3]*w;
}

// Simplex noise in 3D
float simplex3d(float x, float y, float z) {
    float s = (x + y + z) * F3;
    int i = (int)floorf(x + s);
    int j = (int)floorf(y + s);
    int k = (int)floorf(z + s);

    float t = (i + j + k) * G3;
    float X0 = i - t, Y0 = j - t, Z0 = k - t;
    float x0 = x - X0, y0 = y - Y0, z0 = z - Z0;

    int i1, j1, k1;
    int i2, j2, k2;
    if (x0 >= y0) {
        if (y0 >= z0)      { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
        else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
        else               { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
    } else {
        if (y0 < z0)       { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
        else if (x0 < z0)  { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
        else               { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
    }

    float x1 = x0 - i1 + G3, y1 = y0 - j1 + G3, z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2*G3, y2 = y0 - j2 + 2*G3, z2 = z0 - k2 + 2*G3;
    float x3 = x0 - 1 + 3*G3, y3 = y0 - 1 + 3*G3, z3 = z0 - 1 + 3*G3;

    int gi0 = perm[(i + perm[(j + perm[k & 255]) & 255]) & 255] % 12;
    int gi1 = perm[(i+i1 + perm[(j+j1 + perm[(k+k1) & 255]) & 255]) & 255] % 12;
    int gi2 = perm[(i+i2 + perm[(j+j2 + perm[(k+k2) & 255]) & 255]) & 255] % 12;
    int gi3 = perm[(i+1 + perm[(j+1 + perm[(k+1) & 255]) & 255]) & 255] % 12;

    float n0, n1, n2, n3;
    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 < 0) n0 = 0.0f;
    else {
        t0 *= t0;
        n0 = t0 * t0 * dot3(grad3[gi0], x0, y0, z0);
    }

    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 < 0) n1 = 0.0f;
    else {
        t1 *= t1;
        n1 = t1 * t1 * dot3(grad3[gi1], x1, y1, z1);
    }

    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 < 0) n2 = 0.0f;
    else {
        t2 *= t2;
        n2 = t2 * t2 * dot3(grad3[gi2], x2, y2, z2);
    }

    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 < 0) n3 = 0.0f;
    else {
        t3 *= t3;
        n3 = t3 * t3 * dot3(grad3[gi3], x3, y3, z3);
    }

    return 32.0f * (n0 + n1 + n2 + n3);
}

// Simplex noise in 4D
float simplex4d(float x, float y, float z, float w) {
    float s = (x + y + z + w) * F4;
    int i = (int)floorf(x + s);
    int j = (int)floorf(y + s);
    int k = (int)floorf(z + s);
    int l = (int)floorf(w + s);

    float t = (i + j + k + l) * G4;
    float X0 = i - t, Y0 = j - t, Z0 = k - t, W0 = l - t;
    float x0 = x - X0, y0 = y - Y0, z0 = z - Z0, w0 = w - W0;

    int rankx = (x0 > y0) + (x0 > z0) + (x0 > w0);
    int ranky = (y0 > x0) + (y0 > z0) + (y0 > w0);
    int rankz = (z0 > x0) + (z0 > y0) + (z0 > w0);
    int rankw = (w0 > x0) + (w0 > y0) + (w0 > z0);

    int i1 = rankx >= 3 ? 1 : 0;
    int j1 = ranky >= 3 ? 1 : 0;
    int k1 = rankz >= 3 ? 1 : 0;
    int l1 = rankw >= 3 ? 1 : 0;

    int i2 = rankx >= 2 ? 1 : 0;
    int j2 = ranky >= 2 ? 1 : 0;
    int k2 = rankz >= 2 ? 1 : 0;
    int l2 = rankw >= 2 ? 1 : 0;

    int i3 = rankx >= 1 ? 1 : 0;
    int j3 = ranky >= 1 ? 1 : 0;
    int k3 = rankz >= 1 ? 1 : 0;
    int l3 = rankw >= 1 ? 1 : 0;

    float x1 = x0 - i1 + G4;
    float y1 = y0 - j1 + G4;
    float z1 = z0 - k1 + G4;
    float w1 = w0 - l1 + G4;

    float x2 = x0 - i2 + 2.0f * G4;
    float y2 = y0 - j2 + 2.0f * G4;
    float z2 = z0 - k2 + 2.0f * G4;
    float w2 = w0 - l2 + 2.0f * G4;

    float x3 = x0 - i3 + 3.0f * G4;
    float y3 = y0 - j3 + 3.0f * G4;
    float z3 = z0 - k3 + 3.0f * G4;
    float w3 = w0 - l3 + 3.0f * G4;

    float x4 = x0 - 1.0f + 4.0f * G4;
    float y4 = y0 - 1.0f + 4.0f * G4;
    float z4 = z0 - 1.0f + 4.0f * G4;
    float w4 = w0 - 1.0f + 4.0f * G4;

    int gi0 = perm[(i + perm[(j + perm[(k + perm[l & 255]) & 255]) & 255]) & 255] & 31;
    int gi1 = perm[(i+i1 + perm[(j+j1 + perm[(k+k1 + perm[(l+l1) & 255]) & 255]) & 255]) & 255] & 31;
    int gi2 = perm[(i+i2 + perm[(j+j2 + perm[(k+k2 + perm[(l+l2) & 255]) & 255]) & 255]) & 255] & 31;
    int gi3 = perm[(i+i3 + perm[(j+j3 + perm[(k+k3 + perm[(l+l3) & 255]) & 255]) & 255]) & 255] & 31;
    int gi4 = perm[(i+1 + perm[(j+1 + perm[(k+1 + perm[(l+1) & 255]) & 255]) & 255]) & 255] & 31;

    float n0, n1, n2, n3, n4;

    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0 - w0*w0;
    if (t0 < 0) n0 = 0.0f;
    else {
        t0 *= t0;
        n0 = t0 * t0 * dot4(grad4[gi0], x0, y0, z0, w0);
    }

    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1 - w1*w1;
    if (t1 < 0) n1 = 0.0f;
    else {
        t1 *= t1;
        n1 = t1 * t1 * dot4(grad4[gi1], x1, y1, z1, w1);
    }

    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2 - w2*w2;
    if (t2 < 0) n2 = 0.0f;
    else {
        t2 *= t2;
        n2 = t2 * t2 * dot4(grad4[gi2], x2, y2, z2, w2);
    }

    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3 - w3*w3;
    if (t3 < 0) n3 = 0.0f;
    else {
        t3 *= t3;
        n3 = t3 * t3 * dot4(grad4[gi3], x3, y3, z3, w3);
    }

    float t4 = 0.6f - x4*x4 - y4*y4 - z4*z4 - w4*w4;
    if (t4 < 0) n4 = 0.0f;
    else {
        t4 *= t4;
        n4 = t4 * t4 * dot4(grad4[gi4], x4, y4, z4, w4);
    }

    return 27.0f * (n0 + n1 + n2 + n3 + n4);
}
