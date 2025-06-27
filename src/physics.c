#include <ode/ode.h>
#include "physics.h"
#include "audio.h"
#include <stdio.h>
#include <stdint.h>


static dWorldID world;
static dSpaceID space;
static dJointGroupID contactGroup;
static dGeomID groundGeom;
static dGeomID terrainGeom;

typedef struct {
    int id;
    dBodyID body;
    dGeomID geom;
    dVector3 lastVelocity;
    Model model;  // Optional: store model for rendering
} PhysicsObject;

static PhysicsObject objects[MAX_BODIES];

static void nearCallback(void *data, dGeomID o1, dGeomID o2) {
    (void)data;  // Unused parameter
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    dContact contact;
    contact.surface.mode = dContactBounce | dContactApprox1;
    contact.surface.mu = dInfinity;
    contact.surface.bounce = 0.2;
    contact.surface.bounce_vel = 0.1;

    if (dCollide(o1, o2, 1, &contact.geom, sizeof(dContact))) {
        dJointID c = dJointCreateContact(world, contactGroup, &contact);
        dJointAttach(c, b1, b2);
    }
}

// Convert Raylib Mesh to ODE TriMesh
dGeomID CreateODETriMeshFromRaylibMesh(Mesh *mesh, dSpaceID space, dTriMeshDataID *outTriData)
{
    // Copy Raylib mesh vertex data (assumed layout: x,y,z x,y,z ...)
    int vertexCount = mesh->vertexCount;
    float *vertices = malloc(sizeof(float) * vertexCount * 3);
    for (int i = 0; i < vertexCount * 3; i++) {
        vertices[i] = mesh->vertices[i];
    }

    // Copy and widen indices from ushort to int
    int triangleCount = mesh->triangleCount;
    int *indices = malloc(sizeof(int) * triangleCount * 3);
    for (int i = 0; i < triangleCount * 3; i++) {
        indices[i] = mesh->indices[i];
    }

    // Create and build trimesh data
    dTriMeshDataID triData = dGeomTriMeshDataCreate();
    dGeomTriMeshDataBuildSingle(triData,
        vertices,                  // Vertex array
        3 * sizeof(float),         // Stride
        vertexCount,
        indices,                   // Index array
        triangleCount * 3,
        3 * sizeof(int)            // Stride
    );

    // Store the trimesh data if caller wants to keep it
    if (outTriData) *outTriData = triData;

    // Create and return the trimesh geometry
    dGeomID geom = dCreateTriMesh(space, triData, NULL, NULL, NULL);
    return geom;
}

void SetTerrainTriMesh(Mesh *mesh) {
    terrainGeom = CreateODETriMeshFromRaylibMesh(mesh, space, NULL);
    dGeomSetData(terrainGeom, "terrain");
}

size_t SCREEN_WIDTH = SIZE_MAX;
size_t SCREEN_HEIGHT = SIZE_MAX;
float HALF_SCREEN_WIDTH = -1.0f;
float HALF_SCREEN_HEIGHT = -1.0f;

size_t MONITOR_WIDTH = SIZE_MAX;
size_t MONITOR_HEIGHT = SIZE_MAX;
float HALF_MONITOR_WIDTH = -1.0f;
float HALF_MONITOR_HEIGHT = -1.0f;

