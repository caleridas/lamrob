#include "start_screen.h"

#include "clock.h"
#include "texgen.h"

#include <sstream>
#include <iostream>

start_screen::start_screen(application * app, std::shared_ptr<background_renderer> bg)
	: view(app, std::chrono::milliseconds(100))
	, bg_(std::move(bg))
	, highlight_index_(-1)
	, exit_icon_(this, texid_exit_icon, [this](){ app_->quit(); })
{
	auto puzzles = get_puzzles();
	for (std::size_t n = 0; n < puzzles.size(); ++n) {
		puzzles_.emplace_back(std::unique_ptr<board_view>(new board_view()));
		puzzles_.back()->reset(puzzles[n]);
	}
}

void
start_screen::resize(std::size_t width, std::size_t height)
{
	num_columns_ = 1;
	for (;;) {
		col_width_ = width / num_columns_;
		row_height_ = col_width_;
		std::size_t y = row_height_ * ((puzzles_.size() - 1) / num_columns_ + 1);
		if (y < height) {
			row_offset_ = (height - y) / 2;
			col_offset_ = (width - col_width_ * num_columns_) / 2;
			break;
		} else {
			++num_columns_;
		}
	}

	double x1 = width - width / 24.;
	double y1 = width / 24.;
	exit_icon_.configure(x1, 0, width, y1);
}

void start_screen::redraw()
{
	glViewport(0, 0, width(), height());
	glDepthFunc(GL_LEQUAL);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	bg_->draw(width(), height());

	texture_generator::bind_texture();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	for (std::size_t n = 0; n < puzzles_.size(); ++n) {
		if (n <= unlocked_levels_) {
			std::size_t x = (n % num_columns_) * col_width_ + col_offset_;
			std::size_t y = (n / num_columns_) * row_height_ + row_offset_;
			puzzles_[n]->draw(width(), height(), x, y, x + col_width_, y + row_height_, get_current_time());
		}
	}

	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0., width(), height(), -0., -1., +1.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	for (std::size_t n = 0; n < puzzles_.size(); ++n) {
		std::size_t x = (n % num_columns_) * col_width_ + col_offset_;
		std::size_t y = (n / num_columns_) * row_height_ + row_offset_;

		std::size_t x0 = x + col_width_ * 1 / 4;
		std::size_t y0 = y + row_height_ * 1 / 4;
		std::size_t x1 = x + col_width_ * 3 / 4;
		std::size_t y1 = y + row_height_ * 3 / 4;
		if (n <= unlocked_levels_) {
			std::ostringstream os;
			os << (n + 1);
			texture_generator::make_scratch_text(os.str().c_str());
		} else {
			texture_generator::make_scratch_text("??");
		}

		glColor4f(1, 1, 1, 1);
		texture_generator::make_tex_quad2d(
			texid_scratch,
			x0, y0, x1, y0, x1, y1, x0, y1);

		if (highlight_index_ == n) {
			if (highlight_ == highlight_t::hover) {
				glColor4f(1, 1, 1, .25);
				texture_generator::make_tex_quad2d(
					texid_solid,
					x0, y0, x1, y0, x1, y1, x0, y1);
			} else if (highlight_ == highlight_t::pressed) {
				glColor4f(1, 0, 0, .5);
				texture_generator::make_tex_quad2d(
					texid_solid,
					x0, y0, x1, y0, x1, y1, x0, y1);
			}
		}
	}

	exit_icon_.redraw();
}

void
start_screen::notify_activate()
{
	highlight_ = highlight_t::none;
	exit_icon_.reset();
}

void
start_screen::notify_deactivate()
{
}

void
start_screen::handle_button_press(
	int button,
	int x, int y,
	int button_state)
{
	if (exit_icon_.handle_button_press(button, x, y, button_state)) {
		return;
	}
	bool any_index;
	std::size_t index;
	std::tie(any_index, index) = get_index(x, y);

	if (any_index) {
		highlight_ = highlight_t::pressed;
		highlight_index_ = index;
	} else {
		highlight_ = highlight_t::none;
	}
}

void
start_screen::handle_button_release(
	int button,
	int x, int y,
	int button_state)
{
	if (exit_icon_.handle_button_release(button, x, y, button_state)) {
		return;
	}
	bool any_index;
	std::size_t index;
	std::tie(any_index, index) = get_index(x, y);

	if (highlight_ == highlight_t::pressed && any_index && index == highlight_index_) {
		highlight_ = highlight_t::none;
		request_redraw();
		if (start_level_handler_) {
			start_level_handler_(index);
		}
	} else {
		if (highlight_ == highlight_t::pressed) {
			highlight_ = highlight_t::none;
		}
		handle_pointer_motion(x, y, button_state);
	}
}

void
start_screen::handle_pointer_motion(
	int x, int y,
	int button_state)
{
	exit_icon_.handle_pointer_motion(x, y, button_state);

	bool any_index;
	std::size_t index;
	std::tie(any_index, index) = get_index(x, y);
	if (any_index) {
		if (highlight_ == highlight_t::pressed && highlight_index_ == index) {
			return;
		}
		highlight_ = highlight_t::hover;
		highlight_index_ = index;
		request_redraw();
	} else {
		highlight_ = highlight_t::none;
		request_redraw();
	}
}

void
start_screen::handle_key_press(
	int key_code,
	const keyboard_state & state,
	const char * chars)
{
	if (key_code == XK_Escape) {
		app_->quit();
	}
}

void
start_screen::timer_tick()
{
	update_current_time();

	bg_->animate(get_current_time());

	request_redraw();
}

std::pair<bool, std::size_t>
start_screen::get_index(double x, double y) const
{
	if (x < col_offset_) {
		return {false, 0};
	}
	if (x > col_offset_ + num_columns_ * col_width_) {
		return {false, 0};
	}
	std::size_t col = static_cast<std::size_t>((x - col_offset_) / col_width_);

	if (y < row_offset_) {
		return {false, 0};
	}
	std::size_t row = static_cast<std::size_t>((y - row_offset_) / row_height_);
	std::size_t index = col + row * num_columns_;
	if (index >= puzzles_.size() || index > unlocked_levels_) {
		return {false, 0};
	} else {
		return {true, index};
	}
}

void
start_screen::set_start_level_handler(
	std::function<void(std::size_t)> start_level_handler)
{
	start_level_handler_ = std::move(start_level_handler);
}

void
start_screen::set_unlocked_levels(std::size_t unlocked_levels)
{
	unlocked_levels_ = unlocked_levels;
}



