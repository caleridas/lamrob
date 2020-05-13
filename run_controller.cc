#include "run_controller.h"

#include <GL/gl.h>

#include <cmath>
#include <limits>

#include "clock.h"
#include "texgen.h"

namespace {

view_coord_t
to_view_coord(const grid_coord_t & coord)
{
	view_coord_t  result;
	result.x = coord.pos.x;
	result.y = coord.pos.y;
	result.z = 0.;
	result.angle = coord.dir.angle();
	result.tilt = 0.;
	return result;
}

double
get_alternative_opacity(double now, std::size_t index)
{
	double p = (now / 10.);
	p = p - floor(p);

	double o1 = fabs(0.5 - p) * 2;
	double o2 = 1 - o1;

	return index == 0 ? o1 : o2;
}

}

void
run_step_state::set_obstacle(int index, grid_pos_t pos)
{
	auto i = obstacles.find(index);
	if (i != obstacles.end()) {
		grid(i->second.x, i->second.y).obstacle = -1;
	}
	obstacles[index] = pos;
	grid(pos.x, pos.y).obstacle = index;
}

void
run_step_state::clear_obstacle(int index)
{
	auto i = obstacles.find(index);
	if (i != obstacles.end()) {
		grid(i->second.x, i->second.y).obstacle = -1;
		obstacles.erase(i);
	}
}

void
run_step_state::initialize(const puzzle & puz, const command_sequence * init_seq)
{
	seq = init_seq;
	robot.pos = puz.start.pos;
	robot.dir = puz.start.dir;

	branch_states.clear();
	obstacles.clear();
	grid.clear();

	puz.grid.iterate([this](int x, int y, floor_tile_t tile)
	{
		if (tile.trigger_id >= 0) {
			grid(x, y).has_floor = true;
		}
		if (tile.trigger_id > 0) {
			trigger_floors[tile.trigger_id].first = grid_pos_t{x, y};
		}
		if (tile.trigger_id < 0) {
			trigger_floors[-tile.trigger_id].second = grid_pos_t{x, y};
		}
	});
	goal = puz.end;

	for (std::size_t index = 0; index < puz.obstacles.size(); ++index) {
		set_obstacle(index, puz.obstacles[index]);
	}
}


run_step::~run_step()
{
}

std::unique_ptr<run_step>
run_step::make_initial(
	const puzzle & puz,
	const run_step_state & state,
	const std::vector<std::size_t> & alternatives)
{
	std::unique_ptr<run_step> result;
	result.reset(new setup_run_step(0.0, puz, alternatives));
	return result;
}


namespace {

void
animate_falling_obstacles(
	double current_time,
	const std::unordered_map<int, falling_obstacle> & falling_obstacles,
	board_view & bv)
{
	for (const auto & o : falling_obstacles) {
		if (o.second.start_time > current_time) {
			continue;
		}
		double delta_time = current_time - o.second.start_time;
		double x = o.second.start.x + o.second.vx * delta_time;
		double y = o.second.start.y + o.second.vy * delta_time;
		double z = o.second.start.z + o.second.vz * delta_time - .5 * delta_time * delta_time * 8;
		double angle = o.second.start.angle;
		double tilt = delta_time * 240;

		bv.set_obstacle_pos(o.first, x, y, z, angle, tilt);
	}
}

}

setup_run_step::~setup_run_step()
{
}

setup_run_step::setup_run_step(
	double start_time,
	const puzzle & puz,
	const std::vector<std::size_t> & alternatives)
	: run_step(start_time)
{
	double now = get_current_time();
	for (std::size_t n = 0; n < puz.alternative_tiles.size(); ++n) {
		std::size_t option = n >= alternatives.size() ? 0 : alternatives[n];

		floor_tiles_.push_back({
			puz.alternative_tiles[n].first,
			board_view::get_alternative_tile_color(n),
			(option == 0),
			get_alternative_opacity(now, 0)
		});

		floor_tiles_.push_back({
			puz.alternative_tiles[n].second,
			board_view::get_alternative_tile_color(n),
			(option == 1),
			get_alternative_opacity(now, 1)
		});
	}
}

