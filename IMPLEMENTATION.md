# fooBarBaz Nugget Project Implementation Spec

### About
<p> The nugget server reads the board, initializes the game, and then allows players to connect and play. The dynamics and control behind the game happen on the server side, while the inputs are given by and grid output is displayed to the client. All game functionality runs through the server. </p>


### Components
#### Server
* `static struct game`
    * Holds a `struct grid` for mapping purposes.
    * Holds `int num_players` which tracks current number of players.

* `struct grid`
    * Holds `int gold_remaining` which tracks remianing gold nuggets.
    * Contains mapping for the game struct with `int num_rows` and `int num_cols` storing number of rows and columns in map respectively.
    * Holds a 2D array of containing `cell` structures.
    * `int MaxPlayers` is the maximum number of players in the game
    * `player_t** players`Holds an array of non-spectator `player` structures.
    * `player *spectator*` holds a spectator `player` structure.

* `struct cell`
    * Contains properties of a cell object within the grid mapping of the game.
    * `char tag`, which holds the tag of the player occupying the cell. If no player is there, it holds `'\0'`.
    * `int gold`, which is the amount of gold in that cell. 0 if no gold. 
    * The default character to display when no character or gold is occcupying a cell is `default_char`.
    * Holds boolean indicating whether cell `is_walkable`.
    * `int row` is the row of th cell.
    * `int col` is the column.

* `struct player` (each held within an array)
    * Contains `char* player_name` which is the specific player's name.
    * Holds `char player_tag` which is the label  displayed on screen representing this player.
    * Holds `int gold_obtained` which indicates number of gold that has been obtained.
    * Contains `int row` which indicates the player's row position in the grid.
    * Contains `int col` which indicates the player's column position in the grid.
    * `addr_t* addr` is the address of connection to player. The `message.c` module from the support library will be used to communicate with the client.
    * Holds `char *display` which is the string which the player needs to output for the grid.
    * Holds a 2D array `int **known`, where `known[row][col] == 1` if the cell is known to the player, and `known[row][col] == 0` if it is not known.
    * Contains `bool player_quit` which tracks whether the player has disconnected.

### Psuedocode
#### Client module
<p> Reads and validates the path to logfile, hostname, port, and name. Client connects to the hostname and port, sending a message with the player’s name. Client listens for a char array from the server representing the display of the board. Client writes this display to the terminal window if window is large. Client listens for key inputs by the player, sends it to the server, and then handles the server's responses. If the player sends an EOF, the client exits. The client keeps updating until the server sends an end-of-game command. The client then listens for the scores of each player and displays them to the user. Finally, the client exits. </p>

* `main`
    * Attempts to initialize log file and message module.
    * Validates usage, namely hostname, port, and playername (if given).
    * If given playername and it exceeds maximum length, truncate automatically.
    * Attempts to set address to given hostname and port.

    * Initializes ncurses display.
    * Depending on whether client is player or spectator, sends appropriate join message to server.
    * Begins message loop by passing `handle_stdin` and `handle_message` methods.
    * Shut down message module and closes log module
    * Exits with status code 0 if message loop ends due to handler return true; otherwise, exit with status code 4 due to fatal error for which message loop could not keep looping.

* `bool handle_stdin(void *arg)`
    * If argument, ie. address to correspondent, is null, return true to break.
    * If address is valid, upon key press, sends message containing key to server.
    * Otherwise, if address is invalid, notifies user.
    * Return false to exit.

* `bool handle_message(void *arg)`
    * If argument, ie. address to correspondent, is null, return true to break.
    * If message begins with "OK ", return `handle_accept(message)`.
    * Otherwise, if message begins with "NO ", return `handle_reject(message)`.
    * Otherwise, if message begins with "GRID ", return `handle_grid(message)`.
    * Otherwise, if message begins with "DISPLAY", return `handle_display(message)`.
    * Otherwise, if message begins with "GOLD ", return `handle_gold(message)`.
    * Otherwise, if message begins with "QUIT", return `handle_quit(message)`.
    * Otherwise, if message begins with "GAMEOVER", return `handle_gameover(message)`.
    * Otherwise, return false

