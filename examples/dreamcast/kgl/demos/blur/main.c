/* 
   KallistiOS 2.0.0

   main.c
   (c)2014 Josh Pearson
   (c)2001 Benoit Miller
   (c)2000 Tom Stanis/Jeff Molofee

   Radial Blur example, loosely based on nehe08.
*/

#include <kos.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

/* 
   OpenGL example to demonstrate blending and lighting.
   Furthermore, this example demonstrates how to use render-to-texture.
   In this example, we use glutCopyBufferToTexture(...) to render
   submitted vertex data to a texture, without flushing the vertex
   data in main ram.  This makes using the "Radial Blur" effect
   quite efficient in terms of CPU usage.  GPU usage, however, is a
   different story altogehter.  The PowerVR really struggles blending
   overlapped polygons, as we can see as we increase the number of
   polygons to be rendered into the translucent list by the radial
   blur effect.

   Other than that, essentially the same thing as NeHe's lesson08 code.
   To learn more, go to http://nehe.gamedev.net/.

   Radial blur effect was originally demonstrated here:
   http://nehe.gamedev.net/tutorial/radial_blur__rendering_to_a_texture/18004/

   DPAD controls the cube rotation, button A & B control the depth
   of the cube, button X enables radial blur, and button Y disables
   radial blur.  Left Trigger reduces number of times to render the radial blur
   effect, Right Trigger Increases.
*/

static GLfloat xrot;        /* X Rotation */
static GLfloat yrot;        /* Y Rotation */
static GLfloat xspeed;      /* X Rotation Speed */
static GLfloat yspeed;      /* Y Rotation Speed */
static GLfloat z = -5.0f;   /* Depth Into The Screen */

static pvr_ptr_t *RENDER_TEXTURE = NULL;   /* PVR Memory Pointer for Render-To-Texture result */
static GLuint      RENDER_TEXTURE_ID;      /* Render-To-Texture GL Texture ID */
static long unsigned int RENDER_TEXTURE_W; /* Render-To-Texture width */
static long unsigned int RENDER_TEXTURE_H; /* Render-To-Texture height */

extern GLuint glTextureLoadPVR(char *fname, unsigned char isMipMapped, unsigned char glMipMap);

void InitRenderTexture(long unsigned int width, long unsigned int height) {
    /* 1.) Allcoate PVR Texture Memory for Render-To-Texture */
    RENDER_TEXTURE_W = width;
    RENDER_TEXTURE_H = height;
    RENDER_TEXTURE = pvr_mem_malloc(RENDER_TEXTURE_W * RENDER_TEXTURE_H * 2);

    /* 2.) Generate a texture for Open GL, and bind that texxture */
    glGenTextures(1, &RENDER_TEXTURE_ID);
    glBindTexture(GL_TEXTURE_2D, RENDER_TEXTURE_ID);

    /* 3.) Use glKosTexImage2D() to bind the texture address for the Render-To-Texture */
    glKosTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                    RENDER_TEXTURE_W, RENDER_TEXTURE_H, 0,
                    PVR_TXRFMT_NONTWIDDLED, PVR_TXRFMT_RGB565, RENDER_TEXTURE);

    /* 4.) Enable GL_LINEAR Texture Filter to enable the PVR's bilinear filtering */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_LINEAR);
}

#define DEBUG_NULL_DC 1

void RenderBlurEffect(int times, float inc, long unsigned int width, long unsigned int height,
                      GLuint texID)

{
    float spost = 0.0f;                // Starting Texture Coordinate Offset
    float alphainc = 0.9f / times;     // Fade Speed For Alpha Blending
    float alpha = 0.2f;                // Starting Alpha Value
    float U, V;

#ifdef DEBUG_NULL_DC                   // Null DC does not accuratley handle render-to-texture u/v coords

    if(width > (float)vid_mode->width)
        U = 1;
    else
        U = width / (float)vid_mode->width;

    if(height > (float)vid_mode->height)
        V = 1;
    else
        V = height / (float)vid_mode->height;

#else

    if(width > (float)vid_mode->width)
        U = (float)vid_mode->width / width;
    else
        U = 1;

    if(height > (float)vid_mode->height)
        V = (float)vid_mode->height / height;
    else
        V = 1;

#endif
    float W = (float)vid_mode->width;
    float H = (float)vid_mode->height;

    glDisable(GL_LIGHTING);                 // Disable GL Lighting
    glDisable(GL_DEPTH_TEST);               // Disable Depth Testing
    glEnable(GL_TEXTURE_2D);                // Enable 2D Texture Mapping
    glEnable(GL_BLEND);                     // Enable Blending

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);      // Set Blending Mode

    glBindTexture(GL_TEXTURE_2D, texID);  // Bind The Rendered Texture

    alphainc = alpha / times;                         // alphainc=0.2f / Times To Render Blur

    glBegin(GL_QUADS);

    while(times--) {                            // Number Of Times To Render Blur
        glColor4f(1.0f, 1.0f, 1.0f, alpha);     // Set The Alpha Value (Starts At 0.2)

        glTexCoord2f(0 + spost, 0 + spost);
        glVertex2f(0, 0);

        glTexCoord2f(U - spost, 0 + spost);
        glVertex2f(W, 0);

        glTexCoord2f(U - spost, V - spost);
        glVertex2f(W, H);

        glTexCoord2f(0 + spost, V - spost);
        glVertex2f(0, H);

        spost += inc;                   // Gradually Increase spost (Zooming Closer To Texture Center)

        alpha = alpha - alphainc;       // Gradually Decrease alpha (Gradually Fading Image Out)
    }

    glEnd();

    glDisable(GL_TEXTURE_2D);                   // Disable 2D Texture Mapping
    glDisable(GL_BLEND);                        // Disable Blending
    glEnable(GL_DEPTH_TEST);                    // Enable Depth Testing
    glEnable(GL_LIGHTING);                      // Enable Lighting
}

