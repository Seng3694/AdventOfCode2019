#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"
#include "../common/intcode.h"

// grep -Po '\d+' day07/input.txt | wc -l
#define MAX_POSITION_COUNT 600

static void swap_u8(uint8_t *a, uint8_t *b) {
  uint8_t t = *a;
  *a = *b;
  *b = t;
}

static void reverse_u8(uint8_t *a, size_t lo, size_t hi) {
  while (lo < hi) {
    swap_u8(&a[lo], &a[hi]);
    ++lo;
    --hi;
  }
}

static bool permute_next(uint8_t *idx, size_t n) {
  if (n < 2)
    return false;

  size_t i = n - 2;
  while (idx[i] >= idx[i + 1]) {
    if (i == 0) {
      reverse_u8(idx, 0, n - 1);
      return false;
    }
    --i;
  }

  size_t j = n - 1;
  while (idx[j] <= idx[i]) {
    --j;
  }

  swap_u8(&idx[i], &idx[j]);
  reverse_u8(idx, i + 1, n - 1);
  return true;
}

static int64_t solve_part1(const int64_t *const positions, const uint32_t position_count) {
  int64_t copy[MAX_POSITION_COUNT] = {0};
  memcpy(copy, positions, sizeof(int64_t) * position_count);

  intcode_machine_state state = {.code = copy, .code_count = position_count, .memory_capacity = MAX_POSITION_COUNT};
  int64_t input_buffer[4] = {0};
  int64_t output_buffer[64] = {0};
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_buffer) / sizeof(input_buffer[0]), input_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_buffer) / sizeof(output_buffer[0]), output_buffer);

  int64_t biggest_signal = 0;
  uint8_t phase_settings[5] = {0, 1, 2, 3, 4};
  do {
    int64_t signal = 0;
    for (uint8_t i = 0; i < 5; ++i) {
      memcpy(copy, positions, sizeof(uint64_t) * position_count);
      state.pc = 0;
      tlbt_deque_i64_clear(&state.input_queue);
      tlbt_deque_i64_clear(&state.output_queue);
      tlbt_deque_i64_push_back(&state.input_queue, phase_settings[i]);
      tlbt_deque_i64_push_back(&state.input_queue, signal);
      intcode_run(&state);
      signal = *tlbt_deque_i64_peek_back(&state.output_queue);
    }

    if (signal > biggest_signal) {
      biggest_signal = signal;
    }
  } while ((permute_next(phase_settings, 5)));

  return biggest_signal;
}

static int64_t solve_part2(const int64_t *const positions, const uint32_t position_count) {
  intcode_machine_state states[5] = {0};
  int64_t input_buffers[5][64] = {0};
  int64_t output_buffers[5][64] = {0};
  int64_t copies[5][MAX_POSITION_COUNT] = {0};

  for (uint8_t i = 0; i < 5; ++i) {
    memcpy(copies[i], positions, sizeof(int64_t) * position_count);
    states[i].code = copies[i];
    states[i].code_count = position_count;
    states[i].memory_capacity = MAX_POSITION_COUNT;
    tlbt_deque_i64_init(&states[i].input_queue, sizeof(input_buffers[i]) / sizeof(input_buffers[i][0]),
                        input_buffers[i]);
    tlbt_deque_i64_init(&states[i].output_queue, sizeof(output_buffers[i]) / sizeof(output_buffers[i][0]),
                        output_buffers[i]);
  }

  int64_t biggest_signal = 0;
  const uint8_t phase_settings[5] = {5, 6, 7, 8, 9};
  uint8_t indices[5] = {0, 1, 2, 3, 4};
  do {
    for (uint8_t i = 0; i < 5; ++i) {
      states[i].pc = 0;
      tlbt_deque_i64_clear(&states[i].input_queue);
      tlbt_deque_i64_clear(&states[i].output_queue);
      tlbt_deque_i64_push_back(&states[i].input_queue, phase_settings[indices[i]]);
    }

    tlbt_deque_i64_push_back(&states[0].input_queue, 0);
    int64_t signal = 0;

    bool done = false;
    while (!done) {
      for (uint8_t i = 0; i < 5; ++i) {
        done = !intcode_run(&states[i]);
        tlbt_assert_msg(states[i].output_queue.count == 1, "expected output queue to have exactly one output");
        signal = *tlbt_deque_i64_peek_front(&states[i].output_queue);
        tlbt_deque_i64_push_back(&states[(i + 1) % 5].input_queue, signal);
        tlbt_deque_i64_pop_front(&states[i].output_queue);
      }
    }

    if (signal > biggest_signal) {
      biggest_signal = signal;
    }
  } while ((permute_next(indices, 5)));

  return biggest_signal;
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

  const int64_t part1 = solve_part1(positions, position_count);
  const int64_t part2 = solve_part2(positions, position_count);

  printf("%ld\n", part1);
  printf("%ld\n", part2);
  return 0;
}

