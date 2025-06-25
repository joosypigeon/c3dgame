#include "raylib.h"
#include "render.h"
#include "physics.h"
#include "camera.h"
#include "torus.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include <stdlib.h>

#define TORUS_MAJOR_SEGMENTS 256
#define TORUS_MINOR_SEGMENTS 128

static Camera3D camera;

Shader shader = { 0 };
Light lights[MAX_LIGHTS] = { 0 };
Model terrain = { 0 };

void InitRenderer() {
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    shader = LoadShader("../src/lighting.vs","../src/lighting.fs");

    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 0.1f, 0.1f, 0.1f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Create lights
    lights[0] = CreateLight(LIGHT_POINT, (Vector3){ -HALF_SCREEN_WIDTH, 200, -HALF_SCREEN_HEIGHT }, Vector3Zero(), YELLOW, shader);
    lights[1] = CreateLight(LIGHT_POINT, (Vector3){ HALF_SCREEN_WIDTH, 200, HALF_SCREEN_HEIGHT }, Vector3Zero(), RED, shader);
    lights[2] = CreateLight(LIGHT_POINT, (Vector3){ -HALF_SCREEN_WIDTH, 200, HALF_SCREEN_HEIGHT }, Vector3Zero(), GREEN, shader);
    lights[3] = CreateLight(LIGHT_POINT, (Vector3){ HALF_SCREEN_WIDTH, 200, -HALF_SCREEN_HEIGHT }, Vector3Zero(), BLUE, shader);

    float R = SCREEN_WIDTH / (2.0f * PI);
    float r = SCREEN_HEIGHT / (2.0f * PI);
    SetTorusDimensions(R, r);
    Mesh terrain_mesh = MyGenFlatTorusMesh(TORUS_MAJOR_SEGMENTS, TORUS_MINOR_SEGMENTS);
    SetTerrainTriMesh(&terrain_mesh);
    GenMeshTangents(&terrain_mesh);
    terrain = LoadModelFromMesh(terrain_mesh);
    terrain.materials[0].shader = shader;
}

void BeginRender() {
    UpdateCameraManual(&camera);
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
            (float)SCREEN_WIDTH / SCREEN_HEIGHT,
            10.0f,     // near clip
            10000.0f   // far clip
        ));
}

void DrawScene() {
    DrawModel(terrain, Vector3Zero(), 1.0f, WHITE);
    DrawGrid(10, 1.0f);
    DrawCube((Vector3){0.0f, -0.5f, 0.0f}, 10.0f, 1.0f, 10.0f, LIGHTGRAY); // Ground
    DrawCubeWires((Vector3){0.0f, -0.5f, 0.0f}, 10.0f, 1.0f, 10.0f, DARKGRAY);

    for (int i = 0; i < MAX_BODIES; i++) {
        Vector3 pos = GetPhysicsBodyPosition(i);
        DrawCube(pos, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, RED);
        DrawCubeWires(pos, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, BLACK);
    }

    // Draw spheres to show where the lights are
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled) DrawSphereEx(lights[i].position, 100.0f, 8, 8, lights[i].color);
        else DrawSphereWires(lights[i].position, 100.0f, 8, 8, ColorAlpha(lights[i].color, 0.3f));
    }
}

void EndRender() {
    EndMode3D();
}

void ShutdownRenderer() {
    // Unload models, textures, shaders
}