#ifndef TILES_H
#define TILES_H

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

struct tile_display_args_t {
	std::size_t tile_size = 60;
	std::size_t command_point_size = 80;
	std::size_t flow_width = 2;
	std::size_t flow_scale = 20;
};

class command_tile;
class command_tile_owner;
class command_tile_drag;

class elastic_object {
public:
	void
	animate(double now);

	void
	move_to(double x, double y);

	void
	accelerate_to(double x, double y);

	inline double x() const noexcept { return x_; }
	inline double y() const noexcept { return y_; }

private:
	double x_ = 0., y_ = 0.;
	double vx_ = 0., vy_ = 0.;
	double tx_ = 0., ty_ = 0.;
};

class command_tile : public elastic_object {
public:
	enum class kind_t {
		left = 1,
		right = 2,
		fwd1 = 3,
		fwd2 = 4,
		fwd3 = 5,
		conditional = 6,
		rep0 = 7,
		rep1 = 8,
		rep2 = 9,
		rep3 = 10,
		rep4 = 11,
		rep5 = 12
	};

	static constexpr std::size_t min_kind = 1;
	static constexpr std::size_t max_kind = 12;

	enum class state_t {
		normal = 0,
		flashing = 1,
		depleted = 2
	};

	command_tile(kind_t kind, double phase);
	command_tile(const command_tile & other) = default;

	void
	draw(double global_phase, const tile_display_args_t & display_args) const;

	/* reset all visual attributes to initial state */
	void
	replenish();

	/* set tile to exhausted state */
	void
	exhaust();

	inline kind_t kind() const noexcept { return kind_; }

	inline state_t state() const noexcept { return state_; }
	inline void set_state(state_t state) noexcept { state_ = state; }

	inline void set_repetitions_left(int repetitions_left) noexcept { repetitions_left_ = repetitions_left; }

	int num_repetitions() const noexcept;

	inline bool
	is_conditional() const noexcept
	{
		return kind() == kind_t::conditional;
	}

	inline bool
	is_repeat() const noexcept
	{
		return kind() > kind_t::conditional;
	}

	inline std::size_t num_branches() const noexcept
	{
		if (is_conditional()) {
			return 2;
		} else if (is_repeat()) {
			return 1;
		} else {
			return 0;
		}
	}

	inline std::size_t
	num_flow_points() const noexcept
	{
		if (is_conditional() || is_repeat()) {
			return 3;
		} else {
			return 0;
		}
	}

private:
	int
	get_tex_index() const noexcept;

	void
	gl_main_color(double global_phase) const;

	bool
	gl_glow_color(double global_phase) const;

	double
	get_intensity(double global_phase) const;

	/* Behaviour attributes. */
	kind_t kind_;

	/* Visual state */
	state_t state_ = state_t::normal;
	int repetitions_left_;
	double phase_;
};

class command_flow_point : public elastic_object {};

struct layout_extents {
	inline layout_extents() {}
	inline layout_extents(double init_x, double init_y, double init_top, double init_bottom, double init_width)
		: x(init_x)
		, y(init_y)
		, top(init_top)
		, bottom(init_bottom)
		, width(init_width)
	{
	}

	/* location of reference point */
	double x, y;

	/* extents, relative to reference point */
	double top, bottom;

	/* total width */
	double width;

	inline void union_with(const layout_extents& other) {
		width += other.width;
		top = std::min(top, other.top);
		bottom = std::max(bottom, other.bottom);
	}
};

class command_point;
class command_sequence;
class command_sequence_path;
class command_branch_path;

struct commands_layout {
	std::unordered_map<const command_point *, layout_extents> points;
	std::unordered_map<const command_sequence *, layout_extents> sequences;
	std::unordered_map<const command_flow_point *, layout_extents> flow_points;

	inline void clear()
	{
		points.clear();
		sequences.clear();
		flow_points.clear();
	}
};

class command_sequence {
private:
	using repr_type = std::vector<std::unique_ptr<command_point>>;

public:
	using iterator = repr_type::iterator;
	using const_iterator = repr_type::const_iterator;
	using size_type = repr_type::size_type;
	using value_type = repr_type::value_type;

	struct virtual_insertion {
		inline virtual_insertion() noexcept {}
		inline virtual_insertion(
			const command_sequence_path * init_position,
			const layout_extents * init_extents)
			: position(init_position), extents(init_extents)
		{
		}
		const command_sequence_path * position = nullptr;
		const layout_extents * extents = nullptr;
	};

	inline command_sequence() {}
	command_sequence(const command_sequence & other);

	command_sequence & operator=(const command_sequence & other);

	inline size_type size() const noexcept { return elements.size(); }
	inline bool empty() const noexcept { return elements.empty(); }

	inline iterator begin() noexcept { return elements.begin(); }
	inline iterator end() noexcept { return elements.end(); }
	inline const_iterator begin() const noexcept { return elements.begin(); }
	inline const_iterator end() const noexcept { return elements.end(); }
	inline std::unique_ptr<command_point>& operator[](size_type index) noexcept { return elements[index]; }
	inline const std::unique_ptr<command_point>& operator[](size_type index) const noexcept { return elements[index]; }

