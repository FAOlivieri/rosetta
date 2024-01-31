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
#include to <protocols/bootcamp/fold_tree_from_ss.hh> 

static basic::Tracer TR( "test.protocols.bootcamp" );


// --------------- Test Class --------------- //

class FoldTreeFromSSTests : public CxxTest::TestSuite {

public:


	// Shared initialization goes here.
	void setUp() {
		core_init();
	}

	// Shared finalization goes here.
	void tearDown() {
	}

	void test_hello_world() {
		TS_ASSERT( true );
	}

	void test_identify_secondary_structure_spans(){
		utility::vector1< std::pair< core::Size, core::Size > > expected_result_test_1 = { {4, 8}, {12,19}, {22,26}, {36,41}, {45,55}, {58,62}, {65,68} };
		utility::vector1< std::pair< core::Size, core::Size > > expected_result_test_2 = { {1, 7}, {11,22}, {29,40}, {41,50}, {51,57}, {59,62}, {63,65} };
		utility::vector1< std::pair< core::Size, core::Size > > expected_result_test_3 = { {1, 9}, {11,18}, {20,28}, {30,30}, {32,36}, {38,38}, {40,40}, {42,42}, {44,51} };


		utility::vector1< std::pair< core::Size, core::Size > > results;
		results=identify_secondary_structure_spans("   EEEEE   HHHHHHHH  EEEEE   IGNOR EEEEEE   HHHHHHHHHHH  EEEEE  HHHH   ");
		TS_ASSERT_EQUALS(results,expected_result_test_1);

		results=identify_secondary_structure_spans("HHHHHHH   HHHHHHHHHHHH      HHHHHHHHHHHHEEEEEEEEEEHHHHHHH EEEEHHH ");
		TS_ASSERT_EQUALS(results,expected_result_test_2);

		results=identify_secondary_structure_spans("EEEEEEEEE EEEEEEEE EEEEEEEEE H EEEEE H H H EEEEEEEE");
		TS_ASSERT_EQUALS(results,expected_result_test_3);
	}

	void test_fold_tree_from_dssp_string(){
		std::string test_string="   EEEEEEE    EEEEEEE         EEEEEEEEE    EEEEEEEEEE   HHHHHH         EEEEEEEEE         EEEEE     ";
		core::kinematics::FoldTree result_tree=fold_tree_from_dssp_string(test_string);
		std::cout << result_tree << std::endl;
		TS_ASSERT(result_tree.check_fold_tree());

		core::pose::Pose test_pose = create_test_in_pdb_pose();
		core::scoring::dssp::Dssp testIn_dssp = core::scoring::dssp::Dssp(test_pose, true);
		result_tree=fold_tree_from_dssp_string(testIn_dssp.get_dssp_secstruct());
		TS_ASSERT(result_tree.check_fold_tree());

	}


};
