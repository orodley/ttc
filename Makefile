NAME := ttc
CC ?= clang
CFLAGS := -std=c99 -Werror -Wall -Wextra -Wstrict-prototypes
OBJS := $(patsubst src/%.c, src/%.o, $(wildcard src/*.c))

.PHONY: all
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $^ -o $@

.PHONY: clean
clean:
	rm -f src/*.o
	rm -f $(NAME)
