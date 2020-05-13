#ifndef BACKGROUND_RENDERER_H
#define BACKGROUND_RENDERER_H

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

class background_renderer {
public:
	background_renderer();

	void
	draw(std::size_t width, std::size_t height);

	void
	animate(double now);

private:
	struct texsrc {
		double x0, y0, nx0, ny0;
		double x1, y1, nx1, ny1;
	};

	void
	init_phase();

	void
	make_quad(const texsrc & ts, double t, std::size_t width, std::size_t height);

	std::size_t phase_;
	std::size_t time_;
	double last_state_time_;

	texsrc pri, sec, tert;

	std::mt19937 rng;
};

#endif
