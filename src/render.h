#ifndef RENDER_H
#define RENDER_H
void InitRenderer();
void BeginRender();
void DrawScene();
void EndRender();
void ShutdownRenderer();

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern float HALF_SCREEN_WIDTH;
extern float HALF_SCREEN_HEIGHT;
#endif