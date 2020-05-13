#include "view.h"

#include <X11/XKBlib.h>
#include <poll.h>

#include <iostream>

view::view(
	application * app,
	std::chrono::steady_clock::duration tick_duration)
	: app_(app), active_(false), tick_duration_(tick_duration)
{
}

view::~view()
{
	app_->set_active_view(nullptr);
}

void
view::destroy()
{
	if (app_->active_view_ == this) {
		app_->set_active_view(nullptr);
	}
	delete this;
}

void
view::request_redraw()
{
	app_->request_redraw();
}

void
view::redraw()
{
}

void
view::resize(std::size_t width, std::size_t height)
{
}

void
view::handle_button_press(
	int button,
	int x, int y,
	int button_state)
{
}

void
view::handle_button_release(
	int button,
	int x, int y,
	int button_state)
{
}

void
view::handle_pointer_motion(
	int x, int y,
	int button_state)
{
}

void
view::handle_key_press(
	int key_code,
	const keyboard_state & state,
	const char * chars)
{
}

void
view::handle_key_release(
	int key_code,
	const keyboard_state & state)
{
}

void
view::timer_tick()
{
}

void
view::notify_activate()
{
}

void
view::notify_deactivate()
{
}

application::application()
{
	display_ = ::XOpenDisplay(nullptr);
	int screen = DefaultScreen(display_);
	root_ = RootWindow(display_, screen);

	::glXQueryExtension(display_, &glx_error_base_, &glx_event_base_);

	::XSetLocaleModifiers("");

	input_method_ = ::XOpenIM(display_, nullptr, nullptr, nullptr);
	if(!input_method_){
		// fallback to internal input method
		::XSetLocaleModifiers("@im=none");
		input_method_ = ::XOpenIM(display_, nullptr, nullptr, nullptr);
	}

	intern_atoms();

	int glx_attrs[] = {
		GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
		GLX_RENDER_TYPE,    GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER,   True,
		GLX_RED_SIZE,       8,
		GLX_GREEN_SIZE,     8,
		GLX_BLUE_SIZE,      8,
		GLX_DEPTH_SIZE,     8,
		GLX_SAMPLE_BUFFERS, 1,
		GLX_SAMPLES,        2,
		None
	};

	int num_fbconfigs = 0;
	::GLXFBConfig * fbconfigs = ::glXChooseFBConfig(display_, screen, glx_attrs, &num_fbconfigs);
	::XVisualInfo * visinfo = ::glXGetVisualFromFBConfig(display_, fbconfigs[0]);

	glxctx_ = ::glXCreateNewContext(display_, fbconfigs[0], GLX_RGBA_TYPE, 0, GL_TRUE);

	::XSetWindowAttributes win_attr;
	win_attr.background_pixel = 0;
	win_attr.border_pixel = 0;
	win_attr.colormap = ::XCreateColormap(display_, root_, visinfo->visual, AllocNone);
	win_attr.event_mask =
		StructureNotifyMask | ExposureMask |
		KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask |
		PointerMotionMask | FocusChangeMask;

	width_ = 800;
	height_ = 600;

	window_ = ::XCreateWindow(
		display_, root_,
		0, 0,
		width_, height_,
		screen,
		visinfo->depth, InputOutput,
		visinfo->visual,
		CWBackPixel |CWBorderPixel | CWColormap | CWEventMask, &win_attr);

	Atom protocols[] = {atoms_.wm_delete_window};
	::XChangeProperty(display_, window_, atoms_.wm_protocols, XA_ATOM, 32,
		PropModeReplace, (unsigned char *)&protocols, 2);

	input_context_ = ::XCreateIC(input_method_,
		XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, window_,
		XNFocusWindow,  window_,
		nullptr);

	::XMapWindow(display_, window_);
	glxwindow_ = ::glXCreateWindow(display_, fbconfigs[0], window_, glx_attrs);

	// FIXME: maybe move to display loop?
	::glXMakeContextCurrent(display_, glxwindow_, glxwindow_, glxctx_);
	::glXSelectEvent(display_, glxwindow_, GLX_BUFFER_SWAP_COMPLETE_INTEL_MASK);

	::XFree(visinfo);
	::XFree(fbconfigs);

	x11_fd_ = XConnectionNumber(display_);
}

application::~application()
{
	set_active_view(nullptr);
}

void
application::set_active_view(view * active_view)
{
	if (active_view_) {
		active_view_->notify_deactivate();
		active_view_->active_ = false;
		timer_due_ = std::chrono::steady_clock::time_point::max();
	}

	active_view_ = active_view;
	if (active_view_) {
		active_view_->notify_activate();
		active_view_->active_ = true;
		if (visible_) {
			active_view_->resize(width_, height_);
			request_redraw();
		}
		if (active_view_->tick_duration_ != std::chrono::steady_clock::duration::max()) {
			timer_due_ = std::chrono::steady_clock::now();
		}
	}
}

void
application::run()
{
	while (!requested_quit_) {
		while (::XPending(display_)) {
			XEvent ev;
			::XNextEvent(display_, &ev);
			handle_event(ev);
		}

		std::chrono::steady_clock::time_point now;
		for(;;) {
			now = std::chrono::steady_clock::now();
			if (active_view_ && now >= timer_due_) {
				active_view_->timer_tick();
				timer_due_ += active_view_->tick_duration_;
			} else {
				break;
			}
		}

		if (requested_redraw_ && !buffer_swap_pending_) {
			handle_redraw();
		} else {
			struct pollfd pfd[1];
			pfd[0].fd = x11_fd_;
			pfd[0].events = POLLIN;
			int timeout = -1;
			if (active_view_ && timer_due_ != std::chrono::steady_clock::time_point::max()) {
				timeout = (timer_due_ - now) / std::chrono::milliseconds(1);
			}
			poll(pfd, 1, timeout);
		}
	}
	::XDestroyWindow(display_, window_);
	::XFlush(display_);
}

