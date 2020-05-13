#include "texgen.h"

#include <cmath>

bool texture_generator::generated_ = false;
GLuint texture_generator::texture_id_ = 0;

void
texture_generator::make_texture()
{
	std::unique_ptr<uint8_t[]> texture_data = make_rgba(512, 512);

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture_id_);
	glBindTexture(GL_TEXTURE_2D, texture_id_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA,
			512, 512, 0,
			GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data.get());
}

void texture_generator::bind_texture()
{
	if (generated_) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture_id_);
	} else {
		make_texture();
		generated_ = true;
	}
}

std::map<int, std::function<void(cairo_t *)>> get_texture_generator_map()
{
	std::map<int, std::function<void(cairo_t *)>> map = {
		{texid_tile_background, draw_tile_background},
		{texid_left, draw_left_arrow},
		{texid_right, draw_right_arrow},
		{texid_fwd1, draw_straight_arrow},
		{texid_fwd2, draw_straight_arrow2},
		{texid_fwd3, draw_straight_arrow3},

		{texid_conditional, draw_conditional},
		{texid_rep0, [](cairo_t * c){ draw_repeat(c, 0); }},
		{texid_rep1, [](cairo_t * c){ draw_repeat(c, 1); }},
		{texid_rep2, [](cairo_t * c){ draw_repeat(c, 2); }},
		{texid_rep3, [](cairo_t * c){ draw_repeat(c, 3); }},
		{texid_rep4, [](cairo_t * c){ draw_repeat(c, 4); }},

		{texid_button_lowered_bg, draw_button_background},
		{texid_button_raised_bg, draw_pressed_button_background},
		{texid_button_halt, draw_abort_button},
		{texid_button_pause, draw_pause_button},
		{texid_button_run1x, draw_run_button},
		{texid_button_run2x, draw_run2_button},
		{texid_button_run3x, draw_run3_button},

		{texid_solid, draw_solid},
		{texid_cross, draw_cross},
		{texid_trigger1, draw_trigger1},
		{texid_trigger2, draw_trigger2},
		{texid_trigger3, draw_trigger3},

		{texid_back_icon, draw_back_icon},
		{texid_exit_icon, draw_exit_icon}
	};

	return map;
}

static constexpr int blur_kern_size = 8;
static constexpr double blur_kern_scale = .05;

void
blur_horizontal(std::size_t w, std::size_t h, uint8_t * data, std::size_t stride)
{
	std::unique_ptr<uint8_t[]> row(new uint8_t[w * 4]);

	for (std::size_t y = 0; y < h; ++y) {
		for (std::size_t x = 0; x < w; ++x) {
			double acc[4] = {0. , 0., 0., 0.};
			for (int xx = - blur_kern_size; xx <= +blur_kern_size; ++xx) {
				double f = (std::erf((xx + .5) * blur_kern_scale) - std::erf((xx - .5) * blur_kern_scale)) * .5;

				std::size_t xe = (x + xx) & (w - 1);
				for (std::size_t c = 0; c < 4; ++c) {
					acc[c] += f * data [(xe + y * stride) * 4 + c];
				}
			}
			for (std::size_t c = 0; c < 4; ++c) {
				row[x * 4 + c] = std::min(255., acc[c]);
			}
		}
		for (std::size_t x = 0; x < w; ++x) {
			data[(x + y * stride) * 4 + 0] = row[x * 4 + 0];
			data[(x + y * stride) * 4 + 1] = row[x * 4 + 1];
			data[(x + y * stride) * 4 + 2] = row[x * 4 + 2];
			data[(x + y * stride) * 4 + 3] = row[x * 4 + 3];
		}
	}
}

