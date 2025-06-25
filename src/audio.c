#include "raylib.h"
#include "audio.h"

#define ASSET_PATH "../src/assets/"

static Sound impactSound;

void InitAudio() {
    InitAudioDevice();
    impactSound = LoadSound(ASSET_PATH "dink.wav");
}

void ShutdownAudio() {
    UnloadSound(impactSound);
    CloseAudioDevice();
}

void PlayImpactSound() {
    PlaySound(impactSound);
}