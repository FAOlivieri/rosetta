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


//Headers for the main file
#include <basic/Tracer.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/dssp/Dssp.hh>

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
	}

	core::kinematics::FoldTree fold_tree_from_dssp_string(std::string dssp_string){
		core::kinematics::FoldTree foldTree;

		utility::vector1< std::pair< core::Size, core::Size > > ss_boundaries=identify_secondary_structure_spans(dssp_string);
		for ( core::Size ii = 1; ii <= ss_boundaries.size(); ++ii ) {
			std::cout << "SS Element " << ii << " from residue "
			<< ss_boundaries[ ii ].first << " to "
			<< ss_boundaries[ ii ].second << std::endl;
		}
		utility::vector1< std::pair< core::Size, core::Size > > gap_boundaries = identify_gaps_spans(ss_boundaries,dssp_string.size());
		for ( core::Size ii = 1; ii <= gap_boundaries.size(); ++ii ) {
			std::cout << "SS Element " << ii << " from residue "
			<< gap_boundaries[ ii ].first << " to "
			<< gap_boundaries[ ii ].second << std::endl;
		}
		core::Size middle_of_first_ss_boundary_position = ss_boundaries[1].first+((ss_boundaries[1].second-ss_boundaries[1].first)/2);
		core::Size jump_number=0;
		
		//The following is a truly horrifying piece of code. My crimes against good practices are beyond forgiveness. May god have mercy on my soul.

		for ( core::Size ii = 2; ii <= ss_boundaries.size(); ++ii ) {  //For each secondary structure element other than the first add jump
			jump_number++;
			core::Size middle_of_current_ss_boundary_position=ss_boundaries[ii].first+((ss_boundaries[ii].second-ss_boundaries[ii].first)/2);
			foldTree.add_edge(middle_of_first_ss_boundary_position, middle_of_current_ss_boundary_position, jump_number);
		}
		for ( core::Size ii = 2; ii <= gap_boundaries.size()-1; ++ii ) {  //For each gap element except first and last add jump
			jump_number++;
			core::Size middle_of_current_gap_boundary_position=gap_boundaries[ii].first+((gap_boundaries[ii].second-gap_boundaries[ii].first)/2);
			foldTree.add_edge(middle_of_first_ss_boundary_position, middle_of_current_gap_boundary_position, jump_number);
		}

		for ( core::Size ii = 1; ii <= ss_boundaries.size(); ++ii ) {  //For each secondary structure element add peptide edges
			core::Size middle_of_current_ss_boundary_position=ss_boundaries[ii].first+((ss_boundaries[ii].second-ss_boundaries[ii].first)/2);

			if (ii==1){
				if (middle_of_current_ss_boundary_position!=ss_boundaries[ii].first){ //only do this if they are different
					foldTree.add_edge(middle_of_current_ss_boundary_position, 1, -1);
				}
				if (middle_of_current_ss_boundary_position!=ss_boundaries[ii].second){ //only do this if they are different
					foldTree.add_edge(middle_of_current_ss_boundary_position, ss_boundaries[ii].second, -1);
				}
			}else{
				if (ii==ss_boundaries.size()){
					if (middle_of_current_ss_boundary_position!=ss_boundaries[ii].first){ //only do this if they are different
						foldTree.add_edge(middle_of_current_ss_boundary_position, ss_boundaries[ii].first, -1);
					}
					if (middle_of_current_ss_boundary_position!=dssp_string.size()){ //only do this if they are different
						foldTree.add_edge(middle_of_current_ss_boundary_position, dssp_string.size(), -1);
					}
				}else{
					if (middle_of_current_ss_boundary_position!=ss_boundaries[ii].first){ //only do this if they are different
						foldTree.add_edge(middle_of_current_ss_boundary_position, ss_boundaries[ii].first, -1);
					}
					if (middle_of_current_ss_boundary_position!=ss_boundaries[ii].second){ //only do this if they are different
						foldTree.add_edge(middle_of_current_ss_boundary_position, ss_boundaries[ii].second, -1);
					}
				}
			}
			
		}

		for ( core::Size ii = 2; ii <= gap_boundaries.size()-1; ++ii ) {  //For each gap element add peptide edges except first and last
			core::Size middle_of_current_gap_boundary_position=gap_boundaries[ii].first+((gap_boundaries[ii].second-gap_boundaries[ii].first)/2);
			if (middle_of_current_gap_boundary_position!=gap_boundaries[ii].first){ //only do this if they are different
				foldTree.add_edge(middle_of_current_gap_boundary_position, gap_boundaries[ii].first, -1);
			}
			if (middle_of_current_gap_boundary_position!=gap_boundaries[ii].second){ //only do this if they are different
				foldTree.add_edge(middle_of_current_gap_boundary_position, gap_boundaries[ii].second, -1);
			}
		}

		//TR << dssp_string;

		return foldTree;
	}

	core::kinematics::FoldTree fold_tree_from_ss(core::pose::Pose pose){
		core::scoring::dssp::Dssp dssp = core::scoring::dssp::Dssp(pose, true);  //Esto tiene q ser true?
		std::string dssp_string = dssp.get_dssp_secstruct();

		core::kinematics::FoldTree foldTree = fold_tree_from_dssp_string(dssp_string);

		return foldTree;
	}

	utility::vector1< std::pair< core::Size, core::Size > > 
	identify_gaps_spans(utility::vector1< std::pair< core::Size, core::Size > > ss_spans,core::Size last_residue){

		utility::vector1< std::pair< core::Size, core::Size > > gap_boundaries;

		core::Size start_position=1;
		for (core::Size ii = 1; ii <= ss_spans.size(); ++ii ){
			gap_boundaries.push_back( std::make_pair( start_position, ss_spans[ii].first-1 ));
			start_position=ss_spans[ii].second+1;
		}
		gap_boundaries.push_back( std::make_pair( ss_spans[ss_spans.size()].second+1, last_residue ));

		return gap_boundaries;
	}

	utility::vector1< std::pair< core::Size, core::Size > >
	identify_secondary_structure_spans( std::string const & ss_string )
	{
		utility::vector1< std::pair< core::Size, core::Size > > ss_boundaries;
		core::Size strand_start = -1;
		for ( core::Size ii = 0; ii < ss_string.size(); ++ii ) {
			if ( ss_string[ ii ] == 'E' || ss_string[ ii ] == 'H'  ) {
			if ( int( strand_start ) == -1 ) {
				strand_start = ii;
			} else if ( ss_string[ii] != ss_string[strand_start] ) {
				ss_boundaries.push_back( std::make_pair( strand_start+1, ii ) );
				strand_start = ii;
			}
			} else {
			if ( int( strand_start ) != -1 ) {
				ss_boundaries.push_back( std::make_pair( strand_start+1, ii ) );
				strand_start = -1;
			}
			}
		}
		if ( int( strand_start ) != -1 ) {
			// last residue was part of a ss-eleemnt                                                                                                                                
			ss_boundaries.push_back( std::make_pair( strand_start+1, ss_string.size() ));
		}
		for ( core::Size ii = 1; ii <= ss_boundaries.size(); ++ii ) {
			std::cout << "SS Element " << ii << " from residue "
			<< ss_boundaries[ ii ].first << " to "
			<< ss_boundaries[ ii ].second << std::endl;
		}
		return ss_boundaries;
	}


};
