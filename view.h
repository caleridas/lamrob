#ifndef VIEW_H
#define VIEW_H

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include <chrono>
#include <cstdlib>
#include <unordered_set>

class application;
class keyboard_state;
class view;

class view {
public:
	view(
		application * app,
		std::chrono::steady_clock::duration tick_duration = std::chrono::steady_clock::duration::max());

	virtual
	~view();

	void
	request_redraw();

	void
	destroy();

	virtual void
	redraw();

	virtual void
	resize(std::size_t width, std::size_t height);

	virtual void
	handle_button_press(
		int button,
		int x, int y,
		int button_state);

	virtual void
	handle_button_release(
		int button,
		int x, int y,
		int button_state);

	virtual void
	handle_pointer_motion(
		int x, int y,
		int button_state);

	virtual void
	handle_key_press(
		int key_code,
		const keyboard_state & state,
		const char * chars);

	virtual void
	handle_key_release(
		int key_code,
		const keyboard_state & state);

	virtual void
	timer_tick();

protected:
	application * app_;
	bool active_;

	virtual void notify_activate();

	virtual void notify_deactivate();

	inline std::size_t width() const noexcept;
	inline std::size_t height() const noexcept;

private:
	std::chrono::steady_clock::duration tick_duration_;

	friend class application;
};

class keyboard_state {
public:
	const std::unordered_set<int> & pressed_keys() const noexcept
	{
		return pressed_keys_;
	}

private:
	std::unordered_set<int> pressed_keys_;
	friend class application;
};

class application {
public:
	application();
	~application();

	void
	set_active_view(view * active_view);

	void
	run();

	void
	quit();

	void
	request_redraw();

	void
	toggle_fullscreen();

	inline const keyboard_state &
	keyboard() const { return keyboard_state_; }

private:
	void
	intern_atoms();

	void
	handle_redraw();

	void
	handle_client_message(const ::XClientMessageEvent & ev);

	void
	handle_event(const ::XEvent & ev);

	void
	handle_button_press(const ::XEvent & ev);

	void
	handle_button_release(const ::XEvent & ev);

	void
	handle_pointer_motion(const ::XEvent & ev);

	void
	handle_key_press(const ::XEvent & ev);

	void
	handle_key_release(const ::XEvent & ev);

	void
	handle_focus_in(const ::XFocusChangeEvent & ev);

	void
	handle_focus_out(const ::XFocusChangeEvent & ev);

	void
	handle_glx_buffer_swap_complete(const ::XEvent & ev);

	::Display * display_;
	int glx_error_base_;
	int glx_event_base_;
	::XIM input_method_;
	::XIC input_context_;
	::Window root_;
	::Window window_;
	::GLXWindow glxwindow_;
	::GLXContext glxctx_;
	view * active_view_ = nullptr;
	bool requested_quit_ = false;
	bool requested_redraw_ = false;
	bool visible_ = false;
	bool fullscreen_ = false;
	bool buffer_swap_pending_ = false;
	std::size_t width_ = 0;
	std::size_t height_ = 0;

	std::chrono::steady_clock::time_point timer_due_ = std::chrono::steady_clock::time_point::max();
	int x11_fd_;

	/* FIXME: may not need this
	int wake_write_fd_;
	int wake_read_fd_;
	*/

	struct {
		Atom wm_protocols;
		Atom wm_delete_window;
		Atom wm_take_focus;
		Atom net_wm_state;
		Atom net_wm_state_fullscreen;
	} atoms_;

	keyboard_state keyboard_state_;

	friend class view;
};

inline std::size_t
view::width() const noexcept
{
	return app_->width_;
}

inline std::size_t
view::height() const noexcept
{
	return app_->height_;
}

#endif
