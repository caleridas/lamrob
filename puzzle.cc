#include "puzzle.h"

#include <sstream>

static const char puzzle_data[] = R"(
   X
   #
>###
T left 1
T fwd1 5
-------------
>###
   #
   X
T right 1
T fwd2 1
T fwd3 1
-------------
>#####X
T fwd3 1
T rep2 1
-------------
   X
   #
   #
>###
T fwd1 2
T rep3 2
T left 1
-------------
>####
    #
    X
T fwd1 1
T fwd2 1
T rep2 2
T right 1
-------------
^##
  #
X##
T fwd2 1
T right 1
T rep3 1
-------------
>#####X
T fwd1 1
T rep2 1
T rep3 1
-------------
O#O#X
# O
>##
T fwd2 3
T left 1
T right 1
-------------
>##
  O
  ##X
  #
  O
T fwd2 1
T fwd3 2
T left 1
T right 1
-------------
>
#
###
  #
  ##X
T right 1
T fwd2 2
T left 1
T rep2 1
-------------
< #####
# #   #
# # X #
# # # #
# ### #
#     #
#######

T rep2 2
T rep3 1
T fwd3 2
T fwd2 2
T fwd1 2
T left 3
-------------
>##aX
  O
  A
T fwd1 2
T fwd2 2
T right 2
T left 2
-------------
X#O#O#<
    # #
    ###

T rep3 1
T left 3
T right 1
T fwd2 3
-------------
X#aOAO<
    # #
    ###
T fwd1 2
T fwd2 6
T fwd3 2
T right 5
T left 5
-------------
  v
  #
AO#bX
  O
  a
  O
  B

T fwd1 10
T fwd2 10
T fwd3 10
T right 10
T left 10
-------------
  A
  O
  b
Xa<cOB
  #
  O
  C
T rep3 1
T left 3
T fwd2 3
T right 1
-------------
   ###
   #B#
Xba>OA
   ###

T fwd1 10
T fwd2 10
T fwd3 10
T right 10
T left 10
T rep2 3
T rep3 3
-------------
###B
###O
vO#A
a
b
X
T left 8
T fwd1 5
T fwd2 5
T fwd3 5
T right 8
-------------
    v
    #
X####
T fwd2 1
T right 1
T conditional 1
T rep4 1
-------------
###X
1 #
>1#
T fwd1 1
T fwd2 4
T left 2
T right 2
T conditional 1
-------------
)";

/*
 * symbol meanings:
 *   Lines starting with "-": Begin next level
 *   Lines starting with "T": (ex: "T fwd1 4") designate tile counts
 *      possible tiles: fwd1 fwd2 fwd3 left right conditional rep2,,rep5
 *   All other lines describe grid layout, using the following chars:
 *
 *   < > ^ v : robot starting position and orientation
 *   space: empty
 *   # : normal floor
 *   O : obstacle
 *   X : goal
 *   0 1 2 3 4 5 6 7 8 9 : alternative tiles (must come in pairs)
 *   A B C D E F G H : trigger floor tiles
 *   a b c d e f g h : triggerable trap doors
 */

/* Advances first pointer to begin of next line.
 * Returns iterator to end of current line (excluding \n) */
std::string::const_iterator
get_next_line(std::string::const_iterator & i, const std::string::const_iterator limit)
{
	while (i != limit) {
		if (*i == '\n') {
			std::string::const_iterator result = i;
			++i;
			return result;
		} else {
			++i;
		}
	}
	return i;
}

/* Advances first iterator to begin of next token (if any).
 * Returns iterators delimiting next token (separated from
 * other tokens by whitespace).
 * Returns empty string if no further token.
 *
 * Precondition: i does not point to whitespace.
 */
std::string
get_next_token(std::string::const_iterator & i, const std::string::const_iterator limit)
{
	std::string::const_iterator token_begin = i;
	while (i != limit && *i != ' ') {
		++i;
	}
	std::string::const_iterator token_end = i;
	while (i != limit && *i == ' ') {
		++i;
	}

	return {token_begin, token_end};
}

