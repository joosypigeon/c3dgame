#include "raylib.h"
#include "render.h"
#include "physics.h"
#include "audio.h"

int main(void) {
    //SetConfigFlags(FLAG_FULLSCREEN_MODE);

    InitWindow(1600, 1200, "Raylib Physics Example");
    InitAudio();
    InitPhysics();
    InitRenderer();

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdatePhysics();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginRender();
        DrawScene();
        EndRender();
        EndDrawing();
    }

    ShutdownRenderer();
    ShutdownPhysics();
    ShutdownAudio();
    CloseWindow();
    return 0;
}