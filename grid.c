/* 
 * grid.c - the grid for the nuggets game.
 *  Handles display and removal of items on map
 *
 * foobarbaz, April 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grid.h"
#include <math.h>
#include <memory.h>

//Cell struct for holding traits for places on map
typedef struct cell {
	char tag;
	int gold;
	char default_char;
	bool is_walkable;
	int row;
	int col;
} cell_t;


// Function Prototypes
static void generate_cells(cell_t **cells, FILE* map);
static cell_t cell_new(char default_char, int row, int col);
static int calculate_rows(FILE* fp);
static int calculate_cols(FILE* fp);
static int calculate_dots(grid_t *grid);
static cell_t* get_cell(grid_t *grid, int dot_number);
static void grid_populate_gold(grid_t* grid, int min_gold_piles, int max_gold_piles, int total_gold); //populate grid with gold
static cell_t* get_empty_cell(grid_t *grid, int dot_number);
static int calculate_empty_spots(grid_t *grid); 
static bool grid_in_bounds(grid_t *grid, int row, int col); //make sure grid is in bounds 
static bool is_horizontal_wall(grid_t *grid, double x, int y); //check whether the current x,y location has horizontal boundary 
static bool is_vertical_wall(grid_t *grid, int x, double y); //check whether the current x,y location hasvertical boundary 
static bool grid_isVisible(grid_t *grid, int x1, int y1, int x2, int y2 ); //check whether the current x2,y2 location is visible to starting x1,y1

/**************** grid_new ****************/
grid_t*
grid_new(char* filename, int seed, int min_gold_piles, int max_gold_piles, int total_gold, int MaxPlayers){

	grid_t *grid = malloc(sizeof(grid_t));	// Allocates memory for grid
	assertp(grid, "Error allocating memory to grid\n");

	FILE* map = fopen(filename, "r");	// Assume that server is doing this error checking
	grid->num_rows = calculate_rows(map);	// rows
	grid->num_cols = calculate_cols(map);	// Colums

	grid->gold_remaining = total_gold;	// intialize gold_remaining

	// Allocate the array of cells:
	cell_t ** cells = (cell_t **)malloc(grid->num_rows * sizeof(cell_t *));
	assertp(cells, "Error allocating memory to cells\n");

	for (int i = 0; i < grid->num_rows; ++i){
		cells[i] = (cell_t *)malloc(grid->num_cols * sizeof(cell_t));
		assertp(cells[i], "Error allocating memory to a cell row\n");
	}

	grid->cells = cells;
	generate_cells(grid->cells, map); // Function populates the cell datastructure
	grid->MaxPlayers = MaxPlayers;
	grid->players = calloc(MaxPlayers, sizeof(player_t));	// Allocates memory for array of players 
	assertp(grid->players, "Error Allocating memory to player array\n");

	grid->spectator = NULL;
	
	srand((unsigned)seed);

	// we check if there is a potential for us to not have enough spaces to populate gold and users.
	// if this is the case, we tell the server, free the grid, and exit.
	if (calculate_dots(grid) < (MaxPlayers + max_gold_piles)) {
		printf("Insufficient room spots to populate gold and users!\n");
		grid_delete(grid);
		fclose(map);
		return NULL;
	}

	grid_populate_gold(grid, min_gold_piles, max_gold_piles, total_gold);	// puts gold in various piles

	fclose(map);
	return grid;
}

/****************  generate_cells ****************/
// This function populates the cell struct
static void 
generate_cells(cell_t **cells, FILE* map){
	//set file pointer to beginning of map file
	rewind(map);
	//initialize row and column
	int row = 0;
	int col = 0;
	char c;
	//loop through map
	while ((c = fgetc(map)) != EOF) {
		fflush(stdout); //clear the standard output and move it to console
		if (c == '\n') { //if the current character is a new line, increment row and reset col
			row++;
			col = 0;
		}else{ //otherwise, create a new cell at the location with the current character and incrememnt col
			cell_t n_cell = cell_new(c, row, col);
			memcpy(&cells[row][col], &n_cell, sizeof(cell_t)); // Copies the generated cell into cells 2D array
			col++;
		}
	}
	rewind(map); //reset file pointer back to beginning of map
}



