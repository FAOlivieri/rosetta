// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file  protocols/bootcamp/QueueTests.cxxtest.hh
/// @brief  lab3 test
/// @author FAOlivieri (federicoaolivieri@gmail.com)


// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Project Headers
#include <src/protocols/bootcamp/Queue.hh>

// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>

// Utility, etc Headers
#include <basic/Tracer.hh>

static basic::Tracer TR("QueueTests");


class QueueTests : public CxxTest::TestSuite {
	//Define Variables
private:
	protocols::bootcamp::Queue queue_;
public:

	void setUp() {
		core_init();
		queue_ = protocols::bootcamp::Queue();
	}

	void tearDown() {
	}

	void test_first() {

       TS_TRACE( "Running my first unit test!" );
       TS_ASSERT( true );

	}

	void test_enqueue(){
		core::Size initial_size = queue_.size();
		queue_.enqueue("test");
		core::Size final_size = queue_.size();
		TS_ASSERT_EQUALS( final_size , initial_size+1);
	}
	
	void test_dequeue(){
		TS_ASSERT_THROWS_ANYTHING(queue_.dequeue());
		queue_.enqueue("test1");
		queue_.enqueue("test2");
		core::Size initial_size=queue_.size();
		std::string output_string = queue_.dequeue();
		TS_ASSERT_EQUALS(output_string,"test1");
		TS_ASSERT_EQUALS(queue_.size(),initial_size-1);
		output_string = queue_.dequeue();
		TS_ASSERT_EQUALS(output_string,"test2");
		TS_ASSERT_EQUALS(queue_.size(),initial_size-2);
	}
	void test_is_empty(){
		TS_ASSERT(queue_.is_empty());
		queue_.enqueue("test");;
		TS_ASSERT(!queue_.is_empty());
	}
	void test_size(){
		TS_ASSERT_EQUALS(queue_.size(),0);
		for (int i=1; i<=5;i++){
			queue_.enqueue("test");
			TS_ASSERT_EQUALS(queue_.size(),i);
		}
		
	}

};
