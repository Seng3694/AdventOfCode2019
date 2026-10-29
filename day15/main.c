#include <stdio.h>
#include <stdint.h>

#include "../common/fileutils.h"
#include "../common/intcode.h"

#define MAX_CODE_COUNT 6000

typedef enum droid_direction {
  DROID_DIRECTION_NORTH = 1,
  DROID_DIRECTION_SOUTH = 2,
  DROID_DIRECTION_WEST = 3,
  DROID_DIRECTION_EAST = 4,
} droid_direction;

typedef enum direction {
  DIRECTION_UP = 0,
  DIRECTION_RIGHT = 1,
  DIRECTION_DOWN = 2,
  DIRECTION_LEFT = 3,
  DIRECTION_COUNT,
} direction;

static const droid_direction direction_mapping[4] = {
    [DIRECTION_UP] = DROID_DIRECTION_NORTH,
    [DIRECTION_RIGHT] = DROID_DIRECTION_EAST,
    [DIRECTION_DOWN] = DROID_DIRECTION_SOUTH,
    [DIRECTION_LEFT] = DROID_DIRECTION_WEST,
};

static const direction opposite_direction_mapping[4] = {
    [DIRECTION_UP] = DIRECTION_DOWN,
    [DIRECTION_RIGHT] = DIRECTION_LEFT,
    [DIRECTION_DOWN] = DIRECTION_UP,
    [DIRECTION_LEFT] = DIRECTION_RIGHT,
};

typedef enum droid_status {
  DROID_STATUS_WALL = 0,
  DROID_STATUS_MOVE = 1,
  DROID_STATUS_MOVE_ON_OXYGEN = 2,
} droid_status;

typedef struct point {
  int32_t x;
  int32_t y;
} point;

typedef enum map_tile {
  MAP_TILE_UNDISCOVERED,
  MAP_TILE_EMPTY,
  MAP_TILE_WALL,
} map_tile;

typedef enum action_type {
  ACTION_TYPE_EXPLORE,
  ACTION_TYPE_MOVE,
} action_type;

typedef struct action {
  action_type type;
  direction dir;
} action;

static inline uint32_t point_hash(const point p) {
  uint32_t hash = 36591911;
  hash = (hash << 5) + hash + p.x;
  hash = (hash << 5) + hash + p.y;
  return hash;
}

static inline bool point_equals(const point a, const point b) {
  return a.x == b.x && a.y == b.y;
}

#define TLBT_KEY_T point
#define TLBT_VALUE_T map_tile
#define TLBT_VALUE_T_NAME tile
#define TLBT_STATIC
#define TLBT_HASH point_hash
#define TLBT_EQUALS point_equals
#define TLBT_BASE2_CAPACITY
#include "../ext/toolbelt/src/hashmap.h"

#define TLBT_KEY_T point
#define TLBT_STATIC
#define TLBT_HASH point_hash
#define TLBT_EQUALS point_equals
#define TLBT_BASE2_CAPACITY
#include "../ext/toolbelt/src/hashmap.h"

#define TLBT_T action
#define TLBT_NO_SORT
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

#define TLBT_T point
#define TLBT_NO_SORT
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

// TODO: create a separate more efficient representation for the map representation. but hashmap is fine for now
typedef struct context {
  tlbt_map_point_tile map;
  tlbt_set_point visited;
  tlbt_deque_action stack;
  tlbt_deque_point queue;
  point oxygen;
} context;

static const point moves[5] = {
    [DIRECTION_UP] = {0, -1},
    [DIRECTION_RIGHT] = {1, 0},
    [DIRECTION_DOWN] = {0, 1},
    [DIRECTION_LEFT] = {-1, 0},
};

static droid_status droid_move(intcode_machine_state *const state, const direction dir) {
  tlbt_assert_msg(state->input_queue.count == 0, "expected input queue to be empty");
  tlbt_deque_i64_push_back(&state->input_queue, direction_mapping[dir]);
  intcode_run(state);
  tlbt_assert_msg(state->output_queue.count == 1, "expected exactly one output element");
  droid_status status = *tlbt_deque_i64_peek_back(&state->output_queue);
  tlbt_deque_i64_pop_back(&state->output_queue);
  return status;
}

