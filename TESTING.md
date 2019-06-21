# fooBarBaz Nugget Project Testing

## General strategy

Unlike professor's approach, we first wrote the client module and server module, and made sure they can communicate with each other. Once we were done with that, we knew exactly what was needed for the grid module. So, we catered to all those functions, while testing all edge cases, while keeping in mind the assumptions that have been mentioned in the requirement spec. We did unit testing of the grid module by creating test maps, and making sure they act as intended. We made sure we use assertp every time we allocate memory to any object. Everytime we allocate something, before writing other code, we first wrote code to free it so that we know there are no memory leaks. We ran valgrind on both the client and server in many cases and detected no memory leaks caused as a result of our code. 
Further, we were able to make use of the server and player compiled results provided by the professor. We ran simultaneous instances of our server and the provided server to compare the similarities and differences. Whenver we found a difference, we changed our implementation to match the provided one. 

## Server testing

### Commandline

* Passing the wrong number of arguments to `./server` results in a usage error message, telling client it expects either 1 or 2 arguments. 
	* Ex. `./server`
	* Incorrect number of arguments! expected 1 or 2, but got 0
usage: ./server mapfile [seed]
	* Ex. `./server maps/main.txt 1 thisShouldNotBeHere`
	* Incorrect number of arguments! expected 1 or 2, but got 3
usage: ./server mapfile [seed]

* Entering the path to a non-existent or unreadable map file prompts the client accordingly.
	* Ex. `./server notActualMap`
	* File does not exist or is not readable!

* Passing an invalid integer as the (optional) also results in usage error.
	* Ex. `./server maps/main.txt -1`
	* Second argument is not a valid integer! Seed must be nonengative integer
usage: ./server mapfile [seed]

### Supports one player

<p> We test our server with the given binary for a properly functioning player module (not ours, which we will test below!) </p>

* Server can accept player.
	* Ex. `./player 2>player.log localhost 35174 bob`
	* SERVER LOG: message\_loop: content:
PLAY bob
	* CLIENT LOG: message\_loop: 1 lines:
OK B
	* Player is shown a map on which he/she is identified as the character '@', over which the cursor hovers.

* Server can make player move in map upon recipt of a 'hjklyubn' key (or their capital equivalents) for movement in their respective directions.
	* Ex. 'l' key press
	* SERVER LOG: message\_loop: content: KEY l
	* CLIENT LOG: message\_loop: 22 lines:
DISPLAY
	* Player moves one unit to the right.

* Server can kick player out of game in certain occaisions.
	* Ex. Player sends 'Q'
	* SERVER LOG: message\_loop: content: KEY Q
	* CLIENT LOG: message\_loop: 1 lines:
QUIT
	* Program exits for player.
	* Ex. Player collects all gold on map.
	* CLIENT LOG: message\_loop: 5 lines: GAMEOVER 
        * bob A 300 
	* Player sees gameover display and exits program.

* Server sends gold-received message to player upon picking up nuggets.
	* Ex. Player moves to pile of nuggets.
	* SERVER LOG: message\_loop: content:
KEY k
	* CLIENT LOG: message\_loop: content:
GOLD 19 19 281
	* Player receives notification on how much gold he/she has, how much gold is remaining on map, and amount of gold in pile.

* Server sends invalid-key message to player when appropriate.
	* Ex. Player sends ';'
	* SERVER LOG: message\_loop: content: KEY ;
	* CLIENT LOG: message\_loop: content:
NO Invalid Key
	* Player receives notification on status line stating that invalid key was pressed and game proceeds.

### Supports multiple players

<p> Again we test our server with the given binary for a properly functioning player module. </p>

* Server can allow for multiplayer games.
	* Ex. 3 players try joining (MaxPlayers is 3).
	* SERVER LOG:
		* message\_loop: content:
PLAY bob
		* message\_loop: content:
PLAY joe
		* message\_loop: content:
PLAY dan
	* CLIENT LOG (joe):
		* message\_loop: 1 lines:
OK B
	* CLIENT LOG (dan):
		* message\_loop: 1 lines:
OK C
	* The 3 players each exist at different locations on map.
* Server rejects player if too many have already joined.
	* Ex. 4th player tries to join (MaxPlayers is 3).
	* SERVER LOG: message\_loop: content:
PLAY phil
	* 4th player's screen is supposed to close and error message would be sent to client's logfile, but looks like given player module doesn't behave as expected. Instructor confirms this is a bug on his side, not an issue with our server module.

