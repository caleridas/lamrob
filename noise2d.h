#ifndef NOISE2D_H
#define NOISE2D_H

#include <cmath>
#include <vector>

class noise2d {
public:
	noise2d(std::size_t log2resolution, std::size_t seed, std::vector<double> octave_scales);
	noise2d(std::size_t log2resolution, std::size_t seed);

	inline double grid_sample(std::size_t x, std::size_t y) const;

	inline double sample(double x, double y) const;

private:
	std::size_t resolution_;
	std::vector<double> samples_;
	std::vector<double> octave_scales_;
};

inline double noise2d::grid_sample(std::size_t x, std::size_t y) const
{
	x = x & (resolution_ - 1);
	y = y & (resolution_ - 1);
	return samples_[x + y * resolution_];
}

inline double noise2d::sample(double x, double y) const
{
	x = x * resolution_;
	y = y * resolution_;
	double xn = std::floor(x);
	double xf = x - xn;
	double yn = std::floor(y);
	double yf = y - yn;

	double s00 = grid_sample(static_cast<std::size_t>(xn), static_cast<std::size_t>(yn));
	double s10 = grid_sample(static_cast<std::size_t>(xn + 1), static_cast<std::size_t>(yn));
	double s01 = grid_sample(static_cast<std::size_t>(xn), static_cast<std::size_t>(yn + 1));
	double s11 = grid_sample(static_cast<std::size_t>(xn + 1), static_cast<std::size_t>(yn + 1));

	return (s00 * (1 - xf) + s10 * xf) * (1 - yf) + (s01 * (1 - xf) + s11 * xf) * yf;
}



std::vector<double> make_2d_noise(std::size_t log2dim, std::size_t seed);

#endif