* Helper Modules
    * `void initialize_curses()`
        *  Initializes ncurses screen, accepts keyboard input, sets colors of background and characters on screen.
        *  If client is spectator, hides cursor.  
    * `void update_statusline(const char *message)`
        * Stores current location of cursor.
        * If client is player,
	        * Updates status line detailing status of the player, including player tag, nuggets claimed, and total nuggets unclaimed in map.
	        * If nuggets were recently picked up by player, displays amount on status line.
	     * Otherwise, if client is spectator, updates status line detailing nuggets unclaimed in map.
	     * If error message from server is received,
	        * Appends status line with error message.
	        * Returns cursor to original location.
    * `void update_display`
        * For each character in map string,
	        * Displays character on screen in appropriate location.
	        * If character is client's tag (@), saves its specific location on map.
	     * Moves cursor to hover over client's tag.

    * `bool handle_accept(const char *message)`
        * Scans acceptance message for character indicating player's tag on map.
        * Return false.
    * `bool handle_reject(const char *message)`
        * If rejection message is _exactly_ "NO "
	        * Closes display.
	        * Notifies client that unable to join becase too many players are on server.
	        * Return true to break.

        * Otherwise, uses `update_statusline` to notify client with rejection message. Returns false.
    * `bool handle_grid(const char *message)`
        * Extracts numbers of rows and columns in map from message.
        * If terminal window fits is too small for said dimensions,
	        * Notifies user to resize window.
	        * Waits until ENTER is pressed and window has been resized appropriately.
        * Begin listening for future window resizes (not allowed).
        * Return false.

    * `bool handle_display(const char *message)`
        * Stores map as string received as message.
        * Uses `update_statusline` and `update_display` to refresh screen.
        * Return false.
    * `bool handle_gold(const char *message)`
        * Extracts and stores nuggets claimed, nuggets unclaimed, and nuggets recieved from message.
        * Return false.
    * `bool handle_quit()`
        * Closes display.
        * Return true to break.
    * `bool handle_gameover(const char *message)` 
        * Closes display.
        * Displays game over message, ie. summary of how much gold each player in game collected.
        * Return true to break.

#### Server module
<p>The server module, establishes a grid, and handles communication between the grid and players via a message module.</p>

* `main`
    * Call `validate_params(const int argc, const char* argv[])` which validates that the map loaded is readable and the seed is valid (if any)
    * Once validated, create a new grid based on the input with a random int as seed if not specified by user
    * If the grid is NULL return with non-zero exit status.
    * If the number of columns * the number of rows + 10 is greater than or equal to 
    * Initialize the message module
    * Loop through the message module with `handle_message`
    * Free the grid and all of the memory used by it once message module handling is done
* `bool handle_message(void *arg, const addr_t from, const char *message)`
    * If the message equals "PLAY", pass that message onto `add_player` with address parameter `from`
    * Otherwise If the message equals "KEY", pass that message onto `process_keystroke` with address parameter `from`
    * Otherwise If the message equals "SPECTATE", pass that message onto `add_spectator` with address `from`
    * Otherwise, return false
* `void add_player(addr_t from, char* name)`
    * If the number of players is equal to maximum number players or the `from` address is already associated with a player stored in the array, send "NO" to `from` address and break to reject join request
    * Malloc and create a new player with a tag and name corresponding to their connection, along with a `gold_obtained` value of 0
    * malloc a display array for player, and `player_quit` property to false
    * send message to player client "OK <player_tag>" to signify acceptance
    * put the player at a random empty room spot on the grid with `grid_add_player`
    * send gold information to the new player
    * Increment the number of players by 1.
