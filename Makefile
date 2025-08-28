NAME = ft_ping
MODE ?= release

INT_DIR = int
OBJ_DIR = $(addprefix $(INT_DIR)/, obj-$(MODE))
INCLUDES = -Iincludes

CC = cc
CFLAGS = -Wall -Werror -Wextra -MD $(INCLUDES)

ifeq ($(MODE), debug)
	CC = gcc
	CFLAGS = -Wall -Wextra -MD $(INCLUDES) -g3
endif

VPATH = srcs

SRCS =	main.c			\
		options.c		\

OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

RESET			= \033[0m
GRAY			= \033[90m
RED 			= \033[31m
GREEN 			= \033[32m
YELLOW 			= \033[33m
BLUE 			= \033[34m

all:
	$(MAKE) $(NAME)
	printf "$(RESET)"

debug:
	$(MAKE) MODE=debug all
	./ft_ping "8.8.8.8"

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(CLFAGS) -o $(NAME)

$(OBJ_DIR)/%.o: %.c Makefile | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	printf "$(GRAY)compiling: $(BLUE)%-40s $(GRAY)[%d/%d]\n" "$<" "$$(ls $(OBJ_DIR) | grep -c '\.o')" "$(words $(SRCS))"

clean:
	rm -rf $(INT_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re debug

-include $(DEPS)
.SILENT:
MAKEFLAGS=--no-print-directory