/**************** cell_new ****************/
static cell_t 
cell_new(char default_char, int row, int col){
	//initialize cell object with the default character
	cell_t cell; 
	cell.default_char = default_char;
	//if the default chaacter is a # or period set the is_walkable property of the cell to true
	if (default_char == '#'|| default_char == '.') {
		cell.is_walkable = true;
	} else{ //otherwise, set it to false
		cell.is_walkable = false;
	}
	cell.row = row; //update the cell row and col to the respective parameters
	cell.col = col;
	cell.gold = 0; //initialize gold count to 0
	cell.tag = '\0'; //initialize tag to null
	return cell; //return the cell

}

/**************** calculate_rows ****************/
// This is the same as the lines_in_file function from file.h
static int
calculate_rows(FILE* fp)
{	
	if (fp == NULL) { //if the file is null return 0
		return 0;
	}

	rewind(fp); //reset file pointer back to beginning of file

	int nlines = 0; //initialize a count for rows and character holder to iterate the file
	char c = '\0';
	while ( (c = fgetc(fp)) != EOF) { //loop through the file
		if (c == '\n') { //if a newline, increment the row count
			nlines++;
		}
	}

	rewind(fp); //reset file pointer back to beginning of file
	
	return nlines; //return the row count
}

/**************** calculate_cols ****************/
static int
calculate_cols(FILE* fp)
{	
	if (fp == NULL) { // if the file is null return 0
		return 0;
	}

	rewind(fp); //reset file pointer back to beginning of file

	int num_cols = 0; //initialize a count for columns and character holder to iterate the file
	char c = '\0';
	while ( (c = fgetc(fp)) != '\n') { //loop through the file
		num_cols++; //if not a new line, incrememnet the column count
	}

	rewind(fp); //reset file pointer back to beginning of file
	
	return num_cols; //return column count
}

/**************** calculate_dots ****************/
static int
calculate_dots(grid_t *grid)
{	
	int num_dots = 0; //initialize a count for dots
	for (int i = 0; i < grid->num_rows; i++){ //loop through the number of rows
		for (int j = 0; j < grid->num_cols; j++){//loop through the number of cols
			if (grid->cells[i][j].default_char == '.'){//if the current cell is a dot increment dot count			
				num_dots++;
			}
		}
	}
	return num_dots; //return num dots
}

/**************** get_cell ****************/
static cell_t*  
get_cell(grid_t *grid, int dot_number){
	int num_dots = 0; //initialize a count for dots
	for (int i = 0; i < grid->num_rows; i++){ //loop through the number of rows
		for (int j = 0; j < grid->num_cols; j++){ //loop through the number of cols
			if (grid->cells[i][j].default_char == '.'){ //if the cell is a dot and the dot number refers to that dot, return it's cell address
				if (dot_number == num_dots) { 
					return &(grid->cells[i][j]);
				}
				num_dots++; //incrememt num dots
			}
		}
	}
	return NULL; //return null if no dots were found
}


/**************** grid_populate_gold ****************/
static void 
grid_populate_gold(grid_t* grid, int min_gold_piles, int max_gold_piles, int total_gold){
	int num_dots = calculate_dots(grid); //set holder for number of dots in grid
	int num_gold_piles = (rand() % (max_gold_piles - min_gold_piles + 1)) + min_gold_piles; //set the number of gold piles in grid to random number based on max/min params
	int gold_in_piles[num_gold_piles]; //initialize int array for all the gold piles
	for (int i = 0; i < num_gold_piles; i++){ //set every element in int array to 1
		gold_in_piles[i] = 1;
	}

	int remaining_gold = total_gold - num_gold_piles; //set remaining gold to the difference of total amount of gold and the number of gold piles

	for (int i = 0; i < remaining_gold; i++) { //randomly select and increment gold piles by the amount of remaining gold
		int cell_number = rand() % num_gold_piles;
		gold_in_piles[cell_number]++;
	}

	for (int i = 0; i < num_gold_piles;) { //for the number of gold piles
		int dot_number = rand() % num_dots;
		cell_t *cell = get_cell(grid, dot_number); //select a random cell in the grid
		if (cell->gold == 0) { //if the gold property in that cell is equal to 0
			cell->gold = gold_in_piles[i]; //set the gold property in that cell to the count in a gold pile
			i++; //move to another gold pile
		}
	}
}

