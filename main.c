#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "shm.h"
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pixman.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "draw_help.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include <fcft/fcft.h>

#define NELEMS(array) (sizeof(array) / sizeof(array[0]))
#define ARRAY_SUM(arr, length)                             \
	({                                                 \
		int _sum = 0;                              \
		for (size_t _i = 0; _i < (length); ++_i) { \
			_sum += (arr)[_i];                 \
		}                                          \
		_sum;                                      \
	})

struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_region *region;
struct wl_shm *shm;
struct wl_buffer *buffer;

struct xdg_wm_base *XDGWMBase;
struct xdg_surface *XDGSurface;
struct xdg_toplevel *XDGToplevel;
struct zwlr_layer_shell_v1 *WLRLayerShell;
struct zwlr_layer_surface_v1 *WLRLayerSurface;

// static uint32_t box_widths[] = { 8, 10, 14, 16, 18, 20, 25, 30, 40 };
static uint32_t box_widths[] = { 16, 20, 28, 32, 36, 40, 50, 60, 80 };
static uint32_t main_boxwidth;
static uint32_t total_width;
static uint32_t side_width;
static uint32_t height = 120;
uint8_t buffer_scale = 2;
struct fcft_font *font;
struct progress {
	int i;
	int x;
	int xo;
};
struct progress progress = { 0, 0, 0 };
static char text[256];

static pixman_color_t outline_color = {
	.red = 0xb4b4,
	.green = 0x0000,
	.blue = 0xb4b4,
	.alpha = 0x1919,
};
static pixman_color_t bg_color = {
	.red = 0x0000,
	.green = 0x0000,
	.blue = 0x0000,
	.alpha = 0x1919,
};
static pixman_color_t text_color = {
	.red = 0xffff,
	.green = 0xffff,
	.blue = 0xffff,
	.alpha = 0xffff,
};

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
	/* Sent by the compositor when it's no longer using this buffer */
	wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};
static void paint_outlined_boxes(pixman_image_t *final, int x, int xo,
				 int width)
{
	pixman_color_t outline_color_alphad = outline_color;
	outline_color_alphad.alpha *= progress.i;
	outline_color_alphad.red =
		outline_color_alphad.red / 10 * (progress.i + 1);
	outline_color_alphad.blue =
		outline_color_alphad.blue / 10 * (progress.i + 1);
	pixman_image_fill_rectangles(
		PIXMAN_OP_OVER, final, &outline_color_alphad, 2,
		(pixman_rectangle16_t[]){ { x, 0, width, height },
					  { xo, 0, width, height } });

	pixman_color_t bg_color_alphad = bg_color;
	bg_color_alphad.alpha *= progress.i;
	pixman_image_fill_rectangles(
		PIXMAN_OP_SRC, final, &bg_color_alphad, 2,
		(pixman_rectangle16_t[]){
			{ x + 4, 4, width - 8, height - 8 },
			{ xo + 4, 4, width - 8, height - 8 } });
}

static uint32_t *persistent_data = NULL;
static pixman_image_t *final = NULL;

static int stride;
static int size;

void adjust_image_alpha(pixman_image_t *image, int width, int height, int alpha)
{
	pixman_color_t mask_color = { .red = 0x0000,
				      .green = 0x0000,
				      .blue = 0x0000,
				      .alpha = 0xffff - (alpha << 8) - alpha };

	pixman_image_t *alpha_mask =
		pixman_image_create_solid_fill(&mask_color);
	pixman_image_composite32(PIXMAN_OP_IN_REVERSE, alpha_mask, NULL, image,
				 0, 0, 0, 0, 0, 0, width, height);

	pixman_image_unref(alpha_mask);
}
static struct wl_buffer *draw_frame(int width, int height)
{
	if (!buffer) {
		stride = width * 4;
		size = stride * height;

		int fd = allocate_shm_file(size);
		if (fd == -1) {
			return NULL;
		}

		persistent_data = mmap(NULL, size, PROT_READ | PROT_WRITE,
				       MAP_SHARED, fd, 0);
		if (persistent_data == MAP_FAILED) {
			close(fd);
			return NULL;
		}

		struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
		buffer = wl_shm_pool_create_buffer(
			pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
		wl_shm_pool_destroy(pool);
		close(fd);
		final = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height,
						 persistent_data, stride);
	}

