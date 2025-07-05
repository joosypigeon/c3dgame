#include <stdio.h>
#include "camera.h"


void UpdateCameraManual(Camera3D *camera)
{
    static float cameraYaw = PI / 4.0f;
    static float cameraPitch = PI / 4.0f;
    static float cameraDistance = 2000.0f;
    static Vector3 target = { 0.0f, 0.0f, 0.0f };

    float wheel = GetMouseWheelMove();
    cameraDistance -= wheel * 50.0f;
    if (cameraDistance < 10.0f) cameraDistance = 10.0f;

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        cameraYaw -= delta.x * 0.01f;
        cameraPitch += delta.y * 0.01f;

        // Clamp pitch to avoid flipping
        if (cameraPitch > PI/2.0f - 0.01f) cameraPitch = PI/2.0f - 0.01f;
        if (cameraPitch < -PI/2.0f + 0.01f) cameraPitch = -PI/2.0f + 0.01f;
    }

    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        float panSpeed = cameraDistance * 0.001f;

        Vector3 right = (Vector3){
            cosf(cameraYaw), 0.0f, -sinf(cameraYaw)
        };
        Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };

        target = Vector3Add(target, Vector3Scale(right, -delta.x * panSpeed));
        target = Vector3Add(target, Vector3Scale(up, delta.y * panSpeed));
    }

    Vector3 offset = {
        cameraDistance * cosf(cameraPitch) * sinf(cameraYaw),
        cameraDistance * sinf(cameraPitch),
        cameraDistance * cosf(cameraPitch) * cosf(cameraYaw)
    };

    camera->position = Vector3Add(target, offset);
    camera->target = target;
    camera->up = (Vector3){ 0.0f, 1.0f, 0.0f };
}

Vector3 carPosition = { 0.0f, 0.0f, 0.0f };
void UpdateCarPosition(dVector3 pos){
    carPosition.x = pos[0];
    carPosition.y = pos[1];
    carPosition.z = pos[2];
}
float lerp = 0.1f;
void UpdateOffestCameraManual(Camera3D *camera)
{
    static float cameraYaw = PI / 4.0f;
    static float cameraPitch = PI / 4.0f;
    static float cameraDistance = 0.0f;
    static Vector3 deviation = { 0.0f, 0.0f, 0.0f };

    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {

        deviation.x = 0.0f;
        deviation.y = 0.0f;
        deviation.z = 0.0f;
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        cameraYaw -= delta.x * 0.01f;
        cameraPitch += delta.y * 0.01f;

        // Clamp pitch to avoid flipping
        if (cameraPitch > PI/2.0f - 0.01f) cameraPitch = PI/2.0f - 0.01f;
        if (cameraPitch < -PI/2.0f + 0.01f) cameraPitch = -PI/2.0f + 0.01f;
    }

    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        float panSpeed = cameraDistance * 0.001f;

        Vector3 right = (Vector3){
            cosf(cameraYaw), 0.0f, -sinf(cameraYaw)
        };
        Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };

        deviation = Vector3Add(deviation, Vector3Scale(right, -delta.x * panSpeed));
        deviation = Vector3Add(deviation, Vector3Scale(up, delta.y * panSpeed));
    }

    Vector3 offset = {
        cameraDistance * cosf(cameraPitch) * sinf(cameraYaw),
        cameraDistance * sinf(cameraPitch),
        cameraDistance * cosf(cameraPitch) * cosf(cameraYaw)
    };

    camera->position.x -= (camera->position.x - carPosition.x) * lerp;// * (1/ft);
    camera->position.y -= (camera->position.y - carPosition.y) * lerp;// * (1/ft);
    camera->position.z -= (camera->position.z - carPosition.z) * lerp;// * (1/ft);
    cameraDistance = Vector3Length(Vector3Subtract(camera->position, (Vector3){carPosition.x, carPosition.y + 1, carPosition.z}));
    camera->target = Vector3Subtract((Vector3){carPosition.x, carPosition.y + 1, carPosition.z}, deviation);
    camera->up = (Vector3){ 0.0f, 1.0f, 0.0f };
}