void
setup_run_step::step(
	run_step_state & state,
	std::unique_ptr<run_step> & next_step)
{
	for (const auto & floor_tile : floor_tiles_) {
		if (floor_tile.present) {
			state.grid(floor_tile.pos.x, floor_tile.pos.y).has_floor = true;
		} else {
			state.grid.erase(floor_tile.pos.x, floor_tile.pos.y);
		}
	}

	if (!state.seq->empty()) {
		next_step.reset(new command_run_step(start_time() + animation_duration(), command_sequence_path(0), state, {}));
	} else {
		next_step.reset();
	}
}

void
setup_run_step::animate(
	double delta_time,
	board_view & bv) const
{
	double f_end = animation_duration() ? delta_time / animation_duration() : 0.0;
	double f_start = 1 - f_end;
	for (const auto & floor_tile : floor_tiles_) {
		double start_state = floor_tile.start_state;
		double end_state = floor_tile.present ? 1.0 : 0.0;
		board_view::tile_color_t color = floor_tile.color;
		color.a = f_start * start_state + f_end * end_state;
		bv.modify_floor(floor_tile.pos.x, floor_tile.pos.y).color = color;
	}
}

void
setup_run_step::end_animate(
	board_view & bv) const
{
	for (const auto & floor_tile : floor_tiles_) {
		if (floor_tile.present) {
			bv.modify_floor(floor_tile.pos.x, floor_tile.pos.y).color = floor_tile.color;
		} else {
			bv.clear_floor(floor_tile.pos.x, floor_tile.pos.y);
		}
	}
}

double
setup_run_step::animation_duration() const noexcept
{
	return floor_tiles_.empty() ? 0.0 : 1.0;
}

namespace {

void
replenish_repetitions(const command_sequence & seq, run_step_state & state);

void
replenish_repetitions(const command_point & cpt, run_step_state & state)
{
	state.branch_states.erase(&cpt.tile());
	for (std::size_t n = 0; n < cpt.num_branches(); ++n) {
		replenish_repetitions(cpt.branch(n), state);
	}
}

void
replenish_repetitions(const command_sequence & seq, run_step_state & state)
{
	for (const auto & cpt : seq) {
		replenish_repetitions(*cpt, state);
	}
}

}

command_run_step::~command_run_step()
{
}

command_run_step::command_run_step(
	double start_time,
	command_sequence_path current_command,
	run_step_state & rs,
	std::unordered_map<int, falling_obstacle> falling_obstacles)
	: run_step(start_time)
	, current_path_(std::move(current_command))
	, current_command_(rs.seq->lookup(current_path_))
	, falling_obstacles_(std::move(falling_obstacles))
{
	begin_step(rs, 0);
}

void
command_run_step::step(
	run_step_state & state,
	std::unique_ptr<run_step> & next_step)
{
	if (will_drop_) {
		double x = .5 * (robot_coord_origin_.pos.x + robot_coord_target_.pos.x);
		double y = .5 * (robot_coord_origin_.pos.y + robot_coord_target_.pos.y);
		double z = 0;
		double angle = robot_coord_origin_.dir.angle() + robot_coord_origin_.dir.delta_angle(robot_coord_target_.dir) * .5;
		double vx = (robot_coord_target_.pos.x - robot_coord_origin_.pos.x);
		double vy = (robot_coord_target_.pos.y - robot_coord_origin_.pos.y);
		double vz = 0;

		next_step.reset(
			new drop_run_step(
				start_time() + 0.5,
				state,
				dropping_object{{x, y, z, angle}, vx, vy, vz},
				std::move(falling_obstacles_)));
	} else {
		state.robot = robot_coord_target_;

		if (move_obstacle_ != -1) {
			if (!obstacle_will_drop_) {
				state.set_obstacle(move_obstacle_, obstacle_pos_target_);
			} else {
				state.clear_obstacle(move_obstacle_);
			}
		}

		if (closes_trap_) {
			state.grid(closed_trap_.x, closed_trap_.y).has_floor = true;
		}

		if (state.robot.pos == state.goal) {
			next_step.reset(new finish_goal_run_step(start_time() + 1.0, robot_coord_target_, std::move(falling_obstacles_)));
		} else {
			int limit_sub_steps;
			switch (current_command_->tile().kind()) {
				case command_tile::kind_t::fwd2: {
					limit_sub_steps = 2;
					break;
				}
				case command_tile::kind_t::fwd3: {
					limit_sub_steps = 3;
					break;
				}
				default : {
					limit_sub_steps = 1;
				}
			}

			if (used_sub_steps_ + 1 >= limit_sub_steps) {
				if (current_command_->tile().is_repeat()) {
					state.branch_states[&current_command_->tile()] = current_branch_state_ - 1;
				}
				advance_command(state);
				if (!current_command_) {
					next_step.reset(new finish_fail_run_step(start_time() + 1.0, std::move(falling_obstacles_)));
				} else {
					set_start_time(start_time() + 1.0);
					begin_step(state, 0);
				}
			} else {
				set_start_time(start_time() + 1.0);
				begin_step(state, used_sub_steps_ + 1);
			}
		}
	}
}

