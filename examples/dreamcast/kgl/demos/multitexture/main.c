/* 
   KallistiOS 2.0.0
   
   main.c
   (c)2014 Josh Pearson

   2D example of using Multi-Texture with Open GL.

   Controls:
    D-pad UP = Scale image size up
    D-pad DOWN = Scale image size down
*/

#include <kos.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

/* Load a PVR texture - located in pvr-texture.c */
extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

/* Input Callback Return Values */
#define INP_RESIZE_UP   1
#define INP_RESIZE_DOWN 2

/* Simple Input Callback with a return value */
int InputCallback() {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        cont_state_t *state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return 0;

        if(state->buttons & CONT_DPAD_UP)
            return INP_RESIZE_UP;

        if(state->buttons & CONT_DPAD_DOWN)
            return INP_RESIZE_DOWN;
    }

    return 0;
}

/* Very Basic Open GL Initialization for 2D rendering */
void RenderInit() {
    glKosInit(); /* GL Will Initialize the PVR */

    glShadeModel(GL_SMOOTH);
}

/* Simple Demonstration using Multi-Texture with Open GL DC */
void RenderMultiTexturedQuadCentered(GLuint texID0, GLuint texID1, GLfloat width, GLfloat height) {
    GLfloat x1 = (vid_mode->width - width) / 2.0;
    GLfloat x2 = x1 + width;
    GLfloat y1 = (vid_mode->height - height) / 2.0;
    GLfloat y2 = y1 + height;

    /* GL_BLEND Must be disabled for Multi-Texture */
    glDisable(GL_BLEND);

    /* Activate GL_TEXTURE0, bind the base opaque texture, and for fun, enable bi-linear filtering */
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_LINEAR);

    /* Activate GL_TEXTURE1, bind the texture to blend on top, and for fun, enable bi-linear filtering */
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_LINEAR);

    /* Set blending modes to be applied to GL_TEXUTRE1 */
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glBegin(GL_QUADS);

    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.2f, 0.2f);
    glKosVertex2f(x1, y1);

    glMultiTexCoord2f(GL_TEXTURE0, 1.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.8f, 0.2f);
    glKosVertex2f(x2, y1);

    glMultiTexCoord2f(GL_TEXTURE0, 1.0f, 1.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.8f, 0.8f);
    glKosVertex2f(x2, y2);

    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 1.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.2f, 0.8f);
    glKosVertex2f(x1, y2);

    glEnd();

    /* Disable GL_TEXTURE1 */
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    /* Make sure to set glActiveTexture back to GL_TEXTURE0 when finished */
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
}

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

int main(int argc, char **argv) {
    printf("OpenGL MipMap Example (C) 2014 PH3NOM\n");

    RenderInit(); /* Initialize Open GL */

    /* Load a PVR texture to OpenGL with No MipMap */
    GLuint texID0 = glTextureLoadPVR("/rd/wp001vq.pvr", 0, 0);
    GLuint texID1 = glTextureLoadPVR("/rd/FlareWS_256.pvr", 0, 0);

    GLfloat width = 480, height = 480;

    while(1) {
        switch(InputCallback()) {
            case INP_RESIZE_UP:
                ++width;
                ++height;
                break;

            case INP_RESIZE_DOWN:
                if(width > 1 && height > 1) {
                    --width;
                    --height;
                }

                break;
        }

        RenderMultiTexturedQuadCentered(texID0, texID1, width, height);

        glutSwapBuffers();
    }

    return 0;
}
