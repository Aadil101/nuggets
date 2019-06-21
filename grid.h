#ifndef __GRID_H
#define __GRID_H
#include <stdio.h>
#include <message.h>
#include <stdbool.h>


typedef struct cell cell_t;


typedef struct player {
	char* player_name;
	char player_tag;
	int gold_obtained;
	char* display;
	int **known;
	int row;
	int col;
	const addr_t addr;
	bool player_quit;
}
player_t;

typedef struct grid {
	int num_rows;
	int num_cols;
	int gold_remaining;
	int MaxPlayers;
	cell_t **cells;
	player_t** players;
	player_t* spectator;
} grid_t;

//creates and returns a new grid struct given filename, seed, min and max gold piles, total gold in grid, and max players in grid
grid_t *grid_new(char* filename, int seed, int min_gold_piles, int max_gold_piles, int total_gold, int MaxPlayers); 
//removes a given player from grid
void grid_remove_player(grid_t* grid, player_t* player);
//returns an int of a player's gold amount after movement
int grid_move(grid_t *grid, player_t *player, int row, int col);
//returns an int of a player's gold amount after moving to end boundary of grid
int grid_move_to_end(grid_t *grid, player_t *player, int row, int col);
//returns the largest integer value less than or equal to the log of the positive value of a given integer
int int_len(int i);
//adds a given player to grid
void grid_add_player(grid_t* grid, player_t* player);
//displays the grid board
void grid_display_board(grid_t *grid);
//deletes and frees a grid
void grid_delete(grid_t* grid);



#endif // _GRID_H_
