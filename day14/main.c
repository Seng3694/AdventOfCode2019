#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

// grep -Po '[A-Z]+' day14/input.txt | sort -u | wc -l
// -> 60 but use base 2 numbers for faster hashmap operations
#define MAX_ELEMENTS 64

// grep -Po '[A-Z]+' day14/input.txt | awk '{print length}' | sort -u | tail -n 1
#define MAX_ELEMENT_NAME_LENGTH 5

// awk -F, 'NF>m{m=NF} END{print m}' day14/input.txt
#define MAX_INGREDIENT_COUNT 8

typedef struct element {
  char id[MAX_ELEMENT_NAME_LENGTH + 1]; // add +1 for the null terminator
} element;

typedef struct recipe {
  char id[MAX_ELEMENT_NAME_LENGTH + 1];
  // grep -Po '\d+' day14/input.txt | sort -nu | tail -n 1
  // -> 180. so uint8_t is enough
  uint8_t amount;
  uint64_t leftover;

  uint64_t total_produced;
  uint64_t total_ore;

  struct recipe *ingredients[MAX_INGREDIENT_COUNT];
  uint8_t ingredient_amounts[MAX_INGREDIENT_COUNT];
  uint8_t ingredient_count;
} recipe;

typedef struct recipe_allocator {
  recipe recipes[MAX_ELEMENTS];
  uint8_t count;
} recipe_allocator;

static inline uint32_t id_hash(const char *id) {
  uint32_t hash = 36591911;
  while (*id) {
    hash = (hash << 5) + hash + (*id++);
  }
  return hash;
}

static inline bool id_equals(const char *a, const char *b) {
  while (*a && *a == *b) {
    ++a;
    ++b;
  }
  return *a == *b;
}

#define TLBT_KEY_T const char *
#define TLBT_KEY_T_NAME id
#define TLBT_VALUE_T recipe *
#define TLBT_VALUE_T_NAME recipe
#define TLBT_STATIC
#define TLBT_HASH id_hash
#define TLBT_EQUALS id_equals
#define TLBT_BASE2_CAPACITY
#include "../ext/toolbelt/src/hashmap.h"

static void parse_element(char *input, char **output, element *const e, uint8_t *const amount) {
  // format: `count id`
  tlbt_assert_fmt(isdigit(*input), "expected digit, actual: '%c'", *input);
  *amount = strtoul(input, &input, 10);
  tlbt_assert_fmt(*input == ' ', "expected space, actual: '%c'", *input);
  ++input;
  tlbt_assert_fmt(*input >= 'A' && *input <= 'Z', "expected identifier, actual: '%c'", *input);

  char *start = input;
  while (*input >= 'A' && *input <= 'Z') {
    ++input;
  }
  tlbt_assert_fmt(input - start <= MAX_ELEMENT_NAME_LENGTH, "element id too long '%zu', expected '%u' or less",
                  input - start, MAX_ELEMENT_NAME_LENGTH);
  memcpy(e->id, start, input - start);
  e->id[input - start] = '\0';
  *output = input;
}

static void parse_recipe(char *input, char **output, tlbt_map_id_recipe *const recipes,
                         recipe_allocator *const allocator) {
  // format: `count1 id1, count2 id2, count3 id3, ... => count_result id_result`

  recipe r = {0};
  for (;;) {
    switch (*input) {
    case '\0':
    case '\n':
      tlbt_assert_unreachable();
      break;
    case ',':
    case ' ':
      ++input;
      break;
    case '=': {
      tlbt_assert_fmt(input[1] == '>' && input[2] == ' ', "expected '=> ', found '%.*s'", 3, input);
      element e = {0};
      uint8_t amount = 0;
      parse_element(input + 3, &input, &e, &amount);

      if (tlbt_map_id_recipe_contains(recipes, e.id)) {
        recipe *result = NULL;
        // the if is completely pointless, but it's to make gcc happy. apparently it can't assume `result` is never NULL
        if (tlbt_map_id_recipe_get(recipes, e.id, &result)) {
          memcpy(result, &r, sizeof(recipe));
          memcpy(result->id, e.id, MAX_ELEMENT_NAME_LENGTH);
          result->amount = amount;
        }
      } else {
        recipe *result = &allocator->recipes[allocator->count++];
        memcpy(result, &r, sizeof(recipe));
        memcpy(result->id, e.id, MAX_ELEMENT_NAME_LENGTH);
        tlbt_map_id_recipe_insert(recipes, result->id, result);
        result->amount = amount;
      }

      goto done;
    }
    default: {
      tlbt_assert_msg(r.ingredient_count + 1 <= MAX_INGREDIENT_COUNT, "too many ingredients");
      element e = {0};
      uint8_t amount = 0;
      parse_element(input, &input, &e, &amount);

      if (tlbt_map_id_recipe_contains(recipes, e.id)) {
        tlbt_map_id_recipe_get(recipes, e.id, &r.ingredients[r.ingredient_count]);
      } else {
        recipe *ingredient = &allocator->recipes[allocator->count++];
        memcpy(ingredient->id, e.id, MAX_ELEMENT_NAME_LENGTH);
        tlbt_map_id_recipe_insert(recipes, ingredient->id, ingredient);
        r.ingredients[r.ingredient_count] = ingredient;
      }

      r.ingredient_amounts[r.ingredient_count++] = amount;

      break;
    }
    }
  }

done:

  *output = input;
}

