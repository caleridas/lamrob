#ifndef ICON_H
#define ICON_H

#include <functional>

#include "view.h"

class icon {
public:
	icon(view * owner, int texture_id, std::function<void()> on_click);

	void
	redraw();

	void
	reset();

	void
	configure(double x1, double y1, double x2, double y2);

	bool
	handle_button_press(
		int button,
		int x, int y,
		int button_state);

	bool
	handle_button_release(
		int button,
		int x, int y,
		int button_state);

	void
	handle_pointer_motion(
		int x, int y,
		int button_state);

private:
	enum class state_t {
		none,
		hovering,
		pressed,
		pressed_move_away
	};

	void
	request_redraw();

	view * owner_;
	int texture_id_;
	std::function<void()> on_click_;
	double x1_, y1_, x2_, y2_;
	state_t state_;

	static constexpr double pad_ = 4;
};

#endif