* Server refreshes screen for clients when opponents move in map or gold is collected, such that game appears live.
	* Ex. bob moves one unit to right.
	* SERVER LOG: message\_loop: content:
KEY l
	* CLIENT LOG (joe): message\_loop: content:
DISPLAY
	* joe's screen is updated to show bob's new location.
	* Ex. bob moves to pile of nuggets.
	* SERVER LOG: message\_loop: content:
KEY h
	* CLIENT LOG (joe): message_loop: content:
DISPLAY
	* joe's screen is updated to show empty spot where gold initially may have been (cell is visible to joe!).

### Supports spectator 

<p> We again test our server, but without giving a player name to signal our client as a spectator to the server. </p>

* Server allows spectator to join.
	* Example, one spectator joining 
	* SERVER Log:
		* message_init: ready at port '34498' 
		* message_loop: message ready on socket
		* message_loop: from host 127.0.0.1
		* message_loop: from port 50522
		* message_loop: content:
		* SPECTATE
	* CLIENT Log:
		* message_init: ready at port '50522'
		* message_send: TO 127.0.0.1:34498
		* message_send: 1 lines:
		* .....
		* DISPLAY
* Server kicks out current spectator, when a new one wants to join.
	* Example, one spectator `original_spectator` joins, and then another `new_spectator` joins
	* SERVER Log:
		* message_init: ready at port '41832'
		* message_loop: message ready on socket
		* message_loop: from host 127.0.0.1
		* message_loop: from port 57710
		* message_loop: content:
		* SPECTATE
		* message_loop: message ready on socket
		* message_loop: from host 127.0.0.1
		* message_loop: from port 49453
		* message_loop: content:
		* SPECTATE
	* ORIGINAL SPECTATOR LOG:
		* message_init: ready at port '57710'
		* message_send: TO 127.0.0.1:41832
		* message_send: 1 lines:
		* SPECTATE
		* ...
		* DISPLAY
	* NEW SPECTATOR LOG:
		* message_init: ready at port '49453'
		* message_send: TO 127.0.0.1:41832
		* message_send: 1 lines:
		* SPECTATE
		* ...
		* DISPLAY


### Supports ‘visibility’

<p> We ran our server and the professor's server simultaneously to make sure they both react in the exact same way every time. Further, we made sure that the visibility code never tries to access an 'out of bounds' cell. While doing this, we found that our implementation differs from the professor. Upon talking to the professor, we made the decision to mimic the professor's implementation instead of following the requirement spec literally. </p>

### Tracks gold

<p> See "support" sections above for additional information on gold-tracking by server. </p>

* Server displays gold in same locations when given the same seed each time it is run.
	* Ex. seed 1 is passed as parameter.
	* Maps show gold in the same exact locations for each iteration, with same amounts in each pile.

### Produces Game Over summary

<p> Multiple players </p>

* Server presents each player with gameover message when all gold has been collected on map.
	* Ex. dan collects final pile of nuggets.
	* SERVER LOG: message\_loop: content:
KEY j
	* CLIENT LOG (any client): message\_loop: 5 lines: GAMEOVER 	
		* bob A 128
		* joe B 56
		* dan C 116
	* Gameover message is successfully sent for client to display on terminal.

### New, valid, non-trivial mapfile

* Server displays custom-made map.
	* Ex. `./server maps/FUBAR.txt`
	* Map shows brand new passage-ways and rooms. FUBAR.txt is a large map, forcing the user to resize the window (a smaller one could be used to display the map from main.txt). The player is successfully able to traverse the entire map.

## Client testing

### Commandline
* Passing the wrong number of arguments
	* Ex: `./player`
		* CLIENT LOG: 
			* message_init: ready at port '35494'
			* usage: ./player hostname port [yourname]
	* Ex: `./player 2>Myplayer.log localhost 52440 Steve Jobs` - A space in the player name is seen as another argument
		* CLIENT LOG: 
			* message_init: ready at port '59297'
			* usage: ./player hostname port [yourname]

* Nothing happens on either end, when I enter the wrong port for the server
	* Ex: `./player 2>Myplayer.log localhost 00000 isaiah`
*  When entering the correct server port without a player name, client joins as the spectator
	* Ex: `./player 2>Myplayer.log localhost 38808`
	* SERVER LOG:
		* message_init: ready at port '38808'
		* Ready to play, waiting at port 38808
		* message_loop: message ready on socket
		* message_loop: FROM 127.0.0.1:38147
		* message_loop: 1 lines:
		* SPECTATE
