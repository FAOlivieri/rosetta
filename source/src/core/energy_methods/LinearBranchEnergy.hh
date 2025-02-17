// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/energy_methods/EtableEnergy.hh
/// @brief  Etable energy method class declaration
/// @author Phil Bradley
/// @author Andrew Leaver-Fay
/// @author Christopher Miles (cmiles@uw.edu)

#ifndef INCLUDED_core_energy_methods_LinearBranchEnergy_hh
#define INCLUDED_core_energy_methods_LinearBranchEnergy_hh

// Unit headers
#include <core/energy_methods/LinearBranchEnergy.fwd.hh>

// Package headers
#include <core/types.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/scoring/EnergyMap.fwd.hh>
#include <core/scoring/methods/WholeStructureEnergy.hh>

// Project headers

// Third-party headers

// C++ headers
#include <utility/vector1.hh>


namespace core {
namespace energy_methods {


/// @brief LinearBranchEnergy class iterates across all residues in finalize()
/// and determines the penalty between branch-connected residues by how much their
/// psueduo atoms do not align (if they have them).
///
/// @details Calculates linear_branch_conn.
///  linear_branch_conn measures 3 distances (branch variants with cutpoint-like atoms
///  must be added to pose)
///
///  For ideal poses, this score should be very close to 0.  Real PDBs, however have bond length and angle
///   deviations that will cause this score to be fairly high.
///
class LinearBranchEnergy : public core::scoring::methods::WholeStructureEnergy {
public:
	typedef core::scoring::methods::WholeStructureEnergy parent;

	// @brief Creates a new LinearBranchEnergy with the default allowable sequence
	// separation (+inf)
	LinearBranchEnergy();

	/// @brief The auto-generated copy constructor does not properly handle smart
	/// pointer types, so we must explicitly define our own.
	LinearBranchEnergy(const LinearBranchEnergy&);

	/// @brief The auto-generated operator=() method does not properly handle pointer types.
	LinearBranchEnergy& operator=(const LinearBranchEnergy&);

	// @brief Releases resources associated with an instance.
	~LinearBranchEnergy() override;

	/// clone
	core::scoring::methods::EnergyMethodOP clone() const override {
		return utility::pointer::make_shared< LinearBranchEnergy >(*this);
	}

	/// called at the end of energy evaluation
	void finalize_total_energy(
		pose::Pose & pose,
		core::scoring::ScoreFunction const &,
		core::scoring::EnergyMap & totals) const override;

	/// called during gradient-based minimization inside dfunc
	/**
	F1 and F2 are not zeroed -- contributions from this atom are
	just summed in
	**/
	void eval_atom_derivative(id::AtomID const & id,
		pose::Pose const & pose,
		kinematics::DomainMap const & domain_map,
		core::scoring::ScoreFunction const & sfxn,
		core::scoring::EnergyMap const & weights,
		Vector & F1,
		Vector & F2) const override;

	void indicate_required_context_graphs( utility::vector1< bool > & ) const override;

private:

	// Initialization routine common to both constructor
	void initialize(Size allowable_sequence_sep);

	// Maximum allowable sequence separation permitted for scoring
	Size allowable_sequence_sep_;

	core::Size version() const override;
};

} // scoring
} // core
#endif
