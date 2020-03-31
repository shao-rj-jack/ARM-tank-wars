#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

//Constants
#define XMAX 320
#define YMAX 240
#define BLACK 0x0000
#define WHITE 0xFFFF

//Function prototypes
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void draw_line(int x0, int y0, int x1, int y1, int color);
void draw_rect(int x, int y, int color);
void swap(int * a, int * b);
void wait_for_vsync();


//Global variables
volatile int pixel_buffer_start;

int main(void) {
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	
	/* set front pixel buffer to start of FPGA On-chip memory */
	*(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer

	/* now, swap the front/back buffers, to set the front buffer location */
	wait_for_vsync();

	/* initialize a pointer to the pixel buffer, used by drawing functions */
	pixel_buffer_start = *pixel_ctrl_ptr;

	clear_screen(); // pixel_buffer_start points to the pixel buffer

	/* set back pixel buffer to start of SDRAM memory */
	*(pixel_ctrl_ptr + 1) = 0xC0000000;
	pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

	//initialize random numbers
	srand(time(0));

	//Rectangles are 5x5px. There are 8 of them with 2 position coords
	int rects[8][2] = {{((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2},
					   {((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2},
					   {((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2},
					   {((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2},
					   {((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2},
					   {((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2},
					   {((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2},
					   {((double) rand() / RAND_MAX) * (XMAX - 5) + 2, ((double) rand() / RAND_MAX) * (YMAX - 5) + 2}};

	//Rectangle velocities
	int velos[8][2] = {{((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2},
					   {((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2},
					   {((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2},
					   {((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2},
					   {((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2},
					   {((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2},
					   {((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2},
					   {((double) rand() / RAND_MAX) * 2, ((double) rand() / RAND_MAX) * 2}};

	//velocities are now 0 or 1
	//correct 0 to -1
	for(int rect = 0; rect < 8; ++rect) {
		if(velos[rect][0] == 0) velos[rect][0] = -1;
		if(velos[rect][1] == 0) velos[rect][1] = -1;
	}

	// Main animation loop
	while(true) {
		/* Erase any boxes and lines that were drawn in the last iteration */
		clear_screen();

		//Draw lines
		draw_line(rects[0][0], rects[0][1], rects[1][0], rects[1][1], WHITE);
		draw_line(rects[1][0], rects[1][1], rects[2][0], rects[2][1], WHITE);
		draw_line(rects[2][0], rects[2][1], rects[3][0], rects[3][1], WHITE);
		draw_line(rects[3][0], rects[3][1], rects[4][0], rects[4][1], WHITE);
		draw_line(rects[4][0], rects[4][1], rects[5][0], rects[5][1], WHITE);
		draw_line(rects[5][0], rects[5][1], rects[6][0], rects[6][1], WHITE);
		draw_line(rects[6][0], rects[6][1], rects[7][0], rects[7][1], WHITE);
		draw_line(rects[7][0], rects[7][1], rects[0][0], rects[0][1], WHITE);

		for(int rect = 0; rect < 8; ++rect) {
			int x = rects[rect][0];
			int y = rects[rect][1];

			//Draw each rectangle
			draw_rect(x, y, WHITE);

			//check velocities. rectangles are 5x5px so account for that
			if(x == 2) velos[rect][0] = 1;
			else if(x == XMAX - 3) velos[rect][0] = -1;

			if(y == 2) velos[rect][1] = 1;
			else if(y == YMAX - 3) velos[rect][1] = -1;

			//move rectangles
			rects[rect][0] += velos[rect][0];
			rects[rect][1] += velos[rect][1];
		}

		//delay
		wait_for_vsync();
		pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
	}

	
}

void plot_pixel(int x, int y, short int line_color) {
	*(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void clear_screen() {
	for(int x = 0; x < XMAX; ++x) {
		for(int y = 0; y < YMAX; ++y) {
			plot_pixel(x, y, BLACK);
		}
	}
}

void draw_line(int x0, int y0, int x1, int y1, int color) {
	
	bool is_steep = abs(y1 - y0) > abs(x1 - x0);
	
	if(is_steep) {
		swap(&x0, &y0);
		swap(&x1, &y1);
	}

	if(x0 > x1) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}

	int deltax = x1 - x0;
	int deltay = abs(y1 - y0);
	int error = -(deltax / 2);
	int y_step;
	int y = y0;

	if(y0 < y1) y_step = 1;
	else y_step = -1;

	for(int x = x0; x <= x1; ++x) {
		if(is_steep) {
			plot_pixel(y, x, color);
		} else {
			plot_pixel(x, y, color);
		}

		error = error + deltay;
		if(error >= 0) {
			y = y + y_step;
			error = error - deltax;
		}
	}
}

void draw_rect(int x, int y, int color) {
	for(int i = x - 2; i <= x + 2; ++i) {
		for(int j = y - 2; j <= y + 2; ++j) {
			plot_pixel(i, j, color);
		}
	}
}

void swap(int * a, int * b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}

void wait_for_vsync() {
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;

	//write 1 to pixel buffer
	*pixel_ctrl_ptr = 1;
	while(0x00000001 & *(pixel_ctrl_ptr + 3)) {/* wait until S is 0*/}
}