* `void process_keystroke(addr_t from, char keystroke)`
    * Check if there is currently a spectator and the message is from the spectator
    * If so, quit the spectator, free its memory, and set it equal to null
    * get the current player using its unique address
    * for error handling purposes, validate that the player is not null
    * initialize `gold_collected` to zero
    * map each key to the movement
    * if `key` is:
        * `h`, move left using `grid_move(grid, player, row, col)`
        * `j`, move down using `grid_move(grid, player, row, col)`
        * `k`, move up using `grid_move(grid, player, row, col)`
        * `l`, move right using `grid_move(grid, player, row, col)`
        * `y`, move diagonally up and left using using `grid_move(grid, player, row, col)`
        * `u`, move diagonally up and right using using `grid_move(grid, player, row, col)`
        * `b`, move diagonally down and left using `grid_move(grid, player, row, col)`
        * `n`, move diagonally down and right using `grid_move(grid, player, row, col)`
        * `H`, move left using `grid_move_to_end(grid, player,row, col)`
        * `J`, move down using `grid_move_to_end(grid, player,row, col)`
        * `K`, move up using `grid_move_to_end(grid, player,row, col)`
        * `L`, move right using `grid_move_to_end(grid, player,row, col)`
        * `Y`, move diagonally up and left using `grid_move_to_end(grid, player,row, col)`
        * `U`, move diagonally up and right using `grid_move_to_end(grid, player,row, col)`
        * `B`, move diagonally down and left using `grid_move_to_end(grid, player,row, col)`
        * `N`, move diagonally down and right using `grid_move_to_end(grid, player,row, col)`
        * `Q`, remove player with `player_remove`
    * otherwise, send "NO Invalid Key" as any other key is invalid
    * if we collected gold during the move, 
        * send a gold message to the player who collected the gold
        * send a message to the spectator with the updated gold count
        * send a message to the rest of the players with an updated gold count
* `void add_spectator(addr_t from)`
    * If there is already a current spectator, boot that spectator from the game and free them
    * malloc a new spectator and set it to the `from` address.
    * malloc the `display` and `player_name` properties of the spectator
    * set its location and `gold_obtained` properties to zero
    * set its known property to NULL
    * set the grid's spectator to this one
    * send grid and gold messages to spectator
        * "GRID <num_rows> <num_cols>"
        * "GOLD 0 0 <gold_remaining>"
* `void game_over()`
    * Send GAMEOVER summary to each player and spectator.
    * intialize a `strsize` int and `print_size` int array
    * set first element in print_size to 9, as that is the size of "GAMEOVER"
    * for `i` in the size of players
        * get current player
        * add size of playername, gold obtained, and player tag + 3 for spaces and newline to`print_size[i+1]`
        * add size of `print_size[i+1]` to `strsize`
    * create string of adequate size `summary`
    * add "GAMEOVER" command to `summary`
    * initialize a pointer `idx` with the first element of `print_size`
    * for each player summary print the summary to the string
        * print to the next position after the previous print to `summary`
        * increment the start pointer after the message just printed; sum the holder `idx` with the next element of `print_size`
    * send the summary and quit command to each of the players still connected
    * print the `summary` to the server screen
    * send the `summary` and "QUIT" message to the spectator if any 

* `void send_board()`
    * sends the display to each of the players and spectator
    * display the grid board with `grid_display_board`
    * for each player still connected, send the board they would see
        * if the player's `player_quit` property is false
            * send a message of display + "DISPLAY\n" to player address
    * If there's a spectator send them the display

* `void player_remove()`
    * remove the player from the game board
    * set the player's `player_quit` property to true
    * send a quit message to the player client
* `int validate_params(const int argc, const char *argv[])`
    * check if correct number of arguments
    * check if file exists
    * check if the second argument is an integer
    * return 0 if no error
* `player_t get_player_from_addr(const addr_t from)`
    * loop through all players until the player address property matches the address parameter and return that player, if no match return NULL
* `void free_player(player_t* player)`
    * free the player name and display
    * if the `known` property is not null
        * free each individual row within `known`
        * free the struct itself
    * free the player struct
* `void free_grid()`
    * if the spectator is not currently null, free it
    * for all the players in the game free them
    * call `grid_delete` which handles freeing for the grid
* `static bool str2int(const char string[], int *number)`
    * Convert a string to an integer, returning that integer.
    * Returns true if successful, or false if any error. 
    * It is an error if there is any additional character beyond the integer.
    * Assumes number is a valid pointer.

#### Grid module
<p>Defines grid structure with number of rows, number of columns, and 2D array of cells as parameters.
Initializes grid cells at beginning of game.
Updates grid cells during game runtime using given player moves.</p>

* `grid_t *grid_new(char* filename, int seed, int min_gold_piles, int max_gold_piles, int total_gold, int MaxPlayers)`
    * Allocate memory for grid
    * Assume that server is doing this error checking
    * hold the rows and cols of the map
    * initialize gold remaining
    * Allocate the array of cells
    * Populate the cell data structures
    * Along with `MaxPlayers` and `spectator`
    * Allocate memory for the array of players
    * check if there are enough spots to fit MaxPlayers and MaxGoldPiles
    * if not, free the current grid and return NULL
    * put gold in various piles in the grid with `grid_populate_gold`
    * close the map, and return the grid
