#include "physics.h"
#include "vehicle.h"
#include <time.h>

struct timespec start_time, now;
bool ddebug = false; // set to true to enable debug output
bool haveDebugged = false; // set to true to only debug once

#define INITIAL_HEIGHT 50.0f
// reused for all geoms that don't collide
// ie vehicle counter weights
static geomInfo disabled;

vehicle* CreateVehicle(dSpaceID space, dWorldID world)
{
    // TODO these should be parameters
    Vector3 carScale = (Vector3){2.5, 0.5, 2.0};
    float wheelRadius = 0.5, wheelWidth = 0.45;
    
    vehicle* car = RL_MALLOC(sizeof(vehicle));
    
    // car body
    dMass m;
    dMassSetBox(&m, 1, carScale.x, carScale.y, carScale.z);  // density
    dMassAdjust(&m, 150); // mass
    
    car->bodies[0] = dBodyCreate(world);
    dBodySetMass(car->bodies[0], &m);
    dBodySetAutoDisableFlag( car->bodies[0], 0 );


    car->geoms[0] = dCreateBox(space, carScale.x, carScale.y, carScale.z);
    dGeomSetBody(car->geoms[0], car->bodies[0]);
    
    // TODO used a little later and should be a parameter
    dBodySetPosition(car->bodies[0], 15, 6+INITIAL_HEIGHT, 0.0);

    car->geoms[6] = dCreateBox(space, 0.5, 0.5, 0.5);
    dGeomSetBody(car->geoms[6], car->bodies[0]);
    dGeomSetOffsetPosition(car->geoms[6], carScale.x/2-0.25, carScale.y/2+0.25 , 0);

    car->bodies[5] = dBodyCreate(world);
    dBodySetMass(car->bodies[5], &m);
    dBodySetAutoDisableFlag( car->bodies[5], 0 );
    // see previous TODO
    dBodySetPosition(car->bodies[5], 15, 6-2+INITIAL_HEIGHT, 0.0);
    car->geoms[5] = dCreateSphere(space,1);
    dGeomSetBody(car->geoms[5],car->bodies[5]);
    disabled.collidable = false;
    dGeomSetData(car->geoms[5], &disabled);
    
    car->joints[5] = dJointCreateFixed (world, 0);
    dJointAttach(car->joints[5], car->bodies[0], car->bodies[5]);
    dJointSetFixed (car->joints[5]);
    
    // wheels
    dMassSetCylinder(&m, 1, 3, wheelRadius, wheelWidth);
    dMassAdjust(&m, 2); // mass
    dQuaternion q;
    dQFromAxisAndAngle(q, 0, 0, 1, M_PI * 0.5);
    for(int i = 1; i <= 4; ++i)
    {
        car->bodies[i] = dBodyCreate(world);
        dBodySetMass(car->bodies[i], &m);
        dBodySetQuaternion(car->bodies[i], q);
        car->geoms[i] = dCreateCylinder(space, wheelRadius, wheelWidth);
        dGeomSetBody(car->geoms[i], car->bodies[i]);
        dBodySetFiniteRotationMode( car->bodies[i], 1 );
        dBodySetAutoDisableFlag( car->bodies[i], 0 );
    }

    const dReal* cp = dBodyGetPosition(car->bodies[0]);
    // TODO wheel base and axel width should be parameters
    dBodySetPosition(car->bodies[1], cp[0]+1.2, cp[1]-.5, cp[2]-1); 
    dBodySetPosition(car->bodies[2], cp[0]+1.2, cp[1]-.5, cp[2]+1); 
    dBodySetPosition(car->bodies[3], cp[0]-1.2, cp[1]-.5, cp[2]-1); 
    dBodySetPosition(car->bodies[4], cp[0]-1.2, cp[1]-.5, cp[2]+1); 

    // hinge2 (combined steering / suspension / motor !)
    for(int i = 0; i < 4; ++i)
    {
        car->joints[i] = dJointCreateHinge2(world, 0);
        dJointAttach(car->joints[i], car->bodies[0], car->bodies[i+1]);
        const dReal* wPos = dBodyGetPosition(car->bodies[i+1]);
        dJointSetHinge2Anchor(car->joints[i], wPos[0], wPos[1], wPos[2]);
        
        dReal axis1[] = { 0, -1, 0 };
        dReal axis2[] = { 0, 0, ((i % 2) == 0) ? -1 : 1};
        
        // replacement for deprecated calls
        dJointSetHinge2Axes (car->joints[i], axis1, axis2);
        //dJointSetHinge2Axis1(joints[i], 0, 1, 0);
        //dJointSetHinge2Axis2(joints[i], 0, 0, ((i % 2) == 0) ? -1 : 1);

        dJointSetHinge2Param(car->joints[i], dParamLoStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamHiStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamLoStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamHiStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamFMax, 1500);

        dJointSetHinge2Param(car->joints[i], dParamVel2, dInfinity);
        dJointSetHinge2Param(car->joints[i], dParamFMax2, 1500);

        // Suspension: allow some compliance along axis 1
        dJointSetHinge2Param(car->joints[i], dParamSuspensionERP, 0.1);
        dJointSetHinge2Param(car->joints[i], dParamSuspensionCFM, 1e-3);

        // steering
        if (i<2) {
            dJointSetHinge2Param (car->joints[i],dParamFMax,500);
            dJointSetHinge2Param (car->joints[i],dParamLoStop,-0.5);
            dJointSetHinge2Param (car->joints[i],dParamHiStop,0.5);
            dJointSetHinge2Param (car->joints[i],dParamLoStop,-0.5);
            dJointSetHinge2Param (car->joints[i],dParamHiStop,0.5);
            dJointSetHinge2Param (car->joints[i],dParamFudgeFactor,0.1);
        }
        
    }
    // disable motor on front wheels
    dJointSetHinge2Param(car->joints[0], dParamFMax2, 0);
    dJointSetHinge2Param(car->joints[1], dParamFMax2, 0);

    return car;
}

