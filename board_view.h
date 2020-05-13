#ifndef BOARD_VIEW_H
#define BOARD_VIEW_H

#include "puzzle.h"
#include "grid.h"

struct view_coord_t {
	double x, y, z, angle, tilt;
};

class board_view {
public:
	struct tile_color_t {
		double r, g, b, a;
	};

	enum class floor_special_t {
		none = 0,
		end = 1,
		trigger = 2
	};

	enum class robot_beam_t {
		off = 0,
		hits_floor = 1,
		hits_nothing = 2
	};

	struct floor_info {
		tile_color_t color;
		double opened = 0.0;
		floor_special_t special = floor_special_t::none;
	};

	using grid_t = grid_tpl<floor_info>;

	inline const grid_t &
	grid() const noexcept { return grid_; }

	inline const view_coord_t &
	robot_pos() const noexcept { return robot_pos_; }

	static tile_color_t
	get_alternative_tile_color(std::size_t index);

	static tile_color_t
	get_trigger_tile_color(std::size_t index);

	void
	set_robot_pos(double x, double y, double z, double angle, double tilt, double wheel_rot);

	void
	set_robot_beam(robot_beam_t beam);

	void
	set_obstacle_pos(int index, double x, double y, double z, double angle, double tilt);

	void
	clear_obstacle(int index);

	floor_info &
	modify_floor(int x, int y);

	void
	clear_floor(int x, int y);

	void
	draw(std::size_t width, std::size_t height, double global_phase) const;

	void
	draw(
		std::size_t screen_width, std::size_t screen_height,
		std::size_t x0, std::size_t y0,
		std::size_t x1, std::size_t y1,
		double global_phase) const;

	void
	reset(const puzzle & puz);

private:
	void
	draw_floor_tile(int x, int y, const floor_info & floor, double global_phase) const;

	void
	draw_robot(const view_coord_t & coord) const;

	void
	draw_obstacle(const view_coord_t & coord) const;

	void
	draw_board(double global_phase) const;

	view_coord_t robot_pos_ = {
		1., 0., 0., 0., 0.
	};
	double robot_wheel_rot_ = 0.;
	robot_beam_t beam_state_ = robot_beam_t::off;

	std::map<int, view_coord_t> obstacle_pos_;

	grid_t grid_;
};

#endif
