#include <stdint.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <pixman.h>
#include <fcft/fcft.h>

uint32_t draw_text(char *text, uint32_t x, uint32_t y,
			  pixman_image_t *foreground,
			  pixman_image_t *background, pixman_color_t *fg_color,
			  pixman_color_t *bg_color, uint32_t max_x,
			  uint32_t buf_height, uint32_t padding,
			  struct fcft_font *font);

#define TEXT_WIDTH(text, maxwidth, padding, font)                           \
	draw_text(text, 0, 0, NULL, NULL, NULL, NULL, maxwidth, 0, padding, \
		  font)
