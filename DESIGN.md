# fooBarBaz Nugget Project Design Spec

## Components

### Player interface

The player interface is defined within the requirements spec.

### Inputs and outputs
The program takes in keystrokes from the user to represent commands. The program outputs ascii text to the terminal to show game information and status.

### Functional decomposition into modules

* server module

* grid module

* client module

### Major data structures

#### Data structures for server:

* static struct game
    * gold remaining
    * struct grid
    * set of player structs
    * MinGoldPiles
    * MaxGoldPiles

* struct grid
    * NR (number of rows)
    * NC (number of cols)
    * 2D array of struct cell

* struct cell
    * contents (empty or amount of gold)
    * known bag/set (list of all the players to whom this cell is known)
    * character associated (to be displayed)
    * is walkable boolean

* struct player
    * player name
    * player tag
    * gold obtained
    * (row, col) position in grid
    * connection for communicating with associated client
    * Set of known vertices

* array of player structs

#### Data structures for client:

* char array to display map

### Pseudo code for logic/algorithmic flow

#### Server module
Server validates input parameters goldtotal and maxplayers.
Server reads in the board and intializes the game structure.
The server listens to connections and accepts players into the game as they connect.
The server receives commands from the players, updating their positions, if valid, and global gold count.
The server sends the display of the board to the clients.
When there is no longer any gold on the map, the server sends the final scores to each of the players.
The server then closes the connections to the players.

#### Client module
Client reads and validates the path to logfile, hostname, port and name.
Client connects to the hostname and port sending a message with the player's name.
Client listens for a char array from the server representing the display of the board.
Client writes this display to the terminal window.
Client listens for key inputs by the player, which it then validates and sends to the server if it is a valid command.
If the player sends an EOF, the client exits.
The client keeps updating until the server sends an end of game command.
The client then listens for the scores of each players and displays them to the player.
The client then exits.

#### Grid module
Defines grid structure with number of rows, number of columns, and 2D array of cells as parameters.
Initializes grid cells at beginning of game.
Updates grid cells during game runtime from given player moves.

### Dataflow through modules
The server module reads in the map information from the file which it passes to the grid module for board creation.
The server module passes its grid instance and player moves to the grid module for validation and the grid is updated on success.
The server module asks the grid module if there is any gold remaining on its grid instance.
The server module communicates with the client module by sending the board and final player scores to display.
The client module sends the server information about a player when they join and the player's moves.

### Testing plan

#### Unit testing
The board, player, and server modules will have individual functions unittested by passing in sample input and validating that they perform as expected.
Some test cases that we will attempt:
- passing invalid port to server
- passing the server an unreadable map file
- validate server in general by using logs
- making sure the grid will not overlap players and gold
- have player join server and ensure both he/she and server are consistently up-to-date
- prevent player from accidentally leaving map by traveling through walls
- spamming player keyboard input to see whether it will cause unexpected player movement

#### Integration testing
The player and server modules will be playtested by humans to ensure that behavior matches that which is expected. If behavior is unusual, we will use further unit testing to determine the source of this issue.

Some functionality that we will specifically test for
- two players trying to move to the same square
- running into walls to make sure collision/barriers works
- making sure that players and gold are only seen when currently visible by the player. Board squares are seen if they're known.