void
blur_vertical(std::size_t w, std::size_t h, uint8_t * data, std::size_t stride)
{
	std::unique_ptr<uint8_t[]> row(new uint8_t[w * 4]);

	for (std::size_t x = 0; x < w; ++x) {
		for (std::size_t y = 0; y < h; ++y) {
			double acc[4] = {0. , 0., 0., 0.};
			for (int yy = - blur_kern_size; yy <= +blur_kern_size; ++yy) {
				double f = (std::erf((yy + .5) * blur_kern_scale) - std::erf((yy - .5) * blur_kern_scale)) * .5;

				std::size_t ye = (y + yy) & (h - 1);
				for (std::size_t c = 0; c < 4; ++c) {
					acc[c] += f * data [(x + ye * stride) * 4 + c];
				}
			}
			for (std::size_t c = 0; c < 4; ++c) {
				row[y * 4 + c] = std::min(255., 8 * acc[c]);
			}
		}
		for (std::size_t y = 0; y < w; ++y) {
			data[(x + y * stride) * 4 + 0] = row[y * 4 + 0];
			data[(x + y * stride) * 4 + 1] = row[y * 4 + 1];
			data[(x + y * stride) * 4 + 2] = row[y * 4 + 2];
			data[(x + y * stride) * 4 + 3] = row[y * 4 + 3];
		}
	}
}

void
make_border(std::size_t w, std::size_t h, uint8_t * data, std::size_t stride)
{
	for (std::size_t x = 1; x < w - 1; ++x) {
		for (std::size_t c = 0; c < 4; ++c) {
			data[(x + 0 * stride) * 4 + c] = data[(x + 1 * stride) * 4 + c];
		}
		for (std::size_t c = 0; c < 4; ++c) {
			data[(x + (h - 1) * stride) * 4 + c] = data[(x + (h - 2) * stride) * 4 + c];
		}
	}
	for (std::size_t y = 0; y < h; ++y) {
		for (std::size_t c = 0; c < 4; ++c) {
			data[(0 + y * stride) * 4 + c] = data[(1 + y * stride) * 4 + c];
		}
		for (std::size_t c = 0; c < 4; ++c) {
			data[((w - 1) + y * stride) * 4 + c] = data[((w-2) + y * stride) * 4 + c];
		}
	}
}

std::unique_ptr<uint8_t[]>
texture_generator::make_rgba(std::size_t w, std::size_t h)
{
	std::unique_ptr<uint8_t[]> data(new uint8_t[w * h * 4]);

	std::size_t tile_w = w / 8;
	std::size_t tile_h = h / 8;

	std::map<int, std::function<void(cairo_t *)>> gen_map = get_texture_generator_map();
	for (const auto& gen : gen_map) {
		std::size_t index = gen.first;
		const std::function<void(cairo_t *)>& fn = gen.second;

		std::size_t x = (index % 8) * tile_w;
		std::size_t y = (index / 8) * tile_h;

		std::size_t xb = ((index + texid_blur_offset) % 8) * tile_w;
		std::size_t yb = ((index + texid_blur_offset) / 8) * tile_h;

		cairo_draw_gl_rgba(tile_w - 2, tile_h - 2, &data[((x + 1) + (y + 1) * w) * 4], w, fn);
		make_border(tile_w, tile_h, &data[(x + y * w) * 4], w);

		cairo_draw_gl_rgba(tile_w - 2, tile_h - 2, &data[((xb + 1) + (yb + 1) * w) * 4], w, fn);
		blur_horizontal(tile_w - 2, tile_h - 2, &data[((xb + 1) + (yb + 1) * w) * 4], w);
		blur_vertical(tile_w - 2, tile_h - 2, &data[((xb + 1) + (yb + 1) * w) * 4], w);
		make_border(tile_w, tile_h, &data[(xb + yb * w) * 4], w);
	}

	return data;
}

void
texture_generator::make_scratch_text(const char * s)
{
	std::size_t tile_w = 512 / 8;
	std::size_t tile_h = 512 / 8;

	std::unique_ptr<uint8_t[]> data(new uint8_t[tile_w * tile_h * 4]);

	cairo_draw_gl_rgba(tile_w - 2, tile_h - 2, &data[(1 + 1 * tile_w) * 4], tile_w,
		[s](cairo_t * c) { draw_centered_text(c, s, .5); });
	make_border(tile_w, tile_h, &data[0], tile_w);
	std::size_t x = (texid_scratch % 8) * tile_w;
	std::size_t y = (texid_scratch / 8) * tile_h;
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, tile_w, tile_h, GL_RGBA, GL_UNSIGNED_BYTE, data.get());
}

int
texture_generator::get_blur_texture(int texid)
{
	return texid + texid_blur_offset;
}


