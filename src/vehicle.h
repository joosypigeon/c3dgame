#ifndef VEHICLE_H
#define VEHICLE_H

#include "raylib.h"
#include "raymath.h"
#include <ode/ode.h>



// Vehicle models
extern Model ball;
extern Model box;
extern Model cylinder;

// 0 chassis / 1-4 wheel / 5 anti roll counter weight
typedef struct vehicle {
    dBodyID bodies[6];
    dGeomID geoms[10];
    dJointID joints[5];
    float restLength[4]; // for anti roll bar
} vehicle;

vehicle* CreateVehicle(dSpaceID space, dWorldID world);
vehicle* CreateVehicle2(dSpaceID space, dWorldID world);
void updateVehicle(vehicle *car, float accel, float maxAccelForce, 
                    float steer, float steerFactor);
void unflipVehicle (vehicle *car);
void DrawJointAxes(dJointID joint, float scale);
void DrawJoint(dJointID joint);
void DrawSpring(dJointID joint, float restLength);   
#endif // VEHICLE_H