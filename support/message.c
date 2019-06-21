/* 
 * message - a UDP-based messaging module
 *
 * Provides a message-passing abstraction among Internet hosts.  Messages
 * are sent via UDP and are thus limited to UDP packet size, may be lost,
 * and may be reordered, but require no connection setup or teardown.
 * 
 * See message.h for detailed interface description for each function.
 * Depends on the 'log' module and thus must be linked with log.o.
 * 
 * Compile with -DUNIT_TEST for a standalone unit test; see below.
 * Depends on the 'file' module and thus must be linked with file.o.
 *
 * David Kotz - May 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <math.h>
#include "message.h"
#include "log.h"

/**************** file-local constants ****************/
/* See message.h for other constants (shared with users of this module).
 * We restrict our port numbers to the unreserved range; see
 * https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
 */
static const int MinPort = 1024;
static const int MaxPort = 65535;

/**************** file-local global variables ****************/
/* This is an example of a judicious use of a global variable.
 * This module provides init() and done() functions that allow it
 * to initialize and cleanup; specifically, it needs to open and close
 * a socket it will use for all the messaging operations. We keep the
 * socket number (a file descriptor) here inside the module, unseen by
 * any code outside this module, but convenient for use internally.
 * One disadvantage to this approach is that all users of this module
 * must work with the same socket, and thus the same port number,
 * but a more flexible approach would require a much more complex interface.
 */
static int ourSocket = 0;     // socket on which to receive messages

/***********************************************************************/
/**************** message_init ****************/
/* 
 * Set up a socket on which to receive messages; return the port number.
 * Invariant: ourSocket = 0 if we return with error, else ourSocket > 0.
 * Log error and return zero if any error.
 * See message.h for detailed description.
 */
int
message_init(FILE *logFP)
{
  log_init(logFP);

  // Have we already been initialized?
  if (ourSocket != 0) {
    log_v("message_init: called again, when already initialized");
    return 0;
  }

  // Create socket on which to listen (file descriptor)
  ourSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (ourSocket < 0) {
    log_e("message_init: error opening datagram socket");
    ourSocket = 0;
    return 0;
  }

  // Name socket using wildcards
  struct sockaddr_in self;  // our address
  self.sin_family = AF_INET;
  self.sin_addr.s_addr = INADDR_ANY;
  self.sin_port = 0;
  if (bind(ourSocket, (struct sockaddr *) &self, sizeof(self))) {
    log_e("message_init: binding socket name");
    close(ourSocket);
    ourSocket = 0;
    return 0;
  }

  // get our assigned address
  socklen_t selflen = sizeof(self); // length of our address
  if (getsockname(ourSocket, (struct sockaddr *) &self, &selflen)) {
    log_e("message_init: getting socket name");
    close(ourSocket);
    ourSocket = 0;
    return 0;
  }
  // extract our port number
  int port = ntohs(self.sin_port);
  log_d("message_init: ready at port '%d'", port);

  return port;
}

/**************** message_noAddr ****************/
/* 
 * Return an empty/nonexistent address.
 * See message.h for detailed description.
 */
addr_t
message_noAddr(void)
{
  // addr_t is identical to sockaddr_in
  struct sockaddr_in none;
  none.sin_family = 0;
  none.sin_port = 0;
  none.sin_addr.s_addr = 0;

  return none;
}

/**************** message_isAddr ****************/
/* 
 * Return true if this address is valid (in the context of this program).
 * See message.h for detailed description.
 */
bool
message_isAddr(const addr_t addr)
{
  // a valid address will be in Internet Family
  return (addr.sin_family == AF_INET);
}

/**************** message_eqAddr ****************/
/* 
 * Return true if the two addresses are equal.
 * See message.h for detailed description.
 */
bool
message_eqAddr(const addr_t a, const addr_t b)
{
  return 
    a.sin_family == b.sin_family
    && a.sin_port == b.sin_port
    && a.sin_addr.s_addr == b.sin_addr.s_addr;
}

/**************** message_setAddr ****************/
/* 
 * Convert a textual address into a correspondent address.
 * Return true if success, false if any error.
 * If success, the *addr is filled in.
 * See message.h for detailed description.
 */
bool
message_setAddr(const char *hostname, const char *portString, addr_t *addr)
{
  if (hostname == NULL || portString == NULL || addr == NULL) {
    log_v("message_setAddr called with NULL argument");
    return false;
  }
  
  // Look up the hostname
  struct hostent *hostp = gethostbyname(hostname);
  if (hostp == NULL) {
    log_s("message_setAddr: cannot resolve hostname '%s'", hostname);
    return false;
  }

  // parse out the port number
  int port = 0;               // the port number
  char nextchar;              // character after port number
  if (sscanf(portString, "%d%c", &port, &nextchar) != 1) {
    log_s("message_setAddr: bad port number %s", portString);
    return false;
  }

  if (port < MinPort || port > MaxPort) {
    log_d("message_setAddr: illegal port number '%d'", port);
    return false;
  }

  // Initialize fields of the address
  addr->sin_family = AF_INET;
  bcopy(hostp->h_addr_list[0], &addr->sin_addr, hostp->h_length);
  addr->sin_port = htons(port);
  
  return true;
}

