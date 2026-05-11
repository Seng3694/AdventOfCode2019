#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

typedef union space_object_id {
  char text_id[4];
  uint32_t id;
} space_object_id;

typedef struct space_object {
  struct space_object *next;
  space_object_id id;
  uint32_t cache;
  uint8_t flags;
} space_object;

#define YOU_FLAG (1 << 0)
#define SAN_FLAG (1 << 1)

// grep -Po '[^\)]{3}' day06/input.txt | sort -u | wc -l -> 2094 unique space_objects
#define MAX_SPACE_OBJECTS 2100

static inline uint32_t space_object_id_hash(space_object_id x) {
  x.id ^= x.id >> 16;
  x.id *= 0x7feb352dU;
  x.id ^= x.id >> 15;
  x.id *= 0x846ca68bU;
  x.id ^= x.id >> 16;
  return x.id;
}

static inline bool space_object_id_equals(space_object_id left, space_object_id right) {
  return left.id == right.id;
}

#define TLBT_KEY_T space_object_id
#define TLBT_KEY_T_NAME id
#define TLBT_VALUE_T space_object *
#define TLBT_VALUE_T_NAME obj_ptr
#define TLBT_BASE2_CAPACITY
#define TLBT_HASH_FUNC space_object_id_hash
#define TLBT_EQUALS_FUNC space_object_id_equals
#define TLBT_STATIC
#include "../ext/toolbelt/src/hashmap.h"

static inline void parse_name(char *input, char **out, char *name) {
  for (;;) {
    switch (*input) {
    case '\0':
    case '\n':
    case ')':
      *out = input;
      return;
    default:
      *name++ = *input++;
      break;
    }
  }
}

static void parse_input(char *input, space_object *const objs, uint32_t *const obj_count,
                        tlbt_map_id_obj_ptr *const obj_map) {
  uint32_t count = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *obj_count = count;
      return;
    case '\n':
      ++input;
      break;
    default:
      tlbt_assert_fmt((*input >= 'A' && *input <= 'Z') || isdigit(*input), "[A-Z0-9] expected, actual '%c' (%d)",
                      *input, *input);

      space_object_id id1 = {0};
      parse_name(input, &input, id1.text_id);
      space_object *obj1 = NULL;
      if (!tlbt_map_id_obj_ptr_contains(obj_map, id1)) {
        tlbt_assert_fmt(count < MAX_SPACE_OBJECTS, "too many space objects, max: %u", MAX_SPACE_OBJECTS);
        obj1 = &objs[count++];
        obj1->id = id1;
        obj1->next = NULL;
        obj1->cache = 0;
        tlbt_map_id_obj_ptr_insert(obj_map, id1, obj1);
      } else {
        tlbt_map_id_obj_ptr_get(obj_map, id1, &obj1);
      }
      input++;
      tlbt_assert_fmt((*input >= 'A' && *input <= 'Z') || isdigit(*input), "[A-Z0-9] expected, actual '%c' (%d)",
                      *input, *input);

      space_object_id id2 = {0};
      parse_name(input, &input, id2.text_id);
      space_object *obj2 = NULL;
      if (!tlbt_map_id_obj_ptr_contains(obj_map, id2)) {
        tlbt_assert_fmt(count < MAX_SPACE_OBJECTS, "too many space objects, max: %u", MAX_SPACE_OBJECTS);
        obj2 = &objs[count++];
        obj2->id = id1;
        obj2->next = NULL;
        obj2->cache = 0;
        tlbt_map_id_obj_ptr_insert(obj_map, id2, obj2);
      } else {
        tlbt_map_id_obj_ptr_get(obj_map, id2, &obj2);
      }

      obj2->next = obj1;
      input++;
      break;
    }
  }
}

static uint32_t count_objs(space_object *const obj) {
  if (!obj->next)
    return 0;

  if (obj->cache != 0)
    return obj->cache;

  obj->cache = count_objs(obj->next) + 1;
  return obj->cache;
}

static uint32_t solve_part1(space_object *const objs, const uint32_t obj_count) {
  uint32_t solution = 0;
  for (uint32_t i = 0; i < obj_count; ++i)
    solution += count_objs(&objs[i]);
  return solution;
}

static uint32_t solve_part2(tlbt_map_id_obj_ptr *const obj_map) {
  // this function assumes part 1 already ran and the caches are set
  space_object *you = NULL;
  space_object *san = NULL;
  tlbt_map_id_obj_ptr_get(obj_map, (space_object_id){.text_id = "YOU"}, &you);
  tlbt_map_id_obj_ptr_get(obj_map, (space_object_id){.text_id = "SAN"}, &san);

  space_object *you_orbit = you->next;
  space_object *san_orbit = san->next;

  // flag the whole route from YOU to COM
  while (you->next != NULL) {
    you->flags |= YOU_FLAG;
    you = you->next;
  }
  // flag the whole route from SAN to COM
  while (san->next != NULL) {
    san->flags |= SAN_FLAG;
    san = san->next;
  }

  space_object *center = you_orbit;
  // find the first node which is flagged both YOU AND SAN
  while (center->flags != (SAN_FLAG | YOU_FLAG)) {
    center = center->next;
  }
  // add up the cached distances from both nodes substracted by the rest distance to COM which is not required here
  return you_orbit->cache - center->cache + san_orbit->cache - center->cache;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  tlbt_map_id_obj_ptr obj_map = {0};
  tlbt_map_id_obj_ptr_key obj_map_key_buffer[4096];
  space_object *obj_map_buffer[4096];
  tlbt_map_id_obj_ptr_init(&obj_map, 4096, obj_map_key_buffer, obj_map_buffer);

  space_object objs[MAX_SPACE_OBJECTS] = {0};
  uint32_t obj_count = 0;
  parse_input(input, objs, &obj_count, &obj_map);
  free(input);

  uint32_t part1 = solve_part1(objs, obj_count);
  uint32_t part2 = solve_part2(&obj_map);

  printf("%u\n", part1);
  printf("%u\n", part2);
}

