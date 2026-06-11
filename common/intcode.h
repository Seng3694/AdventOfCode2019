#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>

#include "../ext/toolbelt/src/assert.h"

typedef enum int_code_instruction {
  INT_CODE_INSTRUCTION_NONE = 0,
  INT_CODE_INSTRUCTION_ADD = 1,
  INT_CODE_INSTRUCTION_MUL = 2,
  INT_CODE_INSTRUCTION_LOAD = 3,
  INT_CODE_INSTRUCTION_PRINT = 4,
  INT_CODE_INSTRUCTION_JUMP_IF_TRUE = 5,
  INT_CODE_INSTRUCTION_JUMP_IF_FALSE = 6,
  INT_CODE_INSTRUCTION_LESS_THAN = 7,
  INT_CODE_INSTRUCTION_EQUALS = 8,
  INT_CODE_INSTRUCTION_RELATIVE_BASE_OFFSET = 9,
  INT_CODE_INSTRUCTION_HALT = 99,
} int_code_instruction;

#define TLBT_T int64_t
#define TLBT_T_NAME i64
#define TLBT_BASE2_CAPACITY
#define TLBT_NO_SORT
#define TLBT_STATIC
#include "../ext/toolbelt/src/deque.h"

typedef struct intcode_machine_state {
  int64_t *code;
  tlbt_deque_i64 input_queue;
  tlbt_deque_i64 output_queue;
  int64_t pc;
  int64_t relative_base;
  uint32_t code_count;
  uint32_t memory_capacity;
} intcode_machine_state;

typedef enum intcode_param_mode {
  INTCODE_PARAM_MODE_POSITION = 0,
  INTCODE_PARAM_MODE_IMMEDIATE = 1,
  INTCODE_PARAM_MODE_RELATIVE = 2,
} intcode_param_mode;

static inline void intcode_parse_input(char *input, const uint32_t max_code_count, int64_t *const code,
                                       uint32_t *const code_count) {
  uint32_t count = 0;
  for (;;) {
    switch (*input) {
    case '\0':
      *code_count = count;
      return;
    case '\n':
    case ',':
      ++input;
      break;
    default:
      tlbt_assert_fmt(isdigit(*input) || *input == '-', "digit expected, actual '%c' (%d)", *input, *input);
      tlbt_assert_fmt(count < max_code_count, "too much code, max: %u", max_code_count);
      code[count++] = strtoll(input, &input, 10);
      break;
    }
  }
}

static inline int64_t intcode_read_value(intcode_machine_state *const state, const int64_t param,
                                         const intcode_param_mode mode) {
  switch (mode) {
  case INTCODE_PARAM_MODE_POSITION:
    tlbt_assert_fmt(param < state->memory_capacity, "reading from address '%ld' beyond memory capacity '%u'", param,
                    state->memory_capacity);
    tlbt_assert_fmt(param >= 0, "reading from negative address is not allowed '%ld'", param);
    return state->code[param];
  case INTCODE_PARAM_MODE_IMMEDIATE:
    return param;
  case INTCODE_PARAM_MODE_RELATIVE:
    tlbt_assert_fmt(state->relative_base + param < state->memory_capacity,
                    "reading from address '%ld' beyond memory capacity'%u'", state->relative_base + param,
                    state->memory_capacity);
    tlbt_assert_fmt(state->relative_base + param >= 0, "reading from negative address is not allowed '%ld'", param);
    return state->code[state->relative_base + param];
  default:
    tlbt_assert_unreachable();
    return -1;
  }
}

static inline void intcode_write_value(intcode_machine_state *const state, const int64_t param,
                                       const intcode_param_mode mode, const int64_t value) {
  const int64_t address = param + (mode == INTCODE_PARAM_MODE_RELATIVE ? state->relative_base : 0);
  tlbt_assert_fmt(address < state->memory_capacity, "writing to address '%ld' beyond memory capacity '%u'", address,
                  state->memory_capacity);
  tlbt_assert_fmt(address >= 0, "writing to negative address is not allowed '%ld'", address);
  state->code[address] = value;
}

