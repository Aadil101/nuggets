/*
 * player.c – the client module for the nuggets game.
 *  Handles communication between the player/spectator and server.
 *
 * usage: ./player hostname port [playername]
 *
 * exit: 0 on normal run-through; 1 on error initializing message module;
 *  2 on usage error;
 *  3 on error connecting with host; 4 on fatal error from message loop
 *  5 on error from resizing screen during game
 *
 * foobarbaz, May 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include "support/message.h"
#include "support/log.h"

// Function Prototypes
void resize_handler(int sig);
void update_statusline();
void update_display();
bool handle_accept(const char *message);
bool handle_reject(const char *message);
bool handle_display(const char *message);
bool handle_grid(const char *message);
bool handle_gold(const char *message);
bool handle_quit();
bool handle_gameover(const char *message);
bool handle_stdin (void *arg);
bool handle_message (void *arg, const addr_t from, const char *message);
void initialize_curses();

static const int MaxNameLength = 50;   // max number of chars in playerName
static char tag;    // letter representing player on map
static char* map;   // pointer to string representing map
static int N;    // number of nuggets recently collected by player
static int P;    // total number of nuggets collected by player
static int R;    // number of nuggets remaining on map
static char is_player = false;      // false for spectator
static bool show_gold_received = false;  // to assist with status line display  
static char error_message[100];     // stores current error to be displayed on status line
static bool show_error_message = false; // to assist with status line display  

/* ***** resize_handler ***** */
// closes ncurses display and exits game
void resize_handler(int sig)
{
    endwin();
    fprintf(stderr, "Resizing display during game is not allowed.\n");
    exit(5);
}

/* ***** update_statusline ***** */
// refreshes display on top of screen, provides player w/ notifications
void update_statusline()
{
    // remember original position of cursor
    int row = 0; int col = 0;
    getyx(curscr, row, col);

    // reset current status line
    move(0, 0);
    clrtoeol();

    // display relevant game information for client
    if (is_player) {
        printw("Player %c has %d nuggets (%d nuggets unclaimed). ", tag, P, R);
        // momentarily display gold-received message
        if (show_gold_received && N != 0)
            printw("GOLD received: %d ", N);
    } else
        printw("Spectator: %d nuggets unclaimed. ", R); 

    // display error message if necessary
    if (show_error_message) {
        printw("%s ", error_message);
        move(row, col); // return cursor to original position
    }

    refresh();
}

/* ***** update_display ***** */
// refreshes map display
void update_display()
{
    move(1,0);  // prepare to display

    bool searching = true;  // currently searching for client's tag "@"
    int row = 1; int col = 0;   // to store tag location

    // iterate through characters in map string
    for (int i = 1; i < strlen(map); i++) {
        addch(map[i]);
        if (searching && map[i] == '\n') {  // move to next line
            row++;
            col = 0;
        } else if (map[i] == '@') { // tag found!
            searching = false;
        } else if (searching) { // move to next col
            col++;
        }
    }

    move(row, col); // move cursor to tag location
    refresh();
}

/* ***** handle_accept ***** */
// reads and saves player tag from acceptance message
// returns false for continue
// message: acceptance message from server
bool handle_accept(const char *message)
{
    const char *content = message + strlen("OK ");
    sscanf(content, "%c ", &tag);
    return false;
}

/* ***** handle_reject ***** */
// processes possible types of rejection messages
// returns true if break, false if continue
// message: rejection message from server (given optional explanatory text)
bool handle_reject(const char *message)
{
    // check if too-many-clients rejection message
    if (strlen(message) == 2) {
        // turn off curses display
        endwin();
        fprintf(stderr, "Unable to join game, too many players!\n");
        return true;
    // otherwise, display rejection message on status line
    } else {
        const char *content = message + strlen("NO ");
        strcpy(error_message, content);
        show_error_message = true;
        update_statusline();
        return false;
    }
}

/* ***** handle_grid ***** */
// determines if terminal window is large enough to handle map
// returns true if break, false if continue
// message: grid message from server (given numbers of rows and columns)
bool handle_grid(const char *message) 
{
    // determine window x and y
    int win_x = 0; int win_y = 0;
    getmaxyx(stdscr, win_y, win_x);

    // extract nrows and ncols from space-delimited message
    const char *content = message + strlen("GRID ");
    int nrows = 0; int ncols = 0;
    sscanf(content, "%d %d ", &nrows, &ncols);

    // check if grid is too big for window
    if (win_y < nrows + 1 || win_x < ncols + 1) {
        while (true){
            mvprintw(0, 0, "Your window must be at least %d high", nrows);
            mvprintw(1, 0, "Your window must be at least %d wide", ncols);
            mvprintw(2, 0, "Resize your window and press ENTER to continue.");
            int c = getch();
            if (c == '\n') {
                erase();
                getmaxyx(stdscr, win_y, win_x);
                if (win_y >= nrows + 1 && win_x >= ncols + 1)
                    break;
            }
        }
    }

    // begin listening for errant screen resize
    signal(SIGWINCH, resize_handler);

    return false;
}

