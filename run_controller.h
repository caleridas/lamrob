#ifndef RUN_CONTROLLER_H
#define RUN_CONTROLLER_H

#include <functional>

#include "board_view.h"
#include "command_tile_owner.h"
#include "command_queue.h"
#include "command_tile_repository.h"
#include "tiles.h"

struct run_step_state {
	struct tile_t {
		bool has_floor = false;
		int obstacle = -1;
	};

	using grid_t = grid_tpl<tile_t>;

	const command_sequence * seq;

	grid_t grid;
	grid_pos_t goal;
	grid_coord_t robot;
	bool succeeded = false;

	std::unordered_map<int, grid_pos_t> obstacles;

	std::unordered_map<const command_tile *, int> branch_states;

	std::map<int, std::pair<grid_pos_t, grid_pos_t>> trigger_floors;

	void
	set_obstacle(int index, grid_pos_t pos);

	void
	clear_obstacle(int index);

	void
	initialize(const puzzle & puz, const command_sequence * init_seq);
};

struct dropping_object {
	view_coord_t start;
	double vx, vy, vz;
};

struct falling_obstacle {
	view_coord_t start;
	double vx, vy, vz;
	double start_time;
};

class run_step {
public:
	virtual
	~run_step();

	inline explicit run_step(double start_time) noexcept : start_time_(start_time) {}

	virtual void
	step(
		run_step_state & state,
		std::unique_ptr<run_step> & next_step) = 0;

	virtual void
	animate(
		double delta_time,
		board_view & bv) const = 0;

	virtual void
	end_animate(
		board_view & bv) const = 0;

	virtual double
	animation_duration() const noexcept = 0;

	inline double
	start_time() const noexcept { return start_time_; }

	inline double
	end_time() const noexcept { return start_time_ + animation_duration(); }

	static std::unique_ptr<run_step>
	make_initial(
		const puzzle & puz,
		const run_step_state & state,
		const std::vector<std::size_t> & alternatives);

protected:
	inline void set_start_time(double start_time) noexcept { start_time_ = start_time; }

private:
	double start_time_;
};

class setup_run_step final : public run_step {
public:
	~setup_run_step() override;

	setup_run_step(
		double start_time,
		const puzzle & puz,
		const std::vector<std::size_t> & alternatives);

	void
	step(
		run_step_state & state,
		std::unique_ptr<run_step> & next_step) override;

	void
	animate(
		double delta_time,
		board_view & bv) const override;

	void
	end_animate(
		board_view & bv) const override;

	double
	animation_duration() const noexcept override;

private:
	struct floor_tile_goal_t {
		grid_pos_t pos;
		board_view::tile_color_t color;
		bool present;
		double start_state;
	};

	std::vector<floor_tile_goal_t> floor_tiles_;
};

class command_run_step final : public run_step {
public:
	~command_run_step() override;

	command_run_step(
		double start_time,
		command_sequence_path current_command,
		run_step_state & rs,
		std::unordered_map<int, falling_obstacle> falling_obstacles);

	void
	step(
		run_step_state & state,
		std::unique_ptr<run_step> & next_step) override;

	void
	animate(
		double delta_time,
		board_view & bv) const override;

	void
	end_animate(
		board_view & bv) const override;

	double
	animation_duration() const noexcept override;

private:
	void
	begin_step(run_step_state & rs, int used_sub_steps);

	grid_coord_t
	compute_target_coord(grid_coord_t robot) const;

	bool
	check_floor_ahead(const run_step_state & state, grid_coord_t robot) const;

	void
	animate_branch_states() const;

	void
	animate_beam(board_view & bv, double delta_time) const;

	void
	advance_command(run_step_state & state);

	/* path in program to current command */
	command_sequence_path current_path_;
	/* current command point in program */
	command_point * current_command_;
	/* If current command is a repetition, then this
	 * is the number of repeats left (including the current
	 * one). If current command is a conditional, then
	 * this is the alternative to take.
	 * Otherwise, this field is meaningless. */
	std::size_t current_branch_state_;
	/* Number of substep for current command. It is
	 * always zero for every single step command.
	 * It is used to count the number of steps for
	 * "multi-step forward" operations */
	int used_sub_steps_;

	grid_coord_t robot_coord_origin_;
	grid_coord_t robot_coord_target_;
	bool will_drop_;

	/* id of obstacle moved, if any; -1 if nothing moved */
	int move_obstacle_;
	grid_pos_t obstacle_pos_origin_;
	grid_pos_t obstacle_pos_target_;
	bool obstacle_will_drop_;

	/* whether a trap will be closed as consequence of this move */
	bool closes_trap_;
	grid_pos_t closed_trap_;

