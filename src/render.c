#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "render.h"
#include "physics.h"
#include "vehicle.h"
#include "camera.h"
#include "torus.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include <stdlib.h>

#define SHADER_PATH "../src/" 
#define IMAGE_PATH "../src/assets/images/"
#define DATA_PATH "../src/assets/data/"

#define TORUS_MAJOR_SEGMENTS 256
#define TORUS_MINOR_SEGMENTS 128

#define FORWARD_MAX_ACCELERATION 75.0f
#define REVERSE_MAX_ACCELERATION 25.0f
#define ACCELERATION_RATE 2.5f
#define MAX_ACCEL_FORCE 800.0f

static Camera3D camera;

Shader shader = { 0 };
Light lights[MAX_LIGHTS] = { 0 };
Model terrain = { 0 };
Model skySphere = { 0 };

// For vehicle models
Model box;
Model ball;
Model cylinder;
vehicle *car = NULL;

void InitRenderer() {
    //camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    //camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    //camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    //camera.fovy = 45.0f;
    //camera.projection = CAMERA_PERSPECTIVE;

    // Define the camera to look into our 3d world
    camera.position = (Vector3){ 15, 6+100, 0.0 };
    camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;


    shader = LoadShader(SHADER_PATH "lighting.vs", SHADER_PATH  "lighting.fs");

    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 0.1f, 0.1f, 0.1f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Create lights
    lights[0] = CreateLight(LIGHT_POINT, (Vector3){ -HALF_MONITOR_WIDTH, 200, -HALF_MONITOR_HEIGHT }, Vector3Zero(), YELLOW, shader);
    lights[1] = CreateLight(LIGHT_POINT, (Vector3){ HALF_MONITOR_WIDTH, 200, HALF_MONITOR_HEIGHT }, Vector3Zero(), RED, shader);
    lights[2] = CreateLight(LIGHT_POINT, (Vector3){ -HALF_MONITOR_WIDTH, 200, HALF_MONITOR_HEIGHT }, Vector3Zero(), GREEN, shader);
    lights[3] = CreateLight(LIGHT_POINT, (Vector3){ HALF_MONITOR_WIDTH, 200, -HALF_MONITOR_HEIGHT }, Vector3Zero(), BLUE, shader);

    float R = MONITOR_WIDTH / (2.0f * PI);
    float r = MONITOR_HEIGHT / (2.0f * PI);
    SetTorusDimensions(R, r);
    Mesh terrain_mesh = MyGenFlatTorusMesh(TORUS_MAJOR_SEGMENTS, TORUS_MINOR_SEGMENTS);
    SetTerrainTriMesh(&terrain_mesh);
    GenMeshTangents(&terrain_mesh);
    terrain = LoadModelFromMesh(terrain_mesh);
    terrain.materials[0].shader = shader;
    Image checked = GenImageChecked(1024, 1024, 32, 32, DARKGRAY, LIGHTGRAY);
    Texture2D texChecked = LoadTextureFromImage(checked);
    SetTextureWrap(texChecked, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(texChecked, TEXTURE_FILTER_BILINEAR);
    terrain.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texChecked;

    AttachShaderToPhysicsBodies(shader);

    // Load sky texture (should be 2:1 ratio, like 4096x2048)
    printf("Loading sky texture from: %s\n", IMAGE_PATH "starfield.jpg");
    Texture2D skyTex = LoadTexture(IMAGE_PATH "starfield.jpg");

    // Generate a sphere mesh and create a model
    Mesh sphereMesh = GenMeshSphere(10000.0f, 64, 64);
    skySphere = LoadModelFromMesh(sphereMesh);
    skySphere.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = skyTex;

    // Invert culling so inside of sphere is visible
    SetMaterialTexture(&skySphere.materials[0], MATERIAL_MAP_DIFFUSE, skyTex);
    skySphere.transform = MatrixScale(-1.0f, 1.0f, 1.0f);  // Invert normals 

    // Load vehicle models
    box = LoadModelFromMesh(GenMeshCube(1,1,1));
    ball = LoadModelFromMesh(GenMeshSphere(.5,32,32));
    // alas gen cylinder is wrong orientation for ODE...
    // so rather than muck about at render time just make one the right orientation
    cylinder = LoadModel(DATA_PATH "cylinder.obj");

    // texture the models
    Texture earthTx = LoadTexture(DATA_PATH "earth.png");
    Texture crateTx = LoadTexture(DATA_PATH "crate.png");
    Texture drumTx = LoadTexture(DATA_PATH "drum.png");

    box.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = crateTx;
    ball.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = earthTx;
    cylinder.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = drumTx;

    car = CreateVehicle2(GetPhysicsSpace(), GetPhysicsWorld());

}

void BeginRender() {
    ClearBackground(BLACK);

    //UpdateCameraManual(&camera);

    // Projection matrix (pass aspect ratio)
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();

    // Check key inputs to enable/disable lights
    if (IsKeyPressed(KEY_Y)) { lights[0].enabled = !lights[0].enabled; }
    if (IsKeyPressed(KEY_R)) { lights[1].enabled = !lights[1].enabled; }
    if (IsKeyPressed(KEY_G)) { lights[2].enabled = !lights[2].enabled; }
    if (IsKeyPressed(KEY_B)) { lights[3].enabled = !lights[3].enabled; }
    
    // Update light values (actually, only enable/disable them)
    for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(shader, lights[i]);
    BeginMode3D(camera);
        rlSetMatrixProjection(MatrixPerspective(
            DEG2RAD * camera.fovy,
            aspect,
            1.0f,     // near clip
            10000.0f   // far clip
        ));

        // Draw sky sphere centered on camera (moves with it)
        DrawModel(skySphere, camera.position, 1.0f, WHITE);
}

float accel=0,steer=0;
Vector3 debug = {0};
bool antiSway = true;

// keep the physics fixed time in step with the render frame
// rate which we don't know in advance
float frameTime = 0; 
float physTime = 0;
const float physSlice = 1.0 / 240.0;
const int maxPsteps = 6;
int carFlipped = 0; // number of frames car roll is >90


// these two just convert to column major and minor
void rayToOdeMat(Matrix* m, dReal* R) {
    R[ 0] = m->m0;   R[ 1] = m->m4;   R[ 2] = m->m8;    R[ 3] = 0;
    R[ 4] = m->m1;   R[ 5] = m->m5;   R[ 6] = m->m9;    R[ 7] = 0;
    R[ 8] = m->m2;   R[ 9] = m->m6;   R[10] = m->m10;   R[11] = 0;
    R[12] = 0;       R[13] = 0;       R[14] = 0;        R[15] = 1;   
}

// sets a raylib matrix from an ODE rotation matrix
void odeToRayMat(const dReal* R, Matrix* m)
{
    m->m0 = R[0];  m->m1 = R[4];  m->m2 = R[8];      m->m3 = 0;
    m->m4 = R[1];  m->m5 = R[5];  m->m6 = R[9];      m->m7 = 0;
    m->m8 = R[2];  m->m9 = R[6];  m->m10 = R[10];    m->m11 = 0;
    m->m12 = 0;    m->m13 = 0;    m->m14 = 0;        m->m15 = 1;
}

// position rotation scale all done with the models transform...
void MyDrawModel(Model model, Color tint)
{
    
    for (int i = 0; i < model.meshCount; i++)
    {
        Color color = model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;

        Color colorTint = WHITE;
        colorTint.r = (unsigned char)((((float)color.r/255.0)*((float)tint.r/255.0))*255.0f);
        colorTint.g = (unsigned char)((((float)color.g/255.0)*((float)tint.g/255.0))*255.0f);
        colorTint.b = (unsigned char)((((float)color.b/255.0)*((float)tint.b/255.0))*255.0f);
        colorTint.a = (unsigned char)((((float)color.a/255.0)*((float)tint.a/255.0))*255.0f);

        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = colorTint;
        DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]], model.transform);
        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
    }
}

