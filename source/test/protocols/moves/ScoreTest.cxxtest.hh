// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/moves/ScoreTest.cxxtest.hh
/// @brief  tests for container ScoreMover classe.
/// @author Monica Berrondo

// Test headers
#include <test/UMoverTest.hh>

// Unit headers
#include <protocols/simple_moves/ScoreMover.hh>


//Auto Headers
#include <core/pose/Pose.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>
#include <utility/pointer/owning_ptr.hh>


using namespace core;
using namespace core::pose;
using namespace protocols::moves;

///////////////////////////////////////////////////////////////////////////
/// @name ScoreTest
/// @brief: class for Score Mover with different scores unified testing
/// @author Monica Berrondo
///////////////////////////////////////////////////////////////////////////
class ScoreTest : public CxxTest::TestSuite, public test::UMoverTest {

public:
	void setUp() {
		core_init_with_additional_options( "-no_optH -score:weights score12 -out:output -restore_pre_talaris_2013_behavior -INTEGRATION_TEST" );
		residue_set = chemical::ChemicalManager::get_instance()->residue_type_set( chemical::FA_STANDARD );
		//test::UMoverTest::setUp();
	}

	/// @brief test score 12
	void test_Score12() {
		std::cout << "Start All Scoring tests" << "\n";
		//core_init_with_additional_options( "-score:weights score12 -out:output -restore_pre_talaris_2013_behaviorr" );
		one_mover_test(__FILE__, __LINE__, utility::pointer::make_shared< protocols::simple_moves::ScoreMover >(),
			"protocols/moves/test_in.pdb", "protocols/moves/score12.pdb", 0);
		//std::cout << "End Scoring -score:patch score12 test" << "\n";
		//std::cout << "End Scoring tests" << "\n";
		core_init_with_additional_options( "" );
	}
};


