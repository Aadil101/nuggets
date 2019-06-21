/* 
 * server.c - the server for the nuggets game.
 *  Handles communication between the grid and the players.
 *
 * usage: ./server mapfile [seed]
 *
 * foobarbaz, April 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <message.h>
#include <string.h>
#include <file.h>
#include "grid.h"
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <memory.h>

// Function Prototypes
int validate_params(const int argc, const char *argv[]);
bool handle_message(void *arg, const addr_t from, const char *message);
void add_player(addr_t from, const char* name);
void process_keystroke(addr_t from, char key);
void add_spectator(addr_t from);
void game_over();
void send_board();
void player_remove(player_t* player);
player_t* get_player_from_addr(addr_t from);
void free_player(player_t* player);
void free_grid();
static bool str2int(const char string[], int *number);

const int name_width = 10;			   // default width for displaying a name in game_over
const int gold_width = 5;			   // default width for displaying gold count in game_over
static const int MaxPlayers = 26;      // maximum number of players
static const int GoldTotal = 300;      // amount of gold in the game
static const int GoldMinNumPiles = 10; // minimum number of gold piles
static const int GoldMaxNumPiles = 20; // maximum number of gold piles
static const char PlayerTags[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const int MaxBytes = 65507;

//Game struct for holding grid and num_players
typedef struct {
  grid_t* grid;
  int num_players;
} game_t;
static game_t game;


/* ***************** main ********************** */
int
main(const int argc, const char *argv[])
{
	//validate params and return non-zero if invalid params
	int status;
    if ((status=validate_params(argc, argv)) != 0) {
		return status;
	}

	//use random int as seed if not specified
	if (argc == 2) {
		game.grid = grid_new((char*)argv[1], time(NULL), GoldMinNumPiles, GoldMaxNumPiles, GoldTotal, MaxPlayers);
	}
	else {
		game.grid = grid_new((char*)argv[1], atoi(argv[2]), GoldMinNumPiles, GoldMaxNumPiles, GoldTotal, MaxPlayers);
	}

	if (game.grid == NULL) {
		return 4;
	}

	//Probably should be (num_cols+1)*num_rows + 10 to include the newline but this is what the spec said to do.
	if ((game.grid->num_cols*game.grid->num_rows + 10) >= MaxBytes) {
		printf("Map is too large. Exceeds max bytes for display message!\n");
		free_grid();
		return 5;
	}

	//initialize and loop through message
	message_init(stderr);
	message_loop(NULL, 0, NULL, NULL, handle_message); //no timeout and no stdin only message and no argument used
	message_done();

	//frees grid and all of the memory used by it
	free_grid();
}

// function run within message_loop to handle incoming messages
// returns true if break, false if continue
// arg- argument passed (not used)
// from- address message is received from
// message- message contents
bool handle_message(void *arg, const addr_t from, const char *message) {
	// if message equals play
	if (strncmp(message, "PLAY ", strlen("PLAY ")) == 0) {
		add_player(from, &(message[strlen("PLAY ")]));
	}
	// if message equals key
	else if (strncmp(message, "KEY ", strlen("KEY ")) == 0) {
		process_keystroke(from, message[strlen("KEY ")]);
	}
	// if message equals spectate
	else if (strncmp(message, "SPECTATE", strlen("SPECTATE")) == 0) {
		add_spectator(from);
	}
	else {
		return false;
	}
	// no need to handle other message types- player only sends those three

	// update the board for players and spectator
	send_board();

	// if no more gold after processing message
	if (game.grid->gold_remaining == 0) {
		game_over(); // sends gameover message
		return true; // breaks the loop
	}

	
	return false;
}

