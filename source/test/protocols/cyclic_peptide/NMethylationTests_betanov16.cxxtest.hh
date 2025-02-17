// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file  protocols/cyclic_peptide/NMethylationTests_betanov16.cxxtest.hh
/// @brief  Unit tests for working with peptides with N-methylation with the beta_nov16 scorefunction.
/// @author Vikram K. Mulligan (vmullig@u.washington.edu)


// Test headers
#include <test/protocols/cyclic_peptide/NMethylation_functions.hh>
#include <cxxtest/TestSuite.h>

// Project Headers

// Core Headers

// Protocol Headers
#include <basic/Tracer.hh>

#include <core/init_util.hh> // AUTO IWYU For core_init_with_additional_options

static basic::Tracer TR("protocols.cyclic_peptide.NMethylationTests_betanov16");

class NMethylationTests_betanov16 : public CxxTest::TestSuite {

public:

	void setUp(){
		core_init_with_additional_options( "-beta_nov16 -write_all_connect_info" );
	}

	void tearDown(){
	}

	/// @brief Just build an N-methylated peptide, manipulate it a bit, and do a FastRelax.
	///
	void test_linear_nmethyl_peptide() {
		test::protocols::cyclic_peptide::NMethylationTests_functions myfxns;
		myfxns.common_test_linear_nmethyl_peptide();
	}

private:

};



