#ifndef RENDER_H
#define RENDER_H
#include "stdio.h"
void InitRenderer();
void BeginRender();
void DrawScene();
void EndRender();
void ShutdownRenderer();

extern size_t SCREEN_WIDTH;
extern size_t SCREEN_HEIGHT;
extern float HALF_SCREEN_WIDTH;
extern float HALF_SCREEN_HEIGHT;

extern size_t MONITOR_WIDTH;
extern size_t MONITOR_HEIGHT;
extern float HALF_MONITOR_WIDTH;
extern float HALF_MONITOR_HEIGHT;
#endif