// adds a player to the player array held in game
// from - address of player to be added
// name - name of player to be added
void add_player(addr_t from, const char* name) {
	//check if player limit reached or if the client is trying to reconnect
	if (game.num_players == MaxPlayers || get_player_from_addr(from) != NULL) {
		message_send(from, "NO"); // reject join request
		return;
	}

	//malloc a new player and its name
	player_t* player = malloc(sizeof(player_t));
	assertp(player, "Error allocating memory to player");
	char* player_name = malloc(strlen(name)+1);
	assertp(player_name, "Error allocating memory to player name");
	//copy the playername into the name
	strcpy(player_name, name);
	//set the name, tag, and gold obtained
	player->player_name = player_name;
	player->player_tag = PlayerTags[game.num_players];
	player->gold_obtained = 0;
	// malloc known array
	player->known = (int **)calloc(game.grid->num_rows, sizeof(int *)); 
	assertp(player->known, "Error mallocing memory to known player array");
    for (int i=0; i<game.grid->num_rows; i++) {
         player->known[i] = (int *)calloc(game.grid->num_cols, sizeof(int));
		 assertp(player->known[i], "Error allocating memory to player known array");
	}

	// malloc display for player
	player->display = malloc(((game.grid->num_rows * (game.grid->num_cols + 1))+1)*sizeof(char));
	assertp(player->display , "Error allocating memory to player display");
	//addr is const so we need to cast away the const to modify
	*(addr_t *)&player->addr = from;
	// player hasn't quit
	player->player_quit = false;
	// send message to player
	char ok_msg[5];
	sprintf(ok_msg, "OK %c", player->player_tag);
	game.grid->players[game.num_players] = player;
	// place the player on a random empty room spot
	grid_add_player(game.grid, player);
	message_send(from, ok_msg);
	//allocate 4 spaces for GRID, enough for row/col, and 3 for spaces + null
	char grid_msg[4 + (int_len(game.grid->num_rows)*sizeof(char)) + (int_len(game.grid->num_cols)*sizeof(char)) + 3];
	sprintf(grid_msg, "GRID %d %d", game.grid->num_rows, game.grid->num_cols);
	message_send(from, grid_msg);

	// send gold information to the new player
	char gold_msg[4 + 1 + (int_len(player->gold_obtained)*sizeof(char)) + (int_len(game.grid->gold_remaining)*sizeof(char)) + 4];
	sprintf(gold_msg, "GOLD 0 %d %d", player->gold_obtained, game.grid->gold_remaining);
	message_send(from, gold_msg);
	// increment number of players
	game.num_players = game.num_players + 1;
}

// function for handling keystrokes from the player
// from - address of the player sending the message
// key - keystroke sent
void process_keystroke(addr_t from, char key) {

	// if there is currently a spectator and the message is from the spectator
	if (game.grid->spectator != NULL && message_eqAddr(from, game.grid->spectator->addr)) {
		// Quit the spectator, free its memory and set the spectator equal to null
		if (key == 'Q') {
			message_send(game.grid->spectator->addr, "QUIT");
			free_player(game.grid->spectator);
			game.grid->spectator = NULL;
			return; // so we don't continue and trigger a segfault
		}
		else {
			return; //spectator can only send Q
		}
	}

	// get the player using its unique address
	player_t* player = get_player_from_addr(from);

	// error handling, this should never happen but we validate here for ease of debugging
	if (player == NULL) {
		printf("ERROR PLAYER IS NULL IN PROCESS KEYSTROKE");
		fflush(stdout);
	}

	// initial gold collected is zero
	int gold_collected = 0;

	// map each key to the movement. Each int in the move functions represents row, col
	// a positive row represents a move down, a negative row represents a move up.
	// a positive col represents a move left, a negative col represents a move right.
	// move returns the gold collected which is stored.
	switch (key) {
		case 'h': gold_collected = grid_move(game.grid, player, 0, -1);
			break;
		case 'j': gold_collected = grid_move(game.grid, player, 1, 0);
			break;
		case 'k': gold_collected = grid_move(game.grid, player, -1, 0);
			break;
		case 'l': gold_collected = grid_move(game.grid, player, 0, 1);
			break;
		case 'y': gold_collected = grid_move(game.grid, player, -1, -1);
			break;
		case 'u': gold_collected = grid_move(game.grid, player, -1, 1);
			break;
		case 'b': gold_collected = grid_move(game.grid, player, 1, -1);
			break;
		case 'n': gold_collected = grid_move(game.grid, player, 1, 1);
			break;
		case 'H': gold_collected = grid_move_to_end(game.grid, player, 0, -1);
			break;
		case 'J': gold_collected = grid_move_to_end(game.grid, player, 1, 0);
			break;
		case 'K': gold_collected = grid_move_to_end(game.grid, player, -1, 0);
			break;
		case 'L': gold_collected = grid_move_to_end(game.grid, player, 0, 1);
			break;
		case 'Y': gold_collected = grid_move_to_end(game.grid, player, -1, -1);
			break;
		case 'U': gold_collected = grid_move_to_end(game.grid, player, -1, 1);
			break;
		case 'B': gold_collected = grid_move_to_end(game.grid, player, 1, -1);
			break;
		case 'N': gold_collected = grid_move_to_end(game.grid, player, 1, 1);
			break;
		case 'Q': player_remove(player);
			break;
		default:
			message_send(from, "NO Invalid Key"); // any other key is invalid
			break;
	}

	// if we collected gold during the move
	if (gold_collected != 0) {
		//send a gold message to the player who collected the gold
		char gold_msg[4 + (int_len(gold_collected)*sizeof(char)) + (int_len(player->gold_obtained)*sizeof(char)) + (int_len(game.grid->gold_remaining)*sizeof(char)) + 4];
		sprintf(gold_msg, "GOLD %d %d %d", gold_collected, player->gold_obtained, game.grid->gold_remaining);
		message_send(from, gold_msg);

		//send a message to the spectator with the updated gold count
		if (game.grid->spectator != NULL) {
			//4 spaces for GOLD, 2 spaces for 0s, enough space for gold_remaining, 4 spaces for spaces and null
			char gold_spec[4 + (int_len(game.grid->gold_remaining)*sizeof(char)) + 6];
			sprintf(gold_spec, "GOLD 0 0 %d", game.grid->gold_remaining);
			message_send(game.grid->spectator->addr, gold_spec);
		}

		//send a message to the rest of the players with an updated gold count
		for (int i = 0; i < game.num_players; i++) {
			if (message_eqAddr(from, game.grid->players[i]->addr)) {
				continue;
			}
			player = game.grid->players[i];
			if (player->player_quit) {
				continue;
			}
			char others_gold_msg[4 + 1 + (int_len(player->gold_obtained)*sizeof(char)) + (int_len(game.grid->gold_remaining)*sizeof(char)) + 4];
			sprintf(others_gold_msg, "GOLD %d %d %d", 0, player->gold_obtained, game.grid->gold_remaining);
			message_send(player->addr, others_gold_msg);
		}
	}
}


