#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

#define MAX_MODULE_MASSES_COUNT 100

static void parse_input(char *input, uint32_t *const module_masses, uint32_t *module_masses_count) {
  uint32_t count = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *module_masses_count = count;
      return;
    case '\n':
      ++input;
      break;
    default:
      tlbt_assert_fmt(isdigit(*input), "digit expected, actual '%c' (%d)", *input, *input);
      tlbt_assert_fmt(count < MAX_MODULE_MASSES_COUNT, "too many modules, max: %u", MAX_MODULE_MASSES_COUNT);
      module_masses[count++] = strtoul(input, &input, 10);
      tlbt_assert_fmt(*input == '\n' || *input == '\0', "new line or zero terminator expected, actual '%c' (%d)",
                      *input, *input);
      break;
    }
  }
}

static void solve(const uint32_t *const module_masses, const uint32_t module_masses_count, uint32_t *const part1,
                  uint32_t *const part2) {
  uint32_t p1 = 0;
  uint32_t p2 = 0;

  for (uint32_t i = 0; i < module_masses_count; ++i) {
    uint32_t module_mass = module_masses[i];
    int32_t fuel_mass = (module_mass / 3) - 2;
    p1 += fuel_mass;
    p2 += fuel_mass;

    fuel_mass = (fuel_mass / 3) - 2;
    while (fuel_mass > 0) {
      p2 += fuel_mass;
      fuel_mass = (fuel_mass / 3) - 2;
    }
  }

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

  uint32_t module_masses[MAX_MODULE_MASSES_COUNT] = {0};
  uint32_t module_masses_count = 0;

  parse_input(input, module_masses, &module_masses_count);
  free(input);

  uint32_t part1 = 0;
  uint32_t part2 = 0;
  solve(module_masses, module_masses_count, &part1, &part2);

  printf("%u\n", part1);
  printf("%u\n", part2);
}

