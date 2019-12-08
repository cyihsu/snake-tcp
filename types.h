#include "constants.h"

typedef enum{
  UP = UP_KEY,
  DOWN = DOWN_KEY,
  LEFT = LEFT_KEY,
  RIGHT = RIGHT_KEY
} direction;

typedef struct{
  int x, y;
  direction d;
} coordinate;

typedef struct{
  int player_id, current_length;
  coordinate body[MAX_LENGTH];
} snake;
