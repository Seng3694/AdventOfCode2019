#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// grep -o '#' ./day10/input.txt | wc -l
#define MAX_ASTEROIDS 320

typedef struct point {
  int32_t x;
  int32_t y;
} point;

static void parse_input(char *input, point *const asteroids, uint32_t *const count) {
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t i = 0;

  for (;;) {
    switch (*input) {
    case '\0':
      *count = i;
      return;
    case '\n':
      // skip multiple newline
      while (*input == '\n')
        ++input;

      x = 0;
      ++y;
      break;
    case '#':
      tlbt_assert_fmt(i + 1 < MAX_ASTEROIDS, "too many asteroids. max: %u", MAX_ASTEROIDS);
      asteroids[i++] = (point){x++, y};
      ++input;
      break;
    case '.':
      ++x;
      ++input;
      break;
    default:
      tlbt_assert_unreachable();
      break;
    }
  }
}

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
#define TLBT_STATIC
#define TLBT_HASH_FUNC point_hash
#define TLBT_EQUALS_FUNC point_equals
#define TLBT_BASE2_CAPACITY
#include "../ext/toolbelt/src/hashmap.h"

static inline int32_t gcd(int32_t a, int32_t b) {
  while (b != 0) {
    uint32_t t = b;
    b = a % b;
    a = t;
  }
  return a;
}

static uint32_t solve_part1(const point *const asteroids, const uint32_t count, uint32_t *const asteroid_id) {
  uint32_t most_asteroids_in_line_of_sight = 0;
  uint32_t id = 0;

  tlbt_set_point_key buffer[512] = {0};
  tlbt_set_point set = {0};
  tlbt_set_point_init(&set, sizeof(buffer) / sizeof(buffer[0]), buffer);

  // for each asteroid, insert normalized direction into a set
  for (uint32_t i = 0; i < count; ++i) {
    point a = asteroids[i];
    tlbt_set_point_clear(&set);
    for (uint32_t j = 0; j < count; ++j) {
      if (i == j)
        continue;

      // get second asteroid and calculate the direction
      point b = asteroids[j];
      const int32_t dx = a.x - b.x;
      const int32_t dy = a.y - b.y;
      const int32_t g = gcd(abs(dx), abs(dy));
      const point dir = {dx / g, dy / g};
      const uint32_t hash = point_hash(dir);

      if (!tlbt_set_point_contains_ph(&set, dir, hash))
        tlbt_set_point_insert_ph(&set, dir, hash);
    }

    if (set.count > most_asteroids_in_line_of_sight) {
      most_asteroids_in_line_of_sight = set.count;
      id = i;
    }
  }

  *asteroid_id = id;
  return most_asteroids_in_line_of_sight;
}

typedef struct laser_target {
  uint32_t sqr_distance;
  uint32_t index;
  float angle;
} laser_target;

static inline int laser_target_compare(const laser_target *const left, const laser_target *const right) {
  // sort by angle first. then by distance
  const float angle_diff = left->angle - right->angle;
  if (angle_diff == 0.0f) {
    return (int)left->sqr_distance - (int)right->sqr_distance;
  }
  return angle_diff > 0 ? 1 : angle_diff < 0 ? -1 : 0;
}

#define TLBT_T laser_target
#define TLBT_T_NAME target
#define TLBT_NO_SORT
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

uint32_t solve_part2(const point *const asteroids, const uint32_t count, const uint32_t center_asteroid_id) {
  uint32_t solution = 0;

  const point center = asteroids[center_asteroid_id];

  laser_target targets[MAX_ASTEROIDS] = {0};
  uint32_t target_count = 0;
  for (uint32_t i = 0; i < count; ++i) {
    if (i == center_asteroid_id)
      continue;
    const point p = asteroids[i];

    const int32_t dx = p.x - center.x;
    const int32_t dy = p.y - center.y;
    const int32_t sqr_dist = dx * dx + dy * dy;
    // using floats in AoC is a bit weird, but still works. could
    float angle = atan2f(dx, -dy);
    if (angle < 0)
      angle += 2.0 * M_PI;
    targets[target_count++] = (laser_target){.sqr_distance = sqr_dist, .index = i, .angle = angle};
  }

  qsort(targets, target_count, sizeof(laser_target), (__compar_fn_t)laser_target_compare);

  // this only works because it doesn't require more than one rotation to find the 200th asteroid hit
  uint32_t buckets = 1;
  float previous_angle = targets[0].angle;
  for (uint32_t i = 1; i < target_count; ++i) {
    if (targets[i].angle != previous_angle) {
      ++buckets;
      previous_angle = targets[i].angle;

      if (buckets == 200) {
        const point twohundreth = asteroids[targets[i].index];
        solution = twohundreth.x * 100 + twohundreth.y;
        break;
      }
    }
  }

  return solution;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  point asteroids[MAX_ASTEROIDS] = {0};
  uint32_t count = 0;

  parse_input(input, asteroids, &count);
  free(input);

  uint32_t center_asteroid = 0;
  uint32_t part1 = solve_part1(asteroids, count, &center_asteroid);
  uint32_t part2 = solve_part2(asteroids, count, center_asteroid);

  printf("%u\n", part1);
  printf("%u\n", part2);
}

