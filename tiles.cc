#include "tiles.h"

#include <math.h>
#include <GL/gl.h>

#include <chrono>

#include "texgen.h"

/*******************************************************************************
 * elastic_object */

void
elastic_object::animate(double now)
{
	if (fabs(tx_ - x_) < 4. && fabs(ty_ - y_) < 4.) {
		vx_ = 0.;
		vy_ = 0.;
		x_ = tx_;
		y_ = ty_;
	}

	x_ += vx_;
	y_ += vy_;
}

void
elastic_object::move_to(double x, double y)
{
	x_ = x;
	y_ = y;
	tx_ = x;
	ty_ = y;
	vx_ = 0.;
	vy_ = 0.;
}

void
elastic_object::accelerate_to(double x, double y)
{
	tx_ = x;
	ty_ = y;
	vx_ = (tx_ - x_) / 10.;
	vy_ = (ty_ - y_) / 10.;
}

/*******************************************************************************
 * command_tile */

command_tile::command_tile(kind_t kind, double phase)
	: kind_(kind), phase_(phase)
{
	replenish();
}

void
command_tile::draw(double global_phase, const tile_display_args_t & display_args) const
{
	double w = .5 * display_args.tile_size;
	glEnable(GL_TEXTURE_2D);

	glColor4f(1., 1., 1., 1.);
	texture_generator::make_tex_quad2d(
		0,
		x() - w, y() - w,
		x() + w, y() - w,
		x() + w, y() + w,
		x() - w, y() + w);

	int tex_index = get_tex_index();

	gl_main_color(global_phase);
	texture_generator::make_tex_quad2d(
		tex_index,
		x() - w, y() - w,
		x() + w, y() - w,
		x() + w, y() + w,
		x() - w, y() + w);

	if (gl_glow_color(global_phase)) {
		texture_generator::make_tex_quad2d(
			texture_generator::get_blur_texture(tex_index),
			x() - w, y() - w,
			x() + w, y() - w,
			x() + w, y() + w,
			x() - w, y() + w);
	}
}

void
command_tile::replenish()
{
	set_state(state_t::normal);
	int count = static_cast<int>(kind()) - static_cast<int>(kind_t::rep0);
	repetitions_left_ = std::max(0, count);
}

void
command_tile::exhaust()
{
	set_state(state_t::depleted);
	int count = static_cast<int>(kind()) - static_cast<int>(kind_t::rep0);
	repetitions_left_ = std::max(0, count);
}


int
command_tile::num_repetitions() const noexcept
{
	if (is_repeat()) {
		return static_cast<int>(kind()) - static_cast<int>(kind_t::rep0);
	} else {
		return 0;
	}
}

int
command_tile::get_tex_index() const noexcept
{
	switch (kind()) {
		case kind_t::left: {
			return texid_left;
		}
		case kind_t::right: {
			return texid_right;
		}
		case kind_t::fwd1: {
			return texid_fwd1;
		}
		case kind_t::fwd2: {
			return texid_fwd2;
		}
		case kind_t::fwd3: {
			return texid_fwd3;
		}
		case kind_t::conditional: {
			return texid_conditional;
		}
		default : {
			/* repetitions */
			return texid_rep0 + repetitions_left_;
		}
	}
}

void
command_tile::gl_main_color(double global_phase) const
{
	double intensity = get_intensity(global_phase);
	switch (state_) {
		case state_t::normal: {
			glColor4f(0.2, .52 + 0.16 * intensity, 0., 1.);
			break;
		}
		case state_t::flashing: {
			glColor4f(0.6 + 0.2 * intensity, 0.6 + 0.2 * intensity, 0.2 * intensity, 1.);
			break;
		}
		case state_t::depleted: {
			glColor4f(.2, .2, .2, 1.);
			break;
		}
	}
}

bool
command_tile::gl_glow_color(double global_phase) const
{
	double intensity = get_intensity(global_phase);
	switch (state_) {
		case state_t::normal: {
			glColor4f(0.2, .8, 0., 0.64 * intensity);
			return true;
		}
		case state_t::flashing: {
			glColor4f(1., 1., 0., 0.8 * intensity);
			return true;
		}
		default:
		case state_t::depleted: {
			glColor4f(0, 0, 0, 0);
			return false;
		}
	}
}

