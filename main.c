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
void draw_rect(int x, int y, int color, int radius);
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

	// Main animation loop
	while(true) {
		/* Erase any boxes and lines that were drawn in the last iteration */
		clear_screen();

		//Do calculations

		//Draw stuff
		
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

void draw_rect(int x, int y, int color, int radius) {
	for(int i = x - radius; i <= x + radius; ++i) {
		for(int j = y - radius; j <= y + radius; ++j) {
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