vehicle* CreateVehicle2(dSpaceID space, dWorldID world)
{
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    printf("Creating vehicle at %ld.%09ld\n", start_time.tv_sec, start_time.tv_nsec);
    // TODO these should be parameters
    Vector3 carScale = (Vector3){2.5, 0.5, 2.0};
    float wheelRadius = 0.5, wheelWidth = 0.5, axisOffset = 0.75f, axelRadius = 0.05f, axelLength = 1.0f;
    
    vehicle* car = RL_MALLOC(sizeof(vehicle));
    
    // car body
    dMass m;
    dMassSetBox(&m, 1, carScale.x, carScale.y, carScale.z);  // density
    dMassAdjust(&m, 150); // mass
    
    car->bodies[0] = dBodyCreate(world);
    dBodySetMass(car->bodies[0], &m);
    dBodySetAutoDisableFlag( car->bodies[0], 0 );


    car->geoms[0] = dCreateBox(space, carScale.x, carScale.y, carScale.z);
    dGeomSetBody(car->geoms[0], car->bodies[0]);
    
    // TODO used a little later and should be a parameter
    dBodySetPosition(car->bodies[0], 15, 6+INITIAL_HEIGHT, 0.0);

    car->bodies[1] = dBodyCreate(world);
    dBodySetMass(car->bodies[1], &m);
    dBodySetAutoDisableFlag( car->bodies[1], 0 );
    // see previous TODO
    dBodySetPosition(car->bodies[1], 15, 6-2+INITIAL_HEIGHT, 0.0);
    car->geoms[1] = dCreateSphere(space,1);
    dGeomSetBody(car->geoms[1],car->bodies[1]);
    disabled.collidable = false;
    dGeomSetData(car->geoms[1], &disabled);
    
    car->joints[5] = dJointCreateFixed (world, 0);
    dJointAttach(car->joints[5], car->bodies[0], car->bodies[1]);
    dJointSetFixed (car->joints[5]);
    
    // wheels
    dMassSetCylinder(&m, 1, 3, wheelRadius, wheelWidth);
    dMassAdjust(&m, 2); // mass
    dQuaternion q;
    dQFromAxisAndAngle(q, 0, 0, 1, M_PI * 0.5);

    for(int i = 0; i < 4; ++i)
    {
        car->bodies[i+2] = dBodyCreate(world);
        dBodySetMass(car->bodies[i+2], &m);
        dBodySetQuaternion(car->bodies[i+2], q);
        car->geoms[i+2] = dCreateCylinder(space, wheelRadius, wheelWidth);
        dGeomSetBody(car->geoms[i+2], car->bodies[i+2]);
        car->geoms[i+2+4]= dCreateCylinder(space, axelRadius, axelLength);
        dGeomSetBody(car->geoms[i+2+4], car->bodies[i+2]);
        
        dGeomSetOffsetPosition(car->geoms[i+2+4], 0, 0, (axelLength/2.0f+wheelWidth/2.0f) *(((i % 2) == 0) ? 1.0f : -1.0f));
        dBodySetFiniteRotationMode( car->bodies[i+2], 1 );
        dBodySetAutoDisableFlag( car->bodies[i+2], 0 );
        printf("wheel %d position %f %f %f\n", i+2, dBodyGetPosition(car->bodies[i+2])[0], 
            dBodyGetPosition(car->bodies[i+2])[1], dBodyGetPosition(car->bodies[i+2])[2]);
    }



    const dReal* cp = dBodyGetPosition(car->bodies[0]);
    // TODO wheel base and axel width should be parameters
    dBodySetPosition(car->bodies[2], cp[0]+1.2, cp[1]-.5, cp[2]-1 - axisOffset); 
    dBodySetPosition(car->bodies[3], cp[0]+1.2, cp[1]-.5, cp[2]+1 + axisOffset); 
    dBodySetPosition(car->bodies[4], cp[0]-1.2, cp[1]-.5, cp[2]-1 - axisOffset); 
    dBodySetPosition(car->bodies[5], cp[0]-1.2, cp[1]-.5, cp[2]+1 + axisOffset); 

    // hinge2 (combined steering / suspension / motor !)
    for(int i = 0; i < 4; ++i)
    {
        car->joints[i] = dJointCreateHinge2(world, 0);
        dJointAttach(car->joints[i], car->bodies[0], car->bodies[i+2]);
        const dReal* wPos = dBodyGetPosition(car->bodies[i+2]);
        dJointSetHinge2Anchor(car->joints[i], wPos[0], wPos[1], wPos[2] + wheelWidth *(((i % 2) == 0) ? 1.0f : -1.0f));

        dReal axis1[] = { 0, -1, 0 };
        dReal axis2[] = { 0, 0, ((i % 2) == 0) ? -1 : 1};
        
        // replacement for deprecated calls
        dJointSetHinge2Axes (car->joints[i], axis1, axis2);
        //dJointSetHinge2Axis1(joints[i], 0, 1, 0);
        //dJointSetHinge2Axis2(joints[i], 0, 0, ((i % 2) == 0) ? -1 : 1);

        dJointSetHinge2Param(car->joints[i], dParamLoStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamHiStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamLoStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamHiStop, 0);
        dJointSetHinge2Param(car->joints[i], dParamFMax, 1500);

        dJointSetHinge2Param(car->joints[i], dParamVel2, dInfinity);
        dJointSetHinge2Param(car->joints[i], dParamFMax2, 1500);

        // Suspension: allow some compliance along axis 1
        dJointSetHinge2Param(car->joints[i], dParamSuspensionERP, 1.0);
        dJointSetHinge2Param(car->joints[i], dParamSuspensionCFM, 0.001);

        // steering
        if (i<2) {
            dJointSetHinge2Param (car->joints[i],dParamFMax,500);
            dJointSetHinge2Param (car->joints[i],dParamLoStop,-0.5);
            dJointSetHinge2Param (car->joints[i],dParamHiStop,0.5);
            dJointSetHinge2Param (car->joints[i],dParamLoStop,-0.5);
            dJointSetHinge2Param (car->joints[i],dParamHiStop,0.5);
            dJointSetHinge2Param (car->joints[i],dParamFudgeFactor,0.1);
        }

        dBodyID body1 = dJointGetBody(car->joints[i], 0);
        dBodyID body2 = dJointGetBody(car->joints[i], 1);

        const dReal *pos1 = dBodyGetPosition(body1);
        const dReal *pos2 = dBodyGetPosition(body2);
        float dx = pos2[0] - pos1[0];
        float dy = pos2[1] - pos1[1];
        float dz = pos2[2] - pos1[2];
        car->restLength[i] = sqrtf(dx*dx + dy*dy + dz*dz);
        printf("joint %d rest length %f\n", i, car->restLength[i]);
    }
    // disable motor on front wheels
    dJointSetHinge2Param(car->joints[0], dParamFMax2, 0);
    dJointSetHinge2Param(car->joints[1], dParamFMax2, 0);

    return car;
}