//function for adding a spectator
//if there is an existing spectator it is booted from the game and freed
//from - address the spectator message is from
void add_spectator(addr_t from) {
	// if there is currently a spectator boot and free them
	if (game.grid->spectator != NULL) {
		message_send(game.grid->spectator->addr, "QUIT");
		free_player(game.grid->spectator);
		game.grid->spectator = NULL;
	}

	// malloc a new spectator and set its variables
	player_t* spectator = malloc(sizeof(player_t));
	assertp(spectator, "Error allocating memory to spectator");
	*(addr_t *)&spectator->addr = from;
	// malloc display and name
	// spectator doesn't technically need a name but this makes freeing more simple
	// as we can recycle the same function for freeing a player
	spectator->display = malloc(((game.grid->num_rows * (game.grid->num_cols + 1))+1)*sizeof(char));
	spectator->player_name = malloc((strlen("spectator") + 1)*sizeof(char));
	assertp(spectator->display, "Error allocating memory to spectator display");
	assertp(spectator->player_name, "Error allocating memory to spectator name");
	spectator->row = 0;
	spectator->col = 0;
	spectator->known = NULL;
	spectator->gold_obtained = 0;
	strcpy(spectator->player_name, "spectator");
	game.grid->spectator = spectator;

	//send grid and gold messages to spectator

	//allocate 4 spaces for GRID, enough for row/col, and 3 for spaces + null
	char grid_msg[4 + (int_len(game.grid->num_rows)*sizeof(char)) + (int_len(game.grid->num_cols)*sizeof(char)) + 3];
	sprintf(grid_msg, "GRID %d %d", game.grid->num_rows, game.grid->num_cols);
	message_send(from, grid_msg);
	//4 spaces for GOLD, 2 spaces for 0s, enough space for gold_remaining, 4 spaces for spaces and null
	char gold_msg[4 + (int_len(game.grid->gold_remaining)*sizeof(char)) + 6];
	sprintf(gold_msg, "GOLD 0 0 %d", game.grid->gold_remaining);
	message_send(from, gold_msg);
}

// function for sending gameover summary to players and spectator at end of game
void game_over() {
	int strsize = 10; //initial GAMEOVER\n + null char
	//one line per player, with player Letter, purse (gold nugget count), and player real name, in tabular form.
	int print_size[game.num_players + 1]; //keeps track of number of chars to print in each line
	print_size[0] = 9; //the size of GAMEOVER\n
	// for each player determine space needed for its summary
	for (int i = 0; i < game.num_players; i++) {
		player_t* player = game.grid->players[i];
		//add size of playername, gold obtained, and player tag + 3 for spaces and newline
		int gold_str_len = (int_len(player->gold_obtained)*sizeof(char));
		print_size[i+1] = 1;
		print_size[i+1] += ((strlen(player->player_name) > name_width) ? strlen(player->player_name) : name_width);
		print_size[i+1] += ((gold_str_len > gold_width) ? gold_str_len : gold_width);
		print_size[i+1] += 3;
		//add the size of the player summary to total size of string
		strsize += print_size[i+1];
	}

	// create string with adaquate size
	char summary[strsize];
	//add command to string
	sprintf(summary, "GAMEOVER\n");
	int idx = print_size[0]; //start of str after gameover

	// for each player summary print the summary to the string
	for (int i = 0; i < game.num_players; i++) {
		player_t* player = game.grid->players[i];
		// prints to the next position after the previous print
		sprintf(&summary[idx], "%-10s %c %-5d\n", player->player_name, player-> player_tag, player->gold_obtained);
		// increment the start pointer after the message just printed
		idx += print_size[i+1];
	}

	// send the summary and quit command to each of the players still connected
	for (int i = 0; i < game.num_players; i++) {
		player_t* player = game.grid->players[i];
		if (!(player->player_quit)) {
			message_send(player->addr, summary);
			message_send(player->addr, "QUIT");
		}
	}

	// print the summary to the server screen
	printf("%s", summary);

	// send the summary and quit to spectator if any
	if (game.grid->spectator != NULL) {
		message_send(game.grid->spectator->addr, summary);
		message_send(game.grid->spectator->addr, "QUIT");
	}

}

