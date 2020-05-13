#include "command_queue.h"

#include <GL/gl.h>

#include <chrono>

#include "texgen.h"

class command_queue_drag final : public command_tile_drag {
public:
	virtual
	~command_queue_drag()
	{
        undo();
	}

	inline
	command_queue_drag(
		command_queue * origin,
		std::unique_ptr<command_point> cpt,
		command_sequence_path origin_path,
		tile_display_args_t display_args)
		: origin_(origin)
		, cpt_(std::move(cpt))
		, origin_path_(std::move(origin_path))
		, display_args_(display_args)
	{
		extents_ = cpt_->compute_layout({}, layout_);
	}

	void
	undo() override
	{
		if (cpt_) {
			// XXX: are we responsible for clearing any "hover" state?
			origin_->drag_hover_leave(this);
			origin_->insert_at(origin_path_, std::move(cpt_));
		}
	}

	std::unique_ptr<command_point>
	consume() override
	{
		// XXX: are we responsible for clearing any "hover" state?
		origin_->drag_hover_leave(this);
		return std::move(cpt_);
	}

	void
	move(double x, double y) override
	{
		x_ = x;
		y_ = y;
		if (cpt_) {
			cpt_->apply_layout(layout_, x, y, display_args_.command_point_size, true);
		}
	}

	command_point *
	content() override
	{
		return cpt_.get();
	}

	const layout_extents &
	extents() const override
	{
		return extents_;
	}

	void
	draw(double global_phase) const override
	{
		if (cpt_) {
			cpt_->draw(global_phase, display_args_);
		}
	}

	void
	set_display_args(tile_display_args_t display_args) override
	{
		display_args_ = display_args;
		if (cpt_) {
			cpt_->apply_layout(layout_, x_, y_, display_args_.command_point_size, true);
		}
	}

private:
	command_queue * origin_;
	std::unique_ptr<command_point> cpt_;
	command_sequence_path origin_path_;
	commands_layout layout_;
	layout_extents extents_;
	tile_display_args_t display_args_;
	double x_;
	double y_;
};

void
command_queue::rearrange()
{
	seq_.compute_layout({}, layout_);
	seq_.apply_layout(layout_, x_origin(), y_origin(), tile_display_args_.command_point_size, false);
}

void
command_queue::reset()
{
	seq_.clear();
	layout_.clear();
	locked_ = false;
}

std::unique_ptr<command_tile_drag>
command_queue::drag_begin(double x, double y)
{
	if (locked_) {
		return {};
	}

	command_sequence_path path = seq_.compute_path(layout_, x - x_origin(), y - y_origin(), tile_display_args_.command_point_size);

	std::unique_ptr<command_point> cpt = seq_.unlink(path);
	if (!cpt) {
		return {};
	}
	std::unique_ptr<command_tile_drag> drag(new command_queue_drag(this, std::move(cpt), std::move(path), tile_display_args_));
	seq_.compute_layout({}, layout_);
	seq_.apply_layout(layout_, x_origin(), y_origin(), tile_display_args_.command_point_size, false);

	return std::move(drag);
}

void
command_queue::drag_hover_enter(double x, double y, command_tile_drag * drag)
{
	bool new_hover = false;
	if (!hover_) {
		hover_.reset(new hover_state);
		new_hover = true;
	}
	hover_->item = drag->content();
	hover_->item_extents = drag->extents();
	command_sequence_path new_path = seq_.compute_path(layout_, x - x_origin(), y - y_origin(), tile_display_args_.command_point_size);
	bool changed_insertion_position = new_hover || (new_path != hover_->path);
	if (changed_insertion_position) {
		hover_->path = std::move(new_path);
		seq_.compute_layout({&hover_->path, &hover_->item_extents}, hover_->layout);
		seq_.apply_layout(hover_->layout, x_origin(), y_origin(), tile_display_args_.command_point_size, false);
	}
}

void
command_queue::drag_hover_leave(command_tile_drag * drag)
{
	if (hover_) {
		hover_.reset();
		seq_.apply_layout(layout_, x_origin(), y_origin(), tile_display_args_.command_point_size, false);
	}
}

void
command_queue::drag_finish(double x, double y, std::unique_ptr<command_tile_drag> drag)
{
	hover_.reset();
	command_sequence_path path = seq_.compute_path(layout_, x - x_origin(), y - y_origin(), tile_display_args_.command_point_size);
	insert_at(path, drag->consume());
}

void
command_queue::redraw(double global_phase)
{
	if (locked_) {
		glColor4f(.4, .1, .1, .2);
	} else {
		glColor4f(.2, .2, .2, .2);
	}

	texture_generator::make_tex_quad2d(texid_solid,
		bounds_.x1, bounds_.y1,
		bounds_.x2, bounds_.y1,
		bounds_.x2, bounds_.y2,
		bounds_.x1, bounds_.y2);

	if (!seq_.empty()) {
		std::pair<double, double> in_flow = seq_.get_inflow_point();
		draw_flow(bounds_.x1, y_origin(), in_flow.first, in_flow.second, tile_display_args_.flow_scale, tile_display_args_.flow_width, global_phase);
		std::pair<double, double> out_flow = seq_.get_outflow_point();
		draw_flow(out_flow.first, out_flow.second, bounds_.x2, y_origin(), tile_display_args_.flow_scale, tile_display_args_.flow_width, global_phase);
	} else {
		draw_flow(bounds_.x1, y_origin(), bounds_.x2, y_origin(), tile_display_args_.flow_scale, tile_display_args_.flow_width, global_phase);
	}
	seq_.draw_flows(global_phase, tile_display_args_);
	seq_.draw(global_phase, tile_display_args_);
}

void
command_queue::set_bounds(double x1, double y1, double x2, double y2)
{
	bounds_.x1 = x1;
	bounds_.y1 = y1;
	bounds_.x2 = x2;
	bounds_.y2 = y2;

	if (hover_) {
		seq_.apply_layout(hover_->layout, x_origin(), y_origin(), tile_display_args_.command_point_size, true);
	} else {
		seq_.apply_layout(layout_, x_origin(), y_origin(), tile_display_args_.command_point_size, true);
	}
}

void
command_queue::configure(double x1, double y1, double x2, double y2, tile_display_args_t tile_display_args)
{
	tile_display_args_ = tile_display_args;
	set_bounds(x1, y1, x2, y2);
}

const command_queue::bounds_t &
command_queue::bounds() const noexcept
{
	return bounds_;
}

void
command_queue::animate(double now)
{
	seq_.animate(now);
}

command_sequence_path
command_queue::compute_position(double x, double y) const
{
	return command_sequence_path();
}

void
command_queue::insert_at(
	const command_sequence_path& path,
	std::unique_ptr<command_point> cpt)
{
	seq_.insert(path, std::move(cpt));
	seq_.compute_layout({}, layout_);
	seq_.apply_layout(layout_, x_origin(), y_origin(), tile_display_args_.command_point_size, false);
}
