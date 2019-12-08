//
// 106403025
//

#include "constants.h"

// create window
WINDOW* win;
char key = UP_KEY;
int game_result = RUNNING;

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

// Send data
void* send_data(void* arg){
  int sockfd = *(int *) arg;
  struct timespec ts;
  // Send data to socket every 0.15s
  ts.tv_nsec = ((int)(REFRESH * 1000) % 1000)  * 1000000;
  while(game_result == RUNNING){
    nanosleep(&ts, NULL);
    int n = write(sockfd, &key, 1);
    if(n < 0) {
      error("[錯誤] 無法寫入socket.");
    }
  }
  return 0;
}

// Update the screen frolm received data
void* update_screen(void* arg){    
  int sockfd = *(int*) arg;
  int bytes_read;
  int game_map[HEIGHT][WIDTH];
  int map_size = HEIGHT * WIDTH * sizeof(game_map[0][0]);
  char map_buffer[map_size];
  int n;

  while(game_result == RUNNING){
    // Recieve the map status from server
    bytes_read = 0;
    bzero(map_buffer, map_size);
    while(bytes_read < map_size){
      n = read(sockfd, map_buffer + bytes_read, map_size - bytes_read);
      // Jumps if no more data
      if(n <= 0)goto end;
      bytes_read += n;
    }
    memcpy(game_map, map_buffer, map_size);

    clear();
    box(win, '+', '+');
    refresh();
    wrefresh(win);

    // Render every position in array
    for(int i = 1; i < HEIGHT-1; i++){
      for(int j = 1; j < WIDTH-1; j++){
        int current = game_map[i][j];
        int colour = abs(current) % 7;
        attron(COLOR_PAIR(colour));
        if((current > 0) && (current != FOOD)){               
          mvprintw(i, j, "  ");
          attroff(COLOR_PAIR(colour));
        }
        // For different headings
        else if ((current < 0) && (current != FOOD)){
          if(game_map[i-1][j] == -current)mvprintw(i, j, "..");
          else if(game_map[i+1][j] == -current)mvprintw(i, j, "**");
          else if(game_map[i][j-1] == -current)mvprintw(i, j, " :");
          else if(game_map[i][j+1] == -current)mvprintw(i, j, ": ");
          attroff(COLOR_PAIR(colour));
        }                
        else if (current == FOOD){ 
          attroff(COLOR_PAIR(colour));               
          mvprintw(i, j, "o");                    
        }
      }
    }
    refresh();
  }
  end: game_result = game_map[0][0];
  return 0;
}

int main(int argc, char *argv[]){
  // Create the socket instance
  int                 sockfd;
  struct sockaddr_in  serv_addr;
  struct hostent*     server;
  // Key Buffer
  char key_buffer;

  if (argc < 2){
    fprintf(stderr, "[錯誤] 請輸入\n\t %s [伺服器ip]\n 來進入貪食蛇\n", argv[0]);
    exit(0);
  }    

  // Getting socket descriptor 
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("[錯誤] 無法開啟socket!");
  }
    
  // Resolving host
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    fprintf(stderr,"[錯誤] 找不到主機\n");
    exit(0);
  }

  // Initialize socket
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(PORT);
  
  // Attempt connection with server
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    error("[錯誤] 無法連接主機");
  }
  // Create Window using NCURSE
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  start_color();
  use_default_colors();
  curs_set(0);
  
  win = newwin(HEIGHT, WIDTH, 0, 0);

  // Setting Colors
  init_pair(0, COLOR_WHITE, COLOR_BLUE);
  init_pair(1, COLOR_WHITE, COLOR_RED);
  init_pair(2, COLOR_WHITE, COLOR_GREEN);
  init_pair(3, COLOR_BLACK, COLOR_YELLOW);
  init_pair(4, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(5, COLOR_BLACK, COLOR_CYAN);
  init_pair(6, COLOR_BLACK, COLOR_WHITE);

  mvprintw((HEIGHT-20)/2 + 0, (WIDTH-58)/2," 使用方法:");
  mvprintw((HEIGHT-20)/2 + 2, (WIDTH-58)/2," ＊用 w, a, s, d 來進行操作");
  mvprintw((HEIGHT-20)/2 + 3, (WIDTH-58)/2," ＊場地上隨時都會有三個食物，吃了會變大");
  mvprintw((HEIGHT-20)/2 + 4, (WIDTH-58)/2," ＊不要撞牆、還有蛇的身體（包括自己）");
  mvprintw((HEIGHT-20)/2 + 5, (WIDTH-58)/2," ＊按 . 可以直接中離");
  mvprintw((HEIGHT-20)/2 + 7, (WIDTH-58)/2,"請按任意鍵開始...");
  getch();

  // Start writing to the server.
  make_thread(update_screen, &sockfd);
  make_thread(send_data, &sockfd);

  while(game_result == RUNNING){
    // Get user input with time out
    bzero(&key_buffer, 1);
    timeout(REFRESH * 1000);
    key_buffer = getch();
    key_buffer = toupper(key_buffer);
    if(key_buffer == '.'){
      game_result = INTERRUPTED;
      break;
    } else if((key_buffer == UP_KEY) || (key_buffer == DOWN_KEY) || (key_buffer == LEFT_KEY) || (key_buffer == RIGHT_KEY)){
      key = key_buffer;
    }
  }

  // Alert user's death
  WINDOW* announcement = newwin(7, 35, (HEIGHT - 7)/2, (WIDTH - 35)/2);
  box(announcement, 0, 0);
  mvwaddstr(announcement, 2, (35-21)/2, "遊戲結束：你死ㄌ");
  mvwaddstr(announcement, 4, (35-21)/2, "按任意鍵離開...");
  wbkgd(announcement,COLOR_PAIR(1));
  mvwin(announcement, (HEIGHT - 7)/2, (WIDTH - 35)/2);
  wnoutrefresh(announcement);
  wrefresh(announcement);
  sleep(2);
  wgetch(announcement);
  delwin(announcement);
  wclear(win);
  
  echo(); 
  curs_set(1);  
  endwin();
      
  // Closing connection
  close(sockfd);
  return 0;
}
