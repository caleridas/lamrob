#ifndef GRID_H
#define GRID_H

#include <cstdint>
#include <map>

struct grid_pos_t {
	int x, y;

	inline bool
	operator==(const grid_pos_t & other) const noexcept
	{
		return x == other.x && y == other.y;
	}

	inline bool
	operator!=(const grid_pos_t & other) const noexcept
	{
		return ! (*this == other);
	}
};

struct grid_vec_t {
	int dx, dy;
};

class grid_dir_t {
public:
	enum value_t : uint8_t {
		north = 0,
		west = 1,
		south = 2,
		east = 3
	};

	inline constexpr grid_dir_t() noexcept : value_(north) {}
	inline constexpr grid_dir_t(value_t value) noexcept : value_(value) {}
	inline constexpr grid_dir_t(const grid_dir_t &) noexcept = default;
	inline grid_dir_t & operator=(const grid_dir_t &) noexcept = default;
	inline grid_dir_t & operator=(value_t value) noexcept
	{
		value_ = value;
		return *this;
	}

	inline bool operator==(const grid_dir_t & other) const noexcept
	{
		return value_ == other.value_;
	}
	inline bool operator==(value_t value) const noexcept
	{
		return value_ == value;
	}

	inline bool operator!=(const grid_dir_t & other) const noexcept
	{
		return value_ == other.value_;
	}
	inline bool operator!=(value_t value) const noexcept
	{
		return value_ != value;
	}

	inline grid_dir_t left() const noexcept
	{
		return grid_dir_t(static_cast<value_t>((static_cast<uint8_t>(value_) + 1) & 3));
	}

	inline grid_dir_t right() const noexcept
	{
		return grid_dir_t(static_cast<value_t>((static_cast<uint8_t>(value_) - 1) & 3));
	}

	inline const grid_vec_t & vec() const noexcept
	{
		return vecs[static_cast<uint8_t>(value_)];
	}

	inline double angle() const noexcept
	{
		return 90. * static_cast<uint8_t>(value_);
	}

	inline double delta_angle(const grid_dir_t & to) const noexcept
	{
		uint8_t d = (static_cast<uint8_t>(to.value_) - static_cast<uint8_t>(value_)) & 3;
		return d == 3 ? -90. : d * 90.;
	}

private:
	value_t value_;

	static const grid_vec_t vecs[4];
};

struct grid_coord_t {
	grid_pos_t pos;
	grid_dir_t dir;
};

template<typename T>
class grid_tpl {
public:
	using repr_type = std::map<int, std::map<int, T>>;

	inline const T &
	operator()(int x, int y) const noexcept
	{
		return repr_.find(x)->second.find(y)->second;
	}

	inline const T *
	get(int x, int y) const noexcept
	{
		auto i = repr_.find(x);
		if (i != repr_.end()) {
			auto j = i->second.find(y);
			if (j != i->second.end()) {
				return &j->second;
			}
		}
		return nullptr;
	}

	inline T &
	operator()(int x, int y)
	{
		return repr_[x][y];
	}

	void
	erase(int x, int y)
	{
		repr_[x].erase(y);
	}

	template<typename F>
	inline void
	iterate(F f) const
	{
		for (auto i = repr_.begin(); i != repr_.end(); ++i) {
			for (auto j = i->second.begin(); j != i->second.end(); ++j) {
				f(i->first, j->first, j->second);
			}
		}
	}

	template<typename F>
	inline void
	reverse_iterate(F f) const
	{
		for (auto i = repr_.rbegin(); i != repr_.rend(); ++i) {
			for (auto j = i->second.rbegin(); j != i->second.rend(); ++j) {
				f(i->first, j->first, j->second);
			}
		}
	}

	inline void
	clear()
	{
		repr_.clear();
	}

private:
	repr_type repr_;
};

#endif