void drawGeom(dGeomID geom) 
{
    const dReal* pos = dGeomGetPosition(geom);
    const dReal* rot = dGeomGetRotation(geom);
    int class = dGeomGetClass(geom);
    Model* m = 0;
    dVector3 size;
    if (class == dBoxClass) {
        m = &box;
        dGeomBoxGetLengths(geom, size);
    } else if (class == dSphereClass) {
        m = &ball;
        float r = dGeomSphereGetRadius(geom);
        size[0] = size[1] = size[2] = (r*2);
    } else if (class == dCylinderClass) {
        m = &cylinder;
        dReal l,r;
        dGeomCylinderGetParams (geom, &r, &l);
        size[0] = size[1] = r*2;
        size[2] = l;
    }
    if (!m) return;
    
    Matrix matScale = MatrixScale(size[0], size[1], size[2]);
    Matrix matRot;
    odeToRayMat(rot, &matRot);
    Matrix matTran = MatrixTranslate(pos[0], pos[1], pos[2]);
    
    m->transform = MatrixMultiply(MatrixMultiply(matScale, matRot), matTran);
    

    
    Color c = WHITE;

    MyDrawModel(*m, c);
}

void drawAllSpaceGeoms(dSpaceID space) 
{
    int ng = dSpaceGetNumGeoms(space);
    for (int i=0; i<ng; i++) {
        if (i == ng - 1) {
            // last geom is the ground plane, skip it
            continue;
        }
        dGeomID geom = dSpaceGetGeom(space, i);
        drawGeom(geom);

    }
}