void
command_run_step::advance_command(run_step_state & state)
{
	if (current_command_->tile().is_conditional()) {
		current_path_.down(current_branch_state_);
	} else if (current_command_->tile().is_repeat()) {
		current_branch_state_ = 0;
		current_path_.down(0);
	} else {
		current_path_.advance();
	}

	current_command_ = state.seq->lookup(current_path_);
	while (!current_command_) {
		if (current_path_.is_leaf()) {
			// reached end of command sequence
			break;
		}
		current_path_.up();
		current_command_ = state.seq->lookup(current_path_);
		if (current_command_->tile().is_conditional()) {
			current_path_.advance();
			current_command_ = state.seq->lookup(current_path_);
		} else if (current_command_->tile().is_repeat()) {
			std::size_t rep = state.branch_states.find(&current_command_->tile())->second;
			if (rep == 0) {
				current_path_.advance();
				current_command_ = state.seq->lookup(current_path_);
			} else {
				break;
			}
		}
	}
}

void
command_run_step::animate(
	double delta_time,
	board_view & bv) const
{
	double f_target = delta_time;
	double f_origin = 1 - f_target;

	double x = robot_coord_origin_.pos.x * f_origin + robot_coord_target_.pos.x * f_target;
	double y = robot_coord_origin_.pos.y * f_origin + robot_coord_target_.pos.y * f_target;
	double z = 0;
	double angle = robot_coord_origin_.dir.angle() + robot_coord_origin_.dir.delta_angle(robot_coord_target_.dir) * f_target;

	double wheel_angle = current_command_->tile().is_conditional() || current_command_->tile().is_repeat() ? 0.0 : delta_time * M_PI * 2 * 2;
	bv.set_robot_pos(x, y, z, angle, 0., wheel_angle);
	current_command_->tile().set_state(command_tile::state_t::flashing);

	if (delta_time > 0.5) {
		animate_branch_states();
	}

	animate_beam(bv, delta_time);


	if (move_obstacle_ != -1) {
		double x = obstacle_pos_origin_.x * f_origin + obstacle_pos_target_.x * f_target;
		double y = obstacle_pos_origin_.y * f_origin + obstacle_pos_target_.y * f_target;
		double z = 0;
		bv.set_obstacle_pos(move_obstacle_, x, y, z, 0., 0.);
	}

	if (closes_trap_) {
		bv.modify_floor(closed_trap_.x, closed_trap_.y).opened = std::min(1.0, 2 * f_origin);
	}

	animate_falling_obstacles(delta_time + start_time(), falling_obstacles_, bv);
}

void
command_run_step::end_animate(
	board_view & bv) const
{
	if (!will_drop_) {
		bv.set_robot_pos(robot_coord_target_.pos.x, robot_coord_target_.pos.y, 0., robot_coord_target_.dir.angle(), 0., 0.);
	}
	if (move_obstacle_ != -1 && !obstacle_will_drop_) {
		bv.set_obstacle_pos(move_obstacle_, obstacle_pos_target_.x, obstacle_pos_target_.y, 0., 0., 0.);
	}
	if (closes_trap_) {
		bv.modify_floor(closed_trap_.x, closed_trap_.y).opened = 0.0;
	}

	animate_branch_states();

	animate_beam(bv, 1.);

	if (current_command_->tile().is_repeat()) {
		if (current_branch_state_ > 1) {
			current_command_->tile().set_repetitions_left(current_branch_state_ - 1);
			current_command_->tile().set_state(command_tile::state_t::normal);
		} else {
			current_command_->tile().set_repetitions_left(current_command_->tile().num_repetitions());
			current_command_->tile().set_state(command_tile::state_t::depleted);
		}
	} else {
		current_command_->tile().set_state(command_tile::state_t::depleted);
	}
}

