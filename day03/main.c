#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// awk -F',' '{print NF}' day03/input.txt
#define MAX_VERTEX_COUNT 320 // it's 301 in my input. add a bit of tolerance for other inputs

typedef struct vertex {
  int32_t x;
  int32_t y;
} vertex;

typedef struct context {
  vertex wire1[MAX_VERTEX_COUNT];
  vertex wire2[MAX_VERTEX_COUNT];
  uint32_t wire1_vertex_count;
  uint32_t wire2_vertex_count;
} context;

static void parse_wire_vertices(char *input, char **output, vertex *const vertices, uint32_t *const vertex_count) {
  uint32_t count = 0;
  vertex current = {0, 0}; // start at 0, 0
  vertices[count++] = current;
  for (;;) {
    switch (*input) {
    case '\0':
    case '\n':
      *vertex_count = count;
      *output = input;
      return;
    case ',':
      ++input;
      break;
    case 'L': // left
      tlbt_assert_fmt(isdigit(input[1]), "digit expected, actual '%c' (%d)", input[1], input[1]);
      tlbt_assert_fmt(count < MAX_VERTEX_COUNT, "too many vertices, max: %u", MAX_VERTEX_COUNT);
      current.x -= strtol(input + 1, &input, 10);
      vertices[count++] = current;
      break;
    case 'U': // up
      tlbt_assert_fmt(isdigit(input[1]), "digit expected, actual '%c' (%d)", input[1], input[1]);
      tlbt_assert_fmt(count < MAX_VERTEX_COUNT, "too many vertices, max: %u", MAX_VERTEX_COUNT);
      current.y -= strtol(input + 1, &input, 10);
      vertices[count++] = current;
      break;
    case 'R': // right
      tlbt_assert_fmt(isdigit(input[1]), "digit expected, actual '%c' (%d)", input[1], input[1]);
      tlbt_assert_fmt(count < MAX_VERTEX_COUNT, "too many vertices, max: %u", MAX_VERTEX_COUNT);
      current.x += strtol(input + 1, &input, 10);
      vertices[count++] = current;
      break;
    case 'D': // down
      tlbt_assert_fmt(isdigit(input[1]), "digit expected, actual '%c' (%d)", input[1], input[1]);
      tlbt_assert_fmt(count < MAX_VERTEX_COUNT, "too many vertices, max: %u", MAX_VERTEX_COUNT);
      current.y += strtol(input + 1, &input, 10);
      vertices[count++] = current;
      break;
    }
  }
}

static inline uint32_t manhattan_distance(const vertex a, const vertex b) {
  return abs(a.x - b.x) + abs(a.y - b.y);
}

static void parse_input(char *input, context *const ctx) {
  parse_wire_vertices(input, &input, ctx->wire1, &ctx->wire1_vertex_count);
  tlbt_assert_fmt(*input == '\n', "expected newline, actual '%c' (%d)", *input, *input);
  ++input;
  parse_wire_vertices(input, &input, ctx->wire2, &ctx->wire2_vertex_count);
}

static bool lines_intersect(vertex a1, vertex a2, vertex b1, vertex b2, vertex *const intersection) {
  const bool a_vertical = a1.x == a2.x;
  const bool b_vertical = b1.x == b2.x;

  if (a_vertical && b_vertical) {
    // the lines are parallel. there is the case of them being on top of each other which would result in many
    // "intersections". this actually happens with the input but omitting them didn't seem to cause any problems.
    // otherwise this function would have to return an array of intersections. or at least a range I can iterate. but
    // not needed, so I won't deal with it
    return false;
  } else if (!a_vertical && !b_vertical) {
    return false;
  }

  // make "a" the vertical
  if (!a_vertical) {
    vertex tmp = a1;
    a1 = b1;
    b1 = tmp;

    tmp = a2;
    a2 = b2;
    b2 = tmp;
  }

  tlbt_assert_fmt(a1.y != a2.y, "line can't be a point ({%d, %d}, {%d, %d})", a1.x, a1.y, a2.x, a2.y);
  tlbt_assert_fmt(b1.x != b2.x, "line can't be a point ({%d, %d}, {%d, %d})", b1.x, b1.y, b2.x, b2.y);

  // ensure direction
  if (a1.y > a2.y) {
    vertex tmp = a1;
    a1 = a2;
    a2 = tmp;
  }
  if (b1.x > b2.x) {
    vertex tmp = b1;
    b1 = b2;
    b2 = tmp;
  }

  if (a1.y <= b1.y && a2.y >= b1.y && b1.x <= a1.x && b2.x >= a1.x) {
    intersection->x = a1.x;
    intersection->y = b1.y;
    return true;
  }

  return false;
}

static uint32_t solve_part1(const context *const ctx) {
  uint32_t closest = UINT32_MAX;

  vertex central_port = {0, 0};
  vertex intersection = {0};
  for (uint32_t i = 0; i < ctx->wire1_vertex_count - 1; ++i) {
    // for each line segment in the first wire
    const vertex a1 = ctx->wire1[i];
    const vertex a2 = ctx->wire1[i + 1];
    for (uint32_t j = 0; j < ctx->wire2_vertex_count - 1; ++j) {
      // for each line segment in the second wire
      const vertex b1 = ctx->wire2[j];
      const vertex b2 = ctx->wire2[j + 1];
      if (lines_intersect(a1, a2, b1, b2, &intersection)) {
        const uint32_t dist = manhattan_distance(central_port, intersection);
        if (dist != 0 && dist < closest)
          closest = dist;
      }
    }
  }

  return closest;
}

static uint32_t solve_part2(const context *const ctx) {
  uint32_t least_steps = UINT32_MAX;
  uint32_t a_steps = 0;

  vertex central_port = {0, 0};
  vertex intersection = {0};
  for (uint32_t i = 0; i < ctx->wire1_vertex_count - 1; ++i) {
    const vertex a1 = ctx->wire1[i];
    const vertex a2 = ctx->wire1[i + 1];
    uint32_t b_steps = 0;
    for (uint32_t j = 0; j < ctx->wire2_vertex_count - 1; ++j) {
      const vertex b1 = ctx->wire2[j];
      const vertex b2 = ctx->wire2[j + 1];
      if (lines_intersect(a1, a2, b1, b2, &intersection)) {
        const uint32_t dist = manhattan_distance(central_port, intersection);
        // add up steps from the last vertex to the intersection
        const uint32_t steps =
            (a_steps + manhattan_distance(a1, intersection)) + (b_steps + manhattan_distance(b1, intersection));
        if (steps < least_steps && dist != 0) {
          least_steps = steps;
        }
      }
      b_steps += manhattan_distance(b1, b2);
    }
    a_steps += manhattan_distance(a1, a2);
  }

  return least_steps;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  context ctx = {0};

  parse_input(input, &ctx);
  free(input);

  uint32_t part1 = solve_part1(&ctx);
  uint32_t part2 = solve_part2(&ctx);

  printf("%u\n", part1);
  printf("%u\n", part2);
}

