/* KallistiOS ##version##

   nehe02.c
   (c)2001 Benoit Miller

   Parts (c)2000 Jeff Molofee
*/

#include <kos.h>
#include <GL/gl.h>
#include <GL/glu.h>

/* The simplest KGL example ever!

   Essentially the same thing as NeHe's lesson02 code.
   To learn more, go to http://nehe.gamedev.net/.
*/

void draw_gl(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	glTranslatef(-1.5f,0.0f,-6.0f);

	glBegin(GL_TRIANGLES);
		glVertex3f( 0.0f, 1.0f, 0.0f);
		glVertex3f(-1.0f,-1.0f, 0.0f);
		glVertex3f( 1.0f,-1.0f, 0.0f);
	glEnd();

	glTranslatef(3.0f,0.0f,0.0f);

	glBegin(GL_QUADS);
		glVertex3f(-1.0f, 1.0f, 0.0f);
		glVertex3f( 1.0f, 1.0f, 0.0f);
		glVertex3f( 1.0f,-1.0f, 0.0f);
		glVertex3f(-1.0f,-1.0f, 0.0f);
	glEnd();
}

pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 16 */
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },

        /* Vertex buffer size 512K */
        512*1024
};

int main(int argc, char **argv) {
	maple_device_t *cont;
	cont_state_t *state;

	/* Get basic stuff initialized */
        pvr_init(&params);

	printf("nehe02 beginning\n");
	glKosInit();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,640.0f/480.0f,0.1f,100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	while(1) {
		cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

		/* Check key status */
		state = (cont_state_t *)maple_dev_status(cont);
		if (!state) {
			printf("Error reading controller\n");
			break;
		}
		if (state->buttons & CONT_START)
			break;

		/* Begin frame */
		glKosBeginFrame();

		/* Draw the "scene" */
		draw_gl();

		/* Finish the frame */
		glKosFinishFrame();
	}

	return 0;
}