double
command_tile::get_intensity(double global_phase) const
{
	switch (state_) {
		case state_t::normal: {
			return .5 + .5 * sin(phase_ + global_phase);
		}
		case state_t::flashing: {
			return .5 + .5 * sin(phase_ + 4 * global_phase);
		}
		case state_t::depleted:
		default: {
			return 1.;
		}
	}
}

/*******************************************************************************
 * command_sequence_path */

std::size_t
command_sequence_path::depth() const noexcept
{
	std::size_t result = 1;
	const command_sequence_path * current = this;
	while (current->branch) {
		current = &current->branch->seq;
		++result;
	}
	return result;
}

void
command_sequence_path::advance()
{
	command_sequence_path * leaf = this;
	while (leaf->branch) {
		leaf = &leaf->branch->seq;
	}
	++ leaf->index;
}

void
command_sequence_path::up()
{
	command_sequence_path * parent = nullptr;
	command_sequence_path * current = this;
	while (current->branch) {
		parent = current;
		current = &current->branch->seq;
	}
	if (parent) {
		parent->branch.reset();
	}
}

void
command_sequence_path::down(std::size_t branch_index)
{
	command_sequence_path * leaf = this;
	while (leaf->branch) {
		leaf = &leaf->branch->seq;
	}
	leaf->branch.reset(new command_branch_path(branch_index, command_sequence_path(0)));
}

/*******************************************************************************
 * command_sequence */

command_sequence & command_sequence::operator=(const command_sequence & other)
{
	elements.clear();
	for (const auto & cpt : other) {
		append(std::unique_ptr<command_point>(new command_point(*cpt)));
	}

	return *this;
}

command_sequence::command_sequence(const command_sequence & other)
{
	for (const auto & cpt : other) {
		append(std::unique_ptr<command_point>(new command_point(*cpt)));
	}
}

void
command_sequence::draw_flows(double global_phase, const tile_display_args_t & display_args) const
{
	bool first = true;
	std::pair<double, double> last;
	for (const auto & cpt: *this) {
		if (!first) {
			std::pair<double, double> current = cpt->get_inflow_point();
			draw_flow(
				last.first, last.second,
				current.first, current.second,
				display_args.flow_scale, display_args.flow_width,
				global_phase);
		}
		last = cpt->get_outflow_point();
		first = false;
		cpt->draw_flows(global_phase, display_args);
	}
}

std::pair<double, double>
command_sequence::get_inflow_point() const
{
	return elements.front()->get_inflow_point();
}

std::pair<double, double>
command_sequence::get_outflow_point() const
{
	return elements.back()->get_outflow_point();
}

void
command_sequence::draw(double global_phase, const tile_display_args_t & display_args) const
{
	for (const auto & cpt : *this) {
		cpt->draw(global_phase, display_args);
	}
}

void
command_sequence::animate(double now) const
{
	for (const auto & cpt : *this) {
		cpt->animate(now);
	}
}

void
command_sequence::replenish() const
{
	for (const auto & cpt : *this) {
		cpt->replenish();
	}
}

void
command_sequence::exhaust() const
{
	for (const auto & cpt : *this) {
		cpt->exhaust();
	}
}

layout_extents &
command_sequence::compute_layout(
	const virtual_insertion & insertion,
	commands_layout & layout) const
{
	layout_extents & e = layout.sequences[this];
	e.x = 0;
	e.y = 0;
	e.width = 0;
	e.top = 0;
	e.bottom = 0;
	bool seq_insert = insertion.position && !insertion.position->branch;
	bool branch_insert = insertion.position && insertion.position->branch;;
	double x = 0.;
	for (std::size_t n = 0; n < size(); ++n) {
		if (seq_insert && n == insertion.position->index) {
			e.top = std::min(e.top, insertion.extents->top);
			e.bottom = std::max(e.bottom, insertion.extents->bottom);
			e.width += insertion.extents->width;
			x = e.width;
		}

		layout_extents & sub = elements[n]->compute_layout(
			(branch_insert && n == insertion.position->index)
				? command_point::virtual_insertion(insertion.position->branch.get(), insertion.extents)
				:command_point::virtual_insertion(),
			layout);

		sub.x = x;
		sub.y = 0.;

		e.top = std::min(e.top, sub.top);
		e.bottom = std::max(e.bottom, sub.bottom);
		e.width += sub.width;

		x = e.width;
	}
	if (seq_insert && size() <= insertion.position->index) {
		e.top = std::min(e.top, insertion.extents->top);
		e.bottom = std::max(e.bottom, insertion.extents->bottom);
		e.width += insertion.extents->width;
	}

	return e;
}

