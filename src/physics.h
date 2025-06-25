#ifndef PHYSICS_H
#define PHYSICS_H
#include "raylib.h"
void InitPhysics();
void UpdatePhysics();
void ShutdownPhysics();
Vector3 GetPhysicsBodyPosition(int index);
void ApplyRandomJumpToAllBodies();
void SetTerrainTriMesh(Mesh *mesh);
#define CUBE_SIZE 20.0f
#define MAX_BODIES 2000
#endif