	inline void erase(iterator i) { elements.erase(i); }
	inline void clear() { elements.clear(); }

	inline void insert(std::size_t index, std::unique_ptr<command_point> point) {
		elements.insert(elements.begin() + index, std::move(point));
	}

	inline void append(std::unique_ptr<command_point> point) { elements.emplace_back(std::move(point)); }

	void
	draw_flows(double global_phase, const tile_display_args_t & display_args) const;

	std::pair<double, double>
	get_inflow_point() const;

	std::pair<double, double>
	get_outflow_point() const;

	void
	draw(double global_phase, const tile_display_args_t & display_args) const;

	void
	animate(double now) const;

	void
	replenish() const;

	void
	exhaust() const;

	layout_extents &
	compute_layout(
		const virtual_insertion & insertion,
		commands_layout & layout) const;

	void
	apply_layout(
		const commands_layout & layout,
		double x_origin,
		double y_origin,
		double scale,
		bool warp) const;

	command_sequence_path
	compute_path(
		const commands_layout& layout,
		double x,
		double y,
		double scale) const;

	void
	insert(
		const command_sequence_path & path,
		std::unique_ptr<command_point> cpt);

	std::unique_ptr<command_point>
	unlink(
		const command_sequence_path & path);

	command_point *
	lookup(
		const command_sequence_path & path) const;

private:
	std::vector<std::unique_ptr<command_point>> elements;
};

class command_point {
public:
	struct virtual_insertion {
		inline virtual_insertion() noexcept {}
		inline virtual_insertion(
			const command_branch_path * init_position,
			const layout_extents * init_extents)
			: position(init_position), extents(init_extents)
		{
		}
		const command_branch_path * position = nullptr;
		const layout_extents * extents = nullptr;
	};

	command_point(const command_point & other);
	command_point(std::unique_ptr<command_tile> tile);

	command_point & operator=(const command_point & other);

	inline command_tile & tile() const noexcept { return *tile_; }
	std::unique_ptr<command_tile> extract_tile() && noexcept;

	inline const command_sequence & branch(std::size_t index) const noexcept { return branches_[index]; }
	inline command_sequence & branch(std::size_t index) noexcept { return branches_[index]; }
	inline std::size_t num_branches() const noexcept { return tile_ ? tile_->num_branches() : 0; }

	void
	draw_flows(double global_phase, const tile_display_args_t & display_args) const;

	std::pair<double, double>
	get_inflow_point() const;

	std::pair<double, double>
	get_outflow_point() const;

	void
	draw(double global_phase, const tile_display_args_t & display_args) const;

	void
	animate(double now) const;

	void
	replenish() const;

	void
	exhaust() const;

	layout_extents &
	compute_layout(
		const virtual_insertion & insertion,
		commands_layout & layout) const;

	void
	apply_layout(
		const commands_layout & layout,
		double x_origin,
		double y_origin,
		double scale,
		bool warp) const;

	std::unique_ptr<command_branch_path>
	compute_path(
		const commands_layout& layout,
		double x,
		double y,
		double scale) const;

	inline std::size_t num_flow_points() const noexcept { return tile_ ? tile_->num_flow_points() : 0; }

private:
	std::unique_ptr<command_tile> tile_;
	std::unique_ptr<command_sequence[]> branches_;
	std::unique_ptr<command_flow_point[]> flow_points_;
};

class command_branch_path;
class command_sequence_path {
public:
	command_sequence_path() noexcept;
	explicit command_sequence_path(std::size_t init_index) noexcept;
	command_sequence_path(
		std::size_t init_index,
		std::unique_ptr<command_branch_path> init_branch) noexcept;
	command_sequence_path(command_sequence_path && other) noexcept;

	command_sequence_path &
	operator=(command_sequence_path && other) noexcept;

	bool
	operator==(const command_sequence_path& other) const noexcept;
	bool
	operator!=(const command_sequence_path& other) const noexcept;

	std::size_t depth() const noexcept;

	inline bool is_leaf() const noexcept { return !branch; }

	void advance();

	void up();

	void down(std::size_t branch_index);

	/* index to insert before (if branch == nullptr),
	 * or tile index to insert under */
	std::size_t index;
	std::unique_ptr<command_branch_path> branch;
};

class command_branch_path {
public:
	command_branch_path(command_branch_path && other) noexcept;

	command_branch_path(
		std::size_t init_branch,
		command_sequence_path && init_seq) noexcept;

	bool
	operator==(const command_branch_path& other) const noexcept;

	bool
	operator!=(const command_branch_path& other) const noexcept;

	std::size_t branch;
	command_sequence_path seq;
};

void
draw_flow(
	double x1, double y1,
	double x2, double y2,
	double scale,
	double width,
	double phase);

void
draw_flow(
	const std::vector<std::pair<double, double>> & points,
	double scale,
	double width,
	double phase);

#endif
