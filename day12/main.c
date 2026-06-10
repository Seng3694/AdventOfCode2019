#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

#define MAX_MOONS 4

// SoA because it will make part 2 simpler
typedef struct moons {
  int32_t coordinate_x[MAX_MOONS];
  int32_t coordinate_y[MAX_MOONS];
  int32_t coordinate_z[MAX_MOONS];
  int32_t velocity_x[MAX_MOONS];
  int32_t velocity_y[MAX_MOONS];
  int32_t velocity_z[MAX_MOONS];
  uint8_t count;
} moons;

static inline bool is_digit(const char c) {
  return isdigit(c) || c == '-';
}

static void parse_coordinates(char *input, char **out, int32_t *const x, int32_t *const y, int32_t *const z) {
  // there is no point in writing a more sophisticated parser. so just hardcode expectations
  // format `<x=XCOORD, y=YCOORD, z=ZCOORD>`
  tlbt_assert_fmt(input[0] == '<', "expected '<' but found '%c'", input[0]);
  tlbt_assert_fmt(input[1] == 'x', "expected 'x' but found '%c'", input[1]);
  tlbt_assert_fmt(input[2] == '=', "expected '=' but found '%c'", input[2]);
  tlbt_assert_fmt(is_digit(input[3]), "expected digit but found '%c'", input[3]);
  *x = strtol(input + 3, &input, 10);

  tlbt_assert_fmt(input[0] == ',', "expected ',' but found '%c'", input[0]);
  tlbt_assert_fmt(input[1] == ' ', "expected ' ' but found '%c'", input[1]);
  tlbt_assert_fmt(input[2] == 'y', "expected 'y' but found '%c'", input[2]);
  tlbt_assert_fmt(input[3] == '=', "expected '=' but found '%c'", input[3]);
  tlbt_assert_fmt(is_digit(input[4]), "expected digit but found '%c'", input[4]);
  *y = strtol(input + 4, &input, 10);

  tlbt_assert_fmt(input[0] == ',', "expected ',' but found '%c'", input[0]);
  tlbt_assert_fmt(input[1] == ' ', "expected ' ' but found '%c'", input[1]);
  tlbt_assert_fmt(input[2] == 'z', "expected 'z' but found '%c'", input[2]);
  tlbt_assert_fmt(input[3] == '=', "expected '=' but found '%c'", input[3]);
  tlbt_assert_fmt(is_digit(input[4]), "expected digit but found '%c'", input[4]);
  *z = strtol(input + 4, &input, 10);

  tlbt_assert_fmt(input[0] == '>', "expected '>' but found '%c'", input[0]);
  *out = input + 1;
}

static void parse_input(char *input, moons *const m) {
  uint8_t i = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      m->count = i;
      return;
    case '\n':
      ++input;
      break;
    case '<':
      tlbt_assert_fmt(i + 1 <= MAX_MOONS, "too many moon coordinates in input file. max: %u", MAX_MOONS);
      parse_coordinates(input, &input, &m->coordinate_x[i], &m->coordinate_y[i], &m->coordinate_z[i]);
      ++i;
      break;
    }
  }
}

static void tick(moons *const m) {
  const uint8_t count = m->count;
  int32_t *x_pos = m->coordinate_x;
  int32_t *y_pos = m->coordinate_y;
  int32_t *z_pos = m->coordinate_z;
  int32_t *x_vel = m->velocity_x;
  int32_t *y_vel = m->velocity_y;
  int32_t *z_vel = m->velocity_z;

  // apply gravity
  for (uint8_t a = 0; a < count; ++a) {
    for (uint8_t b = 0; b < count; ++b) {
      if (a == b)
        continue;

      x_vel[a] += (x_pos[b] > x_pos[a]) - (x_pos[b] < x_pos[a]);
      y_vel[a] += (y_pos[b] > y_pos[a]) - (y_pos[b] < y_pos[a]);
      z_vel[a] += (z_pos[b] > z_pos[a]) - (z_pos[b] < z_pos[a]);
    }
  }

  // apply velocity
  for (uint8_t i = 0; i < count; ++i) {
    x_pos[i] += x_vel[i];
    y_pos[i] += y_vel[i];
    z_pos[i] += z_vel[i];
  }
}

static inline uint32_t calculate_total_energy(const moons *const m) {
  const uint8_t count = m->count;
  const int32_t *x_pos = m->coordinate_x;
  const int32_t *y_pos = m->coordinate_y;
  const int32_t *z_pos = m->coordinate_z;
  const int32_t *x_vel = m->velocity_x;
  const int32_t *y_vel = m->velocity_y;
  const int32_t *z_vel = m->velocity_z;

  uint32_t energy = 0;
  for (uint8_t i = 0; i < count; ++i) {
    const uint32_t potential_energy = abs(x_pos[i]) + abs(y_pos[i]) + abs(z_pos[i]);
    const uint32_t kinetic_energy = abs(x_vel[i]) + abs(y_vel[i]) + abs(z_vel[i]);
    energy += (potential_energy * kinetic_energy);
  }
  return energy;
}

static uint32_t find_cycle(moons *const m, int32_t *const coordinates, int32_t *const velocities) {
  tlbt_assert_msg(coordinates == m->coordinate_x || coordinates == m->coordinate_y || coordinates == m->coordinate_z,
                  "coordinates array should be part of the moon system");
  tlbt_assert_msg(velocities == m->velocity_x || velocities == m->velocity_y || velocities == m->velocity_z,
                  "velocities array should be part of the moon system");

  const uint8_t count = m->count;
  // store starting state in format: [xpos1, xvel1, xpos2, xvel2, ..., xposn, xveln]
  int32_t starting_state[MAX_MOONS * 2] = {0};

  for (uint8_t i = 0; i < count; ++i) {
    starting_state[i * 2] = coordinates[i];
    starting_state[i * 2 + 1] = velocities[i];
  }

  uint32_t ticks = 1;
  for (;; ++ticks) {
    tick(m);
    bool reached_starting_state = true;
    for (uint8_t i = 0; i < count; ++i) {
      if (coordinates[i] != starting_state[i * 2] || starting_state[i * 2 + 1] != velocities[i]) {
        reached_starting_state = false;
        break;
      }
    }
    if (reached_starting_state) {
      break;
    }
  }

  return ticks;
}

static inline uint64_t gcd(uint64_t a, uint64_t b) {
  while (b != 0) {
    uint64_t t = b;
    b = a % b;
    a = t;
  }
  return a;
}

static inline uint64_t lcm(const uint64_t a, const uint64_t b) {
  return (a * b) / gcd(a, b);
}

static void solve(moons *const m, uint32_t *const part1, uint64_t *const part2) {
  for (uint32_t i = 0; i < 1000; ++i) {
    tick(m);
  }
  *part1 = calculate_total_energy(m);

  const uint64_t ticks_until_cycle_x = find_cycle(m, m->coordinate_x, m->velocity_x);
  const uint64_t ticks_until_cycle_y = find_cycle(m, m->coordinate_y, m->velocity_y);
  const uint64_t ticks_until_cycle_z = find_cycle(m, m->coordinate_z, m->velocity_z);
  *part2 = lcm(ticks_until_cycle_x, lcm(ticks_until_cycle_y, ticks_until_cycle_z));
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  moons m = {0};

  parse_input(input, &m);
  free(input);

  uint32_t part1 = 0;
  uint64_t part2 = 0;
  solve(&m, &part1, &part2);

  printf("%u\n", part1);
  printf("%lu\n", part2);
}

