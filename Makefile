NAME = fractol

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Imlx

MLX_DIR = mlx
MLX_LIB = $(MLX_DIR)/libmlx.a

SRCS = fractol.c
OBJS = $(SRCS:.c=.o)

LDFLAGS = -L$(MLX_DIR) -lmlx -L/usr/X11/lib -lXext -lX11 -lm

all: $(NAME)

$(NAME): $(OBJS) $(MLX_LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(MLX_LIB):
	make -C $(MLX_DIR)

clean:
	rm -f $(OBJS)
	make -C $(MLX_DIR) clean

fclean: clean
	rm -f $(NAME)
	make -C $(MLX_DIR) fclean

re: fclean all

.PHONY: all clean fclean re