void
parse_tile_line(puzzle & puz, std::string::const_iterator i, const std::string::const_iterator & limit)
{
	/* skip leading "T " from line */
	get_next_token(i, limit);
	std::string tile = get_next_token(i, limit);
	std::size_t count;
	std::istringstream(get_next_token(i, limit)) >> count;

	static const std::unordered_map<std::string, command_tile::kind_t> tile_name_map = {
		{"left", command_tile::kind_t::left},
		{"right", command_tile::kind_t::right},
		{"fwd1", command_tile::kind_t::fwd1},
		{"fwd2", command_tile::kind_t::fwd2},
		{"fwd3", command_tile::kind_t::fwd3},
		{"conditional", command_tile::kind_t::conditional},
		{"rep0", command_tile::kind_t::rep0},
		{"rep1", command_tile::kind_t::rep1},
		{"rep2", command_tile::kind_t::rep2},
		{"rep3", command_tile::kind_t::rep3},
		{"rep4", command_tile::kind_t::rep4},
		{"rep5", command_tile::kind_t::rep5}
	};

	auto it = tile_name_map.find(tile);
	if (it != tile_name_map.end()) {
		puz.tiles[it->second] = count;
	}
}

void
parse_grid(
	std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>> grid,
	puzzle & puz,
	int start_x, int start_y)
{
	std::map<char, std::vector<grid_pos_t>> alternative_map;

	int y = start_y;
	for (const auto & row : grid) {
		int x = -start_x;

		for (std::string::const_iterator col = row.first; col != row.second; ++col) {
			switch (*col) {
				case '<':
				case '>':
				case '^':
				case 'v': {
					puz.start.pos.x = x;
					puz.start.pos.y = y;
					puz.grid(x, y).trigger_id = 0;
					switch (*col) {
						case '<': {
							puz.start.dir = grid_dir_t::south;
							break;
						}
						case '>': {
							puz.start.dir = grid_dir_t::north;
							break;
						}
						case '^': {
							puz.start.dir = grid_dir_t::west;
							break;
						}
						case 'v': {
							puz.start.dir = grid_dir_t::east;
							break;
						}
					}
					break;
				}
				case '#': {
					puz.grid(x, y).trigger_id = 0;
					break;
				}
				case 'O': {
					puz.grid(x, y).trigger_id = 0;
					puz.obstacles.push_back(grid_pos_t{x, y});
					break;
				}
				case 'X': {
					puz.grid(x, y).trigger_id = 0;
					puz.end.x = x;
					puz.end.y = y;
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9': {
					alternative_map[*col].push_back(grid_pos_t{x, y});
					break;
				}
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
				case 'G':
				case 'H': {
					puz.grid(x, y).trigger_id = 1 + (*col - 'A');
					break;
				}
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
				case 'g':
				case 'h': {
					puz.grid(x, y).trigger_id = -1 - (*col - 'a');
					break;
				}
				case ' ':
				default: {
					break;
				}
			}
			++x;
		}
		--y;
	}

	for (const auto& alt : alternative_map) {
		if (alt.second.size() == 2) {
			puz.alternative_tiles.emplace_back(alt.second[0], alt.second[1]);
		}
	}
}

int
find_robot(std::string::const_iterator i, const std::string::const_iterator & limit) {
	int x = 0;
	while (i != limit) {
		if (*i == '<' || *i == '>' || *i == '^' || *i == 'v') {
			return x;
		}
		++i;
	}
	return -1;
}

void
aggregate_weights(
	std::string::const_iterator i, const std::string::const_iterator & limit,
	int y, std::size_t & ntiles, int & weight_x, int & weight_y)
{
	int x = 0;
	while (i != limit) {
		if (*i != ' ') {
			++ntiles;
			weight_x += x;
			weight_y += y;
		}
		++x;
		++i;
	}
}

std::vector<puzzle>
parse_puzzles(const std::string & data)
{
	std::vector<puzzle> result;

	int y = 0;

	std::size_t ntiles = 0;
	int weight_x = 0;
	int weight_y = 0;
	result.push_back(puzzle());
	puzzle * current = &result.front();
	std::string::const_iterator i = data.begin();
	std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>> grid;

	do {
		std::string::const_iterator lbegin = i;
		std::string::const_iterator lend = get_next_line(i, data.end());
		if (lbegin != lend) {
			char c = *lbegin;
			if (c == 'T') {
				parse_tile_line(*current, lbegin, lend);
			} else if (c == '-') {
				parse_grid(std::move(grid), *current, weight_x / ntiles, weight_y / ntiles);
				y = 0;
				ntiles = 0;
				weight_x = 0;
				weight_y = 0;
				result.push_back(puzzle());
				current = &result.back();
			} else {
				grid.push_back({lbegin, lend});
				aggregate_weights(lbegin, lend, y, ntiles, weight_x, weight_y);
				++y;
			}
		}
	} while (i != data.end());

	result.erase(std::prev(result.end()));
	return result;
}

const std::vector<puzzle> & get_puzzles()
{
	static const std::vector<puzzle> puzzles = parse_puzzles(puzzle_data);

	return puzzles;
}
