/* 
   KallistiOS 2.0.0

   main.c
   (c)2014 Josh Pearson

   Open GL Multi-Texture example using Vertex Array Submission.
*/

#include <kos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

/* Load a PVR texture - located in pvr-texture.c */
extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

GLfloat VERTEX_ARRAY[4 * 3] = { -1.0f,  1.0f, 0.0f,
                                1.0f,  1.0f, 0.0f,
                                1.0f, -1.0f, 0.0f,
                                -1.0f, -1.0f, 0.0f
                              };

GLfloat TEXCOORD_ARRAY[4 * 2] = { 0, 0,
                                  1, 0,
                                  1, 1,
                                  0, 1
                                };

GLuint ARGB_ARRAY[4 * 1] = { 0xFFFF0000, 0xFF0000FF, 0xFF00FF00, 0xFFFFFF00 };


/* Multi-Texture Example using Open GL Vertex Buffer Submission.
   glClientActiveTexture() must be used for Arrays, instead of glActiveTexture().
   Each texture must recieve its own set of UV Coordinates.
   Currently, Multi-Texture is Supportd only if GL_KOS_NEARZ_CLIPPING is disabled. */
void RenderCallback(GLuint texID0, GLuint texID1) {
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);

    /* GL_BLEND Must be disabled for Multi-Texture */
    glDisable(GL_BLEND);

    /* Enable Vertex, Color and Texture Coord Arrays */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    /* Activate GL_TEXTURE0, bind the base opaque texture, and for fun, enable bi-linear filtering */
    glClientActiveTexture(GL_TEXTURE0); /* glClientActiveTexture(...) For use with Multi-Texture Arrays */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_LINEAR);
    glTexCoordPointer(2, GL_FLOAT, 0, TEXCOORD_ARRAY); /* Bind TexCoord Array for GL_TEXTURE0 */

    /* Activate GL_TEXTURE1, bind the texture to blend on top, and for fun, enable bi-linear filtering */
    glClientActiveTexture(GL_TEXTURE1); /* glClientActiveTexture(...) For use with Multi-Texture Arrays */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_LINEAR);
    glTexCoordPointer(2, GL_FLOAT, 0, TEXCOORD_ARRAY); /* Bind TexCoord Array for GL_TEXTURE1 */

    /* Set blending modes to be applied to GL_TEXUTRE1 */
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    /* Bind the Color Array */
    glColorPointer(1, GL_UNSIGNED_INT, 0, ARGB_ARRAY);

    /* Bind the Vertex Array */
    glVertexPointer(3, GL_FLOAT, 0, VERTEX_ARRAY);
    glDrawArrays(GL_QUADS, 0, 4);

    /* Disable GL_TEXTURE1 */
    glClientActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    /* Make sure to set glActiveTexture back to GL_TEXTURE0 when finished */
    glClientActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    /* Disable Vertex, Color and Texture Coord Arrays */
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
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

    /* Load two PVR textures to OpenGL */
    GLuint texID0 = glTextureLoadPVR("/rd/wp001vq.pvr", 0, 0);
    GLuint texID1 = glTextureLoadPVR("/rd/FlareWS_256.pvr", 0, 0);

    while(1) {
        /* Draw the "scene" */
        RenderCallback(texID0, texID1);

        /* Finish the frame - Notice there is no glKosBegin/FinshFrame */
        glutSwapBuffers();
    }

    return 0;
}

