#include "raylib.h"
#include "audio.h"

#define ASSET_PATH "../src/assets/"

#define MAX_SOUNDS 2000
Sound soundArray[MAX_SOUNDS] = { 0 };
int currentSound = -1;

void InitAudio() {
    InitAudioDevice();
    soundArray[0] = LoadSound(ASSET_PATH "plink.wav");
    SetSoundVolume(soundArray[0], 0.01f);
    for (int i = 1; i < MAX_SOUNDS; i++)
    {
        soundArray[i] = LoadSoundAlias(soundArray[0]);
        SetSoundVolume(soundArray[i], 0.01f);  // Set volume for each sound  
    }
    currentSound = 0;  
}

void ShutdownAudio() {
    UnloadSound(soundArray[0]);
    CloseAudioDevice();
}

void PlayImpactSound() {
    PlaySound(soundArray[currentSound]);            // play the next open sound slot
    currentSound++;                                 // increment the sound slot
    if (currentSound >= MAX_SOUNDS)                 // if the sound slot is out of bounds, go back to 0
        currentSound = 0;
}