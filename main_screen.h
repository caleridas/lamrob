#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

#include "background.h"
#include "board_view.h"
#include "puzzle.h"
#include "command_tile_repository.h"
#include "command_queue.h"
#include "run_controller.h"
#include "tiles.h"
#include "view.h"
#include "icon.h"

class main_screen final : public view {
public:
	main_screen(
		application * app,
		std::shared_ptr<background_renderer> bg);

	void
	resize(std::size_t width, std::size_t height) override;

	void
	redraw() override;

	void
	handle_button_press(
		int button,
		int x, int y,
		int button_state) override;

	void
	handle_button_release(
		int button,
		int x, int y,
		int button_state) override;

	void handle_key_press(
		int key_code,
		const keyboard_state & state,
		const char * chars) override;

	void
	handle_pointer_motion(
		int x, int y,
		int button_state) override;

	void
	timer_tick() override;

	void
	notify_activate() override;

	void
	notify_deactivate() override;

	void
	handle_level_win();

	void
	start_level(
		const puzzle & puz);

	void
	set_exit_main_screen_handler(
		std::function<void()> exit_main_screen_handler);

	void
	set_complete_level_handler(
		std::function<void()> complete_level_handler);

private:
	std::shared_ptr<background_renderer> bg_;

	puzzle puzzle_;

	board_view board_;
	command_tile_repository repo_;
	command_queue cq_;
	run_controller run_controller_;

	std::unique_ptr<command_tile_drag> dragging_;

	bool have_texture_ = false;
	GLuint texture_id_;
	tile_display_args_t tile_display_args_;

	std::function<void()> exit_main_screen_handler_;
	std::function<void()> complete_level_handler_;

	icon back_icon_;
};

#endif
