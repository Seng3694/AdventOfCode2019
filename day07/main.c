#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "../ext/toolbelt/src/assert.h"
#include "../common/fileutils.h"

#ifdef VERBOSE_LOGGING
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

// grep -Po '\d+' day07/input.txt | wc -l
#define MAX_POSITION_COUNT 600

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

#define TLBT_T int64_t
#define TLBT_T_NAME i64
#define TLBT_BASE2_CAPACITY
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

typedef struct intcode_machine_state {
  int64_t *code;
  tlbt_deque_i64 input_queue;
  tlbt_deque_i64 output_queue;
  int64_t pc;
  uint32_t code_count;
} intcode_machine_state;

static bool run_intcode_machine(intcode_machine_state *const state) {
  int64_t *code = state->code;
  int64_t pc = state->pc;
  for (;;) {
    switch ((int_code_instruction)code[pc]) {
    case INT_CODE_INSTRUCTION_HALT:
      DEBUG_LOG("halt\n");
      goto done;

    // ADD INSTRUCTION
    case INT_CODE_INSTRUCTION_ADD_MMM: {
      const int64_t address_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("add [%ld](%ld) [%ld](%ld) [%ld]\n", address_a, code[address_a], address_b, code[address_b], address_c);
      code[address_c] = code[address_a] + code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_ADD_IMM: {
      const int64_t value_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("add %ld [%ld](%ld) [%ld]\n", value_a, address_b, code[address_b], address_c);
      code[address_c] = value_a + code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_ADD_MIM: {
      const int64_t address_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("add [%ld](%ld) %ld [%ld]\n", address_a, code[address_a], value_b, address_c);
      code[address_c] = code[address_a] + value_b;
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_ADD_IIM: {
      const int64_t value_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("add %ld %ld [%ld]\n", value_a, value_b, address_c);
      code[address_c] = value_a + value_b;
      pc += 4;
      break;
    }

      // MUL INSTRUCTION
    case INT_CODE_INSTRUCTION_MUL_MMM: {
      const int64_t address_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("mul [%ld](%ld) [%ld](%ld) [%ld]\n", address_a, code[address_a], address_b, code[address_b], address_c);
      code[address_c] = code[address_a] * code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_MUL_IMM: {
      const int64_t value_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("mul %ld [%ld](%ld) [%ld]\n", value_a, address_b, code[address_b], address_c);
      code[address_c] = value_a * code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_MUL_MIM: {
      const int64_t address_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("mul [%ld](%ld) %ld [%ld]\n", address_a, code[address_a], value_b, address_c);
      code[address_c] = code[address_a] * value_b;
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_MUL_IIM: {
      const int64_t value_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("mul %ld %ld [%ld]\n", value_a, value_b, address_c);
      code[address_c] = value_a * value_b;
      pc += 4;
      break;
    }

    // PRINT
    case INT_CODE_INSTRUCTION_PRINT_M: {
      const int64_t address_a = code[pc + 1];
      DEBUG_LOG("print [%ld](%ld)\n", address_a, code[address_a]);
      tlbt_deque_i64_push_back(&state->output_queue, code[address_a]);
      pc += 2;
      break;
    }

    case INT_CODE_INSTRUCTION_PRINT_I: {
      const int64_t value_a = code[pc + 1];
      DEBUG_LOG("print %ld\n", value_a);
      tlbt_deque_i64_push_back(&state->output_queue, value_a);
      pc += 2;
      break;
    }

    // LOAD
    case INT_CODE_INSTRUCTION_LOAD_M: {
      if (state->input_queue.count == 0) {
        // save pc and return until new input arrives
        DEBUG_LOG("load (waiting for input)\n");
        state->pc = pc;
        return true;
      }

      const int64_t address_a = code[pc + 1];
      DEBUG_LOG("load %ld [%ld](%ld)\n", *tlbt_deque_i64_peek_front(&state->input_queue), address_a, code[address_a]);
      code[address_a] = *tlbt_deque_i64_peek_front(&state->input_queue);
      tlbt_deque_i64_pop_front(&state->input_queue);
      pc += 2;
      break;
    }

    // JMP_IF_TRUE
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_MM: {
      const int64_t address_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      DEBUG_LOG("jit [%ld](%ld) [%ld](%ld) (%s)\n", address_a, code[address_a], address_b, code[address_b],
                code[address_a] != 0 ? "true" : "false");
      pc = code[address_a] != 0 ? code[address_b] : pc + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_IM: {
      const int64_t value_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      DEBUG_LOG("jit %ld [%ld](%ld) (%s)\n", value_a, address_b, code[address_b], value_a != 0 ? "true" : "false");
      pc = value_a != 0 ? code[address_b] : pc + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_MI: {
      const int64_t address_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      DEBUG_LOG("jit [%ld](%ld) %ld (%s)\n", address_a, code[address_a], value_b,
                code[address_a] != 0 ? "true" : "false");
      pc = code[address_a] != 0 ? value_b : pc + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE_II: {
      const int64_t value_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      DEBUG_LOG("jit %ld %ld (%s)\n", value_a, value_b, value_a != 0 ? "true" : "false");
      pc = value_a != 0 ? value_b : pc + 3;
      break;
    }

    // JMP_IF_FALSE
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_MM: {
      const int64_t address_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      DEBUG_LOG("jif [%ld](%ld) [%ld](%ld) (%s)\n", address_a, code[address_a], address_b, code[address_b],
                code[address_a] == 0 ? "true" : "false");
      pc = code[address_a] == 0 ? code[address_b] : pc + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_IM: {
      const int64_t value_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      DEBUG_LOG("jif %ld [%ld](%ld) (%s)\n", value_a, address_b, code[address_b], value_a == 0 ? "true" : "false");
      pc = value_a == 0 ? code[address_b] : pc + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_MI: {
      const int64_t address_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      DEBUG_LOG("jif [%ld](%ld) %ld (%s)\n", address_a, code[address_a], value_b,
                code[address_a] == 0 ? "true" : "false");
      pc = code[address_a] == 0 ? value_b : pc + 3;
      break;
    }
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE_II: {
      const int64_t value_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      DEBUG_LOG("jif %ld %ld (%s)\n", value_a, value_b, value_a == 0 ? "true" : "false");
      pc = value_a == 0 ? value_b : pc + 3;
      break;
    }

    // LESS_THAN
    case INT_CODE_INSTRUCTION_LESS_THAN_MMM: {
      const int64_t address_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("lt [%ld](%ld) [%ld](%ld) [%ld] (%s)\n", address_a, code[address_a], address_b, code[address_b],
                address_c, code[address_a] < code[address_b] ? "true" : "false");
      code[address_c] = code[address_a] < code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_LESS_THAN_IMM: {
      const int64_t value_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("lt %ld [%ld](%ld) [%ld] (%s)\n", value_a, address_b, code[address_b], address_c,
                value_a < code[address_b] ? "true" : "false");
      code[address_c] = value_a < code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_LESS_THAN_MIM: {
      const int64_t address_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("lt [%ld](%ld) %ld [%ld] (%s)\n", address_a, code[address_a], value_b, address_c,
                code[address_a] < value_b ? "true" : "false");
      code[address_c] = code[address_a] < value_b;
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_LESS_THAN_IIM: {
      const int64_t value_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("lt %ld %ld [%ld] (%s)\n", value_a, value_b, address_c, value_a < value_b ? "true" : "false");
      code[address_c] = value_a < value_b;
      pc += 4;
      break;
    }

    // EQUALS
    case INT_CODE_INSTRUCTION_EQUALS_MMM: {
      const int64_t address_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("eq [%ld](%ld) [%ld](%ld) [%ld] (%s)\n", address_a, code[address_a], address_b, code[address_b],
                address_c, code[address_a] == code[address_b] ? "true" : "false");
      code[address_c] = code[address_a] == code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_EQUALS_IMM: {
      const int64_t value_a = code[pc + 1];
      const int64_t address_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("eq %ld [%ld](%ld) [%ld] (%s)\n", value_a, address_b, code[address_b], address_c,
                value_a == code[address_b] ? "true" : "false");
      code[address_c] = value_a == code[address_b];
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_EQUALS_MIM: {
      const int64_t address_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("eq [%ld](%ld) %ld [%ld] (%s)\n", address_a, code[address_a], value_b, address_c,
                code[address_a] == value_b ? "true" : "false");
      code[address_c] = code[address_a] == value_b;
      pc += 4;
      break;
    }
    case INT_CODE_INSTRUCTION_EQUALS_IIM: {
      const int64_t value_a = code[pc + 1];
      const int64_t value_b = code[pc + 2];
      const int64_t address_c = code[pc + 3];
      DEBUG_LOG("eq %ld %ld [%ld] (%s)\n", value_a, value_b, address_c, value_a == value_b ? "true" : "false");
      code[address_c] = value_a == value_b;
      pc += 4;
      break;
    }

    default:
      // either an error occurred or I forgot to implement an opcode
      DEBUG_LOG("unknown opcode: %ld\n", code[pc]);
      tlbt_assert_unreachable();
      break;
    }
  }

done:
  return false;
}

static void swap_u8(uint8_t *a, uint8_t *b) {
  uint8_t t = *a;
  *a = *b;
  *b = t;
}

static void reverse_u8(uint8_t *a, size_t lo, size_t hi) {
  while (lo < hi) {
    swap_u8(&a[lo], &a[hi]);
    ++lo;
    --hi;
  }
}

static bool permute_next(uint8_t *idx, size_t n) {
  if (n < 2)
    return false;

  size_t i = n - 2;
  while (idx[i] >= idx[i + 1]) {
    if (i == 0) {
      reverse_u8(idx, 0, n - 1);
      return false;
    }
    --i;
  }

  size_t j = n - 1;
  while (idx[j] <= idx[i]) {
    --j;
  }

  swap_u8(&idx[i], &idx[j]);
  reverse_u8(idx, i + 1, n - 1);
  return true;
}

static int64_t solve_part1(const int64_t *const positions, const uint32_t position_count) {
  int64_t copy[MAX_POSITION_COUNT] = {0};
  memcpy(copy, positions, sizeof(int64_t) * position_count);

  intcode_machine_state state = {.code = copy, .code_count = position_count};
  int64_t input_buffer[4] = {0};
  int64_t output_buffer[64] = {0};
  tlbt_deque_i64_init(&state.input_queue, sizeof(input_buffer) / sizeof(input_buffer[0]), input_buffer);
  tlbt_deque_i64_init(&state.output_queue, sizeof(output_buffer) / sizeof(output_buffer[0]), output_buffer);

  int64_t biggest_signal = 0;
  uint8_t phase_settings[5] = {0, 1, 2, 3, 4};
  do {
    int64_t signal = 0;
    for (uint8_t i = 0; i < 5; ++i) {
      memcpy(copy, positions, sizeof(uint64_t) * position_count);
      state.pc = 0;
      tlbt_deque_i64_clear(&state.input_queue);
      tlbt_deque_i64_clear(&state.output_queue);
      tlbt_deque_i64_push_back(&state.input_queue, phase_settings[i]);
      tlbt_deque_i64_push_back(&state.input_queue, signal);
      run_intcode_machine(&state);
      signal = *tlbt_deque_i64_peek_back(&state.output_queue);
    }

    if (signal > biggest_signal) {
      biggest_signal = signal;
    }
  } while ((permute_next(phase_settings, 5)));

  return biggest_signal;
}

static int64_t solve_part2(const int64_t *const positions, const uint32_t position_count) {
  intcode_machine_state states[5] = {0};
  int64_t input_buffers[5][64] = {0};
  int64_t output_buffers[5][64] = {0};
  int64_t copies[5][MAX_POSITION_COUNT] = {0};

  for (uint8_t i = 0; i < 5; ++i) {
    memcpy(copies[i], positions, sizeof(int64_t) * position_count);
    states[i].code = copies[i];
    states[i].code_count = position_count;
    tlbt_deque_i64_init(&states[i].input_queue, sizeof(input_buffers[i]) / sizeof(input_buffers[i][0]),
                        input_buffers[i]);
    tlbt_deque_i64_init(&states[i].output_queue, sizeof(output_buffers[i]) / sizeof(output_buffers[i][0]),
                        output_buffers[i]);
  }

  int64_t biggest_signal = 0;
  const uint8_t phase_settings[5] = {5, 6, 7, 8, 9};
  uint8_t indices[5] = {0, 1, 2, 3, 4};
  do {
    for (uint8_t i = 0; i < 5; ++i) {
      states[i].pc = 0;
      tlbt_deque_i64_clear(&states[i].input_queue);
      tlbt_deque_i64_clear(&states[i].output_queue);
      tlbt_deque_i64_push_back(&states[i].input_queue, phase_settings[indices[i]]);
    }

    tlbt_deque_i64_push_back(&states[0].input_queue, 0);
    int64_t signal = 0;

    bool done = false;
    while (!done) {
      for (uint8_t i = 0; i < 5; ++i) {
        done = !run_intcode_machine(&states[i]);
        tlbt_assert_msg(states[i].output_queue.count == 1, "expected output queue to have exactly one output");
        signal = *tlbt_deque_i64_peek_front(&states[i].output_queue);
        tlbt_deque_i64_push_back(&states[(i + 1) % 5].input_queue, signal);
        tlbt_deque_i64_pop_front(&states[i].output_queue);
      }
    }

    if (signal > biggest_signal) {
      biggest_signal = signal;
    }
  } while ((permute_next(indices, 5)));

  return biggest_signal;
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

  const int64_t part1 = solve_part1(positions, position_count);
  const int64_t part2 = solve_part2(positions, position_count);

  printf("%ld\n", part1);
  printf("%ld\n", part2);
  return 0;
}