void
command_run_step::animate_branch_states() const
{
	if (current_command_->tile().is_repeat()) {
		current_command_->branch(0).replenish();
		current_command_->tile().set_repetitions_left(current_branch_state_ - 1);
	} else if (current_command_->tile().is_conditional()) {
		/* fuse out the branch we do not want to take */
		current_command_->branch(1 - current_branch_state_).exhaust();
	}
}

void
command_run_step::animate_beam(board_view & bv, double delta_time) const
{
	if (!current_command_->tile().is_conditional() ||
		(delta_time < .2) ||
		(delta_time >= .4 && delta_time < .6) ||
		(delta_time > .8)) {
		bv.set_robot_beam(board_view::robot_beam_t::off);
	} else {
		bv.set_robot_beam(current_branch_state_ ? board_view::robot_beam_t::hits_nothing : board_view::robot_beam_t::hits_floor);
	}
}

double
command_run_step::animation_duration() const noexcept
{
	return will_drop_ ? 0.5 : 1.0;
}

void
command_run_step::begin_step(run_step_state & state, int used_sub_steps)
{
	used_sub_steps_ = used_sub_steps;
	robot_coord_origin_ = state.robot;
	robot_coord_target_ = compute_target_coord(robot_coord_origin_);
	const auto * tile = state.grid.get(robot_coord_target_.pos.x, robot_coord_target_.pos.y);
	will_drop_ = !(tile && tile->has_floor);
	closes_trap_ = false;

	move_obstacle_ = tile ? tile->obstacle : -1;
	if (move_obstacle_ != -1) {
		obstacle_pos_origin_ = robot_coord_target_.pos;
		obstacle_pos_target_ = obstacle_pos_origin_;
		grid_vec_t v = state.robot.dir.vec();
		obstacle_pos_target_.x += v.dx;
		obstacle_pos_target_.y += v.dy;

		const auto * o_tile = state.grid.get(obstacle_pos_target_.x, obstacle_pos_target_.y);
		obstacle_will_drop_ = !(o_tile && o_tile->has_floor);

		if (obstacle_will_drop_) {
			double x = .5 * (obstacle_pos_origin_.x + obstacle_pos_target_.x);
			double y = .5 * (obstacle_pos_origin_.y + obstacle_pos_target_.y);
			double z = 0;
			double angle = robot_coord_origin_.dir.angle() + robot_coord_origin_.dir.delta_angle(robot_coord_target_.dir) * .5;
			double vx = (obstacle_pos_target_.x - obstacle_pos_target_.x);
			double vy = (obstacle_pos_target_.y - obstacle_pos_target_.y);
			double vz = 0;

			falling_obstacles_[move_obstacle_] = falling_obstacle {
				{x, y, z, angle}, vx, vy, vz, start_time() + .5
			};
		} else if (o_tile && o_tile->obstacle != -1) {
			robot_coord_target_ = robot_coord_origin_;
			move_obstacle_ = -1;
		}

		for (const auto & trigger : state.trigger_floors) {
			if (trigger.second.first == obstacle_pos_target_) {
				closed_trap_ = trigger.second.second;
				closes_trap_ = true;
			}
		}
	}

	if (current_command_->tile().is_repeat()) {
		replenish_repetitions(current_command_->branch(0), state);
		auto i = state.branch_states.emplace(&current_command_->tile(), current_command_->tile().num_repetitions()).first;
		current_branch_state_ = i->second;
		i->second -= 1;
	} else if (current_command_->tile().is_conditional()) {
		current_branch_state_ = check_floor_ahead(state, robot_coord_target_) ? 0 : 1;
	}
}