// sends the display to each of the players and spectator
void send_board() {
	// update each player/spectator display to the most current state
	grid_display_board(game.grid);

	// for each player still connected send the board they would see
	for (int i = 0; i < game.num_players; i++) {
		player_t* player = game.grid->players[i];

		if (!(player->player_quit)) {
			//allocate for display + DISPLAY\n + null term
			char disp[strlen(player->display) + 9];
			sprintf(disp, "DISPLAY\n%s", player->display);
			message_send(player->addr, disp);
		}
	}

	// if there's a spectator send them the display
	if (game.grid->spectator != NULL) {
		char disp[strlen(game.grid->spectator->display) + 9];
		sprintf(disp, "DISPLAY\n%s", game.grid->spectator->display);
		message_send(game.grid->spectator->addr, disp);
	}
}

// method for removing a player from the game
// removed players are not removed from the player array
void player_remove(player_t* player) {
	// remove the player from the game board
	grid_remove_player(game.grid, player);
	// set the quit flag to true
	player->player_quit = true;
	// tell the player to quit
	message_send(player->addr, "QUIT");
}


/**************** validate_params ****************
 * Function checks if paramaters are valid
 * Input:   const int argc- number of arguments
 * 			const char *argv[]- arguments
 * checks if the file is readableTSE Crawler Grader Comments
 and if the number of arguments is correct
*/
int validate_params(const int argc, const char *argv[]) {
	// check if correct number of args
	if (argc != 2 && argc != 3) {
		printf("incorrect number of arguments! expected 1 or 2, but got %d\nusage: ./server mapfile [seed]\n", argc-1);
		return 2;
	}

    FILE* fp;
    //check if file exists
    if ((fp = fopen(argv[1], "r")) == NULL) {
        printf("File does not exist or is not readable!\n");
        return 1;
    }
    fclose(fp);

	// check if the second argument is an integer (seed)
	if (argc == 3) {
		int i;
		if (!str2int(argv[2], &i)|| i<0){
				printf("Second argument is not a valid integer! Seed must be nonengative 32 bit integer\n");
		 		printf("usage: ./server mapfile [seed]\n");
		 		return 3;
		}
	}
    return 0;
}

// gets a player from the address given
// from- address of player you're trying to retrieve
player_t* get_player_from_addr(const addr_t from) {
	player_t* player = NULL;
	// loop through all of the players checking for matches
	for (int i = 0; i < MaxPlayers; i++) {
		// reached the end of the players without match
		if (game.grid->players[i] == NULL) {
			return player;
		}
		if (message_eqAddr(game.grid->players[i]->addr, from)) {
			return game.grid->players[i];
		}
	}
	return player;
}

// frees the player memory (including spectator)
// player- player whose memory to free
void free_player(player_t* player) {
	// free the name and display
	free(player->player_name);
	free(player->display);
	// known is never null for players but is NULL for spectators
	if (player->known != NULL) {
		// free each individual row and the entire struct itself
		for (int i=0; i<game.grid->num_rows; i++) {
			free(player->known[i]);
		}
		free(player->known);
	}
	//free the player struct
	free(player);
}

void free_grid() {
	// if the spectator is not currently null free it
	if (game.grid->spectator != NULL) {
		free_player(game.grid->spectator);
	}
	// for all of the players in the game free them
	for (int i = 0; i < game.num_players; i++) {
		free_player(game.grid->players[i]);
	}

	// call grid_delete which deletes the cells in the grid
	// as well as the pointer to the array of players and itself
	grid_delete(game.grid);
}




/* ***************** str2int ********************** */
/*
 * Convert a string to an integer, returning that integer.
 * Returns true if successful, or false if any error. 
 * It is an error if there is any additional character beyond the integer.
 * Assumes number is a valid pointer.
 */
static bool str2int(const char string[], int *number)
{
  // The following is one of my favorite tricks.
  // We use sscanf() to parse a number, expecting there to be no following
  // character ... but if there is, the input is invalid.
  // For example, 1234x will be invalid, as would 12.34 or just x.
  char nextchar;
  return (sscanf(string, "%d%c", number, &nextchar) == 1);
}