void updateVehicle(vehicle *car, float accel, float maxAccelForce, 
                    float steer, float steerFactor)
{
    float target;
    target = 0;
    if (fabs(accel) > 0.1) target = maxAccelForce;
    
    dJointSetHinge2Param( car->joints[0], dParamVel2, -accel );
    dJointSetHinge2Param( car->joints[1], dParamVel2, accel );
    
    dJointSetHinge2Param( car->joints[2], dParamVel2, -accel );
    dJointSetHinge2Param( car->joints[3], dParamVel2, accel );

    //dJointSetHinge2Param( car->joints[0], dParamFMax2, target );
    //dJointSetHinge2Param( car->joints[1], dParamFMax2, target );
    dJointSetHinge2Param( car->joints[2], dParamFMax2, target );
    dJointSetHinge2Param( car->joints[3], dParamFMax2, target );
    
    float Kp = steerFactor;  // Proportional gain
    float Kd = 0.5f;         // Derivative gain (tweak as needed)

    for (int i = 0; i < 2; i++) {
        float error = steer - dJointGetHinge2Angle1(car->joints[i]);          // desired - actual
        float errorRate = -dJointGetHinge2Angle1Rate(car->joints[i]);         // negative of current angular velocity
        float control = Kp * error + Kd * errorRate;

        dJointSetHinge2Param(car->joints[i], dParamVel, control);
    }
}    


