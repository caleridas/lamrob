#include "main_screen.h"

#include <math.h>

#include "clock.h"
#include "texgen.h"
#include "background.h"

namespace {

static background_renderer bg;

std::unique_ptr<command_tile> new_tile(command_tile::kind_t kind, double phase)
{
	return std::unique_ptr<command_tile>(new command_tile(kind, phase));
}

}

main_screen::main_screen(application * app, std::shared_ptr<background_renderer> bg)
	: view(app, std::chrono::milliseconds(20))
	, bg_(std::move(bg))
	, run_controller_(&cq_, &repo_, &board_, [this](){ handle_level_win(); })
	, back_icon_(this, texid_back_icon, [this]() { exit_main_screen_handler_(); })
{
}

void
main_screen::resize(std::size_t width, std::size_t height)
{
	tile_display_args_.tile_size = width / 32;
	tile_display_args_.command_point_size = width / 24;

	double x1 = width - tile_display_args_.command_point_size * 2;
	double y1 = height - tile_display_args_.command_point_size * 4;

	double x2 = x1 - width / 24.;
	double y2 = width / 24.;

	repo_.configure(x1, 0, width, y1, tile_display_args_);
	cq_.configure(0, y1, width, height, tile_display_args_);
	back_icon_.configure(x2, 0, x1, y2);
	run_controller_.set_bounds(10, 10, 210, 42);
	if (dragging_) {
		dragging_->set_display_args(tile_display_args_);
	}
}

void
main_screen::redraw()
{
	update_current_time();

	double now = get_current_time();

	glViewport(0, 0, width(), height());

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	bg_->draw(width(), height());

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	texture_generator::bind_texture();

	board_.draw(width(), height(), now);

	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0., width(), height(), -0., -1., +1.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	repo_.redraw(now);
	cq_.redraw(now);
	run_controller_.redraw(now);
	if (dragging_) {
		dragging_->draw(now);
	}

	back_icon_.redraw();
}

void
main_screen::handle_button_press(
	int button,
	int x, int y,
	int button_state)
{
	update_current_time();

	if (back_icon_.handle_button_press(button, x, y, button_state)) {
		return;
	}

	if (run_controller_.within(x, y)) {
		run_controller_.handle_button_press(x, y, get_current_time());
		return;
	}

	if (dragging_) {
		return;
	}

	command_tile_owner * owners[] = {&cq_, &repo_};
	for (command_tile_owner * o : owners) {
		if (o->within(x, y)) {
			dragging_ = o->drag_begin(x, y);
			if (dragging_) {
				o->drag_hover_enter(x, y, dragging_.get());
				return;
			}
		}
	}
}

void
main_screen::handle_button_release(
	int button,
	int x, int y,
	int button_state)
{
	if (back_icon_.handle_button_release(button, x, y, button_state)) {
		return;
	}

	if (dragging_) {
		command_tile_owner * owners[] = {&cq_, &repo_};
		for (command_tile_owner * o : owners) {
			if (o->within(x, y)) {
				o->drag_finish(x, y, std::move(dragging_));
				return;
			}
		}
		dragging_->undo();
		dragging_.reset();
	}
}

void
main_screen::handle_key_press(
	int key_code,
	const keyboard_state & state,
	const char * chars)
{
	if (key_code == XK_Escape) {
		exit_main_screen_handler_();
	}
}

void main_screen::handle_pointer_motion(
	int x, int y,
	int button_state)
{
	back_icon_.handle_pointer_motion(x, y, button_state);

	if (dragging_) {
		dragging_->move(x, y);
		command_tile_owner * owners[] = {&cq_, &repo_};
		for (command_tile_owner * o : owners) {
			if (o->within(x, y)) {
				o->drag_hover_enter(x, y, dragging_.get());
			} else {
				o->drag_hover_leave(dragging_.get());
			}
		}
		request_redraw();
	}
}

void
main_screen::notify_activate()
{
	back_icon_.reset();
}

void
main_screen::notify_deactivate()
{
}

void
main_screen::timer_tick()
{
	update_current_time();
	double now = get_current_time();

	repo_.animate(now);
	cq_.animate(now);
	run_controller_.animate(now);
	bg_->animate(now);

	request_redraw();
}

void
main_screen::handle_level_win()
{
	complete_level_handler_();
}

void
main_screen::start_level(const puzzle & puz)
{
	repo_.reset();
	cq_.reset();

	puzzle_ = puz;

	double phase = 0.0;
	for (const auto & tile_kind_count : puzzle_.tiles) {
		for (std::size_t n = 0; n < tile_kind_count.second; ++n) {
			repo_.append(new_tile(tile_kind_count.first, phase));
			phase += 1.5;
		}
	}
	repo_.rearrange(true);

	run_controller_.reset(&puzzle_);

	request_redraw();
}

void
main_screen::set_exit_main_screen_handler(
	std::function<void()> exit_main_screen_handler)
{
	exit_main_screen_handler_ = std::move(exit_main_screen_handler);
}

void
main_screen::set_complete_level_handler(
	std::function<void()> complete_level_handler)
{
	complete_level_handler_ = std::move(complete_level_handler);
}
