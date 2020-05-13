#include "icon.h"

#include "texgen.h"

icon::icon(view * owner, int texture_id, std::function<void()> on_click)
	: owner_(owner), texture_id_(texture_id), on_click_(std::move(on_click))
	, x1_(0), y1_(0), x2_(0), y2_(0)
	, state_(state_t::none)
{
}

void
icon::redraw()
{
	switch (state_) {
		case state_t::none:
		case state_t::pressed_move_away: {
			glColor4f(1., 1., 0., 1.);
			break;
		}
		case state_t::hovering: {
			glColor4f(1., 1., .5, 1.);
			break;
		}
		case state_t::pressed: {
			glColor4f(1., .5, .5, 1.);
			break;
		}
	}
	texture_generator::make_tex_quad2d(
		texture_id_,
		x1_ + pad_, y1_ + pad_,
		x2_ - pad_, y1_ + pad_,
		x2_ - pad_, y2_ - pad_,
		x1_ + pad_, y2_ - pad_);
}

void
icon::reset()
{
	state_ = state_t::none;
}

void
icon::configure(double x1, double y1, double x2, double y2)
{
	x1_ = x1;
	y1_ = y1;
	x2_ = x2;
	y2_ = y2;
}

bool
icon::handle_button_press(
	int button,
	int x, int y,
	int button_state)
{
	bool within = (x >= x1_ && x < x2_ && y >= y1_ && y < y2_);
	if (!within) {
		return false;
	}
	state_ = state_t::pressed;
	request_redraw();
	return true;
}

bool
icon::handle_button_release(
	int button,
	int x, int y,
	int button_state)
{
	bool within = (x >= x1_ && x < x2_ && y >= y1_ && y < y2_);
	bool was_pressed = (state_ == state_t::pressed || state_ == state_t::pressed_move_away);
	state_ = state_t::none;
	request_redraw();

	if (within) {
		if (was_pressed) {
			on_click_();
		}
		return true;
	} else {
		return false;
	}
}

void
icon::handle_pointer_motion(
	int x, int y,
	int button_state)
{
	bool within = (x >= x1_ && x < x2_ && y >= y1_ && y < y2_);
	bool was_pressed = (state_ == state_t::pressed || state_ == state_t::pressed_move_away);
	if (was_pressed) {
		state_ = within ? state_t::pressed : state_t::pressed_move_away;
		request_redraw();
	} else {
		state_ = within ? state_t::hovering : state_t::none;
		request_redraw();
	}
}

void
icon::request_redraw()
{
	owner_->request_redraw();
}
