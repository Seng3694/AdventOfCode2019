#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../common/fileutils.h"
#include "../common/intcode.h"

// grep -Po '\d+' day09/input.txt | wc -l
// it's 973 but the task said to make sure there is way more memory
#define MAX_POSITION_COUNT 3000

static int64_t solve(const int64_t *const positions, const uint32_t position_count, const int64_t input) {
  int64_t copy[MAX_POSITION_COUNT] = {0};
  memcpy(copy, positions, sizeof(int64_t) * MAX_POSITION_COUNT);

  int64_t input_buffer[8] = {0};
  int64_t output_buffer[32] = {0};
  intcode_machine_state state = {.code = copy, .code_count = position_count, .memory_capacity = MAX_POSITION_COUNT};
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_buffer) / sizeof(input_buffer[0]), input_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_buffer) / sizeof(output_buffer[0]), output_buffer);

  tlbt_deque_i64_push_back(&state.input_queue, input);
  intcode_run(&state);

  return *tlbt_deque_i64_peek_back(&state.output_queue);
}

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

  const int64_t part1 = solve(positions, position_count, 1);
  const int64_t part2 = solve(positions, position_count, 2);

  printf("%ld\n", part1);
  printf("%ld\n", part2);
}

