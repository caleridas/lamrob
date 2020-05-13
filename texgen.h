#ifndef TEXGEN_H
#define TEXGEN_H

#include <GL/gl.h>

#include <map>
#include <memory>
#include <vector>

#include "tilegen.h"

std::map<int, std::function<void(cairo_t *)>> get_texture_generator_map();

class texture_generator {
public:
	struct tex_coords {
		double x1, y1;
		double x2, y2;
	};

	static void
	bind_texture();

	static void
	make_texture();

	static std::unique_ptr<uint8_t[]>
	make_rgba(std::size_t w, std::size_t h);

	static void
	make_scratch_text(const char * s);

	static tex_coords
	get_tex_coords(std::size_t index);

	static int
	get_blur_texture(int texid);

	static void
	make_tex_quad(
		std::size_t index,
		double x1, double y1, double z1,
		double x2, double y2, double z2,
		double x3, double y3, double z3,
		double x4, double y4, double z4);

	static void
	make_tex_quad(
		std::size_t index,
		double x1, double y1, double z1,
		double x2, double y2, double z2,
		double x3, double y3, double z3,
		double x4, double y4, double z4,
		double nx, double ny, double nz);

	static void
	make_tex_quad2d(
		std::size_t index,
		double x1, double y1,
		double x2, double y2,
		double x3, double y3,
		double x4, double y4);

	static void
	make_box(
		double x1, double y1, double z1,
		double x2, double y2, double z2);

	static void
	make_solid_tex_coord();

private:
	static bool generated_;
	static GLuint texture_id_;
};

static constexpr int texid_tile_background = 0;

static constexpr int texid_left = 1;
static constexpr int texid_right = 2;
static constexpr int texid_fwd1 = 3;
static constexpr int texid_fwd2 = 4;
static constexpr int texid_fwd3 = 5;
static constexpr int texid_conditional = 6;
static constexpr int texid_rep0 = 7;
static constexpr int texid_rep1 = 8;
static constexpr int texid_rep2 = 9;
static constexpr int texid_rep3 = 10;
static constexpr int texid_rep4 = 11;

static constexpr int texid_button_lowered_bg = 12;
static constexpr int texid_button_raised_bg = 13;

static constexpr int texid_button_halt = 14;
static constexpr int texid_button_pause = 15;
static constexpr int texid_button_run1x = 16;
static constexpr int texid_button_run2x = 17;
static constexpr int texid_button_run3x = 18;

static constexpr int texid_solid = 19;

static constexpr int texid_cross = 20;
static constexpr int texid_trigger1 = 21;
static constexpr int texid_trigger2 = 22;
static constexpr int texid_trigger3 = 23;

static constexpr int texid_back_icon = 24;
static constexpr int texid_exit_icon = 25;

static constexpr int texid_scratch = 26;

static constexpr int texid_blur_offset = 32;

#endif
