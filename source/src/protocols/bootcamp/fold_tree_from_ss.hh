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


// Utility headers

/// Project headers
#include <core/types.hh>
#include <iostream>
// C++ headers

//Auto Headers
#include <core/pack/dunbrack/DunbrackRotamer.hh>


//Headers for the main file
#include <basic/Tracer.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/dssp/Dssp.hh>

namespace protocols {
    namespace bootcamp  {
        core::kinematics::FoldTree fold_tree_from_dssp_string(std::string dssp_string);
        core::kinematics::FoldTree fold_tree_from_ss(core::pose::Pose pose);

        utility::vector1< std::pair< core::Size, core::Size > > 
        identify_gaps_spans(utility::vector1< std::pair< core::Size, core::Size > > ss_spans,core::Size last_residue);

        utility::vector1< std::pair< core::Size, core::Size > >
        identify_secondary_structure_spans( std::string const & ss_string );
    }
}