void DrawVehicle() {
    for (size_t i = 0; i < 6; i++)
    {
        if (checkColliding(car->geoms[i])) drawGeom(car->geoms[i]);
    }

    for (size_t i = 0; i < 4; i++) {
        DrawJointAxes(car->joints[i], 1.0f);
        //DrawJoint(car->joints[i]);
        DrawSpring(car->joints[i], car->restLength[i]);
    }


}


void DrawScene() {
    DrawModel(terrain, Vector3Zero(), 1.0f, WHITE);
    DrawGrid(1000, 10.0f);


        // extract just the roll of the car
        // count how many frames its >90 degrees either way
        const dReal* q = dBodyGetQuaternion(car->bodies[0]);
        float z0 = 2.0f*(q[0]*q[3] + q[1]*q[2]);
        float z1 = 1.0f - 2.0f*(q[1]*q[1] + q[3]*q[3]);
        float roll = atan2f(z0, z1);
        if ( fabs(roll) > (M_PI_2-0.001) ) {
            carFlipped++;
        } else {
            carFlipped=0;
        }
    
        // if the car roll >90 degrees for 100 frames then flip it
        if (carFlipped > 100) {
            unflipVehicle(car);
        }

        accel *= .99;
        if (IsKeyDown(KEY_UP)) accel += ACCELERATION_RATE;
        if (IsKeyDown(KEY_DOWN)) accel -= ACCELERATION_RATE;
        if (accel > FORWARD_MAX_ACCELERATION) accel = FORWARD_MAX_ACCELERATION;
        if (accel < -REVERSE_MAX_ACCELERATION) accel = -REVERSE_MAX_ACCELERATION;


        if (IsKeyDown(KEY_RIGHT)) steer -=.1;
        if (IsKeyDown(KEY_LEFT)) steer +=.1;
        if (!IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT)) steer *= .5;
        if (steer > .5) steer = .5;
        if (steer < -.5) steer = -.5;

        updateVehicle(car, accel, MAX_ACCEL_FORCE, steer, 10.0);

        const dReal* cp = dBodyGetPosition(car->bodies[0]);
        camera.target = (Vector3){cp[0],cp[1]+1,cp[2]};
        
        float lerp = 0.1f;

        dVector3 co;
        dBodyGetRelPointPos(car->bodies[0], -8, 3, 0, co);
        
        camera.position.x -= (camera.position.x - co[0]) * lerp;// * (1/ft);
        camera.position.y -= (camera.position.y - co[1])  * lerp ;// * (1/ft);
        camera.position.z -= (camera.position.z - co[2]) * lerp;// * (1/ft);

        frameTime += GetFrameTime();
        int pSteps = 0;
        physTime = GetTime(); 
        while (frameTime > physSlice) {
            // check for collisions
            // TODO use 2nd param data to pass custom structure with
            // world and space ID's to avoid use of globals...
            CollideBodies();
            
            // step the world
            dWorldQuickStep(GetPhysicsWorld(), physSlice);  // NB fixed time step is important
            dJointGroupEmpty(GetPhysicsContactGroup());
            
            frameTime -= physSlice;
            pSteps++;
            if (pSteps > maxPsteps) {
                frameTime = 0;
                break;      
            }
        }
        
        physTime = GetTime() - physTime;    

    for (int i = 0; i < MAX_BODIES; i++) {
        Vector3 pos = GetPhysicsBodyPosition(i);
        float angle;
        Vector3 axis;
        GetPhysicsBodyAxisAngle(i, &axis, &angle);
        Model model = GetPhysicsBodyModel(i);
        float degrees = angle * RAD2DEG;

        DrawModelEx(model, pos, axis, degrees, (Vector3){1.0f, 1.0f, 1.0f}, RED);
    }
  
    //drawAllSpaceGeoms(GetPhysicsSpace()); 
    DrawVehicle();

    // Draw spheres to show where the lights are
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled) DrawSphereEx(lights[i].position, 100.0f, 8, 8, lights[i].color);
        else DrawSphereWires(lights[i].position, 100.0f, 8, 8, ColorAlpha(lights[i].color, 0.3f));
    }
}




void EndRender() {
    EndMode3D();
    DrawFPS(SCREEN_WIDTH - 100, 10);
}

void ShutdownRenderer() {
    // Unload models, textures, shaders
}