static inline bool intcode_run(intcode_machine_state *const state) {
  int64_t *code = state->code;
  int64_t pc = state->pc;
  int64_t param_a = 0, param_b = 0, param_c = 0, value_a = 0, value_b = 0;

  for (;;) {
    const int64_t c = code[pc];
    const int_code_instruction instr = c % 100;
    const intcode_param_mode param_modes[3] = {
        (c / 100) % 10,
        (c / 1000) % 10,
        (c / 10000) % 10,
    };

    switch (instr) {
    case INT_CODE_INSTRUCTION_HALT:
      goto done;

    case INT_CODE_INSTRUCTION_ADD: {
      param_a = code[pc + 1];
      param_b = code[pc + 2];
      param_c = code[pc + 3];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      value_b = intcode_read_value(state, param_b, param_modes[1]);
      intcode_write_value(state, param_c, param_modes[2], value_a + value_b);
      pc += 4;
      break;
    }

    case INT_CODE_INSTRUCTION_MUL: {
      param_a = code[pc + 1];
      param_b = code[pc + 2];
      param_c = code[pc + 3];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      value_b = intcode_read_value(state, param_b, param_modes[1]);
      intcode_write_value(state, param_c, param_modes[2], value_a * value_b);
      pc += 4;
      break;
    }

    // PRINT
    case INT_CODE_INSTRUCTION_PRINT: {
      param_a = code[pc + 1];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      tlbt_deque_i64_push_back(&state->output_queue, value_a);
      pc += 2;
      break;
    }

    // LOAD
    case INT_CODE_INSTRUCTION_LOAD: {
      if (state->input_queue.count == 0) {
        // save pc and return until new input arrives
        state->pc = pc;
        return true;
      }

      param_a = code[pc + 1];
      intcode_write_value(state, param_a, param_modes[0], *tlbt_deque_i64_peek_front(&state->input_queue));

      tlbt_deque_i64_pop_front(&state->input_queue);
      pc += 2;
      break;
    }

    // JMP_IF_TRUE
    case INT_CODE_INSTRUCTION_JUMP_IF_TRUE: {
      param_a = code[pc + 1];
      param_b = code[pc + 2];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      value_b = intcode_read_value(state, param_b, param_modes[1]);
      pc = value_a != 0 ? value_b : pc + 3;
      break;
    }

    // JMP_IF_FALSE
    case INT_CODE_INSTRUCTION_JUMP_IF_FALSE: {
      param_a = code[pc + 1];
      param_b = code[pc + 2];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      value_b = intcode_read_value(state, param_b, param_modes[1]);
      pc = value_a == 0 ? value_b : pc + 3;
      break;
    }

    // LESS_THAN
    case INT_CODE_INSTRUCTION_LESS_THAN: {
      param_a = code[pc + 1];
      param_b = code[pc + 2];
      param_c = code[pc + 3];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      value_b = intcode_read_value(state, param_b, param_modes[1]);
      intcode_write_value(state, param_c, param_modes[2], value_a < value_b);
      pc += 4;
      break;
    }

    // EQUALS
    case INT_CODE_INSTRUCTION_EQUALS: {
      param_a = code[pc + 1];
      param_b = code[pc + 2];
      param_c = code[pc + 3];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      value_b = intcode_read_value(state, param_b, param_modes[1]);
      intcode_write_value(state, param_c, param_modes[2], value_a == value_b);
      pc += 4;
      break;
    }

    case INT_CODE_INSTRUCTION_RELATIVE_BASE_OFFSET: {
      param_a = code[pc + 1];
      value_a = intcode_read_value(state, param_a, param_modes[0]);
      state->relative_base += value_a;
      pc += 2;
      break;
    }

    default:
      // either an error occurred or I forgot to implement an opcode
      printf("unknown instruction %ld -> %u (%u %u %u)\n", c, instr, param_modes[0], param_modes[1], param_modes[2]);
      tlbt_assert_unreachable();
      break;
    }
  }

done:
  return false;
}

