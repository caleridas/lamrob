#include "board_view.h"

#include <GL/gl.h>
#include <math.h>

#include "robot_view.h"
#include "texgen.h"

board_view::floor_info &
board_view::modify_floor(int x, int y)
{
	return grid_(x, y);
}

void
board_view::clear_floor(int x, int y)
{
	grid_.erase(x, y);
}

void
board_view::set_robot_pos(double x, double y, double z, double angle, double tilt, double wheel_rot)
{
	robot_pos_.x = x;
	robot_pos_.y = y;
	robot_pos_.z = z;
	robot_pos_.angle = angle;
	robot_pos_.tilt = tilt;
	robot_wheel_rot_ = wheel_rot;
}

void
board_view::set_robot_beam(robot_beam_t beam)
{
	beam_state_ = beam;
}


void
board_view::set_obstacle_pos(int index, double x, double y, double z, double angle, double tilt)
{
	obstacle_pos_[index].x = x;
	obstacle_pos_[index].y = y;
	obstacle_pos_[index].z = z;
	obstacle_pos_[index].angle = angle;
	obstacle_pos_[index].tilt = tilt;
}

void
board_view::clear_obstacle(int index)
{
	obstacle_pos_.erase(index);
}

void
board_view::draw_floor_tile(int x, int y, const floor_info & floor, double global_phase) const
{
	double scene_x = x, scene_y = y, scene_z = 0.0;

	static const double tile_w = .49;
	static const double tile_h = .5;

	glColor4f(floor.color.r, floor.color.g, floor.color.b, floor.color.a);

	double x1 = scene_x - tile_w;
	double x2 = scene_x + tile_w;
	double y1 = scene_y - tile_w;
	double y2 = scene_y + tile_w;

	if (!floor.opened) {
		texture_generator::make_tex_quad(texid_solid,
			x1, y1, scene_z,
			x1, y2, scene_z,
			x2, y2, scene_z,
			x2, y1, scene_z,
			0, 0, +1);
	} else {
		double s = floor.opened;
		double c = sqrt(1 - s * s);
		texture_generator::make_tex_quad(texid_solid,
			x2, y1, scene_z,
			x1, y1, scene_z,
			x1, y1 + tile_w * c, scene_z - tile_w * s,
			x2, y1 + tile_w * c, scene_z - tile_w * s,
			0, s, c);
		texture_generator::make_tex_quad(texid_solid,
			x1, y2, scene_z,
			x2, y2, scene_z,
			x2, y2 - tile_w * c, scene_z - tile_w * s,
			x1, y2 - tile_w * c, scene_z - tile_w * s,
			0, 0, +1);
	}

	texture_generator::make_tex_quad(texid_solid,
		x2, y2, scene_z,
		x1, y2, scene_z,
		x1, y2, scene_z - tile_h,
		x2, y2, scene_z - tile_h,
		0, +1, 0);
	texture_generator::make_tex_quad(texid_solid,
		x2, y1, scene_z,
		x2, y1, scene_z - tile_h,
		x1, y1, scene_z - tile_h,
		x1, y1, scene_z,
		0, -1, 0);

	texture_generator::make_tex_quad(texid_solid,
		x2, y2, scene_z,
		x2, y2, scene_z - tile_h,
		x2, y1, scene_z - tile_h,
		x2, y1, scene_z,
		+1, 0, 0);
	texture_generator::make_tex_quad(texid_solid,
		x1, y2, scene_z,
		x1, y1, scene_z,
		x1, y1, scene_z - tile_h,
		x1, y2, scene_z - tile_h,
		-1, 0, 0);

	switch (floor.special) {
		case floor_special_t::end: {
			glDisable(GL_LIGHTING);
			double i = sin(global_phase * 2) * 0.25 + 0.75;
			glColor4f(i, 0., 0., floor.color.a);
			texture_generator::make_tex_quad(texid_cross,
				x1, y1, scene_z,
				x1, y2, scene_z,
				x2, y2, scene_z,
				x2, y1, scene_z,
				0, 0, +1);
			glEnable(GL_LIGHTING);
			break;
		}
		case floor_special_t::trigger: {
			glDisable(GL_LIGHTING);
			double tmp = global_phase / 3;
			int p = static_cast<int>(3. * (tmp - std::floor(tmp)));

			for (int texid : {texid_trigger1, texid_trigger2, texid_trigger3}) {
				if (p == 0) {
					glColor4f(1.0, 0., 0., floor.color.a);
				} else {
					glColor4f(0.1, 0., 0., floor.color.a);
				}
				p = p - 1;
				texture_generator::make_tex_quad(texid,
					x1, y1, scene_z,
					x1, y2, scene_z,
					x2, y2, scene_z,
					x2, y1, scene_z,
					0, 0, +1);
			}
			glEnable(GL_LIGHTING);
			break;
		}
		default: {}
	}
}

