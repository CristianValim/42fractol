#include "mlx/mlx.h"
#include <X11/X.h>
#include <X11/keysym.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para memcpy
#include <unistd.h>

#define NUM_THREADS sysconf(_SC_NPROCESSORS_ONLN)
#define WIDTH 1080
#define HEIGHT 720
#define MAX_ITER 400

typedef struct s_data
{
	void	*mlx;
	void	*win;
	void	*img;
	char	*addr;
	int		bpp;
	int		line_length;
	int		endian;
	double	zoom;
	double	offset_x;
	double	offset_y;
	int		*pixel_buffer;
	int		*prev_pixel_buffer;
	double	prev_zoom;
	double	prev_offset_x;
	double	prev_offset_y;
}			t_data;

typedef struct s_thread_data
{
	t_data	*data;
	int		start_y;
	int		end_y;
}			t_thread_data;

void	flush_buffer_to_image(t_data *data)
{
	memcpy(data->addr, data->pixel_buffer, WIDTH * HEIGHT * sizeof(int));
}

int	get_color(int iter, int max_iter)
{
	double	t;
	int		r;
	int		g;
	int		b;

	if (iter == max_iter)
		return (0x000000);
	t = (double)iter / max_iter;
	r = (int)(9 * (1 - t) * t * t * t * 255);
	g = (int)(15 * (1 - t) * (1 - t) * t * t * 255);
	b = (int)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
	return ((r << 16) | (g << 8) | b);
}

int	calculate_max_iter(double zoom)
{
	(void)zoom;
	return (100);
}

int	mandelbrot(double real, double imaginary, int max_iter)
{
	double	z_real;
	double	z_imaginary;
	double	z_real_sq;
	double	z_imaginary_sq;
	int		iterations;
	double	temp;

	z_real = 0;
	z_imaginary = 0;
	z_real_sq = 0;
	z_imaginary_sq = 0;
	iterations = 0;
	while (z_real_sq + z_imaginary_sq < 4 && iterations < max_iter)
	{
		temp = z_real_sq - z_imaginary_sq + real;
		z_imaginary = 2 * z_real * z_imaginary + imaginary;
		z_real = temp;
		z_real_sq = z_real * z_real;
		z_imaginary_sq = z_imaginary * z_imaginary;
		iterations++;
	}
	return (iterations);
}

void	*draw_mandelbrot_thread(void *arg)
{
	t_thread_data	*thread_data;
	t_data			*data;
	double			width_half;
	double			height_half;
	double			zoom_factor;
	int				max_iter;

	thread_data = (t_thread_data *)arg;
	data = thread_data->data;
	double real, imag;
	int iter, color;
	int x, y = thread_data->start_y;
	width_half = WIDTH / 2.0;
	height_half = HEIGHT / 2.0;
	zoom_factor = 4.0 / (WIDTH * data->zoom);
	max_iter = calculate_max_iter(data->zoom);
	while (y < thread_data->end_y)
	{
		x = 0;
		while (x < WIDTH)
		{
			real = (x - width_half) * zoom_factor + data->offset_x;
			imag = (y - height_half) * zoom_factor + data->offset_y;
			iter = mandelbrot(real, imag, max_iter);
			color = get_color(iter, max_iter);
			data->pixel_buffer[y * WIDTH + x] = color;
			x++;
		}
		y++;
	}
	return (NULL);
}

void	draw_mandelbrot(t_data *data)
{
	pthread_t		threads[NUM_THREADS];
	t_thread_data	thread_data[NUM_THREADS];
	int				i;

	for (i = 0; i < NUM_THREADS; i++)
	{
		thread_data[i].data = data;
		thread_data[i].start_y = i * (HEIGHT / NUM_THREADS);
		thread_data[i].end_y = (i + 1) * (HEIGHT / NUM_THREADS);
		pthread_create(&threads[i], NULL, draw_mandelbrot_thread,
			&thread_data[i]);
	}
	for (i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}
}

void	reuse_pixels(t_data *data)
{
	int	dx;
	int	dy;
	int	new_x;
	int	new_y;

	dx = (data->offset_x - data->prev_offset_x) * WIDTH / 4.0 / data->zoom;
	dy = (data->offset_y - data->prev_offset_y) * HEIGHT / 4.0 / data->zoom;
	// Apenas reutilize pixels se o deslocamento for pequeno
	if (abs(dx) < WIDTH / 10 && abs(dy) < HEIGHT / 10)
	{
		for (int y = 0; y < HEIGHT; y++)
		{
			for (int x = 0; x < WIDTH; x++)
			{
				new_x = x + dx;
				new_y = y + dy;
				if (new_x >= 0 && new_x < WIDTH && new_y >= 0 && new_y < HEIGHT)
				{
					data->pixel_buffer[new_y * WIDTH
						+ new_x] = data->prev_pixel_buffer[y * WIDTH + x];
				}
			}
		}
	}
}