void
application::handle_event(const ::XEvent & event)
{
	switch (event.type) {
		case MapNotify: {
			visible_ = true;
			if (active_view_) {
				active_view_->resize(width_, height_);
			}
			request_redraw();
			break;
		}
		case UnmapNotify: {
			visible_ = false;
			break;
		}
		case ConfigureNotify: {
			width_ = event.xconfigure.width;
			height_ = event.xconfigure.height;
			if (active_view_) {
				active_view_->resize(width_, height_);
			}
			request_redraw();
			break;
		}
		case Expose: {
			request_redraw();
			break;
		}
		case KeyPress: {
			handle_key_press(event);
			break;
		}
		case KeyRelease: {
			handle_key_release(event);
			break;
		}
		case ClientMessage: {
			handle_client_message(event.xclient);
			break;
		}
		case ButtonPress: {
			handle_button_press(event);
			break;
		}
		case ButtonRelease: {
			handle_button_release(event);
			break;
		}
		case MotionNotify: {
			handle_pointer_motion(event);
			break;
		}
		default: {
			if (event.type == glx_event_base_ + GLX_BufferSwapComplete) {
				handle_glx_buffer_swap_complete(event);
			}
			break;
		}
	}
}

void
application::handle_client_message(const ::XClientMessageEvent & ev)
{
	if (ev.message_type == atoms_.wm_protocols) {
		if (static_cast<Atom>(ev.data.l[0]) == atoms_.wm_delete_window) {
			quit();
		}
	}
}

void
application::handle_redraw()
{
	requested_redraw_ = false;
	if (active_view_) {
		glEnable(GL_MULTISAMPLE);
		active_view_->redraw();
		glFlush();
		::glXSwapBuffers(display_, glxwindow_);
		buffer_swap_pending_ = true;
	}
}

void
application::handle_button_press(const ::XEvent & ev)
{
	const ::XButtonEvent & bev = ev.xbutton;
	if (active_view_) {
		active_view_->handle_button_press(bev.button, bev.x, bev.y, bev.state);
	}
}

void
application::handle_button_release(const ::XEvent & ev)
{
	const ::XButtonEvent & bev = ev.xbutton;
	if (active_view_) {
		active_view_->handle_button_release(bev.button, bev.x, bev.y, bev.state);
	}
}

void
application::handle_pointer_motion(const ::XEvent & ev)
{
	const ::XMotionEvent & mev = ev.xmotion;
	if (active_view_) {
		active_view_->handle_pointer_motion(mev.x, mev.y, mev.state);
	}
}

void
application::handle_key_press(const ::XEvent & ev)
{
	bool filtered = ::XFilterEvent(const_cast<::XEvent *>(&ev), None);
	keyboard_state_.pressed_keys_.insert(ev.xkey.keycode);

	if (ev.xkey.state == 8 && ev.xkey.keycode == 36) {
		toggle_fullscreen();
	}

	char chars[8] = {0};
	if (!filtered) {
		KeySym sym;
		Status status;
		int count = ::Xutf8LookupString(input_context_, const_cast<::XKeyEvent *>(&ev.xkey), chars, 8, &sym, &status);
		chars[count] = 0;
	}

	if (active_view_) {
		active_view_->handle_key_press(
			XkbKeycodeToKeysym(display_, ev.xkey.keycode, 0, (ev.xkey.state & ShiftMask) ? 1 : 0),
			keyboard_state_, chars);
	}
}

void
application::handle_key_release(const ::XEvent & ev)
{
	keyboard_state_.pressed_keys_.erase(ev.xkey.keycode);
	if (active_view_) {
		active_view_->handle_key_release(
			XkbKeycodeToKeysym(display_, ev.xkey.keycode, 0, (ev.xkey.state & ShiftMask) ? 1 : 0),
			keyboard_state_);
	}
}

void
application::handle_focus_in(const ::XFocusChangeEvent & ev)
{
	::XSetICFocus(input_context_);
}

void
application::handle_focus_out(const ::XFocusChangeEvent & ev)
{
	::XUnsetICFocus(input_context_);
}

void
application::handle_glx_buffer_swap_complete(const ::XEvent & ev)
{
	buffer_swap_pending_ = false;
}

void
application::quit()
{
	if (!requested_quit_) {
		requested_quit_ = true;
	}
}

void
application::request_redraw()
{
	if (visible_ && !requested_redraw_) {
		requested_redraw_ = true;
	}
}

void
application::toggle_fullscreen()
{
	// FIXME: support setting this ourselves in case WM incapable

	XEvent xev;
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.message_type = atoms_.net_wm_state;
	xev.xclient.window = window_;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = (fullscreen_ ? 0 : 1);
	xev.xclient.data.l[1] = atoms_.net_wm_state_fullscreen;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;

	::XSendEvent(display_, root_, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);

	fullscreen_ = !fullscreen_;
}

void
application::intern_atoms()
{
	static const char * atom_names[5] = {
		"WM_PROTOCOLS",
		"WM_DELETE_WINDOW",
		"WM_TAKE_FOCUS",
		"_NET_WM_STATE",
		"_NET_WM_STATE_FULLSCREEN"
	};
	Atom atoms[5];
	::XInternAtoms(display_, const_cast<char **>(atom_names), 5, false, atoms);

	atoms_.wm_protocols = atoms[0];
	atoms_.wm_delete_window = atoms[1];
	atoms_.wm_take_focus = atoms[2];
	atoms_.net_wm_state = atoms[3];
	atoms_.net_wm_state_fullscreen = atoms[4];
}
