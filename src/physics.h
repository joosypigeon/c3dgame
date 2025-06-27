#ifndef PHYSICS_H
#define PHYSICS_H
#include "raylib.h"
#include "raymath.h"
void InitPhysics();
void UpdatePhysics();
void ShutdownPhysics();
Vector3 GetPhysicsBodyPosition(int index);
void GetPhysicsBodyAxisAngle(int index, Vector3 *axis, float *angle);
Model GetPhysicsBodyModel(int index);
void ApplyRandomJumpToAllBodies();
void SetTerrainTriMesh(Mesh *mesh);
void AttachShaderToPhysicsBodies(Shader shader);
#define CUBE_SIZE 40.0f
#define MAX_BODIES 100
#endif