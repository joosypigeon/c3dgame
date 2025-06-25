#ifndef PHYSICS_H
#define PHYSICS_H
#include "raylib.h"
void InitPhysics();
void UpdatePhysics();
void ShutdownPhysics();
Vector3 GetPhysicsBodyPosition(int index);
void ApplyRandomJumpToAllBodies();
#define MAX_BODIES 50
#endif