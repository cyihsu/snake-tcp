// C libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

// Window libarary
#include <ncurses.h>

// For chinese locale
#include <locale.h>

// Threading and signaling
#include <signal.h>
#include <pthread.h>

// Socket Libraries
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

/*************************/

// screen size definitions
#define HEIGHT  24
#define WIDTH   80

// server-side limits
#define MAX_PLAYERS 7
#define MAX_LENGTH HEIGHT * WIDTH
#define PORT 7070

// keystrokes
#define UP_KEY    'W'
#define DOWN_KEY  'S'
#define LEFT_KEY  'A'
#define RIGHT_KEY 'D'

// map constants
#define FOOD    -1024
#define BORDER  -99
#define REFRESH 0.15
#define RUNNING     -34
#define INTERRUPTED -30