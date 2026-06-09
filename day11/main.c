#include <stdio.h>
#include <stdint.h>

#include "../ext/toolbelt/src/assert.h"
#include "../ext/toolbelt/src/bitutils.h"
#include "../common/fileutils.h"
#include "../common/intcode.h"

// grep -Po '\d+' day11/input.txt | wc -l
#define MAX_CODE_COUNT 3000

typedef enum color {
  COLOR_BLACK = 0,
  COLOR_WHITE = 1,
} color;

typedef struct point {
  int32_t x, y;
} point;

static inline uint32_t point_hash(const point p) {
  uint32_t hash = 36591911;
  hash = (hash << 5) + hash + *(uint32_t *)&p.x;
  hash = (hash << 5) + hash + *(uint32_t *)&p.y;
  return hash;
}

static inline bool point_equals(const point a, const point b) {
  return a.x == b.x && a.y == b.y;
}

#define TLBT_KEY_T point
#define TLBT_VALUE_T color
#define TLBT_STATIC
#define TLBT_HASH_FUNC point_hash
#define TLBT_EQUALS_FUNC point_equals
#define TLBT_BASE2_CAPACITY
#include "../ext/toolbelt/src/hashmap.h"

typedef enum direction {
  DIRECTION_LEFT,
  DIRECTION_UP,
  DIRECTION_RIGHT,
  DIRECTION_DOWN,
  DIRECTION_COUNT,
} direction;

static inline direction wrap_direction(int64_t i) {
  return i > 3 ? i & 3 : i < 0 ? 3 + i + 1 : i;
}

static uint32_t run_robot(int64_t *const code, const uint32_t code_count, tlbt_map_point_color *const painted,
                          const color starting_color) {
  int64_t input_buffer[8] = {0};
  int64_t output_buffer[32] = {0};
  intcode_machine_state state = {
      .code = code,
      .code_count = code_count,
      .memory_capacity = MAX_CODE_COUNT,
  };
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_buffer) / sizeof(input_buffer[0]), input_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_buffer) / sizeof(output_buffer[0]), output_buffer);

  tlbt_deque_i64_push_back(&state.input_queue, (int64_t)starting_color);

  point position = {0, 0};
  direction dir = DIRECTION_UP;

  static const point move_table[DIRECTION_COUNT] = {
      [DIRECTION_LEFT] = {-1, 0},
      [DIRECTION_UP] = {0, -1},
      [DIRECTION_RIGHT] = {1, 0},
      [DIRECTION_DOWN] = {0, 1},
  };

  bool is_running = true;

  while (is_running) {
    color c = COLOR_BLACK;
    uint32_t hash = point_hash(position);
    if (tlbt_map_point_color_contains_ph(painted, position, hash)) {
      tlbt_map_point_color_get_ph(painted, position, &c, hash);
    }

    // push current color, default black
    if (state.input_queue.count == 0)
      tlbt_deque_i64_push_back(&state.input_queue, (int64_t)c);

    is_running = intcode_run(&state);

    tlbt_assert_fmt(state.input_queue.count == 0, "expected input queue to be empty. actual: %zu",
                    state.input_queue.count);
    tlbt_assert_fmt(state.output_queue.count == 2, "expected output queue to have exactly two elements. actual: %zu",
                    state.output_queue.count);

    // get new color
    c = *tlbt_deque_i64_peek_front(&state.output_queue);
    tlbt_deque_i64_pop_front(&state.output_queue);

    // replace or insert
    if (tlbt_map_point_color_contains_ph(painted, position, hash)) {
      tlbt_map_point_color_remove_ph(painted, position, hash);
    }
    tlbt_map_point_color_insert_ph(painted, position, c, hash);

    // do direction change
    const int64_t direction_change = *tlbt_deque_i64_peek_front(&state.output_queue);
    tlbt_deque_i64_pop_front(&state.output_queue);
    dir = wrap_direction(dir + (direction_change == 0 ? -1 : 1));
    position.x += move_table[dir].x;
    position.y += move_table[dir].y;
  }

  return (uint32_t)painted->count;
}

void solve_part2(int64_t *const code, const uint32_t code_count, tlbt_map_point_color *const painted) {
  run_robot(code, code_count, painted, COLOR_WHITE);

  // get dimensions
  int64_t left = INT64_MAX;
  int64_t top = INT64_MAX;
  int64_t right = INT64_MIN;
  int64_t bottom = INT64_MIN;

  tlbt_map_iterator_point_color iter = {0};
  tlbt_map_iterator_point_color_init(&iter, painted);
  point *key = NULL;
  color *value = NULL;

  while (tlbt_map_iterator_point_color_iterate(&iter, &key, &value)) {
    if (key->x < left)
      left = key->x;
    if (key->x > right)
      right = key->x;
    if (key->y < top)
      top = key->y;
    if (key->y > bottom)
      bottom = key->y;
  }

  const int64_t left_offset = -left;
  const int64_t top_offset = -top;
  const uint32_t width = right - left + 1;
  const uint32_t height = bottom - top + 1;

  // create a "screen"
  // it's enough to fit 2048 values. result was 1894. so it should be fine for most inputs
  tlbt_assert_msg(painted->count <= 2048, "printer only supports up to 2048 elements");
  uint64_t area[32] = {0};
  tlbt_map_iterator_point_color_reset(&iter);
  while (tlbt_map_iterator_point_color_iterate(&iter, &key, &value)) {
    if (*value == COLOR_BLACK)
      continue;
    const uint32_t x = key->x + left_offset;
    const uint32_t y = key->y + top_offset;
    const uint32_t global_index = y * width + x;
    const uint32_t array_index = global_index / 64;
    const uint32_t bit_index = global_index % 64;
    area[array_index] = TLBT_SET_BIT(area[array_index], bit_index);
  }

  // print "screen"
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      const uint32_t global_index = y * width + x;
      const uint32_t array_index = global_index / 64;
      const uint32_t bit_index = global_index % 64;
      putchar(TLBT_CHECK_BIT(area[array_index], bit_index) == 0 ? ' ' : '#');
    }
    printf("\n");
  }
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

  int64_t copy[MAX_CODE_COUNT] = {0};
  memcpy(copy, code, sizeof(code));

  tlbt_map_point_color painted = {0};
  tlbt_map_point_color_key key_buffer[4096] = {0};
  color color_buffer[4096] = {0};
  tlbt_map_point_color_init(&painted, sizeof(key_buffer) / sizeof(key_buffer[0]), key_buffer, color_buffer);

  uint32_t part1 = run_robot(code, code_count, &painted, COLOR_BLACK);
  printf("%u\n", part1);

  tlbt_map_point_color_clear(&painted);
  solve_part2(copy, code_count, &painted);
}

