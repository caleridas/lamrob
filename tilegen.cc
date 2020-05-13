#include "tilegen.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <sstream>

#include "noise2d.h"

void
cairo_draw_gl_rgba(
	std::size_t w, std::size_t h,
	uint8_t * data, std::size_t stride,
	const std::function<void(std::size_t, std::size_t, cairo_t *)> & draw_fn)
{
	std::unique_ptr<unsigned char[]> array(new unsigned char[w * h * 4]);
	std::memset(array.get(), 255, w * h * 4);
	cairo_surface_t * s = cairo_image_surface_create_for_data (
		array.get(), CAIRO_FORMAT_ARGB32, w, h, 4 * w);
	cairo_t * cr = cairo_create(s);

	cairo_operator_t op = cairo_get_operator(cr);
	cairo_rectangle(cr, 0., 0., w, h);
	cairo_set_source_rgba(cr, 0., 0., 0., 0.);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_fill(cr);
	cairo_set_operator(cr, op);

	cairo_set_source_rgba(cr, 1., 1., 1., 1.);
	draw_fn(w, h, cr);

	cairo_destroy(cr);
	cairo_surface_destroy(s);

	for (std::size_t y = 0; y < h; ++y) {
		for (std::size_t x = 0; x < w ; ++x) {
			data[(x + y * stride) * 4 + 0] = array[(x + y * w) * 4 + 2]; // r
			data[(x + y * stride) * 4 + 1] = array[(x + y * w) * 4 + 1]; // g
			data[(x + y * stride) * 4 + 2] = array[(x + y * w) * 4 + 0]; // b
			data[(x + y * stride) * 4 + 3] = array[(x + y * w) * 4 + 3]; // a
		}
	}
}

void
cairo_draw_gl_rgba(
	std::size_t w, std::size_t h,
	uint8_t * data, std::size_t stride,
	const std::function<void(cairo_t *)> & draw_fn)
{
	std::unique_ptr<unsigned char[]> array(new unsigned char[w * h * 4]);
	std::memset(array.get(), 255, w * h * 4);
	cairo_surface_t * s = cairo_image_surface_create_for_data (
		array.get(), CAIRO_FORMAT_ARGB32, w, h, 4 * w);
	cairo_t * cr = cairo_create(s);

	cairo_operator_t op = cairo_get_operator(cr);
	cairo_rectangle(cr, 0., 0., w, h);
	cairo_set_source_rgba(cr, 0., 0., 0., 0.);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_fill(cr);
	cairo_set_operator(cr, op);

	cairo_set_source_rgba(cr, 1., 1., 1., 1.);
	cairo_scale(cr, w, h);
	draw_fn(cr);

	cairo_destroy(cr);
	cairo_surface_destroy(s);

	for (std::size_t y = 0; y < h; ++y) {
		for (std::size_t x = 0; x < w ; ++x) {
			data[(x + y * stride) * 4 + 0] = array[(x + y * w) * 4 + 2]; // r
			data[(x + y * stride) * 4 + 1] = array[(x + y * w) * 4 + 1]; // g
			data[(x + y * stride) * 4 + 2] = array[(x + y * w) * 4 + 0]; // b
			data[(x + y * stride) * 4 + 3] = array[(x + y * w) * 4 + 3]; // a
		}
	}
}

void
write_rgba_to_png(std::size_t w, std::size_t h, uint8_t * data, std::size_t stride, const std::string& filename)
{
	std::unique_ptr<unsigned char[]> array(new unsigned char[w * h * 4]);
	for (std::size_t y = 0; y < h; ++y) {
		for (std::size_t x = 0; x < w ; ++x) {
			array[(x + y * w) * 4 + 0] = data[(x + y * stride) * 4 + 2]; // b
			array[(x + y * w) * 4 + 1] = data[(x + y * stride) * 4 + 1]; // g
			array[(x + y * w) * 4 + 2] = data[(x + y * stride) * 4 + 0]; // r
			array[(x + y * w) * 4 + 3] = data[(x + y * stride) * 4 + 3]; // a
		}
	}
	cairo_surface_t * s = cairo_image_surface_create_for_data (
		array.get(), CAIRO_FORMAT_ARGB32, w, h, 4 * w);
	cairo_surface_write_to_png (s, filename.c_str());

	cairo_surface_destroy(s);
}