static void explore_map(int64_t *code, const uint32_t code_count, context *const ctx) {
  int64_t input_queue_buffer[8] = {0};
  int64_t output_queue_buffer[8] = {0};

  intcode_machine_state state = {
      .code = code,
      .code_count = code_count,
      .memory_capacity = MAX_CODE_COUNT,
  };
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_queue_buffer) / sizeof(input_queue_buffer[0]),
                      input_queue_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_queue_buffer) / sizeof(output_queue_buffer[0]),
                      output_queue_buffer);

  point droid_pos = {0};
  point dest = {0};

  tlbt_map_point_tile *map = &ctx->map;
  tlbt_deque_action *stack = &ctx->stack;

  tlbt_map_point_tile_clear(map);
  tlbt_deque_action_clear(stack);

  tlbt_map_point_tile_insert(map, droid_pos, MAP_TILE_EMPTY);

  for (direction dir = 0; dir < DIRECTION_COUNT; ++dir) {
    tlbt_deque_action_push_back(stack, (action){.type = ACTION_TYPE_EXPLORE, .dir = dir});
  }

  while (stack->count > 0) {
    action a = *tlbt_deque_action_peek_back(stack);
    droid_status status = {0};

    if (a.type == ACTION_TYPE_EXPLORE) {
      // remove the explore action because it will be handled here
      tlbt_deque_action_pop_back(stack);
    }

    if (a.type == ACTION_TYPE_MOVE) {
      // an move action only exists for backtracking. so backtrack here
      tlbt_deque_action_pop_back(stack);
      status = droid_move(&state, opposite_direction_mapping[a.dir]);
      droid_pos.x += moves[opposite_direction_mapping[a.dir]].x;
      droid_pos.y += moves[opposite_direction_mapping[a.dir]].y;
      tlbt_assert(status != DROID_STATUS_WALL);
      continue;
    }

    point new_pos = {droid_pos.x + moves[a.dir].x, droid_pos.y + moves[a.dir].y};
    status = droid_move(&state, a.dir);

    switch (status) {
    case DROID_STATUS_WALL:
      tlbt_assert_msg(a.type == ACTION_TYPE_EXPLORE,
                      "droid ran into a wall with unexpected action type. should have been 'explore'");
      if (!tlbt_map_point_tile_contains(map, new_pos)) {
        tlbt_map_point_tile_insert(map, new_pos, MAP_TILE_WALL);
      }

      break;
    case DROID_STATUS_MOVE_ON_OXYGEN:
      dest = new_pos;
      // fallthrough
    case DROID_STATUS_MOVE:
      // droid moved

      if (!tlbt_map_point_tile_contains(map, new_pos)) {
        tlbt_map_point_tile_insert(map, new_pos, MAP_TILE_EMPTY);
        droid_pos = new_pos;

        tlbt_deque_action_push_back(stack, (action){.type = ACTION_TYPE_MOVE, .dir = a.dir});
        const direction new_directions[3] = {a.dir, (a.dir + 1) % DIRECTION_COUNT, (a.dir + 3) % DIRECTION_COUNT};
        for (uint8_t i = 0; i < 3; ++i) {
          new_pos.x = droid_pos.x + moves[new_directions[i]].x;
          new_pos.y = droid_pos.y + moves[new_directions[i]].y;
          if (!tlbt_map_point_tile_contains(map, new_pos)) {
            tlbt_deque_action_push_back(stack, (action){.type = ACTION_TYPE_EXPLORE, .dir = new_directions[i]});
          }
        }
      } else {
        status = droid_move(&state, opposite_direction_mapping[a.dir]);
        droid_pos.x += moves[opposite_direction_mapping[a.dir]].x;
        droid_pos.y += moves[opposite_direction_mapping[a.dir]].y;
        tlbt_assert(status != DROID_STATUS_WALL);
      }

      break;
    }
  }

  ctx->oxygen = dest;
}

