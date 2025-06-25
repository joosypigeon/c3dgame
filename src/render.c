#include "raylib.h"
#include "render.h"
#include "physics.h"
#include "camera.h"

static Camera3D camera;

void InitRenderer() {
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void BeginRender() {
    UpdateCameraManual(&camera);
    BeginMode3D(camera);
}

void DrawScene() {
    DrawGrid(10, 1.0f);
    DrawCube((Vector3){0.0f, -0.5f, 0.0f}, 10.0f, 1.0f, 10.0f, LIGHTGRAY); // Ground
    DrawCubeWires((Vector3){0.0f, -0.5f, 0.0f}, 10.0f, 1.0f, 10.0f, DARKGRAY);

    for (int i = 0; i < MAX_BODIES; i++) {
        Vector3 pos = GetPhysicsBodyPosition(i);
        DrawCube(pos, 1.0f, 1.0f, 1.0f, RED);
        DrawCubeWires(pos, 1.0f, 1.0f, 1.0f, BLACK);
    }
}

void EndRender() {
    EndMode3D();
}

void ShutdownRenderer() {
    // Unload models, textures, shaders
}