void unflipVehicle (vehicle *car)
{
    const dReal* cp = dBodyGetPosition(car->bodies[0]);
    dBodySetPosition(car->bodies[0], cp[0], cp[1]+2, cp[2]);

    const dReal* R = dBodyGetRotation(car->bodies[0]);
    dReal newR[16];
    dRFromEulerAngles(newR, 0, -atan2(-R[2],R[0]) , 0);
    dBodySetRotation(car->bodies[0], newR);
    
    // wheel offsets
    // TODO make configurable & use in vehicle set up 
    dReal wheelOffsets[4][3] = {
           { +1.2, -.6, -1 },
           { +1.2, -.6, +1 },
           { -1.2, -.6, -1 },
           { -1.2, -.6, +1 }
        };

    for (int i=1; i<5; i++) {
        dVector3 pb;
        dBodyGetRelPointPos(car->bodies[0], wheelOffsets[i-1][0], wheelOffsets[i-1][1], wheelOffsets[i-1][2], pb);
        dBodySetPosition(car->bodies[i], pb[0], pb[1], pb[2]);
    }

}


void DrawJointAxes(dJointID joint, float scale)
{
    if (!joint || dJointGetType(joint) != dJointTypeHinge2) return;

    dVector3 anchor, axis1, axis2;

    dJointGetHinge2Anchor(joint, anchor);
    dJointGetHinge2Axis1(joint, axis1);
    dJointGetHinge2Axis2(joint, axis2);

    Vector3 pos  = { anchor[0], anchor[1], anchor[2] };
    Vector3 a1   = { axis1[0], axis1[1], axis1[2] };
    Vector3 a2   = { axis2[0], axis2[1], axis2[2] };

    Vector3 end1 = Vector3Add(pos, Vector3Scale(a1, scale));
    Vector3 end2 = Vector3Add(pos, Vector3Scale(a2, scale));

    DrawLine3D(pos, end1, RED);    // Steering axis (axis1)
    DrawLine3D(pos, end2, GREEN);  // Wheel rolling axis (axis2)
    DrawSphere(pos, 0.06f, BLUE);  // Anchor point
}

void DrawJoint(dJointID joint)
{
    dVector3 anchor, axis1, axis2;
    dJointGetHinge2Anchor(joint, anchor);
    dJointGetHinge2Axis1(joint, axis1);
    dJointGetHinge2Axis2(joint, axis2);

    dBodyID body1 = dJointGetBody(joint, 0);
    dBodyID body2 = dJointGetBody(joint, 1);

    const dReal *pos1 = dBodyGetPosition(body1);
    const dReal *pos2 = dBodyGetPosition(body2);

    // Vector from body1 to body2
    float dx = pos2[0] - pos1[0];
    float dy = pos2[1] - pos1[1];
    float dz = pos2[2] - pos1[2];

    // Project this vector onto axis1 (dot product)
    float compression = dx * axis1[0] + dy * axis1[1] + dz * axis1[2];

    Vector3 anchorVec = { anchor[0], anchor[1], anchor[2] };
    Vector3 axisVec = { axis1[0], axis1[1], axis1[2] };

    // Show the axis direction (e.g., yellow line)
    DrawLine3D(anchorVec,
           Vector3Add(anchorVec, Vector3Scale(axisVec, 1.0f)),
           YELLOW);

    // Show a bar along axis1 with length proportional to compression
    DrawLine3D(anchorVec,
        Vector3Add(anchorVec, Vector3Scale(axisVec, compression)),
        (compression < 0) ? RED : GREEN);
}

