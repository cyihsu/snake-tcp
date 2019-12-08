//
// 106403025
//

#include "constants.h"
#include "types.h"

// Initialize the Map and types.
int map[HEIGHT][WIDTH];
int _map_size = HEIGHT * WIDTH * sizeof(map[0][0]);

// Thread Flags
pthread_mutex_t _map_lock = PTHREAD_MUTEX_INITIALIZER;   

// Snake initializer
snake* init_snake(int player_id, int head_y, int head_x) {
  // Lock the map and place the snake
  pthread_mutex_lock(&_map_lock);
  map[head_y][head_x]   = -player_id;
  map[head_y+1][head_x] = player_id;
  pthread_mutex_unlock(&_map_lock);

  // Allocate memory space
  snake* player = malloc(sizeof(snake));
  player->player_id = player_id;
  player->current_length = 3;

  for(int i = 0 ; i < player->current_length ; i++) {
    player->body[i].y = head_y + i;
    player->body[i].x = head_x;
    player->body[i].d = UP;
  }

  return player;
}

// Function to randomly add a food to the map
void add_food(){
  int x, y;
  do{
    y = rand() % (HEIGHT - 6) + 3;
    x = rand() % (WIDTH - 6) + 3;
  } while (map[y][x] != 0);
  pthread_mutex_lock(&_map_lock);
  map[y][x] = FOOD;
  pthread_mutex_unlock(&_map_lock);
}

// Remove Dead Snakes
void remove_snake(snake *player) {
  pthread_mutex_lock(&_map_lock);
  // Reset map status to empty
  for(int i = 0 ; i < player->current_length ; i++){
    map[player->body[i].y][player->body[i].x] = 0;
  }
  pthread_mutex_unlock(&_map_lock);

  // Free memory
  free(player);
  player = NULL;
}

void move_snake(snake* player, direction d, int ate){
  // Push body coordinations forward
  memmove(&(player->body[2]), &(player->body[1]), (player->current_length - 2) * sizeof(coordinate));
  player->body[1].x = player->body[0].x;
  player->body[1].y = player->body[0].y;
  player->body[1].d = player->body[0].d;

  // Move the head
  switch(d){
    case UP:{
      player->body[0].y = player->body[0].y-1;
      player->body[0].d = UP;
      if(ate){
        pthread_mutex_lock(&_map_lock);
        map[player->body[0].y][player->body[0].x + 1] = 0; 
        pthread_mutex_unlock(&_map_lock);
      }
      break;
    }
    case DOWN:{
      player->body[0].y = player->body[0].y+1;
      player->body[0].d = DOWN;
      if(ate){
        pthread_mutex_lock(&_map_lock);
        map[player->body[0].y][player->body[0].x + 1] = 0; 
        pthread_mutex_unlock(&_map_lock);
      }
      break;
    }
    case LEFT:{
      player->body[0].x = player->body[0].x-1;
      player->body[0].d = LEFT;
      break;
    }
    case RIGHT:{
      player->body[0].x = player->body[0].x+1;
      player->body[0].d = RIGHT;
      break;
    }
    default: break;
  }
  // Update current map status
  pthread_mutex_lock(&_map_lock);
  map[player->body[0].y][player->body[0].x] = -(player->player_id);
  map[player->body[1].y][player->body[1].x] = player->player_id;
  if(ate == 0){
    map[player->body[(player->current_length) - 1].y][player->body[(player->current_length) - 1].x] = 0;
  }
  pthread_mutex_unlock(&_map_lock);
  if(ate == 1){
    // Increment current length
    player->current_length++;
    add_food();
  }
  else {
    // Remove the tail element
    player->body[player->current_length - 1].y = player->body[player->current_length - 2].y;
    player->body[player->current_length - 1].x = player->body[player->current_length - 2].x;
  }
}

