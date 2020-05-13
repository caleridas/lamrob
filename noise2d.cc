#include "noise2d.h"

#include <random>

#include <iostream>

static void apply_adj(std::size_t center_x, std::size_t center_y, double adj, std::size_t dim, int scale, std::vector<double> & data)
{
	for (int x = - scale + 1; x < scale; ++x) {
		double fx = 1.0 - fabs(1.0 * x / scale);
		for (int y = - scale + 1; y < scale; ++y) {
			double fy = 1.0 - fabs(1.0 * y / scale);
			double f = fx * fy;
			data[((x + center_x) & (dim - 1)) + ((y + center_y) & (dim - 1)) * dim] += f * adj;
		}
	}
}

noise2d::noise2d(std::size_t log2resolution, std::size_t seed, std::vector<double> octave_scales)
	: resolution_(1 << log2resolution)
	, samples_(resolution_ * resolution_, 0.0)
	, octave_scales_(std::move(octave_scales))
{
	std::mt19937 rng(seed);

	const double rnd_scale = 2. / (rng.max() - rng.min());
	const double rnd_ofs = -1.;

	std::size_t index = 0;
	for (std::size_t log2scale = 1; log2scale < log2resolution; ++log2scale) {
		std::size_t scale = 1 << (log2resolution - log2scale);
		double strength = index < octave_scales_.size() ? octave_scales_[index] : (1.0 / (1 << log2scale));
		++index;

		for (std::size_t x = 0; x < resolution_; x += scale) {
			for (std::size_t y = 0; y < resolution_; y += scale) {
				double r = rng() * rnd_scale + rnd_ofs;
				double adj = r * strength;
				apply_adj(x, y, adj, resolution_, scale, samples_);
			}
		}
	}
}

noise2d::noise2d(std::size_t log2resolution, std::size_t seed)
	: noise2d(log2resolution, seed, {})
{
}

std::vector<double> make_2d_noise(std::size_t log2dim, std::size_t seed)
{
	std::mt19937 rng(seed + 10);
	const double rnd_scale = 2. / (rng.max() - rng.min());
	const double rnd_ofs = -1.;

	std::size_t dim = 1 << log2dim;
	std::vector<double> data(dim * dim, 0.0);

	std::cout << rnd_scale << " " << rnd_ofs << " " << rng.max() << "\n";
	for (std::size_t log2scale = 1; log2scale < 2; ++log2scale) {
		std::size_t scale = 1 << (log2dim - log2scale);
		double strength = 1.0 / (1 << log2scale);

		std::cout << dim << " " << scale << " " << strength << "\n";
		for (std::size_t x = 0; x < dim; x += scale) {
			for (std::size_t y = 0; y < dim; y += scale) {
				std::cout << rng() * rnd_scale + rnd_ofs << "\n";
				double r = rng() * rnd_scale + rnd_ofs;
				std::cout << x << " " << y << " " << r << "\n";
				double adj = r * strength;
				apply_adj(x, y, adj, dim, scale, data);
			}
		}
	}

	return data;
}
