#include <ode/ode.h>
#include "physics.h"
#include "audio.h"

static dWorldID world;
static dSpaceID space;
static dJointGroupID contactGroup;
static dBodyID bodies[MAX_BODIES];
static dGeomID geoms[MAX_BODIES];
static dGeomID groundGeom;

typedef struct {
    int id;
    dBodyID body;
    dGeomID geom;
    dVector3 lastVelocity;
} PhysicsObject;

static PhysicsObject objects[MAX_BODIES];

static void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    dContact contact;
    contact.surface.mode = dContactBounce;
    contact.surface.mu = dInfinity;
    contact.surface.bounce = 0.2;
    contact.surface.bounce_vel = 0.1;

    if (dCollide(o1, o2, 1, &contact.geom, sizeof(dContact))) {
        // --- Impact Sound Logic ---
        if (b1) {
            PhysicsObject *obj = (PhysicsObject *)dBodyGetData(b1);
            float speed = sqrtf(
                obj->lastVelocity[0] * obj->lastVelocity[0] +
                obj->lastVelocity[1] * obj->lastVelocity[1] +
                obj->lastVelocity[2] * obj->lastVelocity[2]
            );
            if (speed > 5.0f) PlayImpactSound();
        }
        if (b2) {
            PhysicsObject *obj = (PhysicsObject *)dBodyGetData(b2);
            float speed = sqrtf(
                obj->lastVelocity[0] * obj->lastVelocity[0] +
                obj->lastVelocity[1] * obj->lastVelocity[1] +
                obj->lastVelocity[2] * obj->lastVelocity[2]
            );
            if (speed > 1.0f) PlayImpactSound();
        }
        dJointID c = dJointCreateContact(world, contactGroup, &contact);
        dJointAttach(c, b1, b2);
    }
}

void InitPhysics() {
    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contactGroup = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81, 0);

    // Ground plane
    groundGeom = dCreatePlane(space, 0, 1, 0, 0);

    for (int i = 0; i < MAX_BODIES; i++) {
        objects[i].id = i;
        objects[i].body = dBodyCreate(world);
        objects[i].geom = dCreateBox(space, 1.0, 1.0, 1.0);
        dGeomSetBody(objects[i].geom, objects[i].body);

        dMass m;
        dMassSetBox(&m, 1.0, 1.0, 1.0, 1.0);
        dBodySetMass(objects[i].body, &m);
        dBodySetPosition(objects[i].body, (i % 4) * 2.0 - 4.0, 10 + (i / 4) * 2.0, 0);

        dBodySetData(objects[i].body, &objects[i]);
    }
}

void UpdatePhysics() {
    const dReal stepSize = 1.0 / 60.0;
    dSpaceCollide(space, 0, &nearCallback);
    dWorldStep(world, stepSize);
    dJointGroupEmpty(contactGroup);

    for (int i = 0; i < MAX_BODIES; i++) {
        const dReal* v = dBodyGetLinearVel(objects[i].body);
        objects[i].lastVelocity[0] = v[0];
        objects[i].lastVelocity[1] = v[1];
        objects[i].lastVelocity[2] = v[2];
    }
}

void ApplyRandomJumpToAllBodies() {
    for (int i = 0; i < MAX_BODIES; i++) {
        float vx = GetRandomValue(-200, 200) / 100.0f;  // [-2.0f, 2.0f]
        float vy = 8.0f + GetRandomValue(0, 100) / 100.0f;  // [8.0f, 9.0f]
        float vz = GetRandomValue(-200, 200) / 100.0f;  // [-2.0f, 2.0f]
        dBodySetLinearVel(objects[i].body, vx, vy, vz);
    }
}

void ShutdownPhysics() {
    for (int i = 0; i < MAX_BODIES; i++) {
        dGeomDestroy(geoms[i]);
        dBodyDestroy(bodies[i]);
    }
    dJointGroupDestroy(contactGroup);
    dSpaceDestroy(space);
    dWorldDestroy(world);
    dCloseODE();
}

Vector3 GetPhysicsBodyPosition(int index) {
    if (index < 0 || index >= MAX_BODIES) return (Vector3){0};

    const dReal *p = dBodyGetPosition(objects[index].body);
    return (Vector3){ (float)p[0], (float)p[1], (float)p[2] };
}