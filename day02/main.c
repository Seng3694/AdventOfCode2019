#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// grep -Po '\d+' day02/input.txt | wc -l
#define MAX_POSITION_COUNT 125

static void parse_input(char *input, uint32_t *const positions, uint32_t *const position_count) {
  uint32_t count = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *position_count = count;
      return;
    case '\n':
    case ',':
      ++input;
      break;
    default:
      tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);
      tlbt_assert_fmt(count < MAX_POSITION_COUNT, "too many positions, max: %u", MAX_POSITION_COUNT);
      positions[count++] = strtoul(input, &input, 10);
      break;
    }
  }
}

static uint32_t run_intcode_machine(uint32_t *const positions, const uint32_t position_count, const uint32_t noun,
                                    const uint32_t verb) {
  positions[1] = noun;
  positions[2] = verb;

  uint32_t current = 0;
  for (;;) {
    switch (positions[current]) {
    case 99:
      goto done;
    case 1: {
      // ADD instruction. structure: ADD ai bi ci
      // add value from index ai to value from index bi and store in index ci
      tlbt_assert_msg(current + 3 < position_count && positions[current + 1] < position_count &&
                          positions[current + 2] < position_count && positions[current + 3] < position_count,
                      "ADD instruction would read/write out of bounds");
      const uint32_t ai = positions[current + 1];
      const uint32_t bi = positions[current + 2];
      const uint32_t ci = positions[current + 3];
      positions[ci] = positions[ai] + positions[bi];
      current += 4;
      break;
    }
    case 2: {
      // MUL instruction. structure: MUL ai bi ci
      // multiply value from index ai with value from index bi and store in index ci
      tlbt_assert_msg(current + 3 < position_count && positions[current + 1] < position_count &&
                          positions[current + 2] < position_count && positions[current + 3] < position_count,
                      "MUL instruction would read/write out of bounds");
      const uint32_t ai = positions[current + 1];
      const uint32_t bi = positions[current + 2];
      const uint32_t ci = positions[current + 3];
      positions[ci] = positions[ai] * positions[bi];
      current += 4;
      break;
    }
    default:
      tlbt_assert_unreachable();
      break;
    }
  }

done:
  return positions[0];
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  uint32_t positions[MAX_POSITION_COUNT] = {0};
  uint32_t position_count = 0;

  parse_input(input, positions, &position_count);
  free(input);

  uint32_t copy[MAX_POSITION_COUNT] = {0};
  memcpy(copy, positions, sizeof(uint32_t) * position_count);

  uint32_t part1 = run_intcode_machine(positions, position_count, 12, 2);

  uint32_t part2 = 0;
  for (uint8_t noun = 0; noun <= 99; ++noun) {
    for (uint8_t verb = 0; verb <= 99; ++verb) {
      memcpy(positions, copy, sizeof(uint32_t) * position_count);
      if (run_intcode_machine(positions, position_count, noun, verb) == 19690720) {
        part2 = 100 * noun + verb;
        goto done;
      }
    }
  }

done:
  printf("%u\n", part1);
  printf("%u\n", part2);
}

