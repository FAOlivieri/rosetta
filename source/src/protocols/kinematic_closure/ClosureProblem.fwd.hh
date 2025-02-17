// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file
/// @brief  Forward header for ClosureProblem.
/// @author Kale Kundert (kale.kundert@ucsf.edu)

#ifndef INCLUDED_protocols_kinematic_closure_ClosureProblem_FWD_HH
#define INCLUDED_protocols_kinematic_closure_ClosureProblem_FWD_HH

#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace kinematic_closure {

class ClosureProblem;
typedef utility::pointer::shared_ptr<ClosureProblem> ClosureProblemOP;
typedef utility::pointer::shared_ptr<ClosureProblem const> ClosureProblemCOP;

} // namespace kinematic_closure
} // namespace protocols

#endif