void
board_view::draw_robot(const view_coord_t & coord) const
{
	glPushMatrix();
	glTranslatef(coord.x, coord.y, coord.z);
	glRotatef(coord.angle, 0, 0, 1);
	glRotatef(coord.tilt, 0, 1, 0);

	::draw_robot(robot_wheel_rot_);

	if (beam_state_ != robot_beam_t::off) {
		draw_robot_beam(beam_state_ == robot_beam_t::hits_floor ? 1.1 : 1000);
	}

	glPopMatrix();
}

void
board_view::draw_obstacle(const view_coord_t & coord) const
{
	static const double base_w = .45;
	static const double base_h = .5;
	static const double top_w = .3;
	static const double top_h = .75;
	glPushMatrix();
	glTranslatef(coord.x, coord.y, coord.z);
	glRotatef(coord.angle, 0, 0, 1);
	glRotatef(coord.tilt, 0, 1, 0);

	glColor4f(.2, .2, .2, 1.);
	/* bottom */
	texture_generator::make_tex_quad(texid_solid,
		-base_w, -base_w, 0.,
		+base_w, -base_w, 0.,
		+base_w, +base_w, 0.,
		-base_w, +base_w, 0.,
		0, 0, -1);
	/* lower part */
	texture_generator::make_tex_quad(texid_solid,
		-base_w, -base_w, 0.,
		-base_w, +base_w, 0.,
		-base_w, +base_w, base_h,
		-base_w, -base_w, base_h,
		-1, 0, 0);
	texture_generator::make_tex_quad(texid_solid,
		+base_w, -base_w, 0.,
		+base_w, -base_w, base_h,
		+base_w, +base_w, base_h,
		+base_w, +base_w, 0.,
		+1, 0, 0);
	texture_generator::make_tex_quad(texid_solid,
		-base_w, -base_w, 0.,
		-base_w, -base_w, base_h,
		+base_w, -base_w, base_h,
		+base_w, -base_w, 0.,
		0, -1, 0);
	texture_generator::make_tex_quad(texid_solid,
		-base_w, +base_w, 0.,
		+base_w, +base_w, 0.,
		+base_w, +base_w, base_h,
		-base_w, +base_w, base_h,
		0, +1, 0);

	/* upper part */
	texture_generator::make_tex_quad(texid_solid,
		-base_w, -base_w, base_h,
		-base_w, +base_w, base_h,
		-top_w, +top_w, top_h,
		-top_w, -top_w, top_h,
		-1, 0, .2);
	texture_generator::make_tex_quad(texid_solid,
		+base_w, -base_w, base_h,
		+top_w, -top_w, top_h,
		+top_w, +top_w, top_h,
		+base_w, +base_w, base_h,
		+1, 0, .2);
	texture_generator::make_tex_quad(texid_solid,
		-base_w, -base_w, base_h,
		-top_w, -top_w, top_h,
		+top_w, -top_w, top_h,
		+base_w, -base_w, base_h,
		0, -1, .2);
	texture_generator::make_tex_quad(texid_solid,
		-base_w, +base_w, base_h,
		+base_w, +base_w, base_h,
		+top_w, +top_w, top_h,
		-top_w, +top_w, top_h,
		0, +1, .2);
	/* top */
	texture_generator::make_tex_quad(texid_solid,
		-top_w, -top_w, top_h,
		-top_w, +top_w, top_h,
		+top_w, +top_w, top_h,
		+top_w, -top_w, top_h,
		0, 0, +1);

	glPopMatrix();
}