/**************** message_send ****************/
/* 
 * Send a string message to the correspondent address.
 * See message.h for detailed description.
 */
void
message_send(const addr_t to, const char *message)
{
  if (ourSocket == 0) {
    log_v("message_send called before message_init");
    return; // error in usage of this function.
  }
  if (message == NULL) {
    log_v("message_send called with null message");
    return; // error in usage of this function.
  }
  if (sendto(ourSocket, message, strlen(message), 0,
	     (struct sockaddr *) &to, sizeof(to)) < 0) {
    log_e("message_send: error sending to datagram socket");
  }
}

/**************** message_loop ****************/
/* 
 * Loop forever, calling handler functions for stdin or socket,
 * as input is available from either.
 * Returns false on error or true if any of the handlers return true.
 * See message.h for detailed description.
 */
bool
message_loop(void *arg, const float timeout,
	     bool (*handleTimeout)(void *arg),
	     bool (*handleInput)  (void *arg),
             bool (*handleMessage)(void *arg, 
                                   const addr_t from, const char *buf))
{
  // check parameters
  if (ourSocket == 0) {
    log_v("message_loop called before message_init");
    return false; // error in usage of this function.
  }

  // set up for timeouts, if desired
  struct timeval *timerp = NULL; // stays null if no timeout desired
  struct timeval timer;          // timerp = &timer if timeout desired
  struct timeval timeoutval;     // timeval equivalent of parameter 'timeout'
  if (timeout > 0.0) {
    timeoutval.tv_sec  = floor(timeout);
    timeoutval.tv_usec = timeout - floor(timeout);
  }

  // loop until error or some handler indicates time to quit looping
  while (true) {
    // for use with select()
    fd_set rfds;        // set of file descriptors we want to read
    
    // Watch stdin (fd 0) and the socket to see when either has input.
    int nfds = 0;	      // number of file descriptors to monitor
    FD_ZERO(&rfds);	      // default to none
    if (handleInput != NULL) {
      FD_SET(0, &rfds);	      // monitor stdin
      nfds = 1;
    }
    if (handleMessage != NULL && ourSocket != 0) {
      FD_SET(ourSocket, &rfds); // monitor the socket
      nfds = ourSocket+1;	// highest-numbered fd in rfds
    }
    if (timeout > 0.0) {      // is timeout desired?
      timer = timeoutval;     // set the timer to the timeout value
      timerp = &timer;	      // pass that timer to select
    } else {
      timerp = NULL;	      // no timeout is desired
    }

    // Wait for input on either source
    int select_response = select(nfds, &rfds, NULL, NULL, timerp);
    // note: 'rfds' updated
    
    if (select_response < 0) {
      // some error occurred; this should not happen
      log_e("message_loop: select()");
      return false; // error
    } else if (select_response == 0) {
      // timeout occurred
      log_v("message_loop: select() timed out");
      if (handleTimeout != NULL && (*handleTimeout)(arg)) {
	break; // handler says to exit loop 
      }
    } else if (select_response > 0) {
      // some data is ready on either source, or both

      if (FD_ISSET(0, &rfds)) {
        // stdin has input ready
	log_v("message_loop: input ready on stdin");
        if (handleInput != NULL && (*handleInput)(arg)) {
          break; // handler says to exit loop 
        }
      }
      if (FD_ISSET(ourSocket, &rfds)) {
        // socket has input ready
	log_v("message_loop: message ready on socket");
        struct sockaddr_in sender;     // sender of this message
        struct sockaddr *senderp = (struct sockaddr *) &sender;
        socklen_t senderlen = sizeof(sender);  // must pass address to length
        char buf[message_MaxBytes]; // buffer for reading data from socket
        int nbytes = recvfrom(ourSocket, buf, message_MaxBytes-1, 
                              0, senderp, &senderlen);
        if (nbytes < 0) {
          // error, ignore it
          log_e("message_loop: receiving from socket");
        } else {
          buf[nbytes] = '\0';     // null terminate message string
          // where was it from?
          if (sender.sin_family != AF_INET) {
            // ignore it
            log_d("message_loop: non-Internet family %d\n", sender.sin_family);
          } else {
            // handle it
            log_s("message_loop: from host %s", inet_ntoa(sender.sin_addr));
            log_d("message_loop: from port %d", ntohs(sender.sin_port));
            log_s("message_loop: content:\n%s\n", buf);
            if (handleMessage != NULL && (*handleMessage)(arg, sender, buf)) {
              break; // handler says to exit loop 
            }
          }
        }
      }
    }
  }
  return true;
}

/**************** message_done ****************/
/* 
 * Clean up the message module, prior to exit.
 * See message.h for detailed description.
 */
