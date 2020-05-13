#ifndef TILEGEN_H
#define TILEGEN_H

#include <cairo/cairo.h>

#include <cstdint>
#include <cstdlib>
#include <functional>

void
cairo_draw_gl_rgba(
	std::size_t w, std::size_t h,
	uint8_t * data, std::size_t stride,
	const std::function<void(std::size_t, std::size_t, cairo_t *)> & draw_fn);

void
cairo_draw_gl_rgba(
	std::size_t w, std::size_t h,
	uint8_t * data, std::size_t stride,
	const std::function<void(cairo_t *)> & draw_fn);

void
write_rgba_to_png(std::size_t w, std::size_t h, uint8_t * data, std::size_t stride, const std::string& filename);

void
draw_solid(cairo_t * c);

void
draw_tile_background(cairo_t * c);

void
draw_straight_arrow(cairo_t * c);

void
draw_straight_arrow2(cairo_t * c);

void
draw_straight_arrow3(cairo_t * c);

void
draw_right_arrow(cairo_t * c);

void
draw_left_arrow(cairo_t * c);

void
draw_button_background(cairo_t * c);

void
draw_pressed_button_background(cairo_t * c);

void
draw_abort_button(cairo_t * c);

void
draw_pause_button(cairo_t * c);

void
draw_run_button(cairo_t * c);

void
draw_run2_button(cairo_t * c);

void
draw_run3_button(cairo_t * c);

void
draw_cross(cairo_t * c);

void
draw_conditional(cairo_t * c);

void
draw_centered_text(cairo_t * c, const char * text, double font_size);

void
draw_repeat(cairo_t * c, std::size_t count);

void
draw_trigger1(cairo_t * c);

void
draw_trigger2(cairo_t * c);

void
draw_trigger3(cairo_t * c);

void
draw_back_icon(cairo_t * c);

void
draw_exit_icon(cairo_t * c);

#endif
