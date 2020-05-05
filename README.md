# ARM-tank-wars

a classic arcade game where two players pilot two tank avatars and shoot projectiles at each other in a destructable environment
coded in ARM assembly languange in C
to be loaded onto DE1-SoC board and used with PS/2 keyboard

## TODO

- animation of the explosions (circles not drawing correctly) + set ground state of site of explosion to false
- update player status (location, health if needed) after explosion goes off

## How to Play

- press SPACE to start, the program will choose a random player to have the first turn
    - players have 10 seconds to complete their turn
    - going over time will give the turn to the other player
- use the LEFT and RIGHT arrow keys to move the tank, use the UP and DOWN arrow keys to aim the turret
- press SPACE to shoot and end the turn