void
message_done(void)
{
  if (ourSocket != 0) {
    close(ourSocket);
    ourSocket = 0;
  }
  log_v("message_done: message module closing down.");
}


/* ****************************************************************** */
/* ************************* UNIT_TEST ****************************** */
/* 
 * This unit test implements a simple 'chat' program. 
 * Two instances should be run, possibly on separate hosts:
 * The first should run with command line
 *   ./messagetest
 * It will print a port number and wait.
 * The second should run with the command line
 *   ./messagetest hostName portNumber
 * where hostName is the IP address or DNS hostname of the server running
 * the first instance, and portNumber is the number it printed.
 * The second will immediately send "hello!" to the first.
 * At that point, the first knows the address of the second.
 * Subsequently, the user on either side can type a line of input and
 * see it sent to the other side.
 * 
 * For a cleaner view, redirect the stderr and logging output to a file:
 *   ./messagetest 2>first.log
 *   ./messagetest 2>second.log hostName portNumber
 * 
 * ^D (EOF) to exit either side.
 */

#ifdef UNIT_TEST

#include "file.h" // for readlinep()
static bool handleTimeout  (void *arg);
static bool handleInput  (void *arg);
static bool handleMessage(void *arg, const addr_t from, const char *message);

int
main(const int argc, char *argv[])
{
  addr_t other; // address of the other side of this communication (init below)

  // initialize the logging module
  log_init(stderr);

  // initialize the message module
  int ourPort = message_init(stderr);
  if (ourPort == 0) {
    return 2; // failure to initialize message module
  }

  // check arguments
  const char *program = argv[0];
  if (argc == 1) {
    // in this case (no arguments) we don't yet know our correspondent
    printf("waiting on port %d for contact....\n", ourPort);
    other = message_noAddr(); // no correspondent yet
  } else if (argc != 3) {
    fprintf(stderr, "usage: %s hostname port\n", program);
    return 3; // bad commandline
  } else {
    // in this case, commandline provides address for 'other'
    const char *otherHost = argv[1];
    const char *otherPort = argv[2];
    if (message_setAddr(otherHost, otherPort, &other)) {
      // initiate communication
      message_send(other, "hello!");
      printf("Write a message....\n");
    } else {
      fprintf(stderr, "can't form address from %s %s\n", otherHost, otherPort);
      return 4; // bad hostname/port
    }
  }

  // Loop, waiting for input or for messages; provide two callback functions.
  // We use the 'arg' parameter to carry a pointer to 'other',
  // which allows handleMessage to change it and handleInput to use it.
  bool ok = message_loop(&other, 9, handleTimeout, handleInput, handleMessage);

  // shut down the modules
  message_done();
  log_done();
  
  return ok? 0 : 1; // status code depends on result of message_loop
}

/**************** handleTimeout ****************/
/* No input or message has arrived for a while.
 * We use 'arg' to carry an addr_t referring to the 'other' correspondent.
 * Return true if the message loop should exit, otherwise false.
 */
static bool
handleTimeout(void *arg)
{
  const addr_t *otherp = arg;
  message_send(*otherp, "hello?");
  return false;
}

/**************** handleInput ****************/
/* stdin has input ready; read a line and send it to the client.
 * We use 'arg' to carry an addr_t referring to the 'other' correspondent.
 * Return true if the message loop should exit, otherwise false.
 * e.g., return true if EOF was encountered on stdin, or fatal error.
 */
static bool
handleInput(void *arg)
{
  addr_t *otherp = arg;
  if (otherp == NULL) { // defensive
    log_v("handleInput called with arg=NULL");
    return true;
  }

  // read a line from stdin to use as our response
  char *response = readlinep();
  if (response == NULL) {
    return true; // EOF means we should break the loop
  }

  // try to send the line to our correspondent
  if (!message_isAddr(*otherp)) {
    log_v("handleInput called without a correspondent.");
    printf("You have no correspondent.\n");
    fflush(stdout);
    return false;
  } else {
    message_send(*otherp, response);
    free(response);
    return false;
  }
}

/**************** handleMessage ****************/
/* Datagram received; print it.
 * We use 'arg' to carry an addr_t referring to the 'other' correspondent.
 * If 'other' is a valid client, leave it unchanged.
 * If 'other' is not a valid client, update *from to this sender.
 * Return true if the message loop should exit, otherwise false.
 * e.g., return true if fatal error.
 */
static bool
handleMessage(void *arg, const addr_t from, const char *message)
{
  addr_t *otherp = (addr_t *)arg;
  if (otherp == NULL) { // defensive
    log_v("handleMessage called with arg=NULL");
    return true;
  }

  // this sender becomes our correspondent, henceforth
  *otherp = from;
       
  printf("[%s@%05d]: %s\n", 
         inet_ntoa(from.sin_addr), // IP address of the sender
         ntohs(from.sin_port),     // port number of the sender
         message);                 // message from the sender
  fflush(stdout);
  return false;
}

#endif // UNIT_TEST
