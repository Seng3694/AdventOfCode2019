#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"
#include "../common/intcode.h"

// grep -Po '\d+' day05/input.txt | wc -l
#define MAX_POSITION_COUNT 700

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  int64_t positions[MAX_POSITION_COUNT] = {0};
  uint32_t position_count = 0;

  intcode_parse_input(input, MAX_POSITION_COUNT, positions, &position_count);
  free(input);

  int64_t copy[MAX_POSITION_COUNT] = {0};
  memcpy(copy, positions, sizeof(int64_t) * position_count);

  int64_t input_buffer[4] = {0};
  int64_t output_buffer[1024] = {0};
  intcode_machine_state state = {
      .code = positions, .code_count = position_count, .memory_capacity = MAX_POSITION_COUNT};
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_buffer) / sizeof(input_buffer[0]), input_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_buffer) / sizeof(output_buffer[0]), output_buffer);

  tlbt_deque_i64_push_back(&state.input_queue, 1);
  intcode_run(&state);
  const int64_t part1 = *tlbt_deque_i64_peek_back(&state.output_queue);

  state.pc = 0;
  tlbt_deque_i64_clear(&state.input_queue);
  tlbt_deque_i64_clear(&state.output_queue);
  memcpy(state.code, copy, sizeof(uint64_t) * position_count);

  tlbt_deque_i64_push_back(&state.input_queue, 5);
  intcode_run(&state);
  const int64_t part2 = *tlbt_deque_i64_peek_back(&state.output_queue);

  printf("%ld\n", part1);
  printf("%ld\n", part2);
  return 0;
}