* `static void generate_cells(cell_t **cells, FILE* map)`
    * set file pointer to beginning of map file
    * initialize row and column
    * loop through map
        * clear the standard output and move it to console
        * if the current character is a new line
            * increment row, and reset col
        * otherwise
            * create a new cell at the location with the current character
            * increment col
    * reset the file pointer back to beginning of map
* `static cell_t cell_new(char default_char, int row, int col)`
    * initialize cell object with the default character
    * if the default character is a # or a period
        * set the `is_walkable` property of the cell to true
    * otherwise, set the property to false
    * update the cell row and col to the parameters
    * initialize the gold count to 0
    * initialize tag to null
    * return the cell
* `static int calculate_rows(FILE* fp)`
    * If the file is null return 0
    * reset file pointer back to beginning of file
    * initialize a count for rows and character holder to iterate the file
    * loop through the file
        * if a newline, increment the row count
    * reset file pointer back to beginning of file
    * return the row count
* `static int calculate_cols(FILE* fp)`
    * if the file is null return 0
    * reset file pointer back to beginning of file
    * initialize a count for columns and character holder to iterate the file
    * loop through the file
        * if not a new line, increment the column count
    * reset file pointer back to beginning of file
    * return column count
* `static int calculate_dots(grid_t *grid)`
    * initialize a count for dots
    * loop through the number of rows
        * loop through the number of cols
            * if the current cell is a dot, increment dot count
    * return count for dots
* `static cell_t* get_cell(grid_t *grid, int dot_number)`
    * initialize a count for dots
    * loop through the number of rows
        * loop through the number of cols
            * if the cell is a dot and the dot number refers to that dot, return it's cell address
        * increment dot count if cell is a dot
    * return null of no dots were found
* `static void grid_populate_gold(grid_t* grid, int min_gold_piles, int max_gold_piles, int total_gold)`
    * set holder for number of dots in grid
    * set the number of gold piles in grid to random number based on max/min params
    * initialize int array for all the gold piles
    * set every element in int array to 1
    * set remaining gold to the difference of total amount of gold and the number of gold piles
    * randomly select and increment gold piles by the amount of remaining gold
    * for the number of gold piles
        * select a random cell in the grid
        * if the gold property in that cell is equal to 0
            * set the gold property in that cell to the count in a gold pile
            * move to another gold pile
* `void grid_remove_player(grid_t* grid, player_t* player)`
    * set the player tag to null
* `int grid_move(grid_t *grid, player_t *player, int row, int col)`
    * if the player would not be in bounds after move, don't make the move and return 0
    * hold the address of the cell the player would potentially move to
    * check if the move_to cell is not walkable. If not, exit by returning 0
    * hold the current cell the player is in
    * update the player location to the move location
    * save the gold amount of the move to cell, and then set the gold amount at that cell to 0
    * if the player would potentially move to empty square, set the tag at the move_to cell to the player_tag and remove the player tag at the current cell
    * otherwise, move to the cell that the other player is occupying, and swap locations and player tags
    * update remaining gold count in grid
    * update gold obtained by player
    * return the player's gold amount
* `int grid_move_to_end(grid_t *grid, player_t *player, int row, int col)`
    * hold current row and column of player
    * increment gold count by the movement of player with `grid_move`
    * loop through rows and cols in grid
        * if the current cell is visible
            * set the cell to known for the player
    * return gold count if at player location
    * return 0 if at end of method
* `int int_len(int i)`
    * if the int is zero, return 1
    * return the largest integer value less than or equal to the float (log of the positive value of the int param)