void InitPhysics() {
    // Get the primary monitor's resolution before window creation
    size_t CELL_SIZE = 50;
    int monitor = GetCurrentMonitor();
    MONITOR_HEIGHT = GetMonitorHeight(monitor);
    MONITOR_WIDTH = GetMonitorWidth(monitor);
    printf("Monitor %d: %zu x %zu\n", monitor, MONITOR_WIDTH, MONITOR_HEIGHT);
    MONITOR_WIDTH = (MONITOR_WIDTH/CELL_SIZE)*CELL_SIZE;
    MONITOR_HEIGHT = (MONITOR_HEIGHT/CELL_SIZE)*CELL_SIZE;
    printf("Monitor %d: %zu x %zu\n", monitor, MONITOR_WIDTH, MONITOR_HEIGHT);
    HALF_MONITOR_WIDTH = MONITOR_WIDTH / 2.0f;
    HALF_MONITOR_HEIGHT = MONITOR_HEIGHT / 2.0f;

    dInitODE();
    world = dWorldCreate();
    space = dHashSpaceCreate(0);
    contactGroup = dJointGroupCreate(0);
    dWorldSetGravity(world, 0, -9.81, 0);

    // Ground plane
    groundGeom = dCreatePlane(space, 0, 1, 0, 0);
    printf("Ground plane created\n");
    printf("MONITOR_WIDTH: %zu, MONITOR_HEIGHT: %zu\n", MONITOR_WIDTH, MONITOR_HEIGHT);
    for (int i = 0; i < MAX_BODIES; i++) {
        objects[i].id = i;
        objects[i].body = dBodyCreate(world);
        objects[i].geom = dCreateBox(space, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
        dGeomSetBody(objects[i].geom, objects[i].body);

        dMass m;
        dMassSetBox(&m, 1.0, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
        dBodySetMass(objects[i].body, &m);
        dBodySetPosition(objects[i].body, 
                         GetRandomValue(-HALF_MONITOR_HEIGHT, HALF_MONITOR_HEIGHT),
                         GetRandomValue(450, 500),  // Start above ground
                         GetRandomValue(-HALF_MONITOR_WIDTH, HALF_MONITOR_WIDTH));

        dBodySetData(objects[i].body, &objects[i]);
        Mesh mesh = GenMeshCube(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
        objects[i].model = LoadModelFromMesh(mesh);
    }
    SCREEN_HEIGHT = GetScreenHeight();
    SCREEN_WIDTH = GetScreenWidth();
    printf("SCREEN_WIDTH: %zu, SCREEN_HEIGHT: %zu\n", SCREEN_WIDTH, SCREEN_HEIGHT);
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

    if (IsKeyPressed(KEY_SPACE)) {
        ApplyRandomJumpToAllBodies();
    }
}

void ApplyRandomJumpToAllBodies() {
    for (int i = 0; i < MAX_BODIES; i++) {
        float vx = GetRandomValue(-10, 10);
        float vy = 50.0f + GetRandomValue(0, 100);
        float vz = GetRandomValue(-10, 10);  //
        dBodySetLinearVel(objects[i].body, vx, vy, vz);
    }
}

void ShutdownPhysics() {
    for (int i = 0; i < MAX_BODIES; i++) {
        if (objects[i].geom) dGeomDestroy(objects[i].geom);
        if (objects[i].body) dBodyDestroy(objects[i].body);
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

Quaternion QuaternionFromODE(const dReal *R)
{
    Quaternion q;
    float trace = R[0] + R[5] + R[10]; // R[0], R[5], R[10] are diagonal

    if (trace > 0) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (R[9] - R[6]) / s;
        q.y = (R[2] - R[8]) / s;
        q.z = (R[4] - R[1]) / s;
    } else if ((R[0] > R[5]) && (R[0] > R[10])) {
        float s = sqrtf(1.0f + R[0] - R[5] - R[10]) * 2.0f;
        q.w = (R[9] - R[6]) / s;
        q.x = 0.25f * s;
        q.y = (R[1] + R[4]) / s;
        q.z = (R[2] + R[8]) / s;
    } else if (R[5] > R[10]) {
        float s = sqrtf(1.0f + R[5] - R[0] - R[10]) * 2.0f;
        q.w = (R[2] - R[8]) / s;
        q.x = (R[1] + R[4]) / s;
        q.y = 0.25f * s;
        q.z = (R[6] + R[9]) / s;
    } else {
        float s = sqrtf(1.0f + R[10] - R[0] - R[5]) * 2.0f;
        q.w = (R[4] - R[1]) / s;
        q.x = (R[2] + R[8]) / s;
        q.y = (R[6] + R[9]) / s;
        q.z = 0.25f * s;
    }

    return q;
}

void GetPhysicsBodyAxisAngle(int index, Vector3 *axis, float *angle) {
    if (index < 0 || index >= MAX_BODIES) {
        *angle = 0.0f;
        *axis = (Vector3){0};
        return;
    }

    const dReal *R = dBodyGetRotation(objects[index].body);
    Quaternion q = QuaternionFromODE(R);
    QuaternionToAxisAngle(q, axis, angle);
}
Model GetPhysicsBodyModel(int index) {
    if (index < 0 || index >= MAX_BODIES) return (Model){0};
    return objects[index].model;
}

void AttachShaderToPhysicsBodies(Shader shader) {
    for (int i = 0; i < MAX_BODIES; i++) {
        objects[i].model.materials[0].shader = shader;
    }
}