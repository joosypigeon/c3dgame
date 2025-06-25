#ifndef TORUS_H
#define TORUS_H

#include "raylib.h"
#include "raymath.h"
#include <math.h>

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern float HALF_SCREEN_WIDTH;
extern float HALF_SCREEN_HEIGHT;

void SetTorusDimensions(float major, float minor);
Mesh MyGenTorusMesh(int rings, int sides);
Mesh MyGenFlatTorusMesh(int rings, int sides);

Vector3 get_torus_position(float u, float v);
Vector3 get_torus_normal(float u, float v);
Vector3 get_phi_tangent(float u, float v);
Vector3 get_theta_tangent(float u, float v);

void set_torus_coords(float u, float v);
Vector3 get_torus_position_fast();
Vector3 get_torus_normal_fast();
Vector3 get_theta_tangent_fast();
Vector3 get_phi_tangent_fast();

#endif // TORUS_H