void
command_sequence::apply_layout(
	const commands_layout & layout,
	double x_origin,
	double y_origin,
	double scale,
	bool warp) const
{
	for (const auto & cpt : elements) {
		const layout_extents & e = layout.points.find(cpt.get())->second;
		cpt->apply_layout(layout, x_origin + e.x * scale, y_origin + e.y * scale, scale, warp);
	}
}

command_sequence_path
command_sequence::compute_path(
	const commands_layout& layout,
	double x,
	double y,
	double scale) const
{
	std::size_t index = 0;
	for (const auto& cpt : elements) {
		const layout_extents & e = layout.points.find(cpt.get())->second;
		if (x <= (e.width + e.x - .5) * scale) {
			return {index, cpt->compute_path(layout, x - e.x * scale, y - e.y * scale, scale)};
		}
		++index;
	}

	return {index, {}};
}


void
command_sequence::insert(
	const command_sequence_path & path,
	std::unique_ptr<command_point> cpt)
{
	const command_sequence_path * seq_path = &path;
	command_sequence * seq_tgt = this;
	while (seq_path) {
		if (seq_path->index < seq_tgt->size()) {
			command_point & target_point = *(*seq_tgt)[seq_path->index];
			if (seq_path->branch && seq_path->branch->branch < target_point.num_branches()) {
				seq_tgt = &target_point.branch(seq_path->branch->branch);
				seq_path = &seq_path->branch->seq;
			} else {
				seq_tgt->insert(seq_path->index, std::move(cpt));
				seq_path = nullptr;
			}
		} else {
			seq_tgt->append(std::move(cpt));
			seq_path = nullptr;
		}
	}
}

std::unique_ptr<command_point>
command_sequence::unlink(
	const command_sequence_path & path)
{
	const command_sequence_path * seq_path = &path;
	command_sequence * seq_from = this;
	while (seq_path) {
		if (seq_path->index < seq_from->size()) {
			std::unique_ptr<command_point> & from_point = (*seq_from)[seq_path->index];
			if (seq_path->branch && seq_path->branch->branch < from_point->num_branches()) {
				seq_from = &from_point->branch(seq_path->branch->branch);
				seq_path = &seq_path->branch->seq;
			} else {
				std::unique_ptr<command_point> cpt = std::move(from_point);
				seq_from->erase(seq_from->begin() + seq_path->index);
				return std::move(cpt);
			}
		} else {
			return {};
		}
	}
	return {};
}

command_point *
command_sequence::lookup(
	const command_sequence_path & path) const
{
	const command_sequence_path * seq_path = &path;
	const command_sequence * seq_from = this;
	while (seq_path) {
		if (seq_path->index < seq_from->size()) {
			const std::unique_ptr<command_point> & from_point = (*seq_from)[seq_path->index];
			if (seq_path->branch && seq_path->branch->branch < from_point->num_branches()) {
				seq_from = &from_point->branch(seq_path->branch->branch);
				seq_path = &seq_path->branch->seq;
			} else {
				return from_point.get();
			}
		} else {
			return nullptr;
		}
	}
	return nullptr;
}

/*******************************************************************************
 * command_point */

command_point::command_point(const command_point & other)
	: tile_(new command_tile(*other.tile_))
	, branches_(new command_sequence[num_branches()])
 	, flow_points_(new command_flow_point[num_flow_points()])
{
	for (std::size_t n = 0; n < num_branches(); ++n) {
		branches_[n] = other.branches_[n];
	}
}

command_point::command_point(std::unique_ptr<command_tile> tile)
	: tile_(std::move(tile))
	, branches_(new command_sequence[num_branches()])
 	, flow_points_(new command_flow_point[num_flow_points()])
{
}

command_point & command_point::operator=(const command_point & other)
{
	tile_.reset(new command_tile(*other.tile_));
	branches_.reset(new command_sequence[num_branches()]);
	flow_points_.reset(new command_flow_point[num_flow_points()]);

	for (std::size_t n = 0; n < num_branches(); ++n) {
		branches_[n] = other.branches_[n];
	}

	return *this;
}

std::unique_ptr<command_tile>
command_point::extract_tile() && noexcept
{
	return std::move(tile_);
}

