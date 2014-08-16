/* 
   KallistiOS 2.0.0

   main.c
   (c)2014 Josh Pearson

   Open GL example using Vertex Array Submission with Near Z Clipping.
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

/* Load a PVR texture - located in pvr-texture.c */
extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

GLfloat VERTEX_ARRAY[4 * 3 * 2] = { -100.0f, -10.0f, -100.0f,
                                    100.0f, -10.0f, -100.0f,
                                    -100.0f, -10.0f, 100.0f,
                                    100.0f, -10.0f, 100.0f,
                                    -100.0f, -10.0f, 300.0f,
                                    100.0f, -10.0f, 300.0f,
                                  };

GLfloat TEXCOORD_ARRAY[4 * 2 * 2] = { 0, 0,
                                      1, 0,
                                      0, 1,
                                      1, 1,
                                      0, 0,
                                      1, 0,
                                      0, 1,
                                      1, 1
                                    };

GLuint ARGB_ARRAY[4 * 2] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00,
                             0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00
                           };

static GLfloat rx = 1.0f;

/* Example using Open GL Vertex Array Submission. */
void RenderCallback(GLuint texID) {
    glEnable(GL_KOS_NEARZ_CLIPPING);

    glLoadIdentity();
    glRotatef(rx++, 0, 1, 0);

    /* Enable 2D Texturing and bind the Texture */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);

    /* Enable Vertex, Color and Texture Coord Arrays */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    /* Bind Array Data */
    glColorPointer(1, GL_UNSIGNED_INT, 0, ARGB_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, TEXCOORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, VERTEX_ARRAY);

    /* Render the Submitted Vertex Data */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);

    /* Disable Vertex, Color and Texture Coord Arrays */
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_KOS_NEARZ_CLIPPING);
}

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

int main(int argc, char **argv) {
    /* Notice we do not init the PVR here, that is handled by Open GL */
    glKosInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Load a PVR texture to OpenGL */
    GLuint texID = glTextureLoadPVR("/rd/wp001vq.pvr", 0, 0);

    while(1) {
        /* Draw the "scene" */
        RenderCallback(texID);

        /* Finish the frame - Notice there is no glKosBegin/FinshFrame */
        glutSwapBuffers();
    }

    return 0;
}