/**************** grid_remove_player ****************/
void 
grid_remove_player(grid_t* grid, player_t* player) {
	(grid->cells)[player->row][player->col].tag = '\0'; //set the cell tag of that player to null. Rest is handled by server.c
}


/**************** grid_move ****************/
int 
grid_move(grid_t *grid, player_t *player, int row, int col) {
	if (!grid_in_bounds(grid, player->row+row, player->col+col)) { //if the player would not be in bounds after move, don't make the move and return 0
		return 0;
	}

	cell_t* move_to = &(grid->cells)[player->row+row][player->col+col]; //hold the address of the cell the player would potentially move to

	if (!move_to->is_walkable) { //check if the move_to cell is not walkable. If so, return 0
		return 0;
	}

	cell_t* cur_cell = &((grid->cells)[player->row][player->col]); //otherwise, hold the current cell the player is in 

	player->row = player->row + row; //update the player location to the move location
	player->col = player->col + col;

	int gold_amt = move_to->gold; //save the gold amount of the move to cell, and then set the gold amount at that cell to 0
	move_to->gold = 0;
	//if the player would potentially move to empty square, set the tag at the move_to cell to the player_tag and remove the player tag at the current cell
	if (move_to->tag == '\0') { 
		move_to->tag = player->player_tag;
		cur_cell->tag = '\0';
	}
	else { //otherwise, move to the cell that the other player is occupying, and swap locations and player tags 
		player_t* swap_player = ((grid->players)[move_to->tag-'A']);
		cur_cell->tag = swap_player->player_tag;
		move_to->tag = player->player_tag;
		swap_player->row = player->row - row;
		swap_player->col = player->col - col;
	}
	grid->gold_remaining = grid->gold_remaining - gold_amt; //update remaining gold count in grid
	player->gold_obtained+= gold_amt; //update gold obtained by player
	return gold_amt; //return the player's gold amount
}


/**************** grid_move_to_end ****************/
int 
grid_move_to_end(grid_t *grid, player_t *player, int row, int col) {
	int cur_row = player->row; //hold current row and column of player
	int cur_col = player->col;

	int gold_count = 0;
	while (true) { 
		gold_count = gold_count + grid_move(grid, player, row, col);

		for (int x = 0; x < grid->num_rows; x++){ //loop through rows and cols in grid
			for (int y = 0; y < grid->num_cols; y++){
				if (grid_isVisible(grid, cur_row, cur_col, x, y)){ //if the current cell is visible
					player->known[x][y] = 1; //set the cell to known for the player
				}
			}
		}

		if ((player->row == cur_row) && (player->col == cur_col)) {  // If end has been reached
			return gold_count; //return total gold earned over move
		}
		cur_row = player->row; //update player location
		cur_col = player->col;
	}

	return 0;
}

/**************** int_len ****************/
int 
int_len(int i) {
	if (i == 0) {  //if the int is zero, return 1
		return 1;
	}
	return floor(log10(abs(i))) + 1; //return the largest integer value less than or equal to the float (log of the positive value of the int param)
}

