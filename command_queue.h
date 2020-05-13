#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include "command_tile_owner.h"

class command_queue final : public command_tile_owner {
public:
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

	void
	configure(double x1, double y1, double x2, double y2, tile_display_args_t tile_display_args);

	const bounds_t &
	bounds() const noexcept override;

	void
	animate(double now) override;

	void
	insert_at(
		const command_sequence_path& path,
		std::unique_ptr<command_point> cpt);

	command_sequence_path
	compute_position(double x, double y) const;

	void
	rearrange();

	void
	reset();

	inline void
	set_locked(bool locked) noexcept;

	inline const command_sequence &
	commands() const noexcept;

private:
	struct hover_state {
		command_point * item;
		layout_extents item_extents;
		command_sequence_path path;
		commands_layout layout;
	};

	inline double x_origin() const noexcept
	{
		return bounds_.x1 + 80;
	}

	inline double y_origin() const noexcept
	{
		return .5 * (bounds_.y2 + bounds_.y1);
	}

	bounds_t bounds_;

	command_sequence seq_;
	commands_layout layout_;
	std::unique_ptr<hover_state> hover_;

	bool locked_ = false;

	tile_display_args_t tile_display_args_;
};

inline void
command_queue::set_locked(bool locked) noexcept
{
	locked_ = locked;
}

inline const command_sequence &
command_queue::commands() const noexcept
{
	return seq_;
}


#endif
