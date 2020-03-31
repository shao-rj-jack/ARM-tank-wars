# ARM-tank-wars

a classic arcade game where two players pilot two tank avatars and shoot projectiles at each other in a destructable environment
coded in ARM assembly languange in C
to be loaded onto DE1-SoC board and used with PS/2 keyboard

## Functions that may be useful


`draw_image(Image img)`
- Can only have one file in CPUlator
- Image storage has to be in the file itself then
- Use to draw players and non-typical shapes?

`draw_circle(position pos, radius r)`
- useful for drawing impact blasts

`get_key(ps2Keyboard k)`
- going to need to get keys efficiently as they will be used a lot

Can separate game state functionality into separate functinos to organize code


## Data structures that may be needed
(Cannot use classes unfortunately but structures exist)

- Some way to store the terrain
  - 2D array of terrain?
  - line that represents surface and everything below is filled?

- Player struct with information
  - Current health
  - Bullets (With types)
  - Position

- Current state of the game
  - use an integer and defines? Ex:
```
#define P1_MOVE 0
#define P1_SHOOT 1
#define P1_TERRAIN_UPDATE 2

...

main() {
    
    ...

    while() {

        ...

        if(game_state == P1_MOVE) {
            ...
        } else if(game_state == P1_SHOOT) {
            ...
	}

	...

    }
}
```