* When attempting to enter the correct server port with a player name that is too long, the client truncates the player name to a given maximum name length
	* Ex: `./player 2>Myplayer.log localhost 42266 hugeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee`
	* CLIENT LOG GAMEOVER SUMMARY:
		* GAME OVER:
		* A    250 hugeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
* Attempting to enter the correct server port with a player name under 50 characters, joins the player as intended
	* Ex: `./player 2>Myplayer.log localhost 52440 SteveJobs`
	* CLIENT LOG:
		* message_init: ready at port '39899'
		* message_loop: message ready on socket
		* message_loop: from host 127.0.0.1
		* message_loop: from port 52440
		* message_loop: content:
		* OK A
	* SERVER LOG:
		* message_init: ready at port '52440'
		* Ready to play, waiting at port 52440
		* message_loop: message ready on socket
		* message_loop: FROM 127.0.0.1:39899
		* message_loop: 1 lines:
		* PLAY SteveJobs
		* message_send: TO 127.0.0.1:39899
		* message_send: 1 lines:
		* OK A
		* ...
		* DISPLAY
### Plays as player 
* Player Movements
    * ex: 'h', move left (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY h
    * 'j', move down (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY j
    * 'k', move up (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY k
    * 'l', move right (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY l
    * 'y', move diagonally up and left (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY y
    * 'u', move diagonally up and right (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY u
    * 'b', move diagonally down and left (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY b
    * 'n', move diagonally down and right (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY n
    * 'H', move left (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY H
    * 'J', move down (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY J
    * 'K', move up (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY K
    * 'L', move right (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY L
    * 'Y', move diagonally up and left (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY Y
    * 'U', move diagonally up and right (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY U
    * 'B', move diagonally down and left (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY B
    * 'N', move diagonally down and right (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY N
* Quitting the Game
    * 'Q', quit connection with server (command sent to server from client)
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:43049
            * message_loop: 1 lines:
            * KEY Q
        * CLIENT LOG:
            * message_loop: from host 127.0.0.1
            * message_loop: from port 38336
            * message_loop: content:
            * QUIT
            * message_done: message module closing down.
            * END OF LOG
* Sending of invalid keys are properly sent to server and recognized
    * Ex: inputting "1" on client side
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:35563
            * message_loop: 1 lines:
            * KEY 1
            * message_send: TO 127.0.0.1:35563
            * message_send: 1 lines:
            * NO usage: unknown keystroke
* See "Server testing" for testing on player functionalities within game (gold allocation, visibility, etc.) 
### Plays as spectator
* Sending of invalid keys are properly sent to server and recognized
    * Ex: inputting "m" on client side
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:50068
            * message_loop: 1 lines:
            * KEY m
            * message_send: TO 127.0.0.1:50068
            * message_send: 1 lines:
            * NO usage: unknown Spectator keystroke
* Spectator is able to view game display with one player that updates
* Spectator is able to view game display with multiple players that update
* Spectator qutting game
    * Ex: inputting "Q" on client side is properly sent and recognized by server
        * SERVER LOG:
            * message_loop: message ready on socket
            * message_loop: FROM 127.0.0.1:50068
            * message_loop: 1 lines:
            * KEY Q
            * message_send: TO 127.0.0.1:50068
            * message_send: 1 lines:
            * QUIT
### Asks for window to grow
* Client is prompted to heighten or widen screen when necessary.
    * Ex: Client tries to join server with window that is too small for main.txt map being used.
    * SERVER LOG: 
        * message_send: 1 lines:
GRID 21 79
        * message_send: 1 lines:
GOLD 0 0 250
        * message_send: 22 lines:
DISPLAY
    * CLIENT LOG:
        * message_loop: content:
GRID 21 79
    * Client is shown notification stating proper window dimensions (as per grid dimensions sent by server) necessary to play game. Screen remains stagnant until client presses enter AND dimensions satisfy.
    
* Game exits when client resizes terminal window amidst game.
    * Ex: Client tries to decrease window width when map is currently being displayed.
    * Resizing display during game is not allowed.

### Prints Game over summary
<p> Two clients: player and spectator </p>

* Server presents each player and spectator (if exists) with gameover message when all gold has been collected on map.
	* Ex: aadil collects final pile of nuggets.
	* SERVER LOG: 
	    * message\_loop: content: KEY j
	* CLIENT LOG (either client): 
        * message\_loop: 5 lines: GAMEOVER 	
		* aadil A 300
	* After display has been closed, game over summary is printed on terminal, after which program exits.