texture_generator::tex_coords
texture_generator::get_tex_coords(std::size_t index)
{
	tex_coords c;

	double x = (index % 8) / 8.0;
	double y = (index / 8) / 8.0;
	double w = 1 / 8.0;
	double h = 1 / 8.0;

	if (index == texid_solid) {
		c.x1 = x + w / 2.;
		c.y1 = y + h / 2.;
		c.x2 = x + w / 2.;
		c.y2 = y + h / 2.;
	} else {
		c.x1 = x + 1 / 512.;
		c.y1 = y + 1 / 512.;
		c.x2 = x + w - 1 / 512.;
		c.y2 = y + h - 1 / 512.;
	}
	return c;
}

void
texture_generator::make_tex_quad(
	std::size_t index,
	double x1, double y1, double z1,
	double x2, double y2, double z2,
	double x3, double y3, double z3,
	double x4, double y4, double z4)
{
	tex_coords tc = get_tex_coords(index);

	glBegin(GL_QUADS);
	glTexCoord2f(tc.x1, tc.y1);
	glVertex3f(x1, y1, z1);
	glTexCoord2f(tc.x2, tc.y1);
	glVertex3f(x2, y2, z2);
	glTexCoord2f(tc.x2, tc.y2);
	glVertex3f(x3, y3, z3);
	glTexCoord2f(tc.x1, tc.y2);
	glVertex3f(x4, y4, z4);
	glEnd();
}

void
texture_generator::make_tex_quad(
	std::size_t index,
	double x1, double y1, double z1,
	double x2, double y2, double z2,
	double x3, double y3, double z3,
	double x4, double y4, double z4,
	double nx, double ny, double nz)
{
	tex_coords tc = get_tex_coords(index);

	glBegin(GL_QUADS);
	glNormal3f(nx, ny, nz);
	glTexCoord2f(tc.x1, tc.y1);
	glVertex3f(x1, y1, z1);
	glTexCoord2f(tc.x2, tc.y1);
	glVertex3f(x2, y2, z2);
	glTexCoord2f(tc.x2, tc.y2);
	glVertex3f(x3, y3, z3);
	glTexCoord2f(tc.x1, tc.y2);
	glVertex3f(x4, y4, z4);
	glEnd();
}

void
texture_generator::make_tex_quad2d(
	std::size_t index,
	double x1, double y1,
	double x2, double y2,
	double x3, double y3,
	double x4, double y4)
{
	tex_coords tc = get_tex_coords(index);

	glBegin(GL_QUADS);
	glTexCoord2f(tc.x1, tc.y1);
	glVertex2f(x1, y1);
	glTexCoord2f(tc.x2, tc.y1);
	glVertex2f(x2, y2);
	glTexCoord2f(tc.x2, tc.y2);
	glVertex2f(x3, y3);
	glTexCoord2f(tc.x1, tc.y2);
	glVertex2f(x4, y4);
	glEnd();
}

void
texture_generator::make_box(
	double x1, double y1, double z1,
	double x2, double y2, double z2)
{
	make_tex_quad(texid_solid,
		x1, y1, z2,
		x1, y2, z2,
		x2, y2, z2,
		x2, y1, z2,
		0, +1, 0);
	make_tex_quad(texid_solid,
		x1, y1, z1,
		x2, y1, z1,
		x2, y2, z1,
		x1, y2, z1,
		0, -1, 0);
	make_tex_quad(texid_solid,
		x2, y2, z2,
		x2, y2, z1,
		x2, y1, z1,
		x2, y1, z2,
		+1, 0, 0);
	make_tex_quad(texid_solid,
		x1, y2, z2,
		x1, y1, z2,
		x1, y1, z1,
		x1, y2, z1,
		-1, 0, 0);
	make_tex_quad(texid_solid,
		x1, y1, z2,
		x2, y1, z2,
		x2, y1, z1,
		x1, y1, z1,
		0, -1, 0);
	make_tex_quad(texid_solid,
		x1, y2, z2,
		x1, y2, z1,
		x2, y2, z1,
		x2, y2, z2,
		0, +1, 0);
}

void
texture_generator::make_solid_tex_coord()
{
	tex_coords tc = get_tex_coords(texid_solid);
	glTexCoord2f(.5 * (tc.x1 + tc.x2), .5 * (tc.y1 + tc.y2));
}