grid_coord_t
command_run_step::compute_target_coord(grid_coord_t robot) const
{
	switch (current_command_->tile().kind()) {
		case command_tile::kind_t::left: {
			robot.dir = robot.dir.left();
			break;
		}
		case command_tile::kind_t::right: {
			robot.dir = robot.dir.right();
			break;
		}
		case command_tile::kind_t::fwd1:
		case command_tile::kind_t::fwd2:
		case command_tile::kind_t::fwd3: {
			grid_vec_t v = robot.dir.vec();
			robot.pos.x += v.dx;
			robot.pos.y += v.dy;
			break;
		}
		default: {
			break;
		}
	}

	return robot;
}

bool
command_run_step::check_floor_ahead(const run_step_state & state, grid_coord_t robot) const
{
	grid_vec_t v = robot.dir.vec();
	robot.pos.x += v.dx;
	robot.pos.y += v.dy;
	const auto * tile = state.grid.get(robot.pos.x, robot.pos.y);
	return tile && tile->has_floor;
}

drop_run_step::~drop_run_step()
{
}

drop_run_step::drop_run_step(
	double start_time,
	const run_step_state & rs,
	dropping_object dropping_robot,
	std::unordered_map<int, falling_obstacle> falling_obstacles)
	: run_step(start_time)
	, dropping_robot_(std::move(dropping_robot))
	, falling_obstacles_(std::move(falling_obstacles))
{
}

void
drop_run_step::animate(
	double delta_time,
	board_view & bv) const
{
	double x = dropping_robot_.start.x + dropping_robot_.vx * delta_time;
	double y = dropping_robot_.start.y + dropping_robot_.vy * delta_time;
	double z = dropping_robot_.start.z + dropping_robot_.vz * delta_time - .5 * delta_time * delta_time * 8;
	double angle = dropping_robot_.start.angle;
	double tilt = delta_time * 240;

	bv.set_robot_pos(x, y, z, angle, tilt, 0.);

	animate_falling_obstacles(delta_time + start_time(), falling_obstacles_, bv);
}

void
drop_run_step::end_animate(
	board_view & bv) const
{
}

double
drop_run_step::animation_duration() const noexcept
{
	return 3;
}

void
drop_run_step::step(
	run_step_state & state,
	std::unique_ptr<run_step> & next_step)
{
	next_step.reset();
}


finish_goal_run_step::~finish_goal_run_step()
{
}

finish_goal_run_step::finish_goal_run_step(
	double start_time,
	grid_coord_t end_coord,
	std::unordered_map<int, falling_obstacle> falling_obstacles)
	: run_step(start_time)
	, end_coord_(end_coord)
	, falling_obstacles_(std::move(falling_obstacles))
{
}

void
finish_goal_run_step::step(
	run_step_state & state,
	std::unique_ptr<run_step> & next_step)
{
	state.succeeded = true;
	next_step.reset();
}

void
finish_goal_run_step::animate(
	double delta_time,
	board_view & bv) const
{
	view_coord_t coord = to_view_coord(end_coord_);
	coord.angle += delta_time * 240;

	bv.set_robot_pos(coord.x, coord.y, coord.z, coord.angle, 0., 0.);

	animate_falling_obstacles(delta_time + start_time(), falling_obstacles_, bv);
}

void
finish_goal_run_step::end_animate(
	board_view & bv) const
{
}

double
finish_goal_run_step::animation_duration() const noexcept
{
	return 3;
}

finish_fail_run_step::~finish_fail_run_step()
{
}

finish_fail_run_step::finish_fail_run_step(
	double start_time,
	std::unordered_map<int, falling_obstacle> falling_obstacles)
	: run_step(start_time)
	, falling_obstacles_(std::move(falling_obstacles))
{
}

void
finish_fail_run_step::step(
	run_step_state & state,
	std::unique_ptr<run_step> & next_step)
{
	next_step.reset();
}

void
finish_fail_run_step::animate(
	double delta_time,
	board_view & bv) const
{
	animate_falling_obstacles(delta_time + start_time(), falling_obstacles_, bv);
}

void
finish_fail_run_step::end_animate(
	board_view & bv) const
{
}

double
finish_fail_run_step::animation_duration() const noexcept
{
	return 3;
}


