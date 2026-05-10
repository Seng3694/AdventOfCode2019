#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

#ifndef NDEBUG
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

// grep -Po '\d+' day05/input.txt | wc -l
#define MAX_POSITION_COUNT 700

static void parse_input(char *input, int64_t *const positions, uint32_t *const position_count) {
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
      tlbt_assert_fmt(isdigit(*input) || *input == '-', "digit expected, actual '%c' (%d)", *input, *input);
      tlbt_assert_fmt(count < MAX_POSITION_COUNT, "too many positions, max: %u", MAX_POSITION_COUNT);
      positions[count++] = strtoll(input, &input, 10);
      break;
    }
  }
}

// it's more efficient to handle the different modes as separate opcodes than parsing them for every instruction
// sadly this spreads the opcodes a lot which means computed gotos are less feasible
typedef enum int_code_instruction {
  INT_CODE_INSTRUCTION_NONE = 0,
  INT_CODE_INSTRUCTION_ADD_MMM = 1,
  INT_CODE_INSTRUCTION_ADD_IMM = 101,
  INT_CODE_INSTRUCTION_ADD_MIM = 1001,
  INT_CODE_INSTRUCTION_ADD_IIM = 1101,

  INT_CODE_INSTRUCTION_MUL_MMM = 2,
  INT_CODE_INSTRUCTION_MUL_IMM = 102,
  INT_CODE_INSTRUCTION_MUL_MIM = 1002,
  INT_CODE_INSTRUCTION_MUL_IIM = 1102,

  INT_CODE_INSTRUCTION_LOAD_M = 3,

  INT_CODE_INSTRUCTION_PRINT_M = 4,
  INT_CODE_INSTRUCTION_PRINT_I = 104,

  INT_CODE_INSTRUCTION_JUMP_IF_TRUE_MM = 5,
  INT_CODE_INSTRUCTION_JUMP_IF_TRUE_IM = 105,
  INT_CODE_INSTRUCTION_JUMP_IF_TRUE_MI = 1005,
  INT_CODE_INSTRUCTION_JUMP_IF_TRUE_II = 1105,

  INT_CODE_INSTRUCTION_JUMP_IF_FALSE_MM = 6,
  INT_CODE_INSTRUCTION_JUMP_IF_FALSE_IM = 106,
  INT_CODE_INSTRUCTION_JUMP_IF_FALSE_MI = 1006,
  INT_CODE_INSTRUCTION_JUMP_IF_FALSE_II = 1106,

  INT_CODE_INSTRUCTION_LESS_THAN_MMM = 7,
  INT_CODE_INSTRUCTION_LESS_THAN_IMM = 107,
  INT_CODE_INSTRUCTION_LESS_THAN_MIM = 1007,
  INT_CODE_INSTRUCTION_LESS_THAN_IIM = 1107,

  INT_CODE_INSTRUCTION_EQUALS_MMM = 8,
  INT_CODE_INSTRUCTION_EQUALS_IMM = 108,
  INT_CODE_INSTRUCTION_EQUALS_MIM = 1008,
  INT_CODE_INSTRUCTION_EQUALS_IIM = 1108,

  INT_CODE_INSTRUCTION_HALT = 99,
} int_code_instruction;

typedef enum int_code_instruction_mode {
  INT_CODE_INSTRUCTION_MODE_POSITION = 0,
  INT_CODE_INSTRUCTION_MODE_IMMEDIATE = 1,
} int_code_instruction_mode;

