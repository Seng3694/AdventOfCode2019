CC:=gcc

CFLAGS:=-g -O0 -Wall -Wextra -Werror -Wimplicit-function-declaration -std=c17 -fsanitize=undefined -fsanitize=address
LDFLAGS:=-lm
BUILD_MODE:=DEBUG

ifdef release
# exclude the unused parameter warnings for release builds for now due to some parameters only being used for assertions (debug)
CFLAGS:=-O3 -Wall -Wextra -Werror -Wimplicit-function-declaration -Wno-unused-parameter -std=c17 -DNDEBUG
BUILD_MODE:=RELEASE
endif

DAYS:=$(wildcard day*)
TARGETS:=$(DAYS:%=build/%)

all: $(TARGETS)

build/%: %/main.c | build
	$(CC) $(CFLAGS) -MMD -MP $< -o $@ $(LDFLAGS)

$(DAYS): %: build/%

-include $(OBJS:.o=.d)

build:
	mkdir -p $@

clean:
	rm -rf build

.PHONY: all clean $(DAYS)

