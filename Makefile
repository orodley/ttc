NAME := ttc
CC ?= clang
CFLAGS := -std=c99 -Werror -Wall -Wextra -Wstrict-prototypes
CFLAGS += $(shell pkg-config --cflags sdl2)
LDFLAGS := $(shell pkg-config --libs sdl2)
OBJS := $(patsubst src/%.c, src/%.o, $(wildcard src/*.c))

.PHONY: all
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f src/*.o
	rm -f $(NAME)