void
board_view::draw_board(double global_phase) const
{
	grid_.reverse_iterate([this, global_phase](int x, int y, const floor_info & floor) {
		draw_floor_tile(x, y, floor, global_phase);
	});
}

void
board_view::draw(std::size_t width, std::size_t height, double global_phase) const
{
	draw(width, height, 0, 0, width, height, global_phase);
}

void
board_view::draw(
	std::size_t screen_width, std::size_t screen_height,
	std::size_t x0, std::size_t y0,
	std::size_t x1, std::size_t y1,
	double global_phase) const
{
	std::size_t width = x1 - x0;
	std::size_t height = y1 - y0;

	double dimx = width > (height * 4. / 3.) ? (600. * width / height) : 800;
	double dimy = width > (height * 4. / 3.) ? 600 : (800. * height / width);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0., screen_width, screen_height, -0., -600., +600.);
	glTranslatef(x0, y0, 0);
	glScalef(width / dimx, height / dimy, 1);
	glTranslatef(dimx * .45, dimy * .45, 0.);
	glScalef(1., 1., 1.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(-1.0, 1.0, 1.0);
	glRotatef(60, 1, 0, 0);
	glRotatef(200, 0, 0, 1);

	glScalef(60., 60., 60.);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glShadeModel(GL_SMOOTH);

	static const GLfloat global_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
	glEnable(GL_COLOR_MATERIAL);
	glFrontFace(GL_CCW);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	glEnable(GL_LIGHT0);

	const GLfloat light0_position[] = {
		static_cast<GLfloat>(sin(global_phase * .5) * 10),
		static_cast<GLfloat>(cos(global_phase * .5) * 10),
		13, 0
	};
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	static const GLfloat light0_ambient[] = {.0, .0, .0, 1};
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	static const GLfloat light0_diffuse[] = {.9, .9, .9, 1};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	static const GLfloat light0_specular[] = {.9, .9, .9, 1};
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	draw_board(global_phase);
	for (const auto & obstacle : obstacle_pos_) {
		draw_obstacle(obstacle.second);
	}

	draw_robot(robot_pos_);
}

void
board_view::reset(const puzzle & puz)
{
	grid_.clear();
	obstacle_pos_.clear();
	puz.grid.iterate([this](int x, int y, floor_tile_t tile) {
		auto & floor = grid_(x, y);

		if (tile.trigger_id > 0) {
			floor.color = get_trigger_tile_color(tile.trigger_id - 1);
			floor.special = floor_special_t::trigger;
		} else if (tile.trigger_id < 0) {
			floor.color = get_trigger_tile_color(- tile.trigger_id -1);
			floor.opened = 1.;
		} else {
			floor.color = tile_color_t{1.0, 1.0, 1.0, 1.0};
		}
	});
	grid_(puz.end.x, puz.end.y).special = floor_special_t::end;
	set_robot_pos(puz.start.pos.x, puz.start.pos.y, 0., puz.start.dir.angle(), 0., 0.);
	set_robot_beam(robot_beam_t::off);

	for (std::size_t n = 0; n < puz.obstacles.size(); ++n) {
		set_obstacle_pos(n, puz.obstacles[n].x, puz.obstacles[n].y, 0., 0., 0.);
	}
}

board_view::tile_color_t
board_view::get_alternative_tile_color(std::size_t index)
{
	switch (index % 4) {
		default:
		case 0: return {1, 0, 0, 1};
		case 1: return {1, 1, 0, 1};
		case 2: return {1, .5, 0, 0};
		case 3: return {1, 0, .5, 0};
	}
}

board_view::tile_color_t
board_view::get_trigger_tile_color(std::size_t index)
{
	switch (index % 4) {
		default:
		case 0: return {0, 0, 1, 1};
		case 1: return {0, 1, 1, 1};
		case 2: return {.5, 0, 1, 1};
		case 3: return {.2, .2, 1, 1};
	}
}

