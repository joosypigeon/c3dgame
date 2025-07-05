#ifndef CAMERA_H
#define CAMERA_H

#include "raylib.h"
#include "raymath.h"
#include "ode/ode.h"

void UpdateCameraManual(Camera3D *camera);
void UpdateOffestCameraManual(Camera3D *camera);
void UpdateCarPosition(dVector3 pos);
#endif // CAMERA_H