run_controller::run_controller(
	command_queue * cq, command_tile_repository * command_tile_repository, board_view * board_view, std::function<void()> success)
	: run_state_(run_state_t::not_running), puzzle_(nullptr), command_queue_(cq), command_tile_repository_(command_tile_repository)
	, board_view_(board_view), success_(std::move(success))
{
}

run_controller::~run_controller()
{
}

std::unique_ptr<command_tile_drag>
run_controller::drag_begin(double x, double y)
{
	return {};
}

void
run_controller::drag_hover_enter(double x, double y, command_tile_drag * drag)
{
}

void
run_controller::drag_hover_leave(command_tile_drag * drag)
{
}

void
run_controller::drag_finish(double x, double y, std::unique_ptr<command_tile_drag> drag)
{
	drag->undo();
}

void
run_controller::redraw(double global_phase)
{
	glEnable(GL_TEXTURE_2D);

	for (std::size_t n = 0; n < 5; ++n) {
		double x = bounds_.x1 + n * 32;
		double y = bounds_.y1;

		run_state_t kind = static_cast<run_state_t>(n);

		draw_button(x, y, kind, get_button_state(kind));
	}
}

void
run_controller::handle_button_press(double x, double y, double now)
{
	if (x < bounds_.x1 || y < bounds_.y1 || y >= bounds_.y1 + 32) {
		return;
	}
	int index = (x - bounds_.x1) / 32.;
	if (index >= 5) {
		return;
	}
	if (index == 0) {
		handle_stop();
	} else {
		handle_start(now);
	}

	animation_clock_base_ = get_current_animation_clock(now);
	wall_clock_base_ = now;

	run_state_ = static_cast<run_state_t>(index);
}

void
run_controller::draw_button(double x, double y, run_state_t kind, button_state_t state) const
{
	double button_w = 32;
	double button_h = 32;
	glColor4f(1., 1., 1., 1.);

	texture_generator::make_tex_quad2d(
		state == button_state_t::active ? texid_button_lowered_bg : texid_button_raised_bg,
		x, y,
		x + button_w, y,
		x + button_w, y + button_h,
		x, y + button_h);

	switch (state) {
		case button_state_t::disabled: {
			glColor4f(.2, .2, .2, 1.);
			break;
		}
		case button_state_t::enabled: {
			glColor4f(.5, .5, .5, 1.);
			break;
		}
		case button_state_t::active: {
			glColor4f(1., 1., .2, 1.);
			break;
		}
	}

	int texid = texid_button_halt;
	switch (kind) {
		case run_state_t::not_running: {
			texid = texid_button_halt;
			break;
		}
		case run_state_t::paused: {
			texid = texid_button_pause;
			break;
		}
		case run_state_t::run1x: {
			texid = texid_button_run1x;
			break;
		}
		case run_state_t::run2x: {
			texid = texid_button_run2x;
			break;
		}
		case run_state_t::run3x: {
			texid = texid_button_run3x;
			break;
		}
	}

	texture_generator::make_tex_quad2d(
		texid,
		x, y,
		x + button_w, y,
		x + button_w, y + button_h,
		x, y + button_h);
}

run_controller::button_state_t
run_controller::get_button_state(run_state_t kind) const
{
	return kind == run_state_ ? button_state_t::active : button_state_t::enabled;
}

double
run_controller::get_current_animation_clock(double now) const
{
	double clock_speed = 0.0;
	switch (run_state_) {
		case run_state_t::run1x: {
			clock_speed = 0.5;
			break;
		}
		case run_state_t::run2x: {
			clock_speed = 1.0;
			break;
		}
		case run_state_t::run3x: {
			clock_speed = 2.0;
			break;
		}
		default: {
			clock_speed = 0.0;
		}
	}
	return animation_clock_base_ + (now - wall_clock_base_) * clock_speed;
}


void
run_controller::set_bounds(double x1, double y1, double x2, double y2)
{
	bounds_.x1 = x1;
	bounds_.y1 = y1;
	bounds_.x2 = x2;
	bounds_.y2 = y2;
}

const run_controller::bounds_t &
run_controller::bounds() const noexcept
{
	return bounds_;
}

