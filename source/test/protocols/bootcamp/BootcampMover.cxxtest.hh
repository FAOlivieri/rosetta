// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   test/protocols/match/FoldTreeFromSS.cxxtest.hh
/// @brief
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com)



// Test headers
#include <cxxtest/TestSuite.h>

#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Utility headers
#include <utility/pointer/deep_copy.hh>
/// Project headers
#include <core/types.hh>

// C++ headers

//Auto Headers
#include <core/pack/dunbrack/DunbrackRotamer.hh>


//My headers. I dont know where they go
#include <basic/Tracer.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/dssp/Dssp.hh>
#include <protocols/bootcamp/fold_tree_from_ss.hh> 
#include <protocols/moves/MoverFactory.hh> 
#include <protocols/bootcamp/BootcampMover.hh>

static basic::Tracer TR( "test.protocols.bootcamp" );


// --------------- Test Class --------------- //

class BootcampMoverTests : public CxxTest::TestSuite {

public:


	// Shared initialization goes here.
	void setUp() {
		core_init();
	}

	// Shared finalization goes here.
	void tearDown() {
	}

	void test_mover_factory() {
		protocols::moves::MoverOP base_mover_op = protocols::moves::MoverFactory::get_instance()->newMover("BootcampMover");
		protocols::bootcamp::BootcampMoverOP bcm_op =  utility::pointer::dynamic_pointer_cast< protocols::bootcamp::BootcampMover > ( base_mover_op );
		TS_ASSERT(bcm_op!=0);
	}




};
