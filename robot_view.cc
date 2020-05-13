#include "robot_view.h"

#include <GL/gl.h>
#include <math.h>

#include "texgen.h"

static void
draw_wheel(
	double x, double y, double z,
	double r, double dy, double rot)
{
	glBegin(GL_TRIANGLES);
	static constexpr std::size_t nsegments = 12;
	for (std::size_t n = 0; n < nsegments; ++n) {
		double a1 = n * 2 * M_PI / nsegments + rot;
		double a2 = (n + 1) * 2 * M_PI / nsegments + rot;
		double x1 = x + r * sin(a1);
		double z1 = z + r * cos(a1);
		double x2 = x + r * sin(a2);
		double z2 = z + r * cos(a2);

		if ((n >> 1) & 1) {
			glColor4f(.5, 1, 1, 1);
		} else {
			glColor4f(.2, .2, .2, 1);
		}
		glNormal3f(0, 1, 0);
		glVertex3f(x, y + dy, z);
		glNormal3f(0, 1, 0);
		glVertex3f(x2, y + dy, z2);
		glNormal3f(0, 1, 0);
		glVertex3f(x1, y + dy, z1);

		glNormal3f(0, -1, 0);
		glVertex3f(x, y - dy, z);
		glNormal3f(0, -1, 0);
		glVertex3f(x1, y - dy, z1);
		glNormal3f(0, -1, 0);
		glVertex3f(x2, y - dy, z2);
	}
	glEnd();

	glBegin(GL_QUADS);
	for (std::size_t n = 0; n < nsegments; ++n) {
		double a1 = n * 2 * M_PI / nsegments + rot;
		double a2 = (n + 1) * 2 * M_PI / nsegments + rot;
		double s1 = sin(a1);
		double c1 = cos(a1);
		double s2 = sin(a2);
		double c2 = cos(a2);
		double x1 = x + r * s1;
		double z1 = z + r * c1;
		double x2 = x + r * s2;
		double z2 = z + r * c2;

		glColor4f(.3, .3, .3, 1);
		glNormal3f(s1, 0, c1);
		glVertex3f(x1, y - dy, z1);
		glNormal3f(s1, 0, c1);
		glVertex3f(x1, y + dy, z1);
		glNormal3f(s2, 0, c2);
		glVertex3f(x2, y + dy, z2);
		glNormal3f(s2, 0, c2);
		glVertex3f(x2, y - dy, z2);
	}
	glEnd();
}

static constexpr double h1 = .05;
static constexpr double h2 = .15;
static constexpr double l1 = .32;
static constexpr double l2 = .20;
static constexpr double h3 = .50;

