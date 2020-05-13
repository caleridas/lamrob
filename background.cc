#include "background.h"

#include <GL/gl.h>
#include <stdlib.h>

#include <cmath>
#include <memory>

#include "clock.h"
#include "noise2d.h"

static bool bg_tex_generated = false;
static GLuint bg_tex_id = 0;

void
make_bg_texture()
{
	std::unique_ptr<uint8_t[]> data(new uint8_t[1024 * 1024 * 4]);

	noise2d n(10, 70, {0, 0, 0, 1, 0.5, 0.25, 0.125, 0.0625, 0.03125});

	for (std::size_t y = 0; y < 1024; ++y) {
		for (std::size_t x = 0; x < 1024; ++x) {
			double s = n.grid_sample(x, y);
			s = (s + 1) / 2 * 255;
			if (s < 0) { s = 0; }
			if (s > 255) { s = 255; }
			uint8_t c = s;
			data[(x + y * 1024) * 4 + 0] = c;
			data[(x + y * 1024) * 4 + 1] = c;
			data[(x + y * 1024) * 4 + 2] = c;
			data[(x + y * 1024) * 4 + 3] = 255;
		}
	}

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &bg_tex_id);
	glBindTexture(GL_TEXTURE_2D, bg_tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA,
			1024, 1024, 0,
			GL_RGBA, GL_UNSIGNED_BYTE,
			data.get());
}

void
bind_bg_texture()
{
	if (!bg_tex_generated) {
		make_bg_texture();
		bg_tex_generated = true;
	} else {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, bg_tex_id);
	}
}

background_renderer::background_renderer()
	: last_state_time_(get_current_time())
{
	init_phase();
	init_phase();
	init_phase();
}

static constexpr double animation_phase_duration = 2;
static constexpr double color_cycle_duration = 30;

void
background_renderer::draw(std::size_t width, std::size_t height)
{
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0., width, height, -0., -1., +1.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	bind_bg_texture();

	double t = (get_current_time() - last_state_time_) / animation_phase_duration;

	double p1 = .3 * (1 - t);
	double p2 = .3;
	double p3 = .3 * t;

	double a = get_current_time() / color_cycle_duration * 2 * M_PI;

	double red = 0.1 * (1 + sin(a)) + 0.1;
	double green = 0.1 * (1 + sin(a + 2 * M_PI / 3)) + 0.1;
	double blue = 0.15 * (1 + sin(a + 4 * M_PI / 3)) + 0.1;

	glColor4f(red * p1, green * p1, blue * p1, 1);
	make_quad(pri, 0.6 + t * .3, width, height);

	glColor4f(red * p2, green * p2, blue * p2, 1);
	make_quad(sec, 0.3 + t * .3, width, height);

	glColor4f(red * p3, green * p3, blue * p3, 1);
	make_quad(tert, 0.0 + t * .3, width, height);
}

void
background_renderer::make_quad(const texsrc & ts, double t, std::size_t width, std::size_t height)
{
	double x = ts.x1 * t + ts.x0 * (1 - t);
	double y = ts.y1 * t + ts.y0 * (1 - t);
	double nx = ts.nx1 * t + ts.nx0 * (1 - t);
	double ny = ts.ny1 * t + ts.ny0 * (1 - t);

	std::size_t w = std::max(width, height);

	glBegin(GL_QUADS);
	glTexCoord2f(x, y);
	glVertex3f(0, 0, 0);
	glTexCoord2f(x + nx, y + ny);
	glVertex3f(w, 0, 0);
	glTexCoord2f(x + nx - ny, y + ny + nx);
	glVertex3f(w, w, 0);
	glTexCoord2f(x - ny, y + nx);
	glVertex3f(0, w, 0);
	glEnd();

}



void
background_renderer::animate(double now)
{
	double d = now - last_state_time_;
	if (d >= animation_phase_duration) {
		init_phase();
		last_state_time_ = floor(d / animation_phase_duration) * animation_phase_duration + last_state_time_;
	}
}

void
background_renderer::init_phase()
{
	pri = sec;
	sec = tert;

	double alpha = (rng() - rng.min()) * 2. * M_PI / (rng.max() - rng.min());
	double alpha_d = ((rng() - rng.min()) * 2. / (rng.max() - rng.min()) - 1.) * 0.2;

	double s1 = (rng() - rng.min()) * 0.15 / (rng.max() - rng.min()) + .5;
	double s2 = (rng() - rng.min()) * 0.15 / (rng.max() - rng.min()) + .5;

	tert.nx0 = sin(alpha) * s1;
	tert.ny0 = cos(alpha) * s1;
	tert.nx1 = sin(alpha + alpha_d) * s2;
	tert.ny1 = cos(alpha + alpha_d) * s2;

	tert.x0 = (rng() - rng.min()) * 1.0 / (rng.max() - rng.min());
	tert.y0 = (rng() - rng.min()) * 1.0 / (rng.max() - rng.min());

	tert.x1 = tert.x0 + tert.nx0 * .5 - tert.nx1 * .5 + (rng() - rng.min()) * 0.04 / (rng.max() - rng.min());
	tert.y1 = tert.y0 + tert.ny0 * .5 - tert.ny1 * .5 + (rng() - rng.min()) * 0.04 / (rng.max() - rng.min());
}
