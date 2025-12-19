#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

#define DIGIT_COUNT 6

static void parse_input(char *input, uint32_t *const from, uint32_t *const to) {
  *from = strtoul(input, &input, 10);
  tlbt_assert_fmt(*input == '-', "expected '-', actual '%c' (%d)", *input, *input);
  *to = strtoul(input + 1, &input, 10);
}

inline static bool has_adjacent_pair(const uint8_t d0, const uint8_t d1, const uint8_t d2, const uint8_t d3,
                                     const uint8_t d4, const uint8_t d5) {
  return d0 == d1 || d1 == d2 || d2 == d3 || d3 == d4 || d4 == d5;
}

inline static bool has_exact_pair(const uint8_t d0, const uint8_t d1, const uint8_t d2, const uint8_t d3,
                                  const uint8_t d4, const uint8_t d5) {
  return (d0 == d1 && d1 != d2) || (d1 == d2 && d1 != d0 && d2 != d3) || (d2 == d3 && d2 != d1 && d3 != d4) ||
         (d3 == d4 && d3 != d2 && d4 != d5) || (d4 == d5 && d4 != d3);
}

static void solve(const uint32_t from, const uint32_t to, uint32_t *const part1, uint32_t *const part2) {
  uint32_t p1 = 0;
  uint32_t p2 = 0;

  const uint8_t first_digit_from = from / 100000;
  const uint8_t first_digit_to = to / 100000;

  // not pretty. could have built a recursive function but it's 6 fixed digits. can't be more direct than this
  for (uint8_t d0 = first_digit_from; d0 <= first_digit_to; ++d0) {
    const uint32_t d0_value = d0 * 100000;
    for (uint8_t d1 = d0; d1 <= 9; ++d1) {
      const uint32_t d1_value = d1 * 10000;
      for (uint8_t d2 = d1; d2 <= 9; ++d2) {
        const uint32_t d2_value = d2 * 1000;
        for (uint8_t d3 = d2; d3 <= 9; ++d3) {
          const uint32_t d3_value = d3 * 100;
          for (uint8_t d4 = d3; d4 <= 9; ++d4) {
            const uint32_t d4_value = d4 * 10;
            for (uint8_t d5 = d4; d5 <= 9; ++d5) {
              const uint32_t n = d0_value + d1_value + d2_value + d3_value + d4_value + d5;
              if (n < from)
                continue;
              if (n > to)
                goto done;

              p1 += has_adjacent_pair(d0, d1, d2, d3, d4, d5);
              p2 += has_exact_pair(d0, d1, d2, d3, d4, d5);
            }
          }
        }
      }
    }
  }

done:
  *part1 = p1;
  *part2 = p2;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  uint32_t from = 0;
  uint32_t to = 0;

  parse_input(input, &from, &to);
  free(input);

  uint32_t part1 = 0;
  uint32_t part2 = 0;
  solve(from, to, &part1, &part2);

  printf("%u\n", part1);
  printf("%u\n", part2);
}