/**************** grid_isVisible ****************/
static bool 
grid_isVisible(grid_t *grid, int x1, int y1, int x2, int y2 ){
	if (grid->cells[x2][y2].default_char == ' '){ //if the cell is empty return false
		return false;
	}

	if (x1 == x2){ //if in the same column
        if (y1< y2){  //if starting row is above current row
            for (int i = y1+1; i < y2; i++){  //move down vertically and check cells until at current cell
                if (!grid->cells[x1][i].is_walkable){ //if the cell is not walkable, return false
                    return false;
                }
            }
            return true; //return true if all cells in between are walkable
        } else { //otherwise
            for (int i = y1-1; i > y2; i--){ //move up vertically and check for cells until at current cell
                if (!grid->cells[x1][i].is_walkable){ //if the cell is not walkable, return false
                    return false;
                }
            }
            return true; //return true if all cells in between are walkable
        }
        
    }

    if (y1 == y2){ //if in the same row
        if (x1< x2){ //if starting col is to the left of current col
            for (int i = x1+1; i < x2; i++){ //move to the right until at current cell
                if (!grid->cells[i][y1].is_walkable){ //if the cell is not walkable, return false
                    return false;
                }
            }
            return true; //otherwise return true if none of the cells are not walkable
        } else { //otherwise
            for (int i = x1-1; i > x2; i--){ //move to the left until at the current cell
                if (!grid->cells[i][y1].is_walkable){ //if the cell is not walkable, return false
                    return false;
                }
            }
            return true; //return true if all cells in between are walkable
        }
    }

    double slope = ((double)(y2-y1))/(x2-x1); //calculate the slope between the starting location and current location

    if (x1 < x2){ //if starting col is to the left of current col 
        for (int i = x1; i < x2; i++){ //move to the right until at current cell
            double y = y1 + slope*(i-x1); //calculate row based on starting row, col, and slope 
            if (is_vertical_wall(grid, i, y)){ //check if there is a vertical wall at the current column and calculated row
                return false; //if so, return false
            }
        }
    } else{ //otherwise
         for (int i = x1; i > x2; i--){ //move to the right until at current cell
            double y = y1 + slope*(i-x1); //calculate row based on starting row, col, and slope
            if (is_vertical_wall(grid, i, y)){ //check if there is a vertical wall at the current column and calculated row
                return false; //if so, return false
            }
        }
    }

    if (y1 < y2){ //if starting row is above the current col 
        for (int i = y1; i < y2; i++){  //move down vertically and check cells until at current cell
            double x = x1 + (i-y1)/slope; //calculate col based on starting col, row, and slope
            if (is_horizontal_wall(grid, x, i)){ //check if there is a horizontal wall at the current row and calculated column
                return false; //if so, return false
            }
        }
    } else{ //otherwise 
         for (int i = y1; i > y2; i--){ //move up vertically and check cells until at current cell
            double x = x1 + (i-y1)/slope; //calculate col based on starting col, row, and slope
            if (is_horizontal_wall(grid, x, i)){ //check if there is a horizontal wall at the current row and calculated column
                return false;  //if so, return false
            }
        }
    }

    return true; //return true if all cells in between do not conflict with walls
    
}


/**************** is_vertical_wall ****************/
static bool 
is_vertical_wall(grid_t *grid, int x, double y){
    int j = (int)floor(y);
    if (floor(y) == y){	// if [x][y], is an exact grid point      
		if (grid->cells[x][j].default_char != '.'){
			return true;
		}

    } 
	else { // if it is between two grid points
		if (j == grid->num_cols - 1){
			if (grid->cells[x][j].default_char != '.'){
				return true;
			}
		} else{
			if (grid->cells[x][j].default_char != '.' && grid->cells[x][j+1].default_char != '.'){
            	return true;
        	}
		}
        
    }
    return false;
}


/**************** is_horizontal_wall ****************/
static bool 
is_horizontal_wall(grid_t *grid, double x, int y){
	int j = (int)floor(x);
    if (floor(x) == x){     // if [x][y], is an exact grid point      
		if (grid->cells[j][y].default_char != '.'){
			return true;
		}    
    } else{	// if [x][y] is between two grid points
		if (j == grid->num_rows-1){
			if (grid->cells[j][y].default_char != '.'){
				return true;
			}
		}else{
			if (grid->cells[j][y].default_char != '.' && grid->cells[j+1][y].default_char != '.'){
            return true;
        	}
		}
    }
    return false;
}


