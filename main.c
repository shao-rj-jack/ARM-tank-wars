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
void draw_circle(int x, int y, int color, int radius);
void draw_ground();
void draw_player(int x, int y, int player, int current_turn, int angle);
void swap(int * a, int * b);
void wait_for_vsync();
//temp
void HEX_PS2(char b1, char b2, char b3);

// Global variables
volatile int pixel_buffer_start;

int main(void) {
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	volatile int * PS2_ptr = 		(int *)0xFF200100;
	
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

	//set up keyboard
	int PS2_data, RVALID;

	//last 3 bytes send from the ps2 keyboard
	//byte3 is the most recent and byte1 is the oldest
	char byte1 = 0, byte2 = 0, byte3 = 0;

	// PS/2 mouse needs to be reset (must be already plugged in)
	*(PS2_ptr) = 0xFF; // reset

	// Main animation loop
	while(true) {
		/* Erase any boxes and lines that were drawn in the last iteration */
		clear_screen();

		//keyboard
		PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
		RVALID = PS2_data & 0x8000; // extract the RVALID field

		if (RVALID) {
			// shift the most recent data
			byte1 = byte2;
			byte2 = byte3;
			byte3 = PS2_data & 0xFF;

			HEX_PS2(byte1, byte2, byte3); //temp display of recent bytes

			if ((byte2 == (char)0xAA) && (byte3 == (char)0x00)) {
				// mouse inserted; initialize sending of data
				*(PS2_ptr) = 0xF4;
			}
			if(byte3 == (char)0x74) {
				//move right
			}
		}

		//Do calculations

		//Draw stuff

		//delay
		wait_for_vsync();
		pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
	}

	
}


//Plots a single pixel
void plot_pixel(int x, int y, short int line_color) {
	if(x < 0 || x >= XMAX || y < 0 || y >= YMAX) return;
	*(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


//Clears the screen to black
void clear_screen() {
	for(int x = 0; x < XMAX; ++x) {
		for(int y = 0; y < YMAX; ++y) {
			plot_pixel(x, y, BLACK);
		}
	}
}


//Draws a line from (x0, y0) to (x1, y1)
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


//Error helper function
int calc_error(int x, int y, int r) {
	return ((x*x + y*y - r*r + 1 + (y << 1)) << 1) + (1 - (x << 1));
}


//Draws a filled circle at (x, y) with radius r
//https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
void draw_circle(int _x, int _y, int color, int r) {
	int x = r;
	int y = 0;

	//Starting positions
	draw_line(x + _x, y + _y, x + _x, -y + _y, color);
	draw_line(y + _y, x + _x, y + _y, -x + _x, color);
	draw_line(-x + _x, y + _y, -x + _x, -y + _y, color);
	draw_line(-y + _y, x + _x, -y + _y, -x + _x, color);

	//Draw until circle slope switches
	while(x >= y) {
		if(calc_error(x, y, r) > 0) x--;
		y++;
		draw_line(x + _x, y + _y, x + _x, -y + _y, color);
		draw_line(y + _y, x + _x, y + _y, -x + _x, color);
		draw_line(-x + _x, y + _y, -x + _x, -y + _y, color);
		draw_line(-y + _y, x + _x, -y + _y, -x + _x, color);
	}
}


//Draws a rectangle at (x,y) with radius r
void draw_rect(int x, int y, int color, int radius) {
	for(int i = x - radius; i <= x + radius; ++i) {
		for(int j = y - radius; j <= y + radius; ++j) {
			plot_pixel(i, j, color);
		}
	}
}


//Draws the playing field
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


//Draws the specified player at indicated position
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


//Swaps two numbers
void swap(int * a, int * b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}


//Waits for the screen to advance one frame
void wait_for_vsync() {
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;

	//write 1 to pixel buffer
	*pixel_ctrl_ptr = 1;
	while(0x00000001 & *(pixel_ctrl_ptr + 3)) {/* wait until S is 0*/}
}

/****************************************************************************************
* (Temp) Subroutine to show a string of HEX data on the HEX displays
****************************************************************************************/
void HEX_PS2(char b1, char b2, char b3) {
	volatile int * HEX3_HEX0_ptr = (int *)0xFF200020;
	volatile int * HEX5_HEX4_ptr = (int *)0xFF200030;
	
	/* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
	* a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F
	*/

	unsigned char seven_seg_decode_table[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
											  0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
	unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int shift_buffer, nibble;
	unsigned char code;
	int i;

	shift_buffer = (b1 << 16) | (b2 << 8) | b3;

	for (i = 0; i < 6; ++i) {
		nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
		code = seven_seg_decode_table[nibble];
		hex_segs[i] = code;
		shift_buffer = shift_buffer >> 4;
	}

	/* drive the hex displays */
	*(HEX3_HEX0_ptr) = *(int *)(hex_segs);
	*(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
}