void DrawSpring3D(Vector3 start, Vector3 end, float radius, int coils, Color color) {
    Vector3 axis = Vector3Subtract(end, start);
    float length = Vector3Length(axis);
    Vector3 dir = Vector3Normalize(axis);

    // Build a basis: choose a perpendicular vector
    Vector3 up = (fabs(dir.y) < 0.99f) ? (Vector3){0, 1, 0} : (Vector3){1, 0, 0};
    Vector3 side = Vector3Normalize(Vector3CrossProduct(dir, up));
    Vector3 binormal = Vector3Normalize(Vector3CrossProduct(dir, side));

    int segments = coils * 10;
    float angleStep = 2 * PI * coils / segments;

    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;

        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        Vector3 p1 = Vector3Add(start,
            Vector3Add(Vector3Scale(dir, t1 * length),
            Vector3Add(Vector3Scale(side, radius * cosf(angle1)),
                       Vector3Scale(binormal, radius * sinf(angle1)))));

        Vector3 p2 = Vector3Add(start,
            Vector3Add(Vector3Scale(dir, t2 * length),
            Vector3Add(Vector3Scale(side, radius * cosf(angle2)),
                       Vector3Scale(binormal, radius * sinf(angle2)))));

        DrawLine3D(p1, p2, color);
    }
}


void DrawSpring(dJointID joint, float restLength)
{
    if (!joint || dJointGetType(joint) != dJointTypeHinge2) return;


    clock_gettime(CLOCK_MONOTONIC, &now);

    double elapsed = (now.tv_sec - start_time.tv_sec) + (now.tv_nsec - start_time.tv_nsec) / 1e9;

    if (elapsed > 5.0) {
        if (!haveDebugged) {
            ddebug = true;
        }
    }

    if (ddebug){
        printf("DrawSpring: restLength: %f\n",restLength);
    }

    dVector3 anchorRaw, axis1Raw;
    dJointGetHinge2Anchor(joint, anchorRaw);
    dJointGetHinge2Axis1(joint, axis1Raw);

    Vector3 anchor = (Vector3){ anchorRaw[0], anchorRaw[1], anchorRaw[2] };
    Vector3 axis1  = (Vector3){ axis1Raw[0], axis1Raw[1], axis1Raw[2] };
    if(ddebug){
        printf("DrawSpring: anchor: (%f, %f, %f)\n",anchor.x,anchor.y,anchor.z);
        printf("DrawSpring: axis1: (%f, %f, %f)\n",axis1.x,axis1.y,axis1.z);
        float length = Vector3Length(axis1);
        printf("DrawSpring: length: %f\n",length);

    }
    dBodyID body1 = dJointGetBody(joint, 0);
    dBodyID body2 = dJointGetBody(joint, 1);

    const dReal* p1 = dBodyGetPosition(body1);
    const dReal* p2 = dBodyGetPosition(body2);

    Vector3 pos1 = (Vector3){ p1[0], p1[1], p1[2] };
    Vector3 pos2 = (Vector3){ p2[0], p2[1], p2[2] };

    Vector3 delta = Vector3Subtract(pos2, pos1);
    float compression = Vector3DotProduct(delta, axis1);
    float deflection = compression - restLength;

    Vector3 start = anchor;
    Vector3 end = Vector3Add(anchor, Vector3Scale(axis1, deflection));
    if (ddebug){
        printf("DrawSpring: pos1: (%f, %f, %f)\n",pos1.x,pos1.y,pos1.z);
        printf("DrawSpring: pos2: (%f, %f, %f)\n",pos2.x,pos2.y,pos2.z);
        printf("DrawSpring: delta: (%f, %f, %f)\n",delta.x,delta.y,delta.z);
        float length = Vector3Length(delta);
        printf("DrawSpring: delta length: %f\n",length);
        printf("DrawSpring: compression: %f\n",compression);
        printf("DrawSpring: deflection: %f\n",deflection);
        printf("DrawSpring: start: (%f, %f, %f)\n",start.x,start.y,start.z);
        printf("DrawSpring: end: (%f, %f, %f)\n",end.x,end.y,end.z);
        ddebug = false;
        haveDebugged = true;
    }
    DrawSpring3D(start, end, 0.05f, 8, BLUE);
}