/**************** grid_display_board ****************/
void 
grid_display_board(grid_t *grid){
	player_t *player; //initialize player holder and spectator validator boolean
	bool is_spectator = false;
	for (int i = 0; i <= grid->MaxPlayers; i++){ //loop through max amount of players in grid
		if (i==grid->MaxPlayers) { //if at max players
			player = grid->spectator; //set that player to spectator and set spectator validator to true
			is_spectator = true;
		}
		else { //otherwise hold value of current player
			player = grid->players[i];
		}
		if (player == NULL) { //if that player is null, continue iteration
			continue;
		}
		int px = player->row; //hold the location of player and initialize display pointer
		int py = player->col;
		char *end_pointer = player->display; 
		for (int row = 0; row < grid->num_rows; row++){ //loop through grid
			for (int col = 0; col < grid->num_cols; col++){
				cell_t cell = grid->cells[row][col]; //for each current cell
				if (px == row && py == col && !is_spectator){ //if that cell is equal to the player's location and is not a spectator
					player->known[row][col] = 1; //set that cell to known
					sprintf(end_pointer, "@"); //use "@" sign to represent the current player
				}
				else if (is_spectator || grid_isVisible(grid, px, py, row, col)){ //otherwise if a spectator or that cell is visible in grid for player
					if (!is_spectator) { //if the cell is just visible, set it to known for player
						player->known[row][col] = 1;
					}
					if (cell.gold > 0){ //if that cell has gold, use "*" to represent it
						sprintf(end_pointer, "*");
					}
					else if (cell.tag != '\0'){ //if the cell is not empty
						sprintf(end_pointer, "%c", cell.tag); //just display the tag at the cell
					} else{ //if the cell is empty, use default character at cell
						sprintf(end_pointer, "%c", cell.default_char);
					}
					
				}
				else if(player->known[row][col] == 1){ //otherwise if the cell is already known to player
					sprintf(end_pointer, "%c", cell.default_char); //just display the default character at that cell
				}
				else{ //if just a spectator, display empty space
					sprintf(end_pointer, " ");
				}
				end_pointer++; //increment display pointer
			}
			sprintf(end_pointer, "\n"); //display a newline and increment display pointer
			end_pointer++;
		}
	}

}


/**************** grid_delete ****************/
void 
grid_delete(grid_t* grid) {
// Free each row of the cells 2D array
	for (int i = 0; i < grid->num_rows; ++i){
		free(grid->cells[i]);	
	}
	free(grid->cells); 
	free(grid->players);
	free(grid);
}


/**************** grid_delete ****************/
void 
grid_add_player(grid_t* grid, player_t* player) {
//get a random empty cell from grid
	int num_empty = calculate_empty_spots(grid);
	int spot_num = rand() % num_empty;
	cell_t *cell = get_empty_cell(grid, spot_num);
	//set the tag in that cell to the player's tag and set the player's location to that cell's location
	cell->tag = player->player_tag;
	player->row = cell->row;
	player->col = cell->col;
}


/**************** calculate_empty_spots ****************/
static int
calculate_empty_spots(grid_t *grid)
{	
	int num_empty = 0;
	for (int i = 0; i < grid->num_rows; i++){ //loop through grid
		for (int j = 0; j < grid->num_cols; j++){	
			if (grid->cells[i][j].default_char == '.' && grid->cells[i][j].tag == '\0' && grid->cells[i][j].gold == 0){	 //if default char at cell is '.' and it has no tag or gold
				num_empty++; //increment number of empty spots
			}
		}
	}
	return num_empty; //return number of empty spots
}


/**************** get_empty_cell ****************/
static cell_t* 
get_empty_cell(grid_t *grid, int dot_number){
	int num_empty = 0;
	for (int i = 0; i < grid->num_rows; i++){ //loop through grid
		for (int j = 0; j < grid->num_cols; j++){
			if (grid->cells[i][j].default_char == '.' && grid->cells[i][j].tag == '\0' && grid->cells[i][j].gold == 0){	 //if default char at cell is '.' and it has no tag or gold
				if (dot_number == num_empty) { //if it's dot number matches the current empty spot count
					return &(grid->cells[i][j]); //return that spot
				}
				num_empty++; //increment empty spot count
			}
		}
	}
	return NULL; //return NULL if not found
}


/**************** grid_in_bounds ****************/
static bool grid_in_bounds(grid_t *grid, int row, int col) {
	return !(row < 0 || col < 0 || row >= grid->num_rows || col >= grid->num_cols); //return whether the location is not in bounds
}
