#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define XMAX 320
#define YMAX 240

//Color Constants
#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F

//Game State Constants
#define P1 0 // indicates both player and ...
#define P2 1 // current turn
#define move_P1 2
#define move_P2 3
#define shoot_P1 4
#define shoot_P2 5
#define shoot_wait 6
#define game_pause 7
#define game_start 8
#define game_over 9

//Keyboard Constants
#define BREAK (char)0xF0
#define SPACEBAR 	(int)0x00290000
#define UP_ARROW 	(int)0x00E07500
#define DOWN_ARROW 	(int)0x00E07200
#define RIGHT_ARROW (int)0x00E07400
#define LEFT_ARROW 	(int)0x00E06B00
#define ESCAPE 		(int)0x00760000
#define B_SPACEBAR 	  (int)0x00F02900
#define B_UP_ARROW    (int)0x00E0F075
#define B_DOWN_ARROW  (int)0x00E0F072
#define B_RIGHT_ARROW (int)0x00E0F074
#define B_LEFT_ARROW  (int)0x00E0F06B
#define B_ESCAPE 	  (int)0x00F07600

// player data structure definition
struct Player_data {
    int pos_x;
    int pos_y;
    int angle;
    int health;
};

// bullet data structure definition
struct Bullet {
	int pos_x;
	int pos_y;
	int vel_x;
	int vel_y;
	int accel_y;
	short int color;
};

// key data structure definition
struct Pressed_keys {
    bool spacebar;
    bool up_arrow;
    bool down_arrow;
    bool right_arrow;
    bool left_arrow;
    bool escape;
};

//Function prototypes
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void draw_line(int x0, int y0, int x1, int y1, int color);
void draw_rect(int x, int y, int color, int radius);
void draw_circle(int x, int y, int color, int radius);
void init_ground();
void draw_ground();
void draw_player(int x, int y, int player, int current_turn, int angle);
void draw_score(int health_p1, int health_p2, int current_turn);
void draw_border(int player);
void draw_timer(int time);
void advance_key(char * b1, char * b2, char * b3, int PS2_data);
int read_key();
void swap(int * a, int * b);
void wait_for_vsync();
//temp
void HEX_PS2(int key);

// Global variables
volatile int pixel_buffer_start;
bool ground[XMAX][YMAX];

