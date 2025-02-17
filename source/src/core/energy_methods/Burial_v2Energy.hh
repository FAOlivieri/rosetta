// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/energy_methods/Burial_v2Energy.hh
/// @brief  energy term use for score burial of a specific residue
/// @author TJ Brunette

#ifndef INCLUDED_core_energy_methods_Burial_v2Energy_hh
#define INCLUDED_core_energy_methods_Burial_v2Energy_hh

#include <core/energy_methods/Burial_v2EnergyCreator.hh>
#include <core/scoring/methods/WholeStructureEnergy.hh>
#include <core/scoring/ScoreFunction.fwd.hh>

#include <core/pose/Pose.fwd.hh>

#include <utility/vector1.hh>


namespace core {
namespace energy_methods {



class Burial_v2Energy : public core::scoring::methods::WholeStructureEnergy {
public:
	typedef core::scoring::methods::WholeStructureEnergy parent;

	Burial_v2Energy() : core::scoring::methods::WholeStructureEnergy( utility::pointer::make_shared< Burial_v2EnergyCreator >() ) {
		init_from_file();
	}

	/// clone
	core::scoring::methods::EnergyMethodOP
	clone() const override;

	/////////////////////////////////////////////////////////////////////////////
	// scoring
	/////////////////////////////////////////////////////////////////////////////

	void finalize_total_energy(
		pose::Pose & pose,
		core::scoring::ScoreFunction const &,
		core::scoring::EnergyMap & totals
	) const override;

	void indicate_required_context_graphs( utility::vector1< bool > & ) const override {};

	core::Size version() const override;

private:
	core::Real using_atom_distance(core::pose::Pose const & pose) const;
	core::Real using_totalSasa(core::pose::Pose const & pose) const;
	void init_from_file();
	utility::vector1<core::Size> residue_ids_;
};

}
}

#endif // INCLUDED_core_energy_methods_Burial_v2Energy_HH
