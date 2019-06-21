# Nuggets - Requirements Spec

<!--
  IMPORTANT: read this file on [GitHub](https://github.com/cs50spring2019/nuggets/blob/master/REQUIREMENTS.md) instead of reading your local copy.
  It is possible that we have pushed updates to GitHub and you'll want to 
  be sure you are reading the latest version.  To update your repo from 
  the starter kit, see [README](README.md).
  -->

A multi-player exploration game, *Nuggets*, in which a game server maintains all game state, and one or more game clients display the game to a player.
The object of the game is to collect more gold nuggets than any other player. 
The game ends when all gold nuggets have been collected by some player.

* Game play occurs in a set of interconnected *rooms* and *passages*, as defined by a [map](#maps).
* At game start time, `GoldTotal` nuggets are randomly dropped in a random number of random-sized piles, at some spot in a room.  Gold nuggets are indistinguishable; a pile contains at least one nugget.
* There are zero to `MaxPlayers` players, and zero or one *spectators*.
* A new *player* is dropped into a random empty spot within a room.
* A new player initially has 0 nuggets in its *purse*.
* A player can *see* only those spots that are [*visible*](#visibility) from its current location. 
* A player can *know* the boundaries of all rooms and passages it has seen since the player began playing.
* The spectator immediately *knows* and always *sees* all gridpoints. 
* The *display* is an ASCII screen large enough to represent the entire grid.
* At any given time, a player's display illustrates all known boundaries and all visible spots; the spectator's display illustrates all boundaries and spots.
* A player moving into a spot containing a pile of gold *collects* that gold, adding all the pile's nuggets to the player's purse. The pile is then gone.
* A player moving into a spot occupied by another player causes the two players to switch places.
* The game *ends* when all gold nuggets have been collected.
* At end, the game announces to all players (and spectator, if any) the size of each players' purse; the player(s), spectator, and server then quit.

## Constants

The game has several parameters; although other values are reasonable, we specify the following.

```c
static const int MaxBytes = 65507;     // max number of bytes in a message
static const int MaxNameLength = 50;   // max number of chars in playerName
static const int MaxPlayers = 26;      // maximum number of players
static const int GoldTotal = 250;      // amount of gold in the game
static const int GoldMinNumPiles = 10; // minimum number of gold piles
static const int GoldMaxNumPiles = 30; // maximum number of gold piles
```

## Server

	./server map.txt [seed]

The server ***shall***

1. Start from the commandline of the form above; thus the first argument is the pathname for a [map file](#mapfiles) and the second argument is an optional integer providing a seed for the random-number generator.  
2. Verify its arguments; if error, provide a useful error messages and exit non-zero.
2. If the optional seed is provided, the server shall pass it to `srandom(seed)`.  If no seed is provided, the server shall use `srandom(time(NULL))` to produce random behavior.
3. Load the designated map file; the server may assume it is [valid](#validmaps).
4. Initialize the game by dropping at least `GoldMinNumPiles` and at most `GoldMaxNumPiles` gold piles on random empty spots; each pile shall have a random number of nuggets.
5. Initialize the network and announce the port number.
6. Wait for messages from clients.
7. Accept up to `MaxPlayers` players; if a player exits or quits the game, it can neither rejoin nor be replaced.
8. Accept up to 1 spectator; if a new spectator joins while one is active, the server shall tell the current spectator to quit, and the server shall then forget that current spectator.
7. React to each type of inbound message as described in the [protocol](#networkprotocol) below.
8. Update all clients whenever any player moves or gold is collected.
8. Monitor the number of gold nuggets remaining; when it reaches zero, the server shall send a `GAMEOVER` message to all clients, print the Game-over summary, and exit.

The server ***should*** log useful information that can be saved in a logfile;
a typical approach would be to log to stderr and thus usage could be:

	./server 2>server.log map.txt

## Player

	./player hostname port [playername]

The player ***shall***

1. start from the commandline of the form above; thus the first argument is the name or IP address where the server is running, and the second argument is the port number on which the server expects messages; the third (optional) argument determines whether to join as a *player* or *spectator*.
2. Verify its arguments; if error, provide a useful error messages and exit non-zero.
2. If the `playername` argument is provided, the player shall truncate it (if necessary) to limit it to `MaxNameLength` characters.
2. If the `playername` argument is provided, the user joins as a *player* and can interactively play the game.
3. If the `playername` argument is not provided, the user joins as a view-only *spectator*.
4. Initialize ncurses.
5. Initialize the network and join the game with a `PLAY` or `SPECTATE` message accordingly.
6. Upon receipt of a `GRID` message, ensure the window is large enough for the grid (it should be *NR+1* x *NC+1* for best results).
7. Display a status line on the first line of the display, in the [protocol](#networkprotocol) below.
8. Display the game grid on the subsequent lines of the display, as noted in the [protocol](#networkprotocol) below.
9. Update the display any time new information arrives from the server.
10. Quit when told to do so by the server, as noted in the [protocol](#networkprotocol) below.
11. Display a brief note on the status line if an unknown or malformed message arrives from the server.
12. Quit the game if reaching EOF on stdin.
13. Print a Game-over summary and exit, as noted in the [protocol](#networkprotocol) below.


The player ***should*** log useful information that can be saved in a logfile;
a typical approach would be to log to stderr and thus usage could be:

	./player 2>player.log hostname port playername
	./player 2>spectator.log hostname port

### Player interface

The display shall consist of *NR+1* rows and *NC* columns.
The player program shall complain if the window is too small, and wait for the user to enlarge the window.

The top line shall provide game status; for a player, it should look like this:

	Player A has 39 nuggets (211 nuggets unclaimed).

If other information needs to be displayed briefly, it is placed on the right:

	Player A has 39 nuggets (211 nuggets unclaimed).  GOLD received: 39

	Player A has 39 nuggets (211 nuggets unclaimed).  unknown keystroke

The spectator's status line should look like this:

	Spectator: 211 nuggets unclaimed.

**Grid display:**
The remaining *NR* lines present the grid using *map characters:*

 * ` ` solid rock - interstitial space outside rooms
 * `-` a horizontal boundary
 * `|` a vertical boundary
 * `+` a corner boundary
 * `.` an empty spot
 * `#` a passage spot

or *occupant characters:*

 * `@` the player
 * `A`-`Z` another player
 * `*` a pile of gold

**Spectator keystrokes:**
The spectator can type

 * `Q` quit the game.

**Player keystrokes:**
The player can type

 * `Q` quit the game. 
 * `h` move left, if possible
 * `l` move right, if possible
 * `j` move down, if possible
 * `k` move up	, if possible
 * `y` move diagonally up and left, if possible
 * `u` move diagonally up and right, if possible
 * `b` move diagonally down and left, if possible
 * `n` move diagonally down and right, if possible

* where *possible* means the adjacent gridpoint in the given direction is an empty spot, a pile of gold, or another player.
* for each move key, the corresponding Capitalized character will move *as far as possible* automatically and repeatedly in that direction, until it is no longer possible.

## Maps

A *map* defines the set of rooms and passages in which the game is played.

### Valid maps

* The *map* is laid out on a *grid*.
* The grid is *NR* rows by *NC* columns; thus there are *NR x NC* *gridpoints*.
* The grid will fit in a `DISPLAY` message; thus, *NR x NC + 10 < MaxBytes*.
* The grid has enough spots to accommodate `MaxPlayers` players and `GoldMaxNumPiles` gold piles.
* A *room* is a [simple rectilinear polygon](https://en.wikipedia.org/wiki/Rectilinear_polygon). 
* A *spot* is a gridpoint in the interior of a room or along a passage.
* A room is defined by its *boundaries*.
* A horizontal boundary always meets a vertical boundary at a corner boundary.
* Thus, the *boundaries* of rooms are not spots, nor are gridpoints outside rooms and passages.
* A *passage* is one-spot wide and connects rooms to other rooms and passages. Passages are rectilinear but may not be straight, that is, they may have 90-degree turns.
* A passage interrupts a room's vertical or horizontal boundary; a passage never meets a room at a corner.
* The map is one connected component; thus, one can reach any spot from any other spot by moving in some sequence of up,down,left,right.

* Every gridpoint is one of these characters:
   * ` ` solid rock - interstitial space outside rooms
   * `-` a horizontal boundary
   * `|` a vertical boundary
   * `+` a corner boundary
   * `.` an empty spot
   * `#` a passage spot


### Map files

A *map file* is a text file with exactly *NC* lines and in which every line has exactly *NR* characters.

> Your solution may assume all map files are [valid](#validmaps), but must infer *NR* and *NC* by reading the file.


### Example map

The following is a 21x79 map.
The dots represent empty spots; gold pieces and players may occupy these.
The hashes represent passageways; players may occupy these. 
One room is non-convex. 
Some passageways bend, and some fork.
Some rooms have multiple entrances.
The room at upper-left shows a passage to nowhere.

```
  +----------+                                                                 
  |..........|                 +---------+                                     
  |..........####              |.........|                +-------+            
  |..........|                 +-----#---+                |.......|            
  +---------#+                       #              #######.......|            
            #                        #              #     +---#---+            
            #  +-----------+         #     +--------#+        #                
            ####...........##############  |.........|        #                
               |...........|            #  |.........|        #                
               +-----------+            ####.........|        #                
                                           +----#----+        #                
                                                #    +--------#--+             
     +---------------------------------+        #    |...........|             
     |.................................|        ######...........|             
     |.................................|        #    |...........|             
     |......+---------------+..........|        #    |...........|             
     |......|               |..........##########    +-----------+             
     |......|               |..........|                                       
     |......|               |..........|                                       
     |......|               |..........|                                       
     +------+               +----------+                                       
```

## Visibility

Assume a player is at gridpoint *(pr,pc)*.
Another gridpoint *(r,c)* is "visible" from point *(pr,pc)* by reviewing the *map*.
(Only the base map affects visibility; occupants do not affect visibility.)

 * A map gridpoint that is blank (a space) is never visible.                               
 * A map gridpoint on the same row or column as *(pr,pc)* is visible if all intervening map gridpoints are 'spots'.                                      
* Otherwise, we compute the mathematical line segment from *(pr,pc)* through *(r,c)*.
Considering each row in the range *(pr...r)* and each column in the range *(pc...c)*, imagine the line segment passing between pairs of map gridpoints as it travels from *(pr,pc)* to *(r,c)*; if *both* map gridpoints of any such pair are not 'empty spots', then they block our vision and we conclude *(r,c)* is not visible.
Only if there are no such blocking pairs do we conclude that point *(r,c)* is visible.
* Note that gridpoint *(r,c)* itself may be an empty spot, passage spot, or boundary.

Examples, from the above map.
At game start:

```
                        ---------------+                                       
                         ..............|                                       
                           ............|                                       
                            +..........|                                       
                            |......*...#                                       
                            |..........|                                       
                            |....@.....|                                       
                            |*.........|                                       
                            +----------+                                       
```

Consider some of the line segments that may be drawn toward the upper wall:

![diagram with lines drawn on the above map](images/visibility.png)

After moving up a few spots, the passageway became visible as we passed by, and (because it is now "known") remains displayed.
The wall to the left is not quite visible, but the gold and spots above have become visible:

```
     +---------------------------------+                                       
     |...*.............................|                                       
     |.........*...*....*..............|                                       
                            +....@.....|                                       
                            |......*...##########                              
                            |..........|                                       
                            |..........|                                       
                            |*.........|                                       
                            +----------+                                       
```

Going up one and moving left to collect gold, the other part of the room comes into view, and the gold piles in the lower-right disappear from view:

```
     +---------------------------------+                                       
     |...*.............................|                                       
     |.........@.......................|                                       
     |......+---------------+..........|                                       
     |...                   |..........##########                              
     |                      |..........|                                       
                            |..........|                                       
                            |..........|                                       
                            +----------+                                       
```

Backtracking, and going down the passage to the corner, we can see straight up the passage and across the room beyond; the gold in the room behind us is no longer visible:

```
                                                -                              
                                                .                              
                                                .                              
                                                .                              
                                                #                              
                                                #                              
     +---------------------------------+        #                              
     |.................................|        #                              
     |.................................|        #                              
     |......+---------------+..........|        #                              
     |...                   |..........#########@                              
     |                      |..........|                                       
                            |..........|                                       
                            |..........|                                       
                            +----------+                                       
```

## Network protocol

The network protocol connects zero or more game *clients* (players and spectator) with one game *server*.
The server maintains all game state; the clients display the game state to a user, and sends their keystrokes to the server; the server sends back updated displays.

> The protocol runs over UDP/IP, that is, the *user datagram protocol* over the *Internet Protocol*. 
> [UDP/IP](https://en.wikipedia.org/wiki/User_Datagram_Protocol) and [TCP/IP](https://en.wikipedia.org/wiki/Transmission_Control_Protocol) form the core of the Internet.
> In either protocol, communication occurs between two endpoints; the address of an endpoint is a pair *host IP address*, *port number*.
> UDP carries *datagrams* from one port on one host to another port on another host (well, they could be the same host).
> A datagram can hold [zero to 65,507 bytes](https://en.wikipedia.org/wiki/User_Datagram_Protocol).

Our game sends one *message* in each datagram.
Each message is an ASCII string (text).
Most messages are on one line and have no terminating newline.
Some messages are multiple lines long.
Newlines are shown explicitly as `\n` in the specs below.
The first word of the message indicates the type of message.
(The first word begins at the start of the datagram and is terminated by a space, a newline, or the end of the message.)
Message types are in ALL CAPS.

When the server starts, it shall open a new endpoint and announce its port.
When the client starts, it shall send a message to the hostname (or address) and port number where the server awaits.
There are two types of client: *players* and *spectators*; let's look at the messages each can send, then at the messages a server can send. 

### Player to server

When a *player* client starts, it shall send a message to the server:

	PLAY realname

Everything after the `PLAY` and one space is captured as the player's "real name" (free text).
The player shall ensure `realname` is less than `MaxNameLength` characters long.

If there are already `MaxPlayers` clients, the game server shall respond with  

	NO

Otherwise, the server shall respond with 

	OK L

where `L` is this player's letter in the set {`A`, `B`, ... `Z`}.

The server shall then immediately send `GRID` and `GOLD` messages as described below.

The client sends, at any time,

	KEY k
	
where k is the single-character keystroke typed by the user.

When the player's keystroke causes them to collect gold, the server shall inform all clients using a `GOLD` message as described below.

When the player's keystroke causes them to move to a new spot, the server shall inform all clients of a change in the game grid.

### Spectator to server

When a *spectator* client starts, it shall send a message to the server:

	SPECTATE

to join as a spectator.
If there is already a spectator, this spectator takes its place
(the server sends a `QUIT` message to the prior spectator, then forgets it).
Thus, the server tracks only one spectator at a time.

The server shall respond with a `GRID` message as described below.

The server shall then immediately send a `GOLD` message as described below.

The spectator is not assigned a letter and is not represented on the map.

Subsequent `DISPLAY` messages will include a complete view, as if this client *knows* all and *sees* all.

### Server to clients

The server shall send immediately to new clients,

	GRID nrows ncols

where `nrows` and `ncols` are positive integers describing the size of the grid.
This size will never change.

The server shall send immediately to new clients, and at any time to all clients,

	DISPLAY\nstring
	
where the `DISPLAY` is separated from the `string` by a newline, and the `string` is literally a multi-line textual representation of the grid as known/seen by this client.
(Indeed, if you were to just print the message string, it would be recognizable as the game map.  That's why DISPLAY ends with newline, and why the string contains an embedded newline after each row.)
More precisely, `string` has `nrows` lines, each of which has `ncols` characters plus a newline.
Each client receives a different version, because (a) the spectator knows all and sees all, but is not itself represented on the map, (b) players' displays show only the boundaries they know and the spots visible from their current position, and (c) the player's own position is represented by `@`.
Note it is entirely the server's responsibility to produce these display strings.

The server shall send immediately to new clients, and at any time to all clients,

	GOLD n p r

where `n`, `p`, and `r` are positive integers,
to inform the player it has just collected `n` gold nuggets, its purse now has `p` gold nuggets, and there remain `r` nuggets left to be found.
The value of `n` may be zero.
The value of `p` shall be initially zero, but will increase when gold is found.
The spectator shall always receive `n=0`, `p=0`.

The server sends, at any time,

	QUIT

upon which the client should not send any more messages and shall exit.

The server sends, at any time,

	GAMEOVER\nsummary
	
where `summary` shall be a printable, multi-line string summarizing the purses of the game.
The summary shall include one line per player, with player Letter, purse (gold nugget count), and player real name, in tabular form.
After receiving a `GAMEOVER` message the client shall print the summary and shall exit.

The server *may* send, in response to the client,

	NO ...

to indicate it was unable to understand or handle the client's prior message.
The remainder of the line, if present, provides a short explanatory text.
The client shall present this text to its user on the display's status line.

## Inspiration

This project was inspired by a classic game, [Rogue](https://docs.freebsd.org/44doc/usd/30.rogue/paper.pdf). 

Why do we use the H-J-K-L keys?
Because [the original ADM3a terminal had arrows on them](https://catonmat.net/why-vim-uses-hjkl-as-arrow-keys).