int main(void) {
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;

	int game_state = game_pause; // initialize game state to be paused
	int current_player;

	// initializes player data
	struct Player_data player_1;
	player_1.pos_x = 75;
	player_1.pos_y = 175;
	player_1.angle = 30;
	player_1.health = 5;

	struct Player_data player_2;
	player_2.pos_x = 245;
	player_2.pos_y = 175;
	player_2.angle = 30;
	player_2.health = 5;

	// initialize bullet
	struct Bullet bullet;
	bullet.pos_x = 0;
	bullet.pos_y = 0;
	bullet.vel_x = 0;
	bullet.vel_y = 0;
	bullet.accel_y = 5;
	bullet.color = BLACK;

	bool init_bullet = false;

	// setup timer
	int time = 10;
    volatile int * timer_ptr = (int *)0xFFFEC600;
    *timer_ptr = 200000000; // load 1 second time into
    volatile int * timer_ptr_control = (int *)0xFFFEC608;
    *timer_ptr_control = 0; // load 0 into control register
    volatile int * timer_ptr_status = (int *)0xFFFEC60C;

	//set up keyboard
	int key;

	// initialize edge capture of key data
	struct Pressed_keys keys;
	keys.spacebar = false;
	keys.up_arrow = false;
	keys.down_arrow = false;
	keys.right_arrow = false;
	keys.left_arrow = false;
	keys.escape = false;

	/* set front pixel buffer to start of FPGA On-chip memory */
	*(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer

	/* now, swap the front/back buffers, to set the front buffer location */
	wait_for_vsync();

	/* initialize a pointer to the pixel buffer, used by drawing functions */
	pixel_buffer_start = *pixel_ctrl_ptr;

	clear_screen(); // pixel_buffer_start points to the pixel buffer
	init_ground();

	/* set back pixel buffer to start of SDRAM memory */
	*(pixel_ctrl_ptr + 1) = 0xC0000000;
	pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

	// Main animation loop
	while(true) {
		/* Erase last iteration */
		draw_ground();

		//keyboard, read the entire FIFO buffer
		//Capture the rising edges of the keys
		int last_key = 0;
		while((key = read_key()) != 0) {
			if(key == SPACEBAR) keys.spacebar = true;
			else if(key == UP_ARROW) keys.up_arrow = true;
			else if(key == DOWN_ARROW) keys.down_arrow = true;
			else if(key == RIGHT_ARROW) keys.right_arrow = true;
			else if(key == LEFT_ARROW) keys.left_arrow = true;
			else if(key == ESCAPE) keys.escape = true;
			last_key = key;
		}
		HEX_PS2(last_key); //temp display of recent bytes

        if(game_state == game_pause) {
            draw_player(player_1.pos_x, player_1.pos_y, P1, 0, player_1.angle);
            draw_player(player_2.pos_x, player_2.pos_y, P2, 0, player_2.angle);
            draw_score(player_1.health, player_2.health, 0);
            draw_timer(time);

            if(keys.spacebar) { // press spacebar to start game
                game_state = game_start;
                current_player = rand() % 2; // randomly chooses starting player (2 or 3)
            }
        }
        else if(game_state == game_start) {
            // draw players
            draw_player(player_1.pos_x, player_1.pos_y, P1, current_player, player_1.angle);
            draw_player(player_2.pos_x, player_2.pos_y, P2, current_player, player_2.angle);

            game_state = current_player; // sets the turn to the current player
        }
        else if(game_state == P1 || game_state == P2) { // player initial turn
            // check for any directional input
            if(keys.up_arrow || keys.down_arrow || keys.right_arrow || keys.left_arrow) {
                game_state += 2; // change to corresponding player movement game state
            } else if(keys.spacebar) {
            	game_state += 4; // change to corresponding player shooting game state
            }
            draw_player(player_1.pos_x, player_1.pos_y, P1, current_player, player_1.angle);
            draw_player(player_2.pos_x, player_2.pos_y, P2, current_player, player_2.angle);
            draw_score(player_1.health, player_2.health, game_state);
            draw_timer(time);
        }
        else if(game_state == move_P1) {
            if(keys.left_arrow) {
                player_1.pos_x -= 2; // move left
            }
            else if(keys.right_arrow) {
                player_1.pos_x += 2; // move right
            }
            else if(keys.down_arrow) {
                player_1.angle += 2; // angle turret down

                if(player_1.angle > 30) player_1.angle = 30; //max angle is equal to line length
            }
            else if(keys.up_arrow) {
                player_1.angle -= 2; // angle turret up

                if(player_1.angle < -30) player_1.angle = -30; //max angle is equal to line length
            }

            // check for ground around the player to change y position
            while(ground[player_1.pos_x][player_1.pos_y + 2]) { // check if there is ground for the player to ascend (player radius is 3 to check 2 down)
                player_1.pos_y -= 1; // move up in screen
            }

            while(!ground[player_1.pos_x][player_1.pos_y + 3]) { // check if there is no ground beneath the player
                player_1.pos_y += 1; // move down in screen
            }

            if(keys.spacebar) { // check if shoot button was pressed
                time = 10; // reset time
                *timer_ptr_control = 0; // stop timer
                game_state = shoot_P1;
            }

            *timer_ptr_control = 3; // enable timer
            if(*timer_ptr_status == 1) { // check status register
                if(time > 0) time -= 1; // decrement time
                else if(time == 0) { // countdown is over
                    time = 10; // reset time
                    *timer_ptr_control = 0; // stop timer
                    // switch players
                    current_player = P2;
                    game_state = P2;
                }
                *timer_ptr_status = 1; // write 1 pack into status register to reset
            }

            draw_player(player_1.pos_x, player_1.pos_y, P1, current_player, player_1.angle);
            draw_player(player_2.pos_x, player_2.pos_y, P2, current_player, player_2.angle);
            draw_score(player_1.health, player_2.health, P1);
            draw_timer(time);
        }
        else if(game_state == move_P2) {
            if(keys.left_arrow) {
                player_2.pos_x -= 2; // move left
            }
            else if(keys.right_arrow) {
                player_2.pos_x += 2; // move right
            }
            else if(keys.down_arrow) {
                player_2.angle += 2; // angle turret down

                if(player_2.angle > 30) player_2.angle = 30; //max angle is equal to line length
            }
            else if(keys.up_arrow) {
                player_2.angle -= 2; // angle turret up

                if(player_2.angle < -30) player_2.angle = -30; //max angle is equal to line length
            }

            // check for ground around the player to change y position
            while(ground[player_2.pos_x][player_2.pos_y + 2]) { // check if there is ground for the player to ascend (player radius is 3 to check 2 down)
                player_2.pos_y -= 1; // move up in screen
            }

            while(!ground[player_2.pos_x][player_2.pos_y + 3]) { // check if there is no ground beneath the player
                player_2.pos_y += 1; // move down in screen
            }

            if(keys.spacebar) { // check if shoot button was pressed
                time = 10; // reset time
                *timer_ptr_control = 0; // stop timer
                game_state = shoot_P2;
            }

            *timer_ptr_control = 3; // enable timer
            if(*timer_ptr_status == 1) { // check status register
                if(time > 0) time -= 1; // decrement time
                else if(time == 0) { // countdown is over
                    time = 10; // reset time
                    *timer_ptr_control = 0; // stop timer
                    // switch players
                    current_player = P1;
                    game_state = P1;
                }
                *timer_ptr_status = 1; // write 1 pack into status register to reset
            }

            draw_player(player_1.pos_x, player_1.pos_y, P1, current_player, player_1.angle);
            draw_player(player_2.pos_x, player_2.pos_y, P2, current_player, player_2.angle);
            draw_score(player_1.health, player_2.health, game_state);
            draw_timer(time);
        }
        else if(game_state == shoot_P1) {
            // shoot projectile

            if(!init_bullet) {
            	bullet.color = RED;
	        	bullet.pos_x = player_1.pos_x;
	        	bullet.pos_y = player_1.pos_y;
	        	bullet.vel_y = -sqrt(30*30 - player_1.angle * player_1.angle) * 0.25;
	        	bullet.accel_y = 1;

	        	if(player_1.angle > 0) bullet.vel_x = player_1.angle >> 2;
	        	else if(player_1.angle == 0) bullet.vel_x = 0;
	        	else  bullet.vel_x = -player_1.angle >> 2;

	        	init_bullet = true;
            } else {
            	bullet.pos_x += bullet.vel_x;
            	bullet.pos_y += bullet.vel_y;
            	bullet.vel_y += bullet.accel_y;
        	}

        	if(bullet.pos_x < 0 || bullet.pos_x >= XMAX || bullet.pos_y >= YMAX) {
        		init_bullet = false;
        		game_state = P1;
        	}
        	else if(bullet.pos_y >= 0) {
        		if(ground[bullet.pos_x][bullet.pos_y]) {
	            	init_bullet = false;
	            	game_state = shoot_wait;
	            }
        	}

        	draw_player(player_1.pos_x, player_1.pos_y, P1, current_player, player_1.angle);
            draw_player(player_2.pos_x, player_2.pos_y, P2, current_player, player_2.angle);
            draw_score(player_1.health, player_2.health, game_state);
            draw_timer(time);
            draw_rect(bullet.pos_x, bullet.pos_y, RED, 1);
        }
        else if(game_state == shoot_P2) {
            // shoot projectile
        	if(!init_bullet) {
            	bullet.color = BLUE;
	        	bullet.pos_x = player_2.pos_x;
	        	bullet.pos_y = player_2.pos_y;
	        	bullet.vel_y = -sqrt(30*30 - player_2.angle * player_2.angle) * 0.25;
	        	bullet.accel_y = 1;

	        	if(player_2.angle > 0) bullet.vel_x = -player_2.angle >> 2;
	        	else if(player_2.angle == 0) bullet.vel_x = 0;
	        	else  bullet.vel_x = player_2.angle >> 2;

	        	init_bullet = true;
            } else {
            	bullet.pos_x += bullet.vel_x;
            	bullet.pos_y += bullet.vel_y;
            	bullet.vel_y += bullet.accel_y;
        	}

            if(bullet.pos_x < 0 || bullet.pos_x >= XMAX || bullet.pos_y >= YMAX) {
        		init_bullet = false;
        		game_state = P1;
        	}
        	else if(bullet.pos_y >= 0) {
        		if(ground[bullet.pos_x][bullet.pos_y]) {
	            	init_bullet = false;
	            	game_state = shoot_wait;
	            }
        	}

        	draw_player(player_1.pos_x, player_1.pos_y, P1, current_player, player_1.angle);
            draw_player(player_2.pos_x, player_2.pos_y, P2, current_player, player_2.angle);
            draw_score(player_1.health, player_2.health, game_state);
            draw_timer(time);
            draw_rect(bullet.pos_x, bullet.pos_y, BLUE, 1);
        }
        else if(game_state == shoot_wait) {
            // plot explosion at current location of bullet
            draw_circle(bullet.pos_x, bullet.pos_y, BLACK, 10);
        }
        else if(game_state == game_over) {
            // checks which player won
            // displays congrats

            if(keys.spacebar) { // press spacebar to reset game
                game_state = game_pause;
            }
        }

        //reset key values
        keys.spacebar = false;
        keys.up_arrow = false;
		keys.down_arrow = false;
		keys.right_arrow = false;
		keys.left_arrow = false;
		keys.escape = false;

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
    draw_line(y + _x, x + _y, y + _x, -x + _y, color);
    draw_line(-x + _x, y + _y, -x + _x, -y + _y, color);
    draw_line(-y + _x, x + _y, -y + _x, -x + _y, color);

    //Draw until circle slope switches
    while(x >= y) {
        if(calc_error(x, y, r) > 0) x--;
        y++;
        draw_line(x + _x, y + _y, x + _x, -y + _y, color);
        draw_line(y + _x, x + _y, y + _x, -x + _y, color);
        draw_line(-x + _x, y + _y, -x + _x, -y + _y, color);
        draw_line(-y + _x, x + _y, -y + _x, -x + _y, color);
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


//Initialize the playing field with a piecewise function
void init_ground() {
	int y = 180;
    for(int x = 0; x < 80; ++x) {
        for(int j = 0; j < y; ++j) {
        	ground[x][j] = false;
        }
        for(int j = y; j < YMAX; ++j) {
        	ground[x][j] = true;
        }
    }
    int deltaY = 2;
    for(int x = 80; x < 100; ++x) {
        for(int j = 0; j < y; ++j) {
        	ground[x][j] = false;
        }
        for(int j = y; j < YMAX; ++j) {
        	ground[x][j] = true;
        }
        y += deltaY;
    }
    deltaY *= -1;
    for(int x = 100; x < 160; ++x) {
        for(int j = 0; j < y; ++j) {
        	ground[x][j] = false;
        }
        for(int j = y; j < YMAX; ++j) {
        	ground[x][j] = true;
        }
        y += deltaY;
    }
    deltaY *= -1;
    for(int x = 160; x < 220; ++x) {
        for(int j = 0; j < y; ++j) {
        	ground[x][j] = false;
        }
        for(int j = y; j < YMAX; ++j) {
        	ground[x][j] = true;
        }
        y += deltaY;
    }
    deltaY *= -1;
    for(int x = 220; x < 240; ++x) {
        for(int j = 0; j < y; ++j) {
        	ground[x][j] = false;
        }
        for(int j = y; j < YMAX; ++j) {
        	ground[x][j] = true;
        }
        y += deltaY;
    }
    for(int x = 240; x < XMAX; ++x) {
        for(int j = 0; j < y; ++j) {
        	ground[x][j] = false;
        }
        for(int j = y; j < YMAX; ++j) {
        	ground[x][j] = true;
        }
    }
}


//Draws the playing field
void draw_ground() {
    for(int x = 0; x < XMAX; ++x) {
    	for(int y = 0; y < YMAX; ++y) {
    		if(ground[x][y]) plot_pixel(x, y, GREEN);
    		else plot_pixel(x, y, BLACK);
    	}
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
    if(current_turn == 0) { // game hasnt started, no angle indicators
        return;
    }
    else if(current_turn == player) {
        int length = 30; // length of angle indicator
        int deltaX = angle;
        int deltaY = -sqrt(length * length - deltaX * deltaX);

        if(delta_turret < 0) {
            deltaX *= -1; // changes direction based on turret direction
        }

        draw_line(x, y, x + deltaX, y + deltaY, color);
    }
}

// draws the score board at the top of the screen
void draw_score(int health_p1, int health_p2, int current_turn) {
    // border width is 5
    int space = 8; // space between each health bar
    int width = 14; // width of each health bar
    int height = 30; // height of each health bar

    int start_x = 5 + space; // starting x position of first health bar
    int start_y = 5 + space;
    int color = RED;
    for(int i = 0; i < health_p1; ++i) {
        for(int j = start_x + (width + space) * i; j < start_x + (width + space) * i + width; ++j) {
            for(int k = start_y; k < start_y + height; ++k) {
                plot_pixel(j, k, color);
            }
        }
    }

    start_x = 192 + 5 + space; // starting x position of first player 1 health bar
    color = BLUE;
    for(int i = 0; i < health_p2; ++i) {
        for(int j = start_x + (width + space) * i; j < start_x + (width + space) * i + width; ++j) {
            for(int k = start_y; k < start_y + height; ++k) {
                plot_pixel(j, k, color);
            }
        }
    }

    // draw border(s)
    if(current_turn == 0) {
        draw_border(P1);
        draw_border(P2);
    }
    else {
        draw_border(current_turn);
    }
}

// function that draws scoreboard borders
void draw_border(int player) {
    int color, x_start;
    int width = 128;
    int height = 60;
    int border_width = 5;
    if(player % 2 == 0) {
        color = RED;
        x_start = 0;
    }
    else if(player % 2 == 1) {
        color = BLUE;
        x_start = 192;
    }

    for(int i = x_start; i < x_start + width; ++i) {
        for(int j = 0; j < height; ++j) {
            if(i < x_start + border_width || i >= x_start + width - border_width) {
                plot_pixel(i, j, color);
            }
            else {
                if(j < border_width || j >= height - border_width) {
                    plot_pixel(i, j, color);
                }
            }
        }
    }
}

// draws the timer at the top middle of the screen
void draw_timer(int time) {
    int height = 6; // height of each individual segment of timer
    int color = WHITE;
    int y_start = 60;
    int x_start = 128;
    int x_end = 192;

    for(int i = x_start; i < x_end; ++i) {
        for(int j = y_start; j > y_start - time * height; --j) {
            plot_pixel(i, j, color);
        }
    }
}

//Advances the keyboard data. b3 is the most recent data
// byetes [b1 b2 b3] form a PS/2 keyboard code
void advance_key(char * b1, char * b2, char * b3, int PS2_data) {
	int RVALID = PS2_data & 0x8000; // extract the RVALID field

	if (RVALID) {
		// shift the most recent data
		*b1 = *b2;
		*b2 = *b3;
		*b3 = PS2_data & 0xFF;
	}
}

//Reads one key from the PS/2 keyboard
int read_key() {
	volatile int * PS2_ptr = (int *)0xFF200100;
	char byte1 = 0, byte2 = 0, byte3 = 0;
	int key = 0;

	int PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
	advance_key(&byte1, &byte2, &byte3, PS2_data);

	if(byte3 == (char)0xE0) { //there are more bytes of data
		PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
		advance_key(&byte1, &byte2, &byte3, PS2_data);

		if(byte3 == BREAK) { //there is one more byte of data
			PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
			advance_key(&byte1, &byte2, &byte3, PS2_data);
		} else {
			//Shift data to the left
			byte1 = byte2;
			byte2 = byte3;
			byte3 = 0;
		}
	} else if(byte3 == BREAK) { //there is one more byte of data
		PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
		advance_key(&byte1, &byte2, &byte3, PS2_data);

		//Shift data to the left
		byte1 = byte2;
		byte2 = byte3;
		byte3 = 0;
	} else {
		//Shift data to the left
		byte1 = byte3;
		byte2 = 0;
		byte3 = 0;
	}

	key = (byte1 << 16) | (byte2 << 8) | byte3;
	return key;
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
void HEX_PS2(int key) {
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

	shift_buffer = key;

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