void
draw_robot(double wheel_rotation)
{
	glColor4f(.7, .6, .4, 1.);
	/* platform */
	texture_generator::make_box(-l1, -l2, h1, +l1, +l2, h2);
	/* pusher */
	texture_generator::make_box(+.45, -.3, .0, +.5, +.3, .2);
	/* connection to pusher */
	texture_generator::make_box(+l2, -.05, .05, +.46, +.05, .15);
	/* pole to camera */
	texture_generator::make_box(-.04, -.04, h2-0.001, +.04, +.04, h3);

	/* "eye" */
	texture_generator::make_solid_tex_coord();

	glBegin(GL_TRIANGLES);
	constexpr double eyeball_radius = .15;
	constexpr double eye_radius = .18;

	for (std::size_t lat = 0; lat < 8; ++lat) {
		double dx1 = cos(lat * M_PI / 8);
		double dr1 = sin(lat * M_PI / 8);
		double dx2 = cos((lat + 1) * M_PI / 8);
		double dr2 = sin((lat + 1) * M_PI / 8);
		double radius = lat > 2 ? eye_radius : eyeball_radius;
		for (std::size_t lon = 0; lon < 8; ++ lon) {
			double dz1 = sin(lon * M_PI / 4);
			double dy1 = cos(lon * M_PI / 4);
			double dz2 = sin((lon + 1) * M_PI / 4);
			double dy2 = cos((lon + 1) * M_PI / 4);

			if (lat == 0) {
				glColor4f(0, 0, 0, 1);
			} else if (lat <= 2) {
				glColor4f(1, 1, 1, 1);
			} else {
				glColor4f(.6, .5, .4, 1.);
			}
			if (lat != 7) {
				glNormal3f(dx1, dy1*dr1, dz1*dr1);
				glVertex3f(dx1*radius, dy1*dr1*radius, dz1*dr1*radius + h3);

				glNormal3f(dx2, dy2*dr2, dz2*dr2);
				glVertex3f(dx2*radius, dy2*dr2*radius, dz2*dr2*radius + h3);

				glNormal3f(dx2, dy1*dr2, dz1*dr2);
				glVertex3f(dx2*radius, dy1*dr2*radius, dz1*dr2*radius + h3);

			}
			if (lat == 3) {
				glNormal3f(1, 0, 0);
				glVertex3f(dx1*eyeball_radius, dy1*dr1*eyeball_radius, dz1*dr1*eyeball_radius + h3);
				glVertex3f(dx1*eyeball_radius, dy2*dr1*eyeball_radius, dz2*dr1*eyeball_radius + h3);
				glVertex3f(dx1*eye_radius, dy2*dr1*eye_radius, dz2*dr1*eye_radius + h3);

				glVertex3f(dx1*eyeball_radius, dy1*dr1*eyeball_radius, dz1*dr1*eyeball_radius + h3);
				glVertex3f(dx1*eye_radius, dy2*dr1*eye_radius, dz2*dr1*eye_radius + h3);
				glVertex3f(dx1*eye_radius, dy1*dr1*eye_radius, dz1*dr1*eye_radius + h3);
			}
			if (lat != 0) {
				glNormal3f(dx1, dy1*dr1, dz1*dr1);
				glVertex3f(dx1*radius, dy1*dr1*radius, dz1*dr1*radius + h3);

				glNormal3f(dx1, dy2*dr1, dz2*dr1);
				glVertex3f(dx1*radius, dy2*dr1*radius, dz2*dr1*radius + h3);

				glNormal3f(dx2, dy2*dr2, dz2*dr2);
				glVertex3f(dx2*radius, dy2*dr2*radius, dz2*dr2*radius + h3);
			}
		}
	}
	glEnd();

	double r = 1/ M_PI / 4;
	draw_wheel(0.29, 0.25, r, r, 0.05, wheel_rotation);
	draw_wheel(-0.29, 0.25, r, r, 0.05, wheel_rotation);

	draw_wheel(0.29, -0.25, r, r, 0.05, wheel_rotation);
	draw_wheel(-0.29, -0.25, r, r, 0.05, wheel_rotation);
}

void
draw_robot_beam(double length)
{
	glDisable(GL_LIGHTING);
	constexpr double thickness = .02;
	constexpr double dx = .65;
	constexpr double dz = -h3;
	glColor4f(1, 0, 0, 1);
	texture_generator::make_tex_quad(texid_solid,
		+.15, -thickness, h3 + thickness,
		+.15, +thickness, h3 + thickness,
		+.15 + length * dx, +thickness, h3 + thickness + length * dz,
		+.15 + length * dx, -thickness, h3 + thickness + length * dz,
		-dz, 0, dx);

	texture_generator::make_tex_quad(texid_solid,
		+.15, -thickness, h3 + thickness,
		+.15 + length * dx, -thickness, h3 + thickness + length * dz,
		+.15 + length * dx, -thickness, h3 - thickness + length * dz,
		+.15, -thickness, h3 - thickness,
		0, -1, 0);

	texture_generator::make_tex_quad(texid_solid,
		+.15, +thickness, h3 + thickness,
		+.15, +thickness, h3 - thickness,
		+.15 + length * dx, +thickness, h3 - thickness + length * dz,
		+.15 + length * dx, +thickness, h3 + thickness + length * dz,
		0, -1, 0);

	texture_generator::make_tex_quad(texid_solid,
		+.15, -thickness, h3 - thickness,
		+.15 + length * dx, -thickness, h3 - thickness + length * dz,
		+.15 + length * dx, +thickness, h3 - thickness + length * dz,
		+.15, +thickness, h3 - thickness,
		dz, 0, -dx);
	glEnable(GL_LIGHTING);
}