int	render(t_data *data)
{
	flush_buffer_to_image(data);
	mlx_put_image_to_window(data->mlx, data->win, data->img, 0, 0);
	return (0);
}

int	on_destroy(t_data *data)
{
	mlx_destroy_window(data->mlx, data->win);
	mlx_destroy_display(data->mlx);
	free(data->mlx);
	free(data->pixel_buffer);
	free(data->prev_pixel_buffer);
	exit(0);
	return (0);
}

int	on_keypress(int keysym, t_data *data)
{
	if (keysym == XK_Escape)
		on_destroy(data);
	else if (keysym == XK_plus || keysym == XK_equal)
		data->zoom *= 1.1;
	else if (keysym == XK_minus)
		data->zoom /= 1.1;
	else if (keysym == XK_Left)
		data->offset_x -= 0.1 / data->zoom;
	else if (keysym == XK_Right)
		data->offset_x += 0.1 / data->zoom;
	else if (keysym == XK_Up)
		data->offset_y -= 0.1 / data->zoom;
	else if (keysym == XK_Down)
		data->offset_y += 0.1 / data->zoom;
	if (data->zoom != data->prev_zoom || data->offset_x != data->prev_offset_x
		|| data->offset_y != data->prev_offset_y)
	{
		reuse_pixels(data);
		draw_mandelbrot(data);
		data->prev_zoom = data->zoom;
		data->prev_offset_x = data->offset_x;
		data->prev_offset_y = data->offset_y;
	}
	render(data);
	return (0);
}

int	on_mouse(int button, int x, int y, t_data *data)
{
	double	mouse_x;
	double	mouse_y;

	mouse_x = (x - WIDTH / 2.0) * 4.0 / (WIDTH * data->zoom) + data->offset_x;
	mouse_y = (y - HEIGHT / 2.0) * 4.0 / (HEIGHT * data->zoom) + data->offset_y;
	if (button == 4)
		data->zoom *= 1.05;
	else if (button == 5)
		data->zoom /= 1.05;
	data->offset_x = mouse_x - (x - WIDTH / 2.0) * 4.0 / (WIDTH * data->zoom);
	data->offset_y = mouse_y - (y - HEIGHT / 2.0) * 4.0 / (HEIGHT * data->zoom);
	if (data->zoom != data->prev_zoom || data->offset_x != data->prev_offset_x
		|| data->offset_y != data->prev_offset_y)
	{
		reuse_pixels(data);
		draw_mandelbrot(data);
		data->prev_zoom = data->zoom;
		data->prev_offset_x = data->offset_x;
		data->prev_offset_y = data->offset_y;
	}
	render(data);
	return (0);
}

int	main(void)
{
	t_data data;

	data.mlx = mlx_init();
	if (!data.mlx)
		return (1);
	data.win = mlx_new_window(data.mlx, WIDTH, HEIGHT, "Mandelbrot Set");
	if (!data.win)
		return (free(data.mlx), 1);
	data.img = mlx_new_image(data.mlx, WIDTH, HEIGHT);
	if (!data.img)
		return (mlx_destroy_window(data.mlx, data.win), free(data.mlx), 1);
	data.addr = mlx_get_data_addr(data.img, &data.bpp, &data.line_length,
			&data.endian);

	data.pixel_buffer = malloc(WIDTH * HEIGHT * sizeof(int));
	if (!data.pixel_buffer)
	{
		perror("Failed to allocate pixel buffer");
		exit(1);
	}

	data.prev_pixel_buffer = malloc(WIDTH * HEIGHT * sizeof(int));
	if (!data.prev_pixel_buffer)
	{
		perror("Failed to allocate previous pixel buffer");
		exit(1);
	}

	data.zoom = 1.0;
	data.offset_x = 0.0;
	data.offset_y = 0.0;
	data.prev_zoom = data.zoom;
	data.prev_offset_x = data.offset_x;
	data.prev_offset_y = data.offset_y;

	mlx_hook(data.win, KeyRelease, KeyReleaseMask, &on_keypress, &data);
	mlx_hook(data.win, DestroyNotify, StructureNotifyMask, &on_destroy, &data);
	mlx_mouse_hook(data.win, &on_mouse, &data);
	draw_mandelbrot(&data);
	flush_buffer_to_image(&data);
	render(&data);
	mlx_loop_hook(data.mlx, &render, &data);
	mlx_loop(data.mlx);
}