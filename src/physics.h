#ifndef PHYSICS_H
#define PHYSICS_H
#include "raylib.h"
#include "raymath.h"
#include <ode/ode.h>

#define CUBE_SIZE 100.0f
#define MAX_BODIES 100

void InitPhysics();
void UpdatePhysics();
void ShutdownPhysics();
Vector3 GetPhysicsBodyPosition(int index);
void GetPhysicsBodyAxisAngle(int index, Vector3 *axis, float *angle);
Model GetPhysicsBodyModel(int index);
void ApplyRandomJumpToAllBodies();
void SetTerrainTriMesh(Mesh *mesh);
void AttachShaderToPhysicsBodies(Shader shader);
bool checkColliding(dGeomID g);
dWorldID GetPhysicsWorld();
dSpaceID GetPhysicsSpace();
dJointGroupID GetPhysicsContactGroup();
void CollideBodies();

typedef struct geomInfo {
    bool collidable;
} geomInfo ;
#endif