/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fractol.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cvalim-d <cvalim-d@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/27 15:27:48 by cvalim-d          #+#    #+#             */
/*   Updated: 2025/01/28 17:37:10 by cvalim-d         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mlx/mlx.h"
#include <X11/X.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_data
{
	void	*mlx;
	void	*win;
}			t_data;

int	on_destroy(t_data *data)
{
	mlx_destroy_window(data->mlx, data->win);
	mlx_destroy_display(data->mlx);
	free(data->mlx);
	exit(0);
	return (0);
}

int	on_keypress(int keysym, t_data *data)
{
	(void)data;
	printf("Pressed key: %d\\n", keysym);
	if(keysym == 65307)
		on_destroy(data);
	return (0);
}

int mandelbrot(double cr, double ci, int max_iter)
{
    double zr = 0.0, zi = 0.0;
    int iter = 0;
    while (zr * zr + zi * zi < 4.0 && iter < max_iter)
    {
        double tmp = zr * zr - zi * zi + cr;
        zi = 2.0 * zr * zi + ci;
        zr = tmp;
        iter++;
    }
    return iter;
}

void draw_mandelbrot(t_data *data, int width, int height, int max_iter)
{
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            double cr = (x - width / 2.0) * 4.0 / width;
            double ci = (y - height / 2.0) * 4.0 / height;
            int iter = mandelbrot(cr, ci, max_iter);
            int color = iter == max_iter ? 0x000000 : 0xFFFFFF * iter / max_iter;
            mlx_pixel_put(data->mlx, data->win, x, y, color);
        }
    }
}

int	main(void)
{
	t_data data;
    int width = 600;
    int height = 400;
    int max_iter = 1000;
	
     data.mlx = mlx_init();
    if (!data.mlx)
        return (1);
    data.win = mlx_new_window(data.mlx, width, height, "Mandelbrot Set");
    if (!data.win)
        return (free(data.mlx), 1);

    draw_mandelbrot(&data, width, height, max_iter);

    mlx_hook(data.win, KeyRelease, KeyReleaseMask, &on_keypress, &data);

    mlx_hook(data.win, DestroyNotify, StructureNotifyMask, &on_destroy, &data);

    mlx_loop(data.mlx);
    return (0);
}