static int64_t run_intcode_machine(int64_t *const positions, const uint32_t position_count, const uint32_t input,
                                   const uint32_t output_buffer_capacity, int64_t *const output_buffer) {
  uint32_t buffer_count = 0;
  int64_t current = 0;
  for (;;) {
    switch ((int_code_instruction)positions[current]) {
    case INT_CODE_INSTRUCTION_HALT:
      goto done;

    // ADD INSTRUCTION
    case INT_CODE_INSTRUCTION_ADD_MMM: {
      const int64_t address_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("add [%ld](%ld) [%ld](%ld) [%ld]\n", address_a, positions[address_a], address_b, positions[address_b],
                address_c);
      positions[address_c] = positions[address_a] + positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_ADD_IMM: {
      const int64_t value_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("add %ld [%ld](%ld) [%ld]\n", value_a, address_b, positions[address_b], address_c);
      positions[address_c] = value_a + positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_ADD_MIM: {
      const int64_t address_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("add [%ld](%ld) %ld [%ld]\n", address_a, positions[address_a], value_b, address_c);
      positions[address_c] = positions[address_a] + value_b;
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_ADD_IIM: {
      const int64_t value_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("add %ld %ld [%ld]\n", value_a, value_b, address_c);
      positions[address_c] = value_a + value_b;
      current += 4;
      break;
    }

      // MUL INSTRUCTION
    case INT_CODE_INSTRUCTION_MUL_MMM: {
      const int64_t address_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("mul [%ld](%ld) [%ld](%ld) [%ld]\n", address_a, positions[address_a], address_b, positions[address_b],
                address_c);
      positions[address_c] = positions[address_a] * positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_MUL_IMM: {
      const int64_t value_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("mul %ld [%ld](%ld) [%ld]\n", value_a, address_b, positions[address_b], address_c);
      positions[address_c] = value_a * positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_MUL_MIM: {
      const int64_t address_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("mul [%ld](%ld) %ld [%ld]\n", address_a, positions[address_a], value_b, address_c);
      positions[address_c] = positions[address_a] * value_b;
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_MUL_IIM: {
      const int64_t value_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("mul %ld %ld [%ld]\n", value_a, value_b, address_c);
      positions[address_c] = value_a * value_b;
      current += 4;
      break;
    }

    // PRINT
    case INT_CODE_INSTRUCTION_PRINT_M: {
      const int64_t address_a = positions[current + 1];
      DEBUG_LOG("print [%ld](%ld)\n", address_a, positions[address_a]);
      output_buffer[buffer_count++] = positions[address_a];
      current += 2;
      break;
    }

    case INT_CODE_INSTRUCTION_PRINT_I: {
      const int64_t value_a = positions[current + 1];
      DEBUG_LOG("print %ld\n", value_a);
      output_buffer[buffer_count++] = value_a;
      current += 2;
      break;
    }

    // LOAD
    case INT_CODE_INSTRUCTION_LOAD_M: {
      const int64_t address_a = positions[current + 1];
      DEBUG_LOG("load %u [%ld](%ld)\n", input, address_a, positions[address_a]);
      positions[address_a] = input;
      current += 2;
      break;
    }

    // JMP_IF_TRUE
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_MM: {
      const int64_t address_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      DEBUG_LOG("jit [%ld](%ld) [%ld](%ld) (%s)\n", address_a, positions[address_a], address_b, positions[address_b],
                positions[address_a] != 0 ? "true" : "false");
      current = positions[address_a] != 0 ? positions[address_b] : current + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_IM: {
      const int64_t value_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      DEBUG_LOG("jit %ld [%ld](%ld) (%s)\n", value_a, address_b, positions[address_b], value_a != 0 ? "true" : "false");
      current = value_a != 0 ? positions[address_b] : current + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_MI: {
      const int64_t address_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      DEBUG_LOG("jit [%ld](%ld) %ld (%s)\n", address_a, positions[address_a], value_b,
                positions[address_a] != 0 ? "true" : "false");
      current = positions[address_a] != 0 ? value_b : current + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_II: {
      const int64_t value_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      DEBUG_LOG("jit %ld %ld (%s)\n", value_a, value_b, value_a != 0 ? "true" : "false");
      current = value_a != 0 ? value_b : current + 3;
      break;
    }

    // JMP_IF_FALSE
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_MM: {
      const int64_t address_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      DEBUG_LOG("jif [%ld](%ld) [%ld](%ld) (%s)\n", address_a, positions[address_a], address_b, positions[address_b],
                positions[address_a] == 0 ? "true" : "false");
      current = positions[address_a] == 0 ? positions[address_b] : current + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_IM: {
      const int64_t value_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      DEBUG_LOG("jif %ld [%ld](%ld) (%s)\n", value_a, address_b, positions[address_b], value_a == 0 ? "true" : "false");
      current = value_a == 0 ? positions[address_b] : current + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_MI: {
      const int64_t address_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      DEBUG_LOG("jif [%ld](%ld) %ld (%s)\n", address_a, positions[address_a], value_b,
                positions[address_a] == 0 ? "true" : "false");
      current = positions[address_a] == 0 ? value_b : current + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_II: {
      const int64_t value_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      DEBUG_LOG("jif %ld %ld (%s)\n", value_a, value_b, value_a == 0 ? "true" : "false");
      current = value_a == 0 ? value_b : current + 3;
      break;
    }

    // LESS_THAN
    case INT_CODE_INSTRUCTION_LESS_THAN_MMM: {
      const int64_t address_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("lt [%ld](%ld) [%ld](%ld) [%ld] (%s)\n", address_a, positions[address_a], address_b,
                positions[address_b], address_c, positions[address_a] < positions[address_b] ? "true" : "false");
      positions[address_c] = positions[address_a] < positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_LESS_THAN_IMM: {
      const int64_t value_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("lt %ld [%ld](%ld) [%ld] (%s)\n", value_a, address_b, positions[address_b], address_c,
                value_a < positions[address_b] ? "true" : "false");
      positions[address_c] = value_a < positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_LESS_THAN_MIM: {
      const int64_t address_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("lt [%ld](%ld) %ld [%ld] (%s)\n", address_a, positions[address_a], value_b, address_c,
                positions[address_a] < value_b ? "true" : "false");
      positions[address_c] = positions[address_a] < value_b;
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_LESS_THAN_IIM: {
      const int64_t value_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("lt %ld %ld [%ld] (%s)\n", value_a, value_b, address_c, value_a < value_b ? "true" : "false");
      positions[address_c] = value_a < value_b;
      current += 4;
      break;
    }

    // EQUALS
    case INT_CODE_INSTRUCTION_EQUALS_MMM: {
      const int64_t address_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("eq [%ld](%ld) [%ld](%ld) [%ld] (%s)\n", address_a, positions[address_a], address_b,
                positions[address_b], address_c, positions[address_a] == positions[address_b] ? "true" : "false");
      positions[address_c] = positions[address_a] == positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_EQUALS_IMM: {
      const int64_t value_a = positions[current + 1];
      const int64_t address_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("eq %ld [%ld](%ld) [%ld] (%s)\n", value_a, address_b, positions[address_b], address_c,
                value_a == positions[address_b] ? "true" : "false");
      positions[address_c] = value_a == positions[address_b];
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_EQUALS_MIM: {
      const int64_t address_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("eq [%ld](%ld) %ld [%ld] (%s)\n", address_a, positions[address_a], value_b, address_c,
                positions[address_a] == value_b ? "true" : "false");
      positions[address_c] = positions[address_a] == value_b;
      current += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_EQUALS_IIM: {
      const int64_t value_a = positions[current + 1];
      const int64_t value_b = positions[current + 2];
      const int64_t address_c = positions[current + 3];
      DEBUG_LOG("eq %ld %ld [%ld] (%s)\n", value_a, value_b, address_c, value_a == value_b ? "true" : "false");
      positions[address_c] = value_a == value_b;
      current += 4;
      break;
    }

    default:
      // either an error occurred or I forgot to implement an opcode
      DEBUG_LOG("unknown opcode: %ld\n", positions[current]);
      tlbt_assert_unreachable();
      break;
    }
  }

done:
#ifndef NDEBUG
  DEBUG_LOG("output buffer: ");
  for (uint32_t i = 0; i < buffer_count; ++i) {
    DEBUG_LOG("%ld ", output_buffer[i]);
  }
  DEBUG_LOG("\n");
#endif

  return output_buffer[buffer_count - 1];
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

  parse_input(input, positions, &position_count);
  free(input);

  int64_t copy[MAX_POSITION_COUNT] = {0};
  memcpy(copy, positions, sizeof(int64_t) * position_count);

  int64_t output_buffer[1024] = {0};
  const size_t output_buffer_size = sizeof(output_buffer) / sizeof(output_buffer[0]);

  const int64_t part1 = run_intcode_machine(positions, position_count, 1, output_buffer_size, output_buffer);
  memcpy(positions, copy, sizeof(uint64_t) * position_count);
  const int64_t part2 = run_intcode_machine(positions, position_count, 5, output_buffer_size, output_buffer);

  printf("%ld\n", part1);
  printf("%ld\n", part2);
  return 0;
}