void
command_point::draw_flows(double global_phase, const tile_display_args_t & display_args) const
{
	if (tile().kind() == command_tile::kind_t::conditional) {
		if (branch(0).empty()) {
			draw_flow({
				{tile().x(), tile().y()},
				{flow_points_[0].x(), flow_points_[0].y()},
				{flow_points_[2].x(), flow_points_[0].y()},
				{flow_points_[2].x(), flow_points_[2].y()}
			}, display_args.flow_scale, display_args.flow_width, global_phase);
		} else {
			draw_flow({
				{tile().x(), tile().y()},
				{flow_points_[0].x(), flow_points_[0].y()},
				branch(0).get_inflow_point(),
			}, display_args.flow_scale, display_args.flow_width, global_phase);
			draw_flow({
				branch(0).get_outflow_point(),
				{flow_points_[2].x(), flow_points_[0].y()},
				{flow_points_[2].x(), flow_points_[2].y()}
			}, display_args.flow_scale, display_args.flow_width, global_phase);
		}
		if (branch(1).empty()) {
			draw_flow({
				{tile().x(), tile().y()},
				{flow_points_[1].x(), flow_points_[1].y()},
				{flow_points_[2].x(), flow_points_[1].y()},
				{flow_points_[2].x(), flow_points_[2].y()}
			}, display_args.flow_scale, display_args.flow_width, global_phase);
		} else {
			draw_flow({
				{tile().x(), tile().y()},
				{flow_points_[1].x(), flow_points_[1].y()},
				branch(1).get_inflow_point(),
			}, display_args.flow_scale, display_args.flow_width, global_phase);
			draw_flow({
				branch(1).get_outflow_point(),
				{flow_points_[2].x(), flow_points_[1].y()},
				{flow_points_[2].x(), flow_points_[2].y()}
			}, display_args.flow_scale, display_args.flow_width, global_phase);
		}
	} else if (tile().kind() > command_tile::kind_t::conditional) {
		if (branch(0).empty()) {
			draw_flow({
				{tile().x(), tile().y()},
				{flow_points_[0].x(), flow_points_[0].y()},
				{flow_points_[2].x(), flow_points_[0].y()},
				{flow_points_[2].x(), flow_points_[1].y()},
				{flow_points_[1].x(), flow_points_[1].y()},
				{tile().x(), tile().y()}
			}, display_args.flow_scale, display_args.flow_width, global_phase);
		} else {
			draw_flow({
				{tile().x(), tile().y()},
				{flow_points_[0].x(), flow_points_[0].y()},
				branch(0).get_inflow_point()
			}, display_args.flow_scale, display_args.flow_width, global_phase);
			draw_flow({
				branch(0).get_outflow_point(),
				{flow_points_[2].x(), flow_points_[0].y()},
				{flow_points_[2].x(), flow_points_[1].y()},
				{flow_points_[1].x(), flow_points_[1].y()},
				{tile().x(), tile().y()}
			}, display_args.flow_scale, display_args.flow_width, global_phase);
		}
	}

	for (std::size_t n = 0; n < num_branches(); ++n) {
		branch(n).draw_flows(global_phase, display_args);
	}
}

std::pair<double, double>
command_point::get_inflow_point() const
{
	return {tile().x(), tile().y()};
}

std::pair<double, double>
command_point::get_outflow_point() const
{
	if (num_flow_points()) {
		return {flow_points_[2].x(), flow_points_[2].y()};
	} else {
		return {tile().x(), tile().y()};
	}
}

void
command_point::draw(double global_phase, const tile_display_args_t & display_args) const
{
	tile().draw(global_phase, display_args);
	for (std::size_t n = 0; n < num_branches(); ++n) {
		branch(n).draw(global_phase, display_args);
	}
}

void
command_point::animate(double now) const
{
	tile().animate(now);
	for (std::size_t n = 0; n < num_branches(); ++n) {
		branch(n).animate(now);
	}
	for (std::size_t n = 0; n < num_flow_points(); ++n) {
		flow_points_[n].animate(now);
	}
}

void
command_point::replenish() const
{
	tile().replenish();
	for (std::size_t n = 0; n < num_branches(); ++n) {
		branch(n).replenish();
	}
}

void
command_point::exhaust() const
{
	tile().exhaust();
	for (std::size_t n = 0; n < num_branches(); ++n) {
		branch(n).exhaust();
	}
}

