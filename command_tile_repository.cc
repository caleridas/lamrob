#include "command_tile_repository.h"

#include <GL/gl.h>

#include "texgen.h"

#include <iostream>

class command_tile_repository_drag final : public command_tile_drag {
public:
	virtual
	~command_tile_repository_drag()
	{
        undo();
	}

	inline
	command_tile_repository_drag(
		command_tile_repository * origin,
		std::unique_ptr<command_tile> tile,
		tile_display_args_t display_args)
		: origin_(origin)
		, cpt_(new command_point(std::move(tile)))
		, display_args_(display_args)
	{
		extents_ = cpt_->compute_layout({}, layout_);
	}

	void
	undo() override
	{
		if (cpt_) {
			origin_->append(std::move(*cpt_).extract_tile());
			cpt_.reset();
		}
	}

	std::unique_ptr<command_point>
	consume() override
	{
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
	command_tile_repository * origin_;
	std::unique_ptr<command_point> cpt_;
	commands_layout layout_;
	layout_extents extents_;
	tile_display_args_t display_args_;
	double x_;
	double y_;
};

void
command_tile_repository::rearrange(bool warp)
{
	std::unordered_map<command_tile *, layout_extents> layout;

	double xcol[2] = {
		bounds_.x1 + (bounds_.x2 - bounds_.x1) * 0.25,
		bounds_.x1 + (bounds_.x2 - bounds_.x1) * 0.75
	};
	for (std::size_t n = command_tile::min_kind; n <= command_tile::max_kind; ++n) {
		command_tile::kind_t kind = static_cast<command_tile::kind_t>(n);
		auto i = piles_.find(kind);
		if (i == piles_.end()) {
			continue;
		}
		const auto & tiles = i->second;

		std::size_t row = (n - command_tile::min_kind) / 2;
		std::size_t col = (n - command_tile::min_kind) % 2;
		double x = xcol[col];
		double y = (row + .5) * tile_display_args_.command_point_size + bounds_.y1;

		for (std::size_t k = 0; k < tiles.size(); ++k) {
			layout_extents e;
			e.x = x;
			e.y = y + (tiles.size() - k - 1) * 5.0;
			e.width = tile_display_args_.command_point_size;
			e.top = -tile_display_args_.command_point_size * .5;
			e.bottom = +tile_display_args_.command_point_size * .5;
			layout[tiles[k].get()] = e;
		}
	}

	for (const auto& pile : piles_) {
		for (const auto& tile : pile.second) {
			const layout_extents& e = layout[tile.get()];
			if (warp) {
				tile->move_to(e.x, e.y);
			} else {
				tile->accelerate_to(e.x, e.y);
			}
		}
	}
}

void
command_tile_repository::reset()
{
	piles_.clear();
	locked_ = false;
}


std::unique_ptr<command_tile_drag>
command_tile_repository::drag_begin(double x, double y)
{
	if (locked_) {
		return {};
	}

	std::size_t column = (x - bounds_.x1) / (.5 * (bounds_.x2 - bounds_.x1));
	std::size_t row = (y - bounds_.y1) / tile_display_args_.command_point_size;
	if (column < 0 || column > 1) {
		return {};
	}
	std::size_t index = column + 2 * row + command_tile::min_kind;
	if (index > command_tile::max_kind) {
		return {};
	}
	auto & pile = piles_[static_cast<command_tile::kind_t>(index)];
	if (pile.empty()) {
		return {};
	}
	std::unique_ptr<command_tile> tile = std::move(pile.back());
	pile.pop_back();
	rearrange(false);

	std::unique_ptr<command_tile_repository_drag> drag(new command_tile_repository_drag(this, std::move(tile), tile_display_args_));
	drag->move(x, y);

	return std::move(drag);
}

void
command_tile_repository::drag_hover_enter(double x, double y, command_tile_drag * drag)
{
}

void
command_tile_repository::drag_hover_leave(command_tile_drag * drag)
{
}

void
command_tile_repository::drag_finish(double x, double y, std::unique_ptr<command_tile_drag> drag)
{
	put_internal(drag->consume());
	rearrange(false);
}

void
command_tile_repository::redraw(double global_phase)
{
	glColor4f(.2, .3, .1, .2);
	texture_generator::make_tex_quad2d(texid_solid,
		bounds_.x1, bounds_.y1,
		bounds_.x2, bounds_.y1,
		bounds_.x2, bounds_.y2,
		bounds_.x1, bounds_.y2);

	for (const auto & pile : piles_) {
		for (const auto & tile : pile.second) {
			tile->draw(global_phase, tile_display_args_);
		}
	}
}

void
command_tile_repository::set_bounds(double x1, double y1, double x2, double y2)
{
	bounds_.x1 = x1;
	bounds_.y1 = y1;
	bounds_.x2 = x2;
	bounds_.y2 = y2;
	rearrange(true);
}

void
command_tile_repository::configure(double x1, double y1, double x2, double y2, tile_display_args_t tile_display_args)
{
	tile_display_args_ = tile_display_args;
	set_bounds(x1, y1, x2, y2);
}

const command_tile_repository::bounds_t &
command_tile_repository::bounds() const noexcept
{
	return bounds_;
}

void
command_tile_repository::animate(double now)
{
	for (auto & pile : piles_) {
		for (const auto & tile : pile.second) {
			tile->animate(now);
		}
	}
}

void
command_tile_repository::append(std::unique_ptr<command_tile> tile)
{
	put_internal(std::move(tile));
	rearrange(false);
}

void
command_tile_repository::put_internal(std::unique_ptr<command_point> cpt)
{
	for (std::size_t n = 0; n < cpt->num_branches(); ++n) {
		put_internal(std::move(cpt->branch(n)));
	}
	put_internal(std::move(*cpt).extract_tile());
}

void
command_tile_repository::put_internal(command_sequence seq)
{
	for (auto& point : seq) {
		put_internal(std::move(point));
	}
}

void
command_tile_repository::put_internal(std::unique_ptr<command_tile> tile)
{
	auto& pile = piles_[tile->kind()];
	pile.push_back(std::move(tile));
}

void
command_tile_repository::set_locked(bool locked)
{
	locked_ = locked;
}