static uint32_t find_shortest_path(context *const ctx) {
  uint32_t count = 0;

  tlbt_deque_point *queue = &ctx->queue;
  tlbt_set_point *visited = &ctx->visited;
  tlbt_map_point_tile *map = &ctx->map;

  tlbt_deque_point_clear(queue);
  tlbt_set_point_clear(visited);

  tlbt_deque_point_push_back(queue, (point){0, 0});
  tlbt_set_point_insert(visited, (point){0, 0});

  while (queue->count > 0) {
    size_t current_count = queue->count;
    for (size_t i = 0; i < current_count; ++i) {
      point current = *tlbt_deque_point_at(queue, i);
      if (current.x == ctx->oxygen.x && current.y == ctx->oxygen.y) {
        goto done;
      }

      for (direction d = 0; d < DIRECTION_COUNT; ++d) {
        const point new_pos = {current.x + moves[d].x, current.y + moves[d].y};
        if (tlbt_set_point_contains(visited, new_pos)) {
          continue;
        }
        if (tlbt_map_point_tile_contains(map, new_pos)) {
          map_tile tile = {0};
          (void)tlbt_map_point_tile_get(map, new_pos, &tile);
          if (tile == MAP_TILE_EMPTY) {
            tlbt_deque_point_push_back(queue, new_pos);
            tlbt_set_point_insert(visited, new_pos);
          }
        }
      }
    }

    // TODO: should create a function to pop many
    for (size_t i = 0; i < current_count; ++i) {
      tlbt_deque_point_pop_front(queue);
    }

    ++count;
  }
done:
  return count;
}

static uint32_t solve_part1(int64_t *code, const uint32_t code_count, context *const ctx) {
  explore_map(code, code_count, ctx);
  return find_shortest_path(ctx);
}

static uint32_t solve_part2(context *const ctx) {
  uint32_t count = 0;

  tlbt_deque_point *queue = &ctx->queue;
  tlbt_set_point *visited = &ctx->visited;
  tlbt_map_point_tile *map = &ctx->map;

  tlbt_deque_point_clear(queue);
  tlbt_set_point_clear(visited);

  tlbt_deque_point_push_back(queue, ctx->oxygen);
  tlbt_set_point_insert(visited, ctx->oxygen);

  while (queue->count > 0) {
    size_t current_count = queue->count;
    for (size_t i = 0; i < current_count; ++i) {
      point current = *tlbt_deque_point_at(queue, i);

      for (direction d = 0; d < DIRECTION_COUNT; ++d) {
        const point new_pos = {current.x + moves[d].x, current.y + moves[d].y};
        if (tlbt_set_point_contains(visited, new_pos)) {
          continue;
        }
        if (tlbt_map_point_tile_contains(map, new_pos)) {
          map_tile tile = {0};
          (void)tlbt_map_point_tile_get(map, new_pos, &tile);
          if (tile == MAP_TILE_EMPTY) {
            tlbt_deque_point_push_back(queue, new_pos);
            tlbt_set_point_insert(visited, new_pos);
          }
        }
      }
    }

    // TODO: should create a function to pop many
    for (size_t i = 0; i < current_count; ++i) {
      tlbt_deque_point_pop_front(queue);
    }

    ++count;
  }

  // the last count increase is wrong. so substract one
  return count - 1;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  int64_t code[MAX_CODE_COUNT] = {0};
  uint32_t code_count = 0;

  intcode_parse_input(input, MAX_CODE_COUNT, code, &code_count);
  free(input);

// use static memory. the counts are from running it with dynamic memory first. these are safe values for my input
#define MAP_CAPACITY 4096
#define VISITED_CAPACITY 2048
#define STACK_CAPACITY 1024
#define QUEUE_CAPACITY 32

  context ctx = {0};
  tlbt_map_point_tile_key map_key_buffer[MAP_CAPACITY] = {0};
  map_tile map_value_buffer[MAP_CAPACITY] = {0};
  tlbt_map_point_tile_init(&ctx.map, MAP_CAPACITY, map_key_buffer, map_value_buffer);

  tlbt_set_point_key visited_key_buffer[VISITED_CAPACITY] = {0};
  tlbt_set_point_init(&ctx.visited, VISITED_CAPACITY, visited_key_buffer);

  action stack_buffer[STACK_CAPACITY] = {0};
  tlbt_deque_action_init(&ctx.stack, STACK_CAPACITY, stack_buffer);

  point queue_buffer[QUEUE_CAPACITY] = {0};
  tlbt_deque_point_init(&ctx.queue, QUEUE_CAPACITY, queue_buffer);

  uint32_t part1 = solve_part1(code, code_count, &ctx);
  uint32_t part2 = solve_part2(&ctx);

  printf("%u\n", part1);
  printf("%u\n", part2);
}