	/* obstacles currently in free-fall */
	std::unordered_map<int, falling_obstacle> falling_obstacles_;
};

class drop_run_step final : public run_step {
public:
	~drop_run_step() override;

	drop_run_step(
		double start_time,
		const run_step_state & rs,
		dropping_object dropping_robot,
		std::unordered_map<int, falling_obstacle> falling_obstacles);

	void
	step(
		run_step_state & state,
		std::unique_ptr<run_step> & next_step) override;

	void
	animate(
		double delta_time,
		board_view & bv) const override;

	void
	end_animate(
		board_view & bv) const override;

	double
	animation_duration() const noexcept override;

private:
	dropping_object dropping_robot_;

	/* obstacles currently in free-fall */
	std::unordered_map<int, falling_obstacle> falling_obstacles_;
};

class finish_goal_run_step final : public run_step {
public:
	~finish_goal_run_step() override;

	finish_goal_run_step(
		double start_time,
		grid_coord_t end_coord,
		std::unordered_map<int, falling_obstacle> falling_obstacles);

	void
	step(
		run_step_state & state,
		std::unique_ptr<run_step> & next_step) override;

	void
	animate(
		double delta_time,
		board_view & bv) const override;

	void
	end_animate(
		board_view & bv) const override;

	double
	animation_duration() const noexcept override;

private:
	grid_coord_t end_coord_;

	/* obstacles currently in free-fall */
	std::unordered_map<int, falling_obstacle> falling_obstacles_;
};

class finish_fail_run_step final : public run_step {
public:
	~finish_fail_run_step() override;

	finish_fail_run_step(
		double start_time,
		std::unordered_map<int, falling_obstacle> falling_obstacles);

	void
	step(
		run_step_state & state,
		std::unique_ptr<run_step> & next_step) override;

	void
	animate(
		double delta_time,
		board_view & bv) const override;

	void
	end_animate(
		board_view & bv) const override;

	double
	animation_duration() const noexcept override;
private:

	/* obstacles currently in free-fall */
	std::unordered_map<int, falling_obstacle> falling_obstacles_;
};

/* Simulates execution, returns number of steps after which
 * program fails, or numeric_limits<int>::max() on success. */
int
simulate_execution(
	const puzzle * puz,
	const command_sequence * commands,
	const std::vector<std::size_t> & alternatives);

/* Simulates execution, returns number of steps after which
 * program fails, or numeric_limits<int>::max() on success. */
int
simulate_execution(
	const puzzle & puz,
	run_step_state & state,
	const std::vector<std::size_t> & alternatives);

/* Tests program execution against all alternatives. If
 * an alternative fails, then return the shortest
 * failing alternative. Otherwise, return any succeeding
 * one.
 * Note: function picks at random. Not deterministic. */
std::vector<std::size_t>
try_find_failing_alternative(
	const puzzle * puz,
	const command_sequence * commands);

class run_controller final : public command_tile_owner {
public:
	run_controller(
		command_queue * cq,
		command_tile_repository * command_tile_repository,
		board_view * board_view,
		std::function<void()> success);

	~run_controller() override;

	enum class run_state_t {
		not_running = 0,
		paused = 1,
		run1x = 2,
		run2x = 3,
		run3x = 4
	};

	std::unique_ptr<command_tile_drag>
	drag_begin(double x, double y) override;

	void
	drag_hover_enter(double x, double y, command_tile_drag * drag) override;

	void
	drag_hover_leave(command_tile_drag * drag) override;

	void
	drag_finish(double x, double y, std::unique_ptr<command_tile_drag> drag) override;

	void
	redraw(double global_phase) override;

	void
	set_bounds(double x1, double y1, double x2, double y2) override;

	const bounds_t &
	bounds() const noexcept override;

	void
	animate(double now) override;

	void
	handle_start(double now);

	void
	handle_stop();

	void
	handle_button_press(double x, double y, double now);

	void
	reset(puzzle * puz);

	inline run_state_t run_state() const noexcept { return run_state_; }

private:
	enum class button_state_t {
		disabled = 0,
		enabled = 1,
		active = 2
	};

	void
	animate_idle(double now);

	void
	draw_button(double x, double y, run_state_t kind, button_state_t state) const;

	button_state_t
	get_button_state(run_state_t kind) const;

	double
	get_current_animation_clock(double now) const;

	run_state_t run_state_;

	puzzle * puzzle_;
	command_queue * command_queue_;
	command_tile_repository * command_tile_repository_;
	board_view * board_view_;

	// Pair of corresponding time points.
	double wall_clock_base_;
	double animation_clock_base_;

	run_step_state run_step_state_;
	std::unique_ptr<run_step> run_step_;

	bounds_t bounds_;

	std::function<void()> success_;
};

#endif