/* ***** handle_display ***** */
// processes map sent by server to be displayed
// returns false for continue
// message: display message from server (given map in string form)
bool handle_display(const char *message)
{
    map = (char *) (message + strlen("DISPLAY")); 
    update_statusline();
    update_display();
    return false;
}

/* ***** handle_gold ***** */
// processes data on gold nuggets in game
// returns false for continue
// message: gold message from server (given 3 entries; see N, P, R declarations above)
bool handle_gold(const char *message)
{
    const char *content = message + strlen("GOLD ");
    sscanf(content, "%d %d %d ", &N, &P, &R);
    show_gold_received = true;
    return false;
}

/* ***** handle_quit ***** */
// closes game display for client
// returns true for break
bool handle_quit()
{
    // turn off curses display
    endwin();
    return true;
}

/* ***** handle_gameover ***** */
// closes display and provides client w/ game summary
// returns true for break
// message: gameover message from server (given string with players and scores)
bool handle_gameover(const char *message)
{
    // turn off curses display
    endwin();

    // display gameover messaage
    const char *content = message + strlen("GAMEOVER");
    fprintf(stdout, "GAME OVER:");
    fprintf(stdout, "%s", content);

    return true;
}

/* ***** handle_stdin ***** */
// processes keystrokes from client
// returns true if break, false if continue
// arg: argument passed (assume address)
bool handle_stdin (void *arg)
{
    addr_t *otherp = arg;

    if (otherp == NULL) { // defensive
        log_v("handle_stdin called with arg=NULL");
        return true;
    }

    // try to send the line to our correspondent
    if (message_isAddr(*otherp)) {
        // to steer cursor
        int c = getch();    // read one character
        char message[30];
        sprintf(message, "KEY %c", c);
        message_send(*otherp, message); // communicate to server
    } else {
        log_v("handle_stdin called without a correspondent.");
        fprintf(stderr, "You have no correspondent.\n");
        fflush(stdout);
    }

    // forget old status line messages, new move has begun!
    show_gold_received = false;
    show_error_message = false;

    return false;
}

/* ***** handle_message ***** */
// processes messages received from server
// returns true if break, false if continue
// arg: argument passed (assume address)
// from: address message is received from
// message: message contents
bool handle_message (void *arg, const addr_t from, const char *message)
{
    addr_t *otherp = (addr_t *)arg;
    if (otherp == NULL) { // defensive
        log_v("handle_message called with arg=NULL");
        return true;
    }

    // this sender becomes our correspondent, henceforth
    if (strncmp(message, "OK ", strlen("OK ")) == 0)
        return handle_accept(message);
    else if (strncmp(message, "NO", strlen("NO")) == 0)
        return handle_reject(message);
    else if (strncmp(message, "GRID ", strlen("GRID ")) == 0)
        return handle_grid(message);
    else if (strncmp(message, "DISPLAY", strlen("DISPLAY")) == 0)
        return handle_display(message);
    else if (strncmp(message, "GOLD ", strlen("GOLD ")) == 0)
        return handle_gold(message);
    else if (strncmp(message, "QUIT", strlen("QUIT")) == 0)
        return handle_quit();
    else if (strncmp(message, "GAMEOVER", strlen("GAMEOVER")) == 0)
        return handle_gameover(message);

    return false;
}

/* ***** initialize_curses ***** */
/* initialize ncurses display on terminal window */
void 
initialize_curses()
{
    initscr(); // initialize the screen
    cbreak();  // accept keystrokes immediately
    noecho();  // don't echo characters to the screen
    start_color(); // turn on color controls
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // define color 1
    attron(COLOR_PAIR(1)); // set the screen color using color 1
    if (!is_player)  // hide cursor for spectator
        curs_set(0);
}

/* **************************************** */
int main(const int argc, const char *argv[])
{
    addr_t other; // address of the other side of this communication

    // initialize the logging module
    log_init(stderr);

    // initialize the message module
    int our_port = message_init(stderr);
    if (our_port == 0) {
        fprintf(stderr, "Unable to initialize message module.\n");
        exit(1);
    }

    // check usage
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: ./player hostname port [yourname] \n");
        exit(2); 
    }

    // handle playername (4th argument) if provided
    char playername[MaxNameLength + 1];
    if (argc == 4) {
        strncpy(playername, argv[3], MaxNameLength);
        playername[MaxNameLength] = '\0';
        is_player = true;
    }

    // set address of correspondent
    if (!message_setAddr(argv[1], argv[2], &other)) {
        fprintf(stderr, "Unable to initialize address to given hostname and port.\n");
        exit(3);
    }

    // initialize curses library
    initialize_curses();

    // client speaks first
    // determine appropriate message to join server with
    char message[10 + MaxNameLength];
    if (is_player) {
        strcpy(message, "PLAY ");
        strcat(message, playername);   // pass playername
    } else
        strcpy(message, "SPECTATE");
    message_send(other, message);
    
    // optional message loop parameters timeout and handleTimeout are left 0 and NULL 
    bool ok = message_loop(&other, 0, NULL, handle_stdin, handle_message);
    
    // shut down modules
    message_done();
    log_done();
    
    // status code depends on result of message_loop
    exit(ok ? 0 : 4);
}