namespace {

struct draw_command {
	char type;
	double x1, y1, x2, y2, x3, y3;
};

void execute_command(const draw_command & cmd, cairo_t * c, double xo, double yo)
{
	switch (cmd.type) {
		case 'm': {
			cairo_move_to(c, cmd.x1 + xo, cmd.y1 + yo);
			break;
		}
		case 'l': {
			cairo_line_to(c, cmd.x1 + xo, cmd.y1 + yo);
			break;
		}
		case 'c': {
			cairo_curve_to(c, cmd.x1 + xo, cmd.y1 + yo, cmd.x2 + xo, cmd.y2 + yo, cmd.x3 + xo, cmd.y3 + yo);
			break;
		}
		case 'z': {
			cairo_close_path(c);
			break;
		}
	}
}

void execute_commands(const draw_command * cmds, std::size_t count, cairo_t * cr, double xo = 0.0, double yo = 0.0)
{
	for (size_t n = 0; n < count; ++n) {
		execute_command(cmds[n], cr, xo, yo);
	}
	cairo_fill(cr);
}

}

void
draw_straight_arrow(cairo_t * c)
{
	const draw_command cmds[] = {
		{'m', 0.4, 0.9},
		{'l', 0.4, 0.25},
		{'l', 0.3, 0.25},
		{'l', 0.5, 0.1},
		{'l', 0.7, 0.25},
		{'l', 0.6, 0.25},
		{'l', 0.6, 0.9},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_straight_arrow2(cairo_t * c)
{
	const draw_command cmds[] = {
		{'m', 0.4, 0.45},
		{'l', 0.4, 0.25},
		{'l', 0.3, 0.25},
		{'l', 0.5, 0.1},
		{'l', 0.7, 0.25},
		{'l', 0.6, 0.25},
		{'l', 0.6, 0.45},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c, 0.0, 0.45);
}

void
draw_straight_arrow3(cairo_t * c)
{
	const draw_command cmds[] = {
		{'m', 0.4, 0.3},
		{'l', 0.4, 0.25},
		{'l', 0.3, 0.25},
		{'l', 0.5, 0.1},
		{'l', 0.7, 0.25},
		{'l', 0.6, 0.25},
		{'l', 0.6, 0.3},
		{'z'},
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c, 0.0, 0.3);
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c, 0.0, 0.6);
}

void
draw_right_arrow(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0.2, 0.9},
		{'c', 0.2, 0.45,  0.35, 0.3,  0.75, 0.3},
		{'l', 0.75, 0.2},
		{'l', 0.9, 0.4},
		{'l', 0.75, 0.6},
		{'l', 0.75, 0.5},
		{'c', 0.4, 0.5,  0.4, 0.7,  0.4, 0.9},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_left_arrow(cairo_t * c)
{
	cairo_matrix_t m = {-1., 0., 0., 1., 1., 0.};
	cairo_save(c);
	cairo_transform(c, &m);
	draw_right_arrow(c);
	cairo_restore(c);
}

static void release_data(void* data)
{
	delete []reinterpret_cast<uint32_t *>(data);
}

cairo_surface_t * make_marble_surface()
{
	static const noise2d noise(6, 0);

	std::size_t dim = 64;
	uint32_t * data = new uint32_t[dim * dim * 4];

	for (std::size_t y = 0; y < 64; ++y) {
		for (std::size_t x = 0; x < 64; ++x) {
			double r = noise.sample(x / 64., y / 64.);
			r = sin(((x / 64.) + (y / 64.) * .4 + r * .7) * 15);
			r = (r * 0.05 + 0.3);

			unsigned char c = static_cast<unsigned char>(r * 255);
			uint32_t p = 0xff000000UL + c * 0x00010101UL;
			data[y * 64 + x] = p;
		}
	}

	cairo_surface_t * surface = cairo_image_surface_create_for_data(
		reinterpret_cast<unsigned char *>(data), CAIRO_FORMAT_ARGB32, 64, 64, 64 * 4);

	static const cairo_user_data_key_t key = {};

	cairo_surface_set_user_data(surface, &key, data, release_data);

	return surface;
}

void
draw_tile_background(cairo_t * c)
{
	cairo_surface_t * marble = make_marble_surface();

	cairo_rectangle(c, 0, 0, 1, 1);
	cairo_set_source_rgba(c, .5, .5, .5, 1);
	cairo_fill(c);

	static constexpr double d0 = 0.00;
	static constexpr double d1 = 0.04;
	static constexpr double d2 = 0.06;
	static constexpr double d3 = 0.08;

	static const draw_command highlight[] = {
		{'m', 1. - d0, 1. - d0},
		{'l', 0. + d0, 1. - d0},
		{'l', 0. + d1, 1. - d1},
		{'l', 1. - d1, 1. - d1},
		{'l', 1. - d1, 0. + d1},
		{'l', 1. - d0, 0. + d0},
		{'z'},
		{'m', 0. + d2, 0. + d2},
		{'l', 1. - d2, 0. + d2},
		{'l', 1. - d3, 0. + d3},
		{'l', 0. + d3, 0. + d3},
		{'l', 0. + d3, 1. - d3},
		{'l', 0. + d2, 1. - d2},
		{'z'},
	};
	cairo_set_source_rgba(c, 1, 1, 1, 1);
	execute_commands(highlight, sizeof(highlight) / sizeof(highlight[0]), c);

	static const draw_command shade[] = {
		{'m', 0. + d0, 0. + d0},
		{'l', 1. - d0, 0. + d0},
		{'l', 1. - d1, 0. + d1},
		{'l', 0. + d1, 0. + d1},
		{'l', 0. + d1, 1. - d1},
		{'l', 0. + d0, 1. - d0},
		{'z'},
		{'m', 1. - d2, 1. - d2},
		{'l', 0. + d2, 1. - d2},
		{'l', 0. + d3, 1. - d3},
		{'l', 1. - d3, 1. - d3},
		{'l', 1. - d3, 0. + d3},
		{'l', 1. - d2, 0. + d2},
		{'z'},
	};
	cairo_set_source_rgba(c, .2, .2, .2, 1);
	execute_commands(shade, sizeof(shade) / sizeof(shade[0]), c);

	cairo_set_operator(c, CAIRO_OPERATOR_MULTIPLY);
	cairo_rectangle(c, 0, 0, 1., 1.);

	cairo_scale(c, 1/63., 1/63.);
	cairo_set_source_surface(c, marble, -0.5, -0.5);
	cairo_fill(c);

	cairo_surface_destroy(marble);
}

void
draw_solid(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0, 0},
		{'l', 1, 0},
		{'l', 1, 1},
		{'l', 0, 1},
		{'z'},
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

static const double edge_width = 0.05;

static const draw_command tl_edge_cmds[] = {
	{'m', 0.0, 0.0},
	{'l', 1.0, 0.0},
	{'l', 1.0 - edge_width, edge_width},
	{'l', edge_width, edge_width},
	{'l', edge_width, 1.0 - edge_width},
	{'l', 0.0, 1.0},
	{'z'}
};

static const draw_command br_edge_cmds[] = {
	{'m', 1.0, 1.0},
	{'l', 0.0, 1.0},
	{'l', edge_width, 1.0 - edge_width},
	{'l', 1.0 - edge_width, 1.0 - edge_width},
	{'l', 1.0 - edge_width, edge_width},
	{'l', 1.0, 0.0},
	{'z'}
};


void
draw_button_background(cairo_t * c)
{
	cairo_rectangle(c, 0., 0., 1., 1.);
	cairo_set_source_rgba(c, .2, .2, .2, 1.);
	cairo_fill(c);

	cairo_set_source_rgba(c, .1, .1, .1, 1.);
	execute_commands(tl_edge_cmds, sizeof(tl_edge_cmds) / sizeof(tl_edge_cmds[0]), c);

	cairo_set_source_rgba(c, .3, .3, .3, 1.);
	execute_commands(br_edge_cmds, sizeof(br_edge_cmds) / sizeof(br_edge_cmds[0]), c);
}

void
draw_pressed_button_background(cairo_t * c)
{
	cairo_rectangle(c, 0., 0., 1., 1.);
	cairo_set_source_rgba(c, .15, .15, .15, 1.);
	cairo_fill(c);

	cairo_set_source_rgba(c, .3, .3, .3, 1.);
	execute_commands(tl_edge_cmds, sizeof(tl_edge_cmds) / sizeof(tl_edge_cmds[0]), c);

	cairo_set_source_rgba(c, .1, .1, .1, 1.);
	execute_commands(br_edge_cmds, sizeof(br_edge_cmds) / sizeof(br_edge_cmds[0]), c);
}

void
draw_abort_button(cairo_t * c) {
	static const draw_command cmds[] = {
		{'m', 0.30, 0.90},
		{'c', 0.25, 0.80, 0.25, 0.80, 0.25, 0.70},
		{'l', 0.25, 0.30},
		{'c', 0.25, 0.25, 0.33, 0.25, 0.33, 0.30},
		{'l', 0.33, 0.55},
		{'l', 0.35, 0.55},
		{'l', 0.35, 0.15},
		{'c', 0.35, 0.10, 0.43, 0.10, 0.43, 0.15},
		{'l', 0.43, 0.50},
		{'l', 0.45, 0.50},
		{'l', 0.45, 0.10},
		{'c', 0.45, 0.05, 0.53, 0.05, 0.53, 0.10},
		{'l', 0.53, 0.50},
		{'l', 0.55, 0.50},
		{'l', 0.55, 0.20},
		{'c', 0.55, 0.15, 0.63, 0.15, 0.63, 0.20},
		{'l', 0.63, 0.65},
		{'l', 0.65, 0.65},
		{'l', 0.65, 0.40},
		{'c', 0.65, 0.35, 0.73, 0.35, 0.73, 0.40},
		{'l', 0.73, 0.70},
		{'c', 0.73, 0.80, 0.70, 0.80, 0.65, 0.90},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_pause_button(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0.25, 0.3},
		{'l', 0.45, 0.3},
		{'l', 0.45, 0.7},
		{'l', 0.25, 0.7},
		{'z'},
		{'m', 0.55, 0.3},
		{'l', 0.75, 0.3},
		{'l', 0.75, 0.7},
		{'l', 0.55, 0.7},
		{'z'},
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_run_button(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0.3, 0.3},
		{'l', 0.7, 0.5},
		{'l', 0.3, 0.7},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_run2_button(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0.2, 0.3},
		{'l', 0.45, 0.5},
		{'l', 0.2, 0.7},
		{'z'},
		{'m', 0.55, 0.3},
		{'l', 0.8, 0.5},
		{'l', 0.55, 0.7},
		{'z'},
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_run3_button(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0.2, 0.3},
		{'l', 0.4, 0.5},
		{'l', 0.2, 0.7},
		{'z'},
		{'m', 0.4, 0.3},
		{'l', 0.6, 0.5},
		{'l', 0.4, 0.7},
		{'z'},
		{'m', 0.6, 0.3},
		{'l', 0.8, 0.5},
		{'l', 0.6, 0.7},
		{'z'},
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_cross(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0.2, 0.3},
		{'l', 0.3, 0.2},
		{'l', 0.5, 0.4},
		{'l', 0.7, 0.2},
		{'l', 0.8, 0.3},
		{'l', 0.6, 0.5},
		{'l', 0.8, 0.7},
		{'l', 0.7, 0.8},
		{'l', 0.5, 0.6},
		{'l', 0.3, 0.8},
		{'l', 0.2, 0.7},
		{'l', 0.4, 0.5},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_centered_text(cairo_t * c, const char * text, double font_size)
{
	cairo_text_extents_t extents;
	cairo_set_font_size(c, font_size);
	cairo_text_extents(c, text, &extents);

	double x = (1 - extents.width) / 2 - extents.x_bearing;
	double y = (1 - extents.height) / 2 - extents.y_bearing;
	cairo_move_to(c, x, y);
	cairo_show_text(c, text);
}

void
draw_circle(cairo_t * c, double x, double y, double r, bool ccw)
{
	static constexpr double twopi = M_PI * 2;
	cairo_move_to(c, x, y + r);
	for (std::size_t n = 0; n < 8; ++n) {
		double s1 = sin(n * twopi / 8);
		double c1 = cos(n * twopi / 8);
		double s2 = sin((n + 1) * twopi / 8);
		double c2 = cos((n + 1) * twopi / 8);

		double f = 1.0;
		if (ccw) {
			s1 = -s1;
			s2 = -s2;
			f = -1;
		}
		double x1 = r * s1 + x;
		double y1 = r * c1 + y;
		double x2 = r * s2 + x;
		double y2 = r * c2 + y;
		cairo_curve_to(
			c,
			x1 + f * c1 * r / 4, y1 - f * s1 * r / 4,
			x2 - f * c2 * r / 4, y2 + f * s2 * r / 4,
			x2, y2);
	}
	cairo_close_path(c);
}

void
draw_conditional(cairo_t * c)
{
	static const draw_command cmds[] = {
		{'m', 0.10, 0.50},
		{'c', 0.20, 0.35, 0.40, 0.30, 0.50, 0.30},
		{'c', 0.60, 0.30, 0.80, 0.35, 0.90, 0.50},
		{'c', 0.80, 0.65, 0.60, 0.70, 0.50, 0.70},
		{'c', 0.40, 0.70, 0.20, 0.65, 0.10, 0.50},
		{'z'},
		{'m', 0.20, 0.50},
		{'c', 0.30, 0.60, 0.40, 0.63, 0.50, 0.63},
		{'c', 0.60, 0.63, 0.70, 0.60, 0.80, 0.50},
		{'c', 0.70, 0.40, 0.60, 0.37, 0.50, 0.37},
		{'c', 0.40, 0.37, 0.30, 0.40, 0.20, 0.50},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);

	draw_circle(c, .50, .50, .10, false);
	draw_circle(c, .50, .50, .02, true);
	cairo_fill(c);
}

void
draw_repeat(cairo_t * c, std::size_t count)
{
	std::ostringstream os;
	os << count << "x";
	draw_centered_text(c, os.str().c_str(), .5);
}

void
draw_trigger1(cairo_t * c)
{
	draw_circle(c, .5, .5, .45, false);
	draw_circle(c, .5, .5, .4, true);
	cairo_fill(c);
}

void
draw_trigger2(cairo_t * c)
{
	draw_circle(c, .5, .5, .3, false);
	draw_circle(c, .5, .5, .25, true);
	cairo_fill(c);
}
void
draw_trigger3(cairo_t * c)
{
	draw_circle(c, .5, .5, .15, false);
	draw_circle(c, .5, .5, .10, true);
	cairo_fill(c);
}

void
draw_back_icon(cairo_t * c)
{
	static constexpr double r1l = 0.5 - 0.4 * 0.6;
	static constexpr double r2l = 0.5 - 0.2 * 0.6;
	static constexpr double r1h = 1 - r1l;
	static constexpr double r2h = 1 - r2l;
	static const draw_command cmds[] = {
		{'m', 0.3, 0.2},
		{'l', 0.5, 0.0},
		{'l', 0.5, 0.1},
		{'c', r1h, 0.1, 0.9, r1l, 0.9, 0.5},
		{'c', 0.9, r1h, r1h, 0.9, 0.5, 0.9},
		{'c', r1l, 0.9, 0.1, r1h, 0.1, 0.5},
		{'l', 0.3, 0.5},
		{'c', 0.3, r2h, r2l, 0.7, 0.5, 0.7},
		{'c', r2h, 0.7, 0.7, r2h, 0.7, 0.5},
		{'c', 0.7, r2l, r2h, 0.3, 0.5, 0.3},
		{'l', 0.5, 0.4},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

void
draw_exit_icon(cairo_t * c)
{
	static constexpr double w = .15;
	static constexpr double p1 = w;
	static constexpr double p2 = 1 - w;
	static constexpr double c1 = 0.5 - w;
	static constexpr double c2 = 0.5 + w;
	static const draw_command cmds[] = {
		{'m', p1, .0},
		{'l', .5, c1},
		{'l', p2, .0},
		{'l', 1., p1},
		{'l', c2, .5},
		{'l', 1., p2},
		{'l', p2, 1.},
		{'l', .5, c2},
		{'l', p1, 1.},
		{'l', 0., p2},
		{'l', c1, .5},
		{'l', 0., p1},
		{'z'}
	};
	execute_commands(cmds, sizeof(cmds) / sizeof(cmds[0]), c);
}