void
run_controller::animate_idle(double now)
{
	for (std::size_t n = 0; n < puzzle_->alternative_tiles.size(); ++n) {
		const auto & alt =  puzzle_->alternative_tiles[n];
		const grid_pos_t & pos1 = alt.first;
		const grid_pos_t & pos2 = alt.second;
		auto color = board_view::get_alternative_tile_color(n);

		color.a = get_alternative_opacity(now, 0);
		board_view_->modify_floor(pos1.x, pos1.y).color = color;
		color.a = get_alternative_opacity(now, 1);
		board_view_->modify_floor(pos2.x, pos2.y).color = color;
	}
}

void
run_controller::animate(double now)
{
	if (run_state_ == run_state_t::not_running) {
		animate_idle(now);
		return;
	}

	double animation_clock = get_current_animation_clock(now);
	while (run_step_ && animation_clock > run_step_->end_time()) {
		run_step_->end_animate(*board_view_);
		run_step_->step(run_step_state_, run_step_);
	}
	if (run_step_) {
		run_step_->animate(animation_clock - run_step_->start_time(), *board_view_);
	} else {
		bool succeeded = run_step_state_.succeeded;
		handle_stop();
		if (succeeded) {
			success_();
			run_step_state_.succeeded = false;
		}
	}
}

void
run_controller::handle_start(double now)
{
	if (run_state_ != run_state_t::not_running) {
		return;
	}

	std::vector<std::size_t> alternatives = try_find_failing_alternative(puzzle_, &command_queue_->commands());

	run_step_state_.initialize(*puzzle_, &command_queue_->commands());
	run_step_ = run_step::make_initial(*puzzle_, run_step_state_, alternatives);
	if (!run_step_) {
		return;
	}

	run_state_ = run_state_t::run1x;

	wall_clock_base_ = now;
	animation_clock_base_ = 0.0;

	command_queue_->set_locked(true);
	command_tile_repository_->set_locked(true);
}

void
run_controller::handle_stop()
{
	if (run_state_ == run_state_t::not_running) {
		return;
	}

	run_state_ = run_state_t::not_running;
	command_queue_->commands().replenish();
	command_queue_->set_locked(false);
	command_tile_repository_->set_locked(false);
	board_view_->reset(*puzzle_);
	run_step_.reset();
}

void
run_controller::reset(puzzle * puz)
{
	puzzle_ = puz;
	handle_stop();
	board_view_->reset(*puz);
}

int
simulate_execution(const puzzle * puz, const command_sequence * commands, const std::vector<std::size_t> & alternatives)
{
	run_step_state state;
	state.initialize(*puz, commands);

	return simulate_execution(*puz, state, alternatives);
}

int
simulate_execution(const puzzle & puz, run_step_state & state, const std::vector<std::size_t> & alternatives)
{
	int nsteps = 0;

	std::unique_ptr<run_step> step = run_step::make_initial(puz, state, alternatives);
	if (!step) {
		return nsteps;
	}

	while (step) {
		if (typeid(*step) == typeid(finish_goal_run_step)) {
			return std::numeric_limits<int>::max();
		} else if (typeid(*step) == typeid(finish_fail_run_step)) {
			return nsteps;
		} else if (typeid(*step) == typeid(drop_run_step)) {
			return nsteps;
		} else if (typeid(*step) == typeid(command_run_step) || typeid(*step) == typeid(setup_run_step)) {
			step->step(state, step);
			++nsteps;
		} else {
			step.reset();
		}
	}
	return nsteps;
}

std::vector<std::size_t>
try_find_failing_alternative(
	const puzzle * puz,
	const command_sequence * commands)
{
	std::size_t num_alternatives = 1 << (puz->alternative_tiles.size());

	std::vector<std::size_t> best_alternative;
	int best_score = -1;
	std::vector<std::size_t> alternatives(puz->alternative_tiles.size());

	std::size_t offset = static_cast<size_t>(random());
	for (std::size_t n = 0; n < num_alternatives; ++n) {
		for (std::size_t k = 0; k < alternatives.size(); ++k) {
			alternatives[k] = !!((n + offset) & (1<<k));
		}
		int score = simulate_execution(puz, commands, alternatives);
		if (best_score == -1 || score < best_score) {
			best_alternative = alternatives;
			best_score = score;
		}
	}

	return best_alternative;
}