	if (progress.i < NELEMS(box_widths)) {
		progress.xo -= box_widths[progress.i] + 10;
		paint_outlined_boxes(final, progress.x, progress.xo,
				     box_widths[progress.i]);
		progress.x += box_widths[progress.i] + 10;
	} else if (progress.i == NELEMS(box_widths)) {
		bg_color.alpha *= progress.i;
		pixman_image_fill_rectangles(PIXMAN_OP_OVER, final,
					     &outline_color, 1,
					     &(pixman_rectangle16_t){
						     progress.x,
						     0,
						     main_boxwidth,
						     height,
					     });

		pixman_image_fill_rectangles(PIXMAN_OP_SRC, final, &bg_color, 1,
					     &(pixman_rectangle16_t){
						     progress.x + 4,
						     4,
						     main_boxwidth - 8,
						     height - 8,
					     });

		draw_text(text, progress.x, height / 2 - font->height / 2,
			  final, NULL, &text_color, NULL, 3840, 0, 20, font);

	} else if (progress.i > 200) {
		adjust_image_alpha(final, width, height, progress.i - 200);
	}
	progress.i += 1;
	return buffer;
}

static void
zwlr_surface_configure(void *data,
		       struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1,
		       uint32_t serial, uint32_t width, uint32_t height)
{
	zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);
	buffer = draw_frame(total_width, height);
	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_commit(surface);
}

static const struct zwlr_layer_surface_v1_listener
	zwlr_layer_surface_listener = { .configure = zwlr_surface_configure };

static void registry_global(void *data, struct wl_registry *wl_registry,
			    uint32_t name, const char *interface,
			    uint32_t version)
{
	struct client_state *state = data;
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(wl_registry, name,
					      &wl_compositor_interface, 4);
	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		WLRLayerShell = wl_registry_bind(
			wl_registry, name, &zwlr_layer_shell_v1_interface, 3);
	}
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry,
				   uint32_t name)
{
	/* This space deliberately left blank */
}
static const struct wl_registry_listener wl_registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

static const struct wl_callback_listener wl_surface_frame_listener;

static void wl_surface_frame_done(void *data, struct wl_callback *cb,
				  uint32_t time)
{
	/* Destroy this callback */
	wl_callback_destroy(cb);
	if (progress.i > 250) {
		pixman_image_unref(final);
		return;
	}

	/* Request another frame */
	cb = wl_surface_frame(surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, data + 1);
	/* Submit a frame for this event */
	struct wl_buffer *buffer = draw_frame(total_width, height);
	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_damage_buffer(surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface);
}

static const struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

void init_fonts()
{
	fcft_init(0, 0, FCFT_LOG_CLASS_NONE);
	fcft_set_scaling_filter(FCFT_SCALING_FILTER_LANCZOS3);

	font = fcft_from_name(1, (const char *[]){ "Hack:size=18" },
			      "dpi=184:weight=bold");
}
int main()
{
	fgets(text, sizeof(text), stdin);
	size_t len = strlen(text);
	if (len > 0 && text[len - 1] == '\n') {
		text[len - 1] = 0;
	}
	init_fonts();
	main_boxwidth = TEXT_WIDTH(text, 3840, 20, font);
	side_width = (ARRAY_SUM(box_widths, NELEMS(box_widths)) +
		      NELEMS(box_widths) * 10);
	total_width = main_boxwidth + side_width * 2;
	progress.xo = total_width + 10;
	struct wl_display *display = wl_display_connect(NULL);
	struct wl_registry *wl_registry = wl_display_get_registry(display);
	wl_registry_add_listener(wl_registry, &wl_registry_listener, NULL);
	wl_display_roundtrip(display);
	surface = wl_compositor_create_surface(compositor);
	wl_surface_set_buffer_scale(surface, 2);
	WLRLayerSurface = zwlr_layer_shell_v1_get_layer_surface(
		WLRLayerShell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
		"cooltify");
	zwlr_layer_surface_v1_add_listener(WLRLayerSurface,
					   &zwlr_layer_surface_listener, NULL);

	zwlr_layer_surface_v1_set_size(
		WLRLayerSurface, total_width / 2 + total_width % 2, height);
	zwlr_layer_surface_v1_set_anchor(WLRLayerSurface,
					 ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);

	zwlr_layer_surface_v1_set_margin(WLRLayerSurface, 50, 0, 0, 0);
	wl_surface_commit(surface);

	struct wl_callback *cb = wl_surface_frame(surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, 0);

	while (wl_display_dispatch(display) && progress.i < 250) {
		/* This space deliberately left blank */
	}
	zwlr_layer_surface_v1_destroy(WLRLayerSurface);
	wl_surface_destroy(surface);

	zwlr_layer_shell_v1_destroy(WLRLayerShell);
	wl_buffer_destroy(buffer);

	wl_compositor_destroy(compositor);
	wl_shm_destroy(shm);
	wl_registry_destroy(wl_registry);
	wl_display_roundtrip(display);
	wl_display_disconnect(display);

	return 0;
}