layout_extents &
command_point::compute_layout(
	const virtual_insertion & insertion,
	commands_layout & layout) const
{
	/* the tile itself */
	layout_extents & e = layout.points[this];
	e.x = 0;
	e.y = 0;
	e.top = -.5;
	e.bottom = +.5;
	e.width = 1.;

	double top_branch_y = -.5;
	double bottom_branch_y = +.5;

	for (std::size_t n = 0; n < num_branches(); ++n) {
		layout_extents & branch_extents =
			branch(n).compute_layout(
				insertion.position && insertion.position->branch == n
					? command_sequence::virtual_insertion(&insertion.position->seq, insertion.extents)
					: command_sequence::virtual_insertion(),
				layout);
		double branch_height = branch_extents.bottom - branch_extents.top;
		branch_extents.x = 1.0;
		if (n == 0) {
			branch_extents.y = -.5 * branch_height;
			e.top = std::min(-1., -branch_height);
			top_branch_y = std::min(top_branch_y, branch_extents.y);
		} else {
			branch_extents.y = +.5 * branch_height;
			e.bottom = std::max(+1., branch_height);
			bottom_branch_y = std::max(bottom_branch_y, branch_extents.y);
		}
		e.width = std::max(e.width, branch_extents.width + 2.0);
	}

	if (num_branches()) {
		layout.flow_points[&flow_points_[0]] = layout_extents(0., top_branch_y, 0., 0., 0.);
		layout.flow_points[&flow_points_[1]] = layout_extents(0., bottom_branch_y, 0., 0., 0.);
		layout.flow_points[&flow_points_[2]] = layout_extents(e.width - 1., 0., 0., 0., 0.);
	}

	return e;
}

void
command_point::apply_layout(
	const commands_layout & layout,
	double x_origin,
	double y_origin,
	double scale,
	bool warp) const
{
	if (warp) {
		tile().move_to(x_origin, y_origin);
	} else {
		tile().accelerate_to(x_origin, y_origin);
	}
	for (std::size_t n = 0; n < num_branches(); ++n) {
		const layout_extents & e = layout.sequences.find(&branch(n))->second;
		branch(n).apply_layout(layout, x_origin + e.x * scale, y_origin + e.y * scale, scale, warp);
	}
	for (std::size_t n = 0; n < num_flow_points(); ++n) {
		const layout_extents & e = layout.flow_points.find(&flow_points_[n])->second;
		if (warp) {
			flow_points_[n].move_to(x_origin + e.x * scale, y_origin + e.y * scale);
		} else {
			flow_points_[n].accelerate_to(x_origin + e.x * scale, y_origin + e.y * scale);
		}
	}
}

std::unique_ptr<command_branch_path>
command_point::compute_path(
	const commands_layout & layout,
	double x,
	double y,
	double scale) const
{
	if (x < scale * .5 || num_branches() == 0) {
		return {};
	}

	std::size_t index = (y > 0 && num_branches() > 1) ? 1 : 0;

	const layout_extents & e = layout.sequences.find(&branch(index))->second;

	std::unique_ptr<command_branch_path> path(
		new command_branch_path(
			index,
			branch(index).compute_path(layout, x - e.x * scale, y - e.y * scale, scale)));

	return path;
}

/*******************************************************************************
 * paths */

command_sequence_path::command_sequence_path() noexcept
	: index(0)
{
}

command_sequence_path::command_sequence_path(std::size_t init_index) noexcept
	: index(init_index)
{
}

command_sequence_path::command_sequence_path(std::size_t init_index,
								std::unique_ptr<command_branch_path> init_branch) noexcept
	: index(init_index)
	, branch(std::move(init_branch))
{
}

command_sequence_path::command_sequence_path(command_sequence_path && other) noexcept
	: index(other.index), branch(std::move(other.branch))
{
}

command_sequence_path &
command_sequence_path::operator=(command_sequence_path && other) noexcept
{
	index = other.index;
	branch = std::move(other.branch);
	return *this;
}

bool
command_sequence_path::operator==(const command_sequence_path& other) const noexcept
{
	if (!branch && other.branch) {
		return false;
	}
	if (branch && !other.branch) {
		return false;
	}
	return index == other.index && (!branch || (*branch == *other.branch));
}

bool
command_sequence_path::operator!=(const command_sequence_path& other) const noexcept
{
	return ! (*this == other);
}

command_branch_path::command_branch_path(command_branch_path && other) noexcept
	: branch(other.branch)
	, seq(std::move(other.seq))
{
}

