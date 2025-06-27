#ifndef TORUS_H
#define TORUS_H

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

extern size_t SCREEN_WIDTH;
extern size_t SCREEN_HEIGHT;
extern float HALF_SCREEN_WIDTH;
extern float HALF_SCREEN_HEIGHT;

extern size_t MONITOR_WIDTH;
extern size_t MONITOR_HEIGHT;
extern float HALF_MONITOR_WIDTH;
extern float HALF_MONITOR_HEIGHT;

void SetTorusDimensions(float major, float minor);
Mesh MyGenTorusMesh(size_t rings, size_t sides);
Mesh MyGenFlatTorusMesh(size_t rings, size_t sides);

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