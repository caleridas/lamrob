#ifndef COMMAND_TILE_OWNER_H
#define COMMAND_TILE_OWNER_H

#include "tiles.h"

class command_tile_owner {
public:
	struct bounds_t {
		double x1, y1, x2, y2;
	};

	virtual
	~command_tile_owner();

	virtual std::unique_ptr<command_tile_drag>
	drag_begin(double x, double y) = 0;

	virtual void
	drag_hover_enter(double x, double y, command_tile_drag * drag) = 0;

	virtual void
	drag_hover_leave(command_tile_drag * drag) = 0;

	virtual void
	drag_finish(double x, double y, std::unique_ptr<command_tile_drag> drag) = 0;

	virtual void
	redraw(double global_phase) = 0;

	virtual void
	set_bounds(double x1, double y1, double x2, double y2) = 0;

	virtual const bounds_t &
	bounds() const noexcept = 0;

	virtual void
	animate(double now) = 0;

	inline bool
	within(double x, double y) const noexcept
	{
		const bounds_t & b = bounds();
		return x >= b.x1 && x < b.x2 && y >= b.y1 && y < b.y2;
	}
};

class command_tile_drag {
public:
	virtual
	~command_tile_drag();

	virtual void
	undo() = 0;

	virtual std::unique_ptr<command_point>
	consume() = 0;

	virtual void
	move(double x, double y) = 0;

	virtual command_point *
	content() = 0;

	virtual const layout_extents &
	extents() const = 0;

	virtual void
	draw(double global_phase) const = 0;

	virtual void
	set_display_args(tile_display_args_t args) = 0;
};

#endif
