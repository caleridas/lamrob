#include <stdlib.h>

#include <cairo/cairo.h>

struct point {
	double x;
	double y;
};

void
clear_background(cairo_t * c)
{
	cairo_operator_t op = cairo_get_operator(c);
	cairo_rectangle(c, 0., 0., 1., 1.);
	cairo_set_source_rgba(c, 0., 0., 0., 0.);
	cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
	cairo_fill(c);
	cairo_set_operator(c, op);
}

void
points_to_path(cairo_t * c, const point * points, size_t npoints)
{
	cairo_move_to(c, points[0].x, points[0].y);
	for (size_t n = 1; n < npoints; ++n) {
		cairo_line_to(c, points[n].x, points[n].y);
	}
	cairo_close_path(c);
}

void
straight_arrow(cairo_t * c)
{
	const point arrow_points[] = {
		{0.4, 0.9},
		{0.4, 0.35},
		{0.25, 0.35},
		{0.5, 0.1},
		{0.75, 0.35},
		{0.6, 0.35},
		{0.6, 0.9}
	};
	points_to_path(c, arrow_points, sizeof(arrow_points) / sizeof(point));
}

void
right_arrow(cairo_t * c)
{
	cairo_move_to(c, 0.2, 0.9);
	cairo_curve_to(c, 0.2, 0.45,  0.35, 0.3,  0.65, 0.3);
	cairo_line_to(c, 0.65, 0.15);
	cairo_line_to(c, 0.9, 0.4);
	cairo_line_to(c, 0.65, 0.65);
	cairo_line_to(c, 0.65, 0.5);
	cairo_curve_to(c, 0.4, 0.5,  0.4, 0.7,  0.4, 0.9);
	cairo_close_path(c);
}

void
left_arrow(cairo_t * c)
{
	cairo_matrix_t m = {-1., 0., 0., 1., 1., 0.};
	cairo_save(c);
	cairo_transform(c, &m);
	right_arrow(c);
	cairo_restore(c);
}

int main()
{
	cairo_surface_t * s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 128, 128);
	cairo_t * c = cairo_create(s);
	cairo_scale(c, 128, 128);

	clear_background(c);

	cairo_set_source_rgba(c, 1., 1., 1., 1.);
//	straight_arrow(c);
	right_arrow(c);
	cairo_fill(c);

	cairo_destroy(c);

	cairo_surface_write_to_png(s, "/tmp/tmp.png");
}