command_branch_path::command_branch_path(
	std::size_t init_branch,
	command_sequence_path && init_seq) noexcept
	: branch(init_branch)
	, seq(std::move(init_seq))
{
}

bool
command_branch_path::operator==(const command_branch_path& other) const noexcept
{
	return branch == other.branch && seq == other.seq;
}

bool
command_branch_path::operator!=(const command_branch_path& other) const noexcept
{
	return ! (*this == other);
}


/*******************************************************************************
 * mutation */

void
insert_by_path(
	const command_sequence_path & path,
	std::unique_ptr<command_point> cpt,
	command_sequence & target)
{
	const command_sequence_path * seq_path = &path;
	command_sequence * seq_tgt = &target;
	while (seq_path) {
		if (seq_path->index < seq_tgt->size()) {
			command_point & target_point = *(*seq_tgt)[seq_path->index];
			if (seq_path->branch && seq_path->branch->branch < target_point.num_branches()) {
				seq_tgt = &target_point.branch(seq_path->branch->branch);
				seq_path = &seq_path->branch->seq;
			} else {
				seq_tgt->insert(seq_path->index, std::move(cpt));
				seq_path = nullptr;
			}
		} else {
			seq_tgt->append(std::move(cpt));
			seq_path = nullptr;
		}
	}
}

std::unique_ptr<command_point>
unlink_by_path(
	const command_sequence_path & path,
	command_sequence & from)
{
	const command_sequence_path * seq_path = &path;
	command_sequence * seq_from = &from;
	while (seq_path) {
		if (seq_path->index < seq_from->size()) {
			std::unique_ptr<command_point> & from_point = (*seq_from)[seq_path->index];
			if (seq_path->branch && seq_path->branch->branch < from_point->num_branches()) {
				seq_from = &from_point->branch(seq_path->branch->branch);
				seq_path = &seq_path->branch->seq;
			} else {
				std::unique_ptr<command_point> cpt = std::move(from_point);
				seq_from->erase(seq_from->begin() + seq_path->index);
				return std::move(cpt);
			}
		} else {
			return {};
		}
	}
	return {};
}

command_tile *
tile_by_path(
	const command_sequence_path & path,
	const command_sequence & seq)
{
	const command_sequence_path * seq_path = &path;
	const command_sequence * seq_from = &seq;
	while (seq_path) {
		if (seq_path->index < seq_from->size()) {
			const std::unique_ptr<command_point> & from_point = (*seq_from)[seq_path->index];
			if (seq_path->branch && seq_path->branch->branch < from_point->num_branches()) {
				seq_from = &from_point->branch(seq_path->branch->branch);
				seq_path = &seq_path->branch->seq;
			} else {
				return &from_point->tile();
			}
		} else {
			return nullptr;
		}
	}
	return nullptr;
}

namespace {

void gl_flow_color(double phase)
{
	glColor3f(.5 * (phase + 0.5), 0., .5 * (phase + 0.5));
}

}

#if 0
void
draw_flow(
	double x1, double y1,
	double x2, double y2,
	double scale,
	double phase)
{
	phase = 1.0 - phase + floor(phase);
	double dx = x2 - x1;
	double dy = y2 - y1;
	double d = sqrt(dx * dx + dy * dy);
	if (!d) {
		return;
	}

	double udx = dx / d;
	double udy = dy / d;
	double sdx = scale * udx;
	double sdy = scale * udy;

	double nx = udy * 2;
	double ny = -udx * 2;

	glBegin(GL_QUADS);
	texture_generator::make_solid_tex_coord();

	double p = 0.0;
	if (phase) {
		double l = std::min(1.0, d / scale + phase);
		double w = l - phase;
		gl_flow_color(phase);
		glVertex2f(x1 + nx, y1 + ny);
		glVertex2f(x1 - nx, y1 - ny);
		gl_flow_color(l);
		glVertex2f(x1 + sdx * w - nx, y1 + sdy * w - ny);
		glVertex2f(x1 + sdx * w + nx, y1 + sdy * w + ny);
		p += w;
	}

	while (p + 1 < d / scale) {
		gl_flow_color(0.);
		glVertex2f(x1 + sdx * p + nx, y1 + sdy * p + ny);
		glVertex2f(x1 + sdx * p - nx, y1 + sdy * p - ny);
		gl_flow_color(1.0);
		p += 1.0;
		glVertex2f(x1 + sdx * p - nx, y1 + sdy * p - ny);
		glVertex2f(x1 + sdx * p + nx, y1 + sdy * p + ny);
	}

	if (p < d / scale) {
		double w = d / scale - p;
		gl_flow_color(0.);
		glVertex2f(x1 + sdx * p + nx, y1 + sdy * p + ny);
		glVertex2f(x1 + sdx * p - nx, y1 + sdy * p - ny);
		gl_flow_color(w);
		glVertex2f(x2 - nx, y2 - ny);
		glVertex2f(x2 + nx, y2 + ny);
	}

	glEnd();
}
#endif

