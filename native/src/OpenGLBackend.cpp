#include "backend/OpenGLBackend.h"
#include <GL/gl.h>
static bool g_inited=false;
bool GL_Init(int w,int h){ glViewport(0,0,w,h); g_inited=true; return true; }
void GL_Render(float partialTicks, uint64_t frame){
    if(!g_inited) return;
    float t=(frame%300)/300.0f;
    glDisable(GL_DEPTH_TEST);
    glClearColor(t,0.1f,0.3f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
      glColor3f(1,0,0); glVertex2f(-0.5f,-0.5f);
      glColor3f(0,1,0); glVertex2f(0.5f,-0.5f);
      glColor3f(0,0,1); glVertex2f(0,0.5f);
    glEnd();
}
void GL_Resize(int w,int h){ if(g_inited) glViewport(0,0,w,h); }
void GL_Shutdown(){ g_inited=false; }