static void parse_input(char *input, tlbt_map_id_recipe *const recipes, recipe_allocator *const allocator) {
  for (;;) {
    switch (*input) {
    case '\0':
      return;
    case '\n':
      ++input;
      break;
    default: {
      parse_recipe(input, &input, recipes, allocator);
    }
    }
  }
}

static uint64_t calculate_ore_amount(recipe *const r, const uint64_t count) {
  uint64_t ore = 0;

  if (r->leftover >= count) {
    r->leftover -= count;
    return 0;
  }

  const uint64_t needed = count - r->leftover;
  const uint64_t iterations = (needed + r->amount - 1) / r->amount;

  for (uint32_t i = 0; i < r->ingredient_count; ++i) {
    if (!id_equals(r->ingredients[i]->id, "ORE")) {
      ore += calculate_ore_amount(r->ingredients[i], r->ingredient_amounts[i] * iterations);
    } else {
      ore += r->ingredient_amounts[i] * iterations;
    }
  }

  r->leftover += ((iterations * r->amount) - count);
  r->total_ore += ore;
  r->total_produced += iterations * r->amount;

  return ore;
}

static uint64_t solve_part1(tlbt_map_id_recipe *const recipes) {
  recipe *r = NULL;
  tlbt_assert(tlbt_map_id_recipe_contains(recipes, "FUEL"));
  tlbt_map_id_recipe_get(recipes, "FUEL", &r);

  uint64_t result = calculate_ore_amount(r, 1);

  return result;
}

static void reset_counts(recipe *const r) {
  r->leftover = 0;
  r->total_ore = 0;
  r->total_produced = 0;
  for (uint8_t i = 0; i < r->ingredient_count; ++i) {
    reset_counts(r->ingredients[i]);
  }
}

static uint64_t solve_part2(tlbt_map_id_recipe *const recipes, const uint64_t cost_for_one_fuel) {
  recipe *r = NULL;
  tlbt_assert(tlbt_map_id_recipe_contains(recipes, "FUEL"));
  tlbt_map_id_recipe_get(recipes, "FUEL", &r);

  const uint64_t ore_to_spend = 1000000000000LU;
  // due to leftovers, we can make a lot more fuel than calculated here
  const uint64_t under_estimated_fuel_amount = ore_to_spend / cost_for_one_fuel;
  const uint64_t over_estimated_fuel_amount = under_estimated_fuel_amount * 3;

  // do a binary search for the right amount of fuel
  uint64_t lower = under_estimated_fuel_amount;
  uint64_t upper = over_estimated_fuel_amount;
  uint64_t mid = lower + (upper - lower) / 2 - 1;
  uint64_t result = 0;

  for (;;) {
    reset_counts(r);
    uint64_t res = calculate_ore_amount(r, mid);

    // adjust binary search boundaries
    uint64_t previous_mid = mid;
    if (res < ore_to_spend) {
      lower = mid;
      mid = lower + (upper - lower) / 2 - 1;
    } else if (res > ore_to_spend) {
      upper = mid;
      mid = lower + (upper - lower) / 2 - 1;
    } else {
      result = mid;
      break;
    }

    // it's possible that the goal can never be reached exactly. in this case the "mid" value should be the same
    // there are two cases:
    // 1. the mid value is below the goal (can still spend more ore)
    //   -> increment mid until it's higher than the goal. one below is the result because can't spend more than
    //   available
    // 2. the mid value is above the goal (spent more ore than there is available)
    //   -> substract from mid until it's lower than the goal. one above is the result
    // I don't think case 2 will ever happen, but still handled it just in case
    if (previous_mid == mid) {
      if (res < ore_to_spend) {
        while (res < ore_to_spend) {
          ++mid;
          reset_counts(r);
          res = calculate_ore_amount(r, mid);
        }
        // ore amount went above threshold, so subtract one fuel. might end up with leftover ore, but can't make more
        // fuel with not enough ore
        result = mid - 1;
        break;
      } else if (res > ore_to_spend) {
        while (res > ore_to_spend) {
          --mid;
          reset_counts(r);
          res = calculate_ore_amount(r, mid);
        }
        break;
      }
    }
  }

  return result;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  char *input = NULL;
  size_t length = 0;
  if (!fileutils_read_all(argv[1], &input, &length))
    return 1;

  tlbt_map_id_recipe_key recipe_key_buffer[MAX_ELEMENTS * 2] = {0};
  recipe *recipe_value_buffer[MAX_ELEMENTS * 2] = {0};
  tlbt_map_id_recipe recipes = {0};
  tlbt_map_id_recipe_init(&recipes, sizeof(recipe_key_buffer) / sizeof(recipe_key_buffer[0]), recipe_key_buffer,
                          recipe_value_buffer);

  recipe_allocator allocator = {0};

  parse_input(input, &recipes, &allocator);
  free(input);

  uint64_t part1 = solve_part1(&recipes);
  uint64_t part2 = solve_part2(&recipes, part1);

  printf("%lu\n", part1);
  printf("%lu\n", part2);
}