// Handle exit signal
void exit_handler(){
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  printf("[%d-%d-%d %d:%d:%d] Server closed.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  exit(0);
}

// Make a detachable thread
int make_thread(void* (*fn)(void *), void* arg){
  int             err;
  pthread_t       tid;
  pthread_attr_t  attr;

  err = pthread_attr_init(&attr);
  if(err != 0)return err;
  err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if(err == 0)err = pthread_create(&tid, &attr, fn, arg);
  pthread_attr_destroy(&attr);
  return err;
}

// Error Handler
void error(const char* msg){
  perror(msg);
  fflush(stdout);
  exit(1);
}

// Logic function
void* gameplay(void* arg){
  // Get current time
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  // Determine player_id from file descriptor
  int fd = *(int*) arg;
  int player_id = fd-3;
  printf("[%d-%d-%d %d:%d:%d] Player %d entered.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
  
  // Find available space for snake's birth.
  int head_y, head_x;
  srand(time(NULL));
  do {
    head_y = rand() % (HEIGHT - 6) + 3;
    head_x = rand() % (WIDTH - 6) + 3;
  } while (!(((map[head_y][head_x] == map[head_y+1][head_x]) == map[head_y+2][head_x]) == 0));

  // Create the user instance.
  snake* player = init_snake(player_id, head_y, head_x);

  // Variables for user input
  char key = UP;
  char key_buffer;
  char map_buffer[_map_size];
  int bytes_sent, n;
  int success = 1;

  while(success){
    // Renewing the timestamp
    t = time(NULL);
    tm = *localtime(&t);

    // Copy to buffer and send to client
    memcpy(map_buffer, map, _map_size);
    bytes_sent = 0;
    while(bytes_sent < _map_size){         
      bytes_sent += write(fd, map, _map_size);
      if (bytes_sent < 0) error("Cannot write to socket");
    } 

    // Receive Keystrokes
    bzero(&key_buffer, 1);
    n = read(fd, &key_buffer, 1);
    if (n <= 0)break;

    key_buffer = toupper(key_buffer);   
    if(((key_buffer == UP)    && !(player->body[0].d == DOWN))
      ||((key_buffer == DOWN)  && !(player->body[0].d == UP))
      ||((key_buffer == LEFT)  && !(player->body[0].d == RIGHT)) 
      ||((key_buffer == RIGHT) && !(player->body[0].d == LEFT))
    )
      key = key_buffer;

    switch(key){
      case UP:{
        if((map[player->body[0].y-1][player->body[0].x] == 0) && 
          !(map[player->body[0].y-1][player->body[0].x+1] == FOOD)){
          move_snake(player, UP, 0);
          printf("[%d-%d-%d %d:%d:%d] Player %d is heading up.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else if((map[player->body[0].y-1][player->body[0].x] == FOOD) || 
          (map[player->body[0].y-1][player->body[0].x+1] == FOOD)){
          move_snake(player, UP, 1);
          printf("[%d-%d-%d %d:%d:%d] Player %d ate a food.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else {
          move_snake(player, LEFT, 0);
          success = 0;
        }
        break;
      }

      case DOWN:{
        if((map[player->body[0].y+1][player->body[0].x] == 0)&& 
          !(map[player->body[0].y+1][player->body[0].x+1] == FOOD)){
          move_snake(player, DOWN, 0);
          printf("[%d-%d-%d %d:%d:%d] Player %d is heading down.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else if((map[player->body[0].y+1][player->body[0].x] == FOOD) || 
          (map[player->body[0].y+1][player->body[0].x+1] == FOOD)){
          move_snake(player, DOWN, 1);
          printf("[%d-%d-%d %d:%d:%d] Player %d ate a food.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else{
          move_snake(player, DOWN, 0);
          success = 0;
        }
        break;
      }

      case LEFT:{
        if(map[player->body[0].y][player->body[0].x-1] == 0){
          move_snake(player, LEFT, 0);
          printf("[%d-%d-%d %d:%d:%d] Player %d is heading left.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else if(map[player->body[0].y][player->body[0].x-1] == FOOD){
          move_snake(player, LEFT, 1);
          printf("[%d-%d-%d %d:%d:%d] Player %d ate a food.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else{
          move_snake(player, LEFT, 0);
          success = 0;
        }
        break;
      }

      case RIGHT:{
        if(map[player->body[0].y][player->body[0].x+1] == 0){
          move_snake(player, RIGHT, 0);
          printf("[%d-%d-%d %d:%d:%d] Player %d is heading right.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else if(map[player->body[0].y][player->body[0].x+1] == FOOD){
          move_snake(player, RIGHT, 1);
          printf("[%d-%d-%d %d:%d:%d] Player %d ate a food.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
        }
        else{
          move_snake(player, RIGHT, 0);
          success = 0;
        }
        break;
      }

      default: break;
    }
  }
  printf("[%d-%d-%d %d:%d:%d] Player %d is dead.\n", tm.tm_year + 1900, tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, player_id);
  remove_snake(player);
  close(fd);  
  return 0;
}

int main(){
  int                socket_fd[MAX_PLAYERS];     
  struct sockaddr_in socket_addr[MAX_PLAYERS];
  int                i;

  // Handle interrupt
  signal(SIGINT, exit_handler);

  // Init the map
  memset(map, 0, _map_size);
  for(i = 0; i < HEIGHT; i++)map[i][0] = map[i][WIDTH-2] = BORDER;     
  for(i = 0; i < WIDTH; i++)map[0][i] = map[HEIGHT-1][i] = BORDER;
  for(i = 0; i < 3; i++)add_food();

  // Create socket
  socket_fd[0] = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd[0] < 0)error("Cannot open a socket");
        
  // Socket Settings
  bzero((char *) &socket_addr[0], sizeof(socket_addr[0]));  
  socket_addr[0].sin_family = AF_INET;
  socket_addr[0].sin_addr.s_addr = INADDR_ANY;
  socket_addr[0].sin_port = htons(PORT);
  if (bind(socket_fd[0], (struct sockaddr *) &socket_addr[0], sizeof(socket_addr[0])) < 0){
    error("Cannot binding socket.");
  }

  listen(socket_fd[0], 5);
  socklen_t cli_len = sizeof(socket_addr[0]);

  for(i = 1;; i++){
    // Accepting requests
    socket_fd[i] = accept(socket_fd[0], (struct sockaddr *) &socket_addr[i], &cli_len);
    if (socket_fd[i] < 0) {
      error("Connection Refused.");
    }
    make_thread(&gameplay, &socket_fd[i]); 
  }
     
  // Closing the server socket
  close(socket_fd[0]);  
  return 0; 
}
