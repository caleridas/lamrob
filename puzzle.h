#ifndef PUZZLE_H
#define PUZZLE_H

#include "grid.h"
#include "tiles.h"

struct floor_tile_t {
	/*
	 * zero: normal floor (no trigger)
	 * positive: floor tile that can trigger a trap door
	 * negative: trapdoor corresponding to trigger
	 */
	int trigger_id;
};

struct puzzle {
	using grid_t = grid_tpl<floor_tile_t>;

	grid_t grid;

	struct {
		grid_pos_t pos;
		grid_dir_t dir;
	} start;

	grid_pos_t end;

	std::map<command_tile::kind_t, std::size_t> tiles;

	std::vector<grid_pos_t> obstacles;

	std::vector<std::pair<grid_pos_t, grid_pos_t>> alternative_tiles;
};

const std::vector<puzzle> & get_puzzles(); // XXX use this

#endif