void draw_gl(GLuint texID) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, z);

    glRotatef(xrot, 1.0f, 0.0f, 0.0f);
    glRotatef(yrot, 0.0f, 1.0f, 0.0f);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, texID);

    glBegin(GL_QUADS);
    /* Front Face */
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    /* Back Face */
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    /* Top Face */
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    /* Bottom Face */
    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    /* Right face */
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    /* Left Face */
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    xrot += xspeed;
    yrot += yspeed;
}

#define ENABLE_RADIAL_BLUR 2
#define DISABLE_RADIAL_BLUR 3
#define INCREASE_RADIAL_BLUR 4
#define DECREASE_RADIAL_BLUR 5

int InputCallback() {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    /* Check key status */
    state = (cont_state_t *)maple_dev_status(cont);

    if(!state) {
        printf("Error reading controller\n");
        return -1;
    }

    if(state->buttons & CONT_START)
        return 0;

    if(state->buttons & CONT_A)
        z -= 0.02f;

    if(state->buttons & CONT_B)
        z += 0.02f;

    if((state->buttons & CONT_X)) {
        return ENABLE_RADIAL_BLUR;
    }

    if((state->buttons & CONT_Y)) {
        return DISABLE_RADIAL_BLUR;
    }

    if(state->buttons & CONT_DPAD_UP)
        xspeed -= 0.01f;

    if(state->buttons & CONT_DPAD_DOWN)
        xspeed += 0.01f;

    if(state->buttons & CONT_DPAD_LEFT)
        yspeed -= 0.01f;

    if(state->buttons & CONT_DPAD_RIGHT)
        yspeed += 0.01f;

    if(state->ltrig)
        return DECREASE_RADIAL_BLUR;

    if(state->rtrig)
        return INCREASE_RADIAL_BLUR;

    return 1;
}

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);


int main(int argc, char **argv) {
    printf("glRadialBlur beginning\n");

    /* Get basic stuff initialized */
    glKosInit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, vid_mode->width / vid_mode->height, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /* Enable Lighting and GL_LIGHT0 */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* Set up the textures */
    GLuint tex0 = glTextureLoadPVR("/rd/glass.pvr", 0, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_FILTER_BILINEAR);

    /* Set the Render Texture and Render-To-Texture Viewport Dimensions - Must be Power of two */
    InitRenderTexture(1024, 512);

    GLubyte enable_radial = 0, radial_iterations = 1;

    while(1) {
        /* Draw the GL "scene" */
        draw_gl(tex0);

        if(enable_radial) {
            glutCopyBufferToTexture(RENDER_TEXTURE, &RENDER_TEXTURE_W, &RENDER_TEXTURE_H);

            RenderBlurEffect(radial_iterations, 0.02f, RENDER_TEXTURE_W, RENDER_TEXTURE_H, RENDER_TEXTURE_ID);
        }

        glutSwapBuffers(); /* Finish the GL "scene" */

        /* Very simple callback to handle user input based on static global vars */
        switch(InputCallback()) {

            case ENABLE_RADIAL_BLUR:
                enable_radial = 1;
                break;

            case DISABLE_RADIAL_BLUR:
                enable_radial = 0;
                break;

            case INCREASE_RADIAL_BLUR:
                if(radial_iterations < 18)
                    ++radial_iterations;

                printf("radial iterations: %i\n", radial_iterations);
                break;

            case DECREASE_RADIAL_BLUR:
                if(radial_iterations > 0)
                    --radial_iterations;

                printf("radial iterations: %i\n", radial_iterations);
                break;
        }
    }

    return 0;
}

