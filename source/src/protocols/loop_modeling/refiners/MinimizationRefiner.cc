// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

// Unit headers
#include <protocols/loop_modeling/types.hh>
#include <protocols/loop_modeling/refiners/MinimizationRefiner.hh>
#include <protocols/loop_modeling/refiners/MinimizationRefinerCreator.hh>
#include <protocols/loop_modeling/utilities/rosetta_scripts.hh>

// Core headers
#include <core/kinematics/MoveMap.hh>
#include <core/optimization/MinimizerOptions.hh>
#include <core/pose/Pose.hh>
#include <core/pose/symmetry/util.hh>
#include <core/scoring/ScoreFunction.hh>

// Protocols headers
#include <protocols/loops/loops_main.hh>
#include <protocols/minimization_packing/MinMover.hh>

// C++ headers
#include <iostream>
// XSD XRW Includes
#include <utility/tag/XMLSchemaGeneration.hh>

#include <core/pack/task/TaskFactory.hh> // AUTO IWYU For TaskFactory
#include <core/pack/task/PackerTask.hh> // AUTO IWYU For PackerTask

namespace protocols {
namespace loop_modeling {
namespace refiners {

using namespace std;
using core::optimization::MinimizerOptions;
using core::optimization::MinimizerOptionsOP;
using core::optimization::MinimizerOptionsCOP;
using core::scoring::ScoreFunctionOP;
using core::scoring::ScoreFunctionCOP;
using protocols::minimization_packing::MinMover;
using protocols::minimization_packing::MinMoverOP;



MinimizationRefiner::MinimizationRefiner(
	bool cartesian, MinimizerOptionsOP options) {

	use_cartesian(cartesian);
	set_min_options(options);
}

void MinimizationRefiner::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & data
) {

	LoopMover::parse_my_tag(tag, data);
	utilities::set_scorefxn_from_tag(*this, tag, data);
}

bool MinimizationRefiner::do_apply(Pose & pose) {
	using core::kinematics::MoveMap;
	using core::kinematics::MoveMapOP;
	using core::pose::symmetry::is_symmetric;
	using core::scoring::ScoreFunctionCOP;
	using protocols::loops::loops_set_move_map;

	pose.update_residue_neighbors();

	Loops const & loops = *get_loops();
	MinMoverOP minimizer( new MinMover );
	ScoreFunctionCOP score_function = get_score_function();

	// Set the move map according to the task factory

	core::pack::task::TaskFactoryOP task_factory = get_tool<core::pack::task::TaskFactoryOP>(loop_modeling::ToolboxKeys::TASK_FACTORY,
		core::pack::task::TaskFactoryOP());
	core::Real neighbor_radius = task_factory ? 0 : 10.0;

	MoveMapOP move_map( new MoveMap );
	loops_set_move_map(
		pose, loops,
		/*fix sidechains:*/ false,
		*move_map,
		neighbor_radius,
		/*allow omega moves:*/ true,
		/*allow takeoff torsion moves:*/ false);

	if ( task_factory ) {
		core::pack::task::PackerTaskOP task = task_factory->create_task_and_apply_taskoperations(pose);

		for ( core::Size i = 1; i<= pose.size(); ++i ) {
			if ( task->being_packed(i) ) {
				move_map->set_chi(i, true);
			}
		}
	}

	minimizer->score_function(score_function);
	minimizer->movemap(move_map);
	minimizer->min_options(min_options_);
	minimizer->cartesian(use_cartesian_);
	minimizer->apply(pose);

	return true;
}

ScoreFunctionOP MinimizationRefiner::get_score_function() {
	return get_tool<ScoreFunctionOP>(ToolboxKeys::SCOREFXN);
}

void MinimizationRefiner::set_score_function(ScoreFunctionOP score_function) {
	set_tool(ToolboxKeys::SCOREFXN, score_function);
}

void MinimizationRefiner::set_min_options(MinimizerOptionsOP options) {
	// If no minimizer options are given, use default values that seem to work
	// well.  These values were chosen based a simple benchmark run, but in that
	// benchmark no parameters really performed that much better or worse than
	// any others.  So loop modeling doesn't seem to be that sensitive to the
	// exact choice of minimizer options.

	if ( ! options ) {
		min_options_ = utility::pointer::make_shared< MinimizerOptions >(
			"lbfgs_armijo_nonmonotone",   // min_type
			1e-3,       // min_tolerance
			true,       // use_nblist
			false);   // deriv_check
	} else {
		min_options_ = options;
	}
}

MinimizerOptionsOP MinimizationRefiner::get_min_options() {
	return min_options_;
}

MinimizerOptionsCOP MinimizationRefiner::get_min_options() const {
	return min_options_;
}

void MinimizationRefiner::use_cartesian(bool setting) {
	use_cartesian_ = setting;
}

bool MinimizationRefiner::use_cartesian() const {
	return use_cartesian_;
}

std::string MinimizationRefiner::get_name() const {
	return mover_name();
}

std::string MinimizationRefiner::mover_name() {
	return "MinimizationRefiner";
}

void MinimizationRefiner::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{

	// Calls :LoopMover::parse_my_tag: and "set_scorefxn_from_tag"
	using namespace utility::tag;
	AttributeList attlist;

	utilities::attributes_for_set_scorefxn_from_tag( attlist );

	// Create a complex type and  get the LoopMover attributes, as parse_my_tag calls LoopMover::parse_my_tag
	XMLSchemaSimpleSubelementList subelement_list;
	XMLSchemaComplexTypeGenerator ct_gen;
	LoopMover::define_composition_schema( xsd, ct_gen, subelement_list );
	ct_gen.element_name( mover_name() )
		.description(
		"Perform gradient minimization on the loop being sampled. Both the sidechain"
		"and backbone atoms are allowed to move, and no restraints are used" )
		.add_attributes( attlist  )
		.write_complex_type_to_schema( xsd );
}

std::string MinimizationRefinerCreator::keyname() const {
	return MinimizationRefiner::mover_name();
}

protocols::moves::MoverOP
MinimizationRefinerCreator::create_mover() const {
	return utility::pointer::make_shared< MinimizationRefiner >();
}

void MinimizationRefinerCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	MinimizationRefiner::provide_xml_schema( xsd );
}


}
}
}
