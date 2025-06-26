#ifndef PHYSICS_H
#define PHYSICS_H
#include "raylib.h"
#include <ode/ode.h>
void InitPhysics();
void UpdatePhysics();
void ShutdownPhysics();
Vector3 GetPhysicsBodyPosition(int index);
void GetPhysicsBodyAxisAngle(int index, Vector3 *axis, float *angle);
Model GetPhysicsBodyModel(int index);
void ApplyRandomJumpToAllBodies();
void SetTerrainTriMesh(Mesh *mesh);
Quaternion QuaternionFromODE(const dReal *R);
void AttachShaderToPhysicsBodies(Shader shader);
#define CUBE_SIZE 40.0f
#define MAX_BODIES 1000
#endif