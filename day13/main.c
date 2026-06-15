#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"
#include "../common/intcode.h"

#define MAX_CODE_COUNT 6000

typedef enum tile_type {
  TILE_TYPE_EMPTY = 0,
  TILE_TYPE_WALL = 1,
  TILE_TYPE_BLOCK = 2,
  TILE_TYPE_HORIZONTAL_PADDLE = 3,
  TILE_TYPE_BALL = 4,
} tile_type;

static uint32_t solve_part1(int64_t *const code, const uint32_t code_count) {
  int64_t input_buffer[4] = {0};
  // I should probably make it possible to interrupt the machine at certain output size intervals
  int64_t output_buffer[4096] = {0};
  intcode_machine_state state = {
      .code = code,
      .code_count = code_count,
      .memory_capacity = MAX_CODE_COUNT,
  };
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_buffer) / sizeof(input_buffer[0]), input_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_buffer) / sizeof(output_buffer[0]), output_buffer);

  const bool done = !intcode_run(&state);
  tlbt_assert_msg(done, "should finish in one run because it doesn't expect input yet");
  tlbt_assert_msg(state.output_queue.count > 0 && state.output_queue.count % 3 == 0,
                  "output queue should have a multiple of 3 elements");
  tlbt_assert(state.output_queue.head == 0); // just to make sure I can safely iterate it

  uint32_t block_tiles = 0;
  // start with the third element and move in a stride of 3. so just the types
  for (size_t i = 2; i < state.output_queue.count; i += 3) {
    block_tiles += state.output_queue.data[i] == TILE_TYPE_BLOCK;
  }

  return block_tiles;
}

static int64_t solve_part2(int64_t *const code, const uint32_t code_count) {
  int64_t input_buffer[4] = {0};
  int64_t output_buffer[4096] = {0};
  intcode_machine_state state = {
      .code = code,
      .code_count = code_count,
      .memory_capacity = MAX_CODE_COUNT,
  };
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_buffer) / sizeof(input_buffer[0]), input_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_buffer) / sizeof(output_buffer[0]), output_buffer);

  state.code[0] = 2;

  while (intcode_run(&state)) {
    tlbt_assert_msg(state.output_queue.count > 0 && state.output_queue.count % 3 == 0,
                    "output queue should have a multiple of 3 elements");

    int64_t ball_x = 0;
    int64_t paddle_x = 0;

    while (state.output_queue.count > 0) {
      const int64_t x = *tlbt_deque_i64_peek_front(&state.output_queue);
      tlbt_deque_i64_pop_front(&state.output_queue);
      tlbt_deque_i64_pop_front(&state.output_queue); // pop y value. there is no point in y right now
      const int64_t tile = *tlbt_deque_i64_peek_front(&state.output_queue);
      tlbt_deque_i64_pop_front(&state.output_queue);

      switch ((tile_type)tile) {
      case TILE_TYPE_HORIZONTAL_PADDLE:
        paddle_x = x;
        break;
      case TILE_TYPE_BALL:
        ball_x = x;
        break;
      default:
        break;
      }
    }

    // just move towards the ball. try keeping it below the ball as much as possible
    const int64_t move = (ball_x > paddle_x) - (ball_x < paddle_x);
    tlbt_deque_i64_push_back(&state.input_queue, move);
  }

  // go through the rest of the output queue after the intcode machine has finished and retrieve the score
  tlbt_assert_msg(state.output_queue.count > 0 && state.output_queue.count % 3 == 0,
                  "output queue should have a multiple of 3 elements");
  int64_t score = 0;
  while (state.output_queue.count > 0) {
    const int64_t x = *tlbt_deque_i64_peek_front(&state.output_queue);
    tlbt_deque_i64_pop_front(&state.output_queue);
    const int64_t y = *tlbt_deque_i64_peek_front(&state.output_queue);
    tlbt_deque_i64_pop_front(&state.output_queue);
    const int64_t tile = *tlbt_deque_i64_peek_front(&state.output_queue);
    tlbt_deque_i64_pop_front(&state.output_queue);

    if (x == -1 && y == 0) {
      score = tile;
    }
  }
  return score;
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

  uint32_t part1 = solve_part1(code, code_count);
  int64_t part2 = solve_part2(code, code_count);

  printf("%u\n", part1);
  printf("%ld\n", part2);
}