* `static bool grid_isVisible(grid_t *grid, int x1, int y1, int x2, int y2 )`
    * if the cell is empty return false
    * if in the same column
        * if starting row is above current row
            * move down vertically and check cells until at current cell
                * if the cell is not walkable, return false
        * return true if all cells in between are walkable
    * otherwise
        * move up vertically and check for cells until at current cell
            * if the cell is not walkable, return false
        * return true if all cells in between are walkable
    * if in the same row
        * if starting col is to the left of current col
            * move to the right until at current cell
                * if the cell is not walkable, return false
        * return true if none of the cells are not walkable
    * otherwise
        * move to the left until at the current cell
            * if the cell is not walkable, return false
        * return true if all cells in between are walkable
    * calculate the slope between the starting location and current location
    * if starting col is to the left of current col 
        * move to the right until at current cell
            * calculate row based on starting row, col, and slope 
                * check if there is a vertical wall at the current column and calculated row
                    * if so, return false
    * otherwise
        * move to the right until at current cell
            * calculate row based on starting row, col, and slope
            * check if there is a vertical wall at the current column and calculated row
                * if so, return false
    * if starting row is above the current col
        * move down vertically and check cells until at current cell
            * calculate col based on starting col, row, and slope
            * check if there is a horizontal wall at the current row and calculated column
                * if so, return false
    * otherwise
        * move up vertically and check cells until at current cell
            * calculate col based on starting col, row, and slope
            * check if there is a horizontal wall at the current row and calculated column
                * if so, return false
    * return true if all cells in between do not conflict with walls
* `static bool is_vertical_wall(grid_t *grid, int x, double y)`
    * if the largest integer value `j` is equal to the float value of the row
    	* return true, if the spot is a point in the grid
    * otherwise, the largest integer value is less than the float value of the row
    	* if `j` is equal to the last col
			* return true, if the spot is a point in the grid
	* otherwise 
		* return true, if the spot and the spot right below it are grid points
    * return false, if no grid point in grid found
* `static bool is_horizontal_wall(grid_t *grid, double x, int y)`
    * if the largest integer value `j` is equal to the float value of the col
    	* return true, if the spot is a point in the grid
    * otherwise, the largest integer value is less than the float value of the col
    	* if `j` is equal to the last row
			* return true, if the spot is a point in the grid
	* otherwise 
		* return true, if the spot and the spot to the right of it are points in the grid
    * return false, if no grid point in grid found
* `void grid_display_board(grid_t *grid)`
    * initialize player holder and spectator validator boolean
    * loop through max amount of players in grid
        * if at max players
            * set that player to spectator and set spectator validator to true
        * otherwise hold value of current player
        * if that player is null, continue iteration
        * hold the location of player and initialize display pointer
        * loop through grid
            * for each current cell
            * if that cell is equal to the player's location and is not a spectator
                * set that cell to known
                * use "@" sign to represent the current player
            * otherwise if a spectator or that cell is visible in grid for player
            * if the cell is just visible, set it to known for player
            * if that cell has gold, use "*" to represent it
            * if the cell is not empty
                * just display the tag at the cell
            * if the cell is empty, use default character at cell
        * otherwise if the cell is already known to player
            * just display the default character at that cell
        * if just a spectator, display empty space
        * increment display pointer
    * display a newline and increment display pointer
* `void grid_delete(grid_t* grid)`
    * Free each row of the cells 2D array
    * Free the `cells` and `players` property
    * Free the grid
* `void grid_add_player(grid_t* grid, player_t* player)`
    * get a random empty cell from grid
    * set the tag in that cell to the player's tag and set the player's location to that cell's location
* `static int calculate_empty_spots(grid_t *grid)`
    * loop through grid
        * if default char at cell is '.' and it has no tag or gold
            * increment number of empty spots
    * return number of empty spots
* `static cell_t* get_empty_cell(grid_t *grid, int dot_number)`
    * loop through grid
        * if default char at cell is '.' and it has no tag or gold
            * if it's dot number matches the current empty spot count
                * return that spot
            * increment empty spot count
    * return NULL if not found
* `static bool grid_in_bounds(grid_t *grid, int row, int col)`
    * return whether the location is not in bounds
### Security, error handling, and recovery
<p>Arguments are validated for the server and client. Specifically, the server validates that there is a correct number of arguments and that there is a readable file for the map filename passed. It also validates that the seed is an integer if provided one. The client reads and validates the path to logfile, hostname, port and name. The client sends all inputs from the player to the server–validation is done on the server side. We will conduct unit testing on server client interaction, the presentation of the game in the client module, the game control functionality from client to server, and the board display on the client end when players exit, the game ends, or players move as they obtain gold. Once all of those tests are passed, we can test the game as a whole.</p>
