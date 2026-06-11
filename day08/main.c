#include <stdio.h>
#include <stdint.h>

#include "../common/fileutils.h"

static uint32_t solve_part1(char *input, const uint32_t width, const uint32_t height, uint32_t *layers) {
  const uint32_t image_size = width * height;
  char *p = input;
  uint32_t min_zeros = UINT32_MAX;
  uint32_t min_ones = 0;
  uint32_t min_twos = 0;
  uint32_t layer = 0;
  while (*p != '\n' && *p != '\0') {
    uint32_t zeros = 0;
    uint32_t ones = 0;
    uint32_t twos = 0;
    for (uint32_t i = 0; i < image_size; ++i) {
      zeros += p[i] == '0';
      ones += p[i] == '1';
      twos += p[i] == '2';
    }
    if (zeros < min_zeros) {
      min_zeros = zeros;
      min_ones = ones;
      min_twos = twos;
    }
    p += image_size;
    layer++;
  }
  *layers = layer;
  return min_ones * min_twos;
}

static void solve_part2(char *input, const uint32_t width, const uint32_t height, const uint32_t layers) {
  // go through every pixel and jump through the layers from top to bottom until a pixel which isn't transparent appears
  // and print it. 0=black, 1=white, 2=transparent
  const uint32_t image_size = width * height;
  for (uint32_t y = 0; y < height; ++y) {
    const uint32_t y_offset = y * width;
    for (uint32_t x = 0; x < width; ++x) {
      for (uint32_t l = 0; l < layers; ++l) {
        const uint32_t index = l * image_size + y_offset + x;
        if (input[index] != '2') {
          putchar(input[index] == '0' ? '.' : '#');
          break;
        }
      }
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

  uint32_t layers = 0;
  printf("%u\n", solve_part1(input, 25, 6, &layers));
  solve_part2(input, 25, 6, layers);

  free(input);
}