void
draw_flow_line_segment(
	double x1l, double y1l, double x1r, double y1r, double phase1,
	double x2l, double y2l, double x2r, double y2r, double phase2)
{
	gl_flow_color(phase1);
	glVertex2f(x1l, y1l);
	glVertex2f(x1r, y1r);
	gl_flow_color(phase2);
	glVertex2f(x2r, y2r);
	glVertex2f(x2l, y2l);
}

void
draw_flow_line(
	double x1l, double y1l, double x1r, double y1r, double phase1,
	double x2l, double y2l, double x2r, double y2r, double phase2)
{
	double sb = phase1;
	double se = std::min(floor(phase1 + 1), phase2);
	while (sb < phase2) {
		double fb2 = (sb - phase1) / (phase2 - phase1);
		double fb1 = 1 - fb2;
		double xbl = x1l * fb1 + x2l * fb2;
		double ybl = y1l * fb1 + y2l * fb2;
		double xbr = x1r * fb1 + x2r * fb2;
		double ybr = y1r * fb1 + y2r * fb2;

		double fe2 = (se - phase1) / (phase2 - phase1);
		double fe1 = 1 - fe2;
		double xel = x1l * fe1 + x2l * fe2;
		double yel = y1l * fe1 + y2l * fe2;
		double xer = x1r * fe1 + x2r * fe2;
		double yer = y1r * fe1 + y2r * fe2;

		draw_flow_line_segment(xbl, ybl, xbr, ybr, sb - floor(sb), xel, yel, xer, yer, se - floor(sb));

		sb = se;
		se = std::min(se + 1, phase2);
	}
}

namespace {

std::pair<double, double>
corner_offset(
	double dx1, double dy1,
	double dx2, double dy2,
	double w)
{
	double det = dy2 * dx1 - dy1 * dx2;

	if (det) {
		return {w * (dx2 - dx1) / det, w * (dy2 - dy1) / det};
	} else {
		return {-w * dy1, w * dx1};
	}
}

}

void
draw_flow(
	const std::vector<std::pair<double, double>> & points,
	double scale, double width, double phase)
{
	phase = floor(phase) - phase;
	glBegin(GL_QUADS);
	texture_generator::make_solid_tex_coord();

	double dx1 = points[1].first - points[0].first;
	double dy1 = points[1].second - points[0].second;
	double d1 = sqrt(dx1 * dx1 + dy1 * dy1);
	dx1 /= d1;
	dy1 /= d1;

	double cx1 = -dy1 * width;
	double cy1 = dx1 * width;

	for (std::size_t n = 1; n < points.size(); ++n) {
		double dx2 = dx1, dy2 = dy1, d2 = 0, cx2, cy2;
		if (n + 1 < points.size()) {
			dx2 = points[n + 1].first - points[n + 0].first;
			dy2 = points[n + 1].second - points[n + 0].second;
			d2 = sqrt(dx2 * dx2 + dy2 * dy2);
			dx2 /= d2;
			dy2 /= d2;
		}
		std::tie(cx2, cy2) = corner_offset(dx1, dy1, dx2, dy2, width);

		double x1 = points[n - 1].first;
		double y1 = points[n - 1].second;
		double x2 = points[n].first;
		double y2 = points[n].second;

		draw_flow_line(
			x1 + cx1, y1 + cy1, x1 - cx1, y1 - cy1, phase,
			x2 + cx2, y2 + cy2, x2 - cx2, y2 - cy2, phase + d1 / scale);

		phase = phase + d1 / scale;

		dx1 = dx2;
		dy1 = dy2;
		d1 = d2;
		cx1 = cx2;
		cy1 = cy2;
	}

	glEnd();
}

void
draw_flow(
	double x1, double y1,
	double x2, double y2,
	double scale,
	double width,
	double phase)
{
	draw_flow({{x1, y1}, {x2, y2}}, scale, width, phase);
}

