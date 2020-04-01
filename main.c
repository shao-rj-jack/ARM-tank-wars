#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

//Color Constants
#define XMAX 320
#define YMAX 240
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F

//Game State Constants
#define turn_control 0 // indicates if player is controlling his avatar ...
#define turn_wait 1 // or waiting for animation
#define P1 2 // indicates both player and ...
#define P2 3 // current turn
#define move_P1 4
#define move_P2 5
#define shoot_P1 6
#define shoot_P2 7
#define game_pause 8
#define game_start 9

//Function prototypes
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void draw_line(int x0, int y0, int x1, int y1, int color);
void draw_rect(int x, int y, int color, int radius);
void draw_ground();
void draw_player(int x, int y, int player, int current_turn, int angle);
void swap(int * a, int * b);
void wait_for_vsync();


// Global variables
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

void draw_ground() {
    int y = 180;
    for(int x = 0; x < 80; ++x) {
        draw_line(x, y, x, YMAX - 1, GREEN);
    }
    int deltaY = 2;
    for(int x = 80; x < 100; ++x) {
        draw_line(x, y, x, YMAX - 1, GREEN);
        y += deltaY;
    }
    deltaY *= -1;
    for(int x = 100; x < 160; ++x) {
        draw_line(x, y, x, YMAX - 1, GREEN);
        y += deltaY;
    }
    deltaY *= -1;
    for(int x = 160; x < 220; ++x) {
        draw_line(x, y, x, YMAX - 1, GREEN);
        y += deltaY;
    }
    deltaY *= -1;
    for(int x = 220; x < 240; ++x) {
        draw_line(x, y, x, YMAX - 1, GREEN);
        y += deltaY;
    }
    for(int x = 240; x < XMAX; ++x) {
        draw_line(x, y, x, YMAX - 1, GREEN);
    }
}

void draw_player(int x, int y, int player, int current_turn, int angle) {
    int color = BLACK;
    int delta_turret = 0;
    int player_radius = 3;
    int turret_radius = 1;
    if(player == P1) {
        color = RED;
        delta_turret = 4; // turret faces right
    }
    else if(player == P2) {
        color = BLUE;
        delta_turret = -4; // turret faces left
    }

    // draws player model
    draw_rect(x, y, color, player_radius); // main body
    draw_rect(x + delta_turret, y, color, turret_radius); // turret

    // draws angle indicator if it is players turn
    if(current_turn == player) {
        int length = 30; // length of angle indicator
        int deltaX = length * cos(angle);
        if(delta_turret < 0) {
            deltaX *= -1; // changes direction based on turret direction
        }
        int deltaY = length * sin(angle) * -1;
        draw_line(x, y, x + deltaX, y + deltaY, color);
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