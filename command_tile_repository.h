#ifndef COMMAND_TILE_REPOSITORY_H
#define COMMAND_TILE_REPOSITORY_H

#include "command_tile_owner.h"

class command_tile_repository final : public command_tile_owner {
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
	append(std::unique_ptr<command_tile> tile);

	void
	rearrange(bool warp);

	void
	reset();

	void
	set_locked(bool locked);

private:
	void
	put_internal(std::unique_ptr<command_point> cpt);

	void
	put_internal(command_sequence seq);

	void
	put_internal(std::unique_ptr<command_tile> tile);

	bounds_t bounds_;
	std::map<command_tile::kind_t, std::vector<std::unique_ptr<command_tile>>> piles_;
	bool locked_ = false;

	tile_display_args_t tile_display_args_;
};

#endif
