#ifndef START_SCREEN_H
#define START_SCREEN_H

#include "background.h"
#include "board_view.h"
#include "view.h"
#include "icon.h"

class start_screen final : public view {
public:
	start_screen(
		application * app,
		std::shared_ptr<background_renderer> bg);

	void
	resize(std::size_t width, std::size_t height) override;

	void
	redraw() override;

	void
	timer_tick() override;

	void
	notify_activate() override;

	void
	notify_deactivate() override;

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

	void
	handle_pointer_motion(
		int x, int y,
		int button_state) override;

	void
	handle_key_press(
		int key_code,
		const keyboard_state & state,
		const char * chars);

	void
	set_start_level_handler(
		std::function<void(std::size_t)> start_level_handler);

	void
	set_unlocked_levels(std::size_t unlocked_levels);

private:
	enum class highlight_t {
		none,
		hover,
		pressed
	};

	std::pair<bool, std::size_t>
	get_index(double x, double y) const;

	std::shared_ptr<background_renderer> bg_;
	std::vector<std::unique_ptr<board_view>> puzzles_;
	std::size_t unlocked_levels_;

	std::size_t num_columns_;
	std::size_t col_width_;
	std::size_t col_offset_;
	std::size_t row_offset_;
	std::size_t row_height_;

	highlight_t highlight_;
	std::size_t highlight_index_;

	std::function<void(std::size_t)> start_level_handler_;

	icon exit_icon_;
};

#endif
