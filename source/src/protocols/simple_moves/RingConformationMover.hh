// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file    protocols/simple_moves/RingConformationMover.hh
/// @brief   Declarations and simple accessor/mutator definitions for RingConformationMover.
/// @author  Labonte <JWLabonte@jhu.edu>

#ifndef INCLUDED_protocols_simple_moves_RingConformationMover_HH
#define INCLUDED_protocols_simple_moves_RingConformationMover_HH

// Unit headers
#include <protocols/simple_moves/RingConformationMover.fwd.hh>
#include <protocols/moves/Mover.hh>

// Project headers
#include <core/types.hh>
#include <core/kinematics/MoveMap.fwd.hh>
#include <core/select/movemap/MoveMapFactory.fwd.hh>

// Utility headers

// C++ headers
#include <string>

#include <utility/vector1.hh> // AUTO IWYU For vector1


namespace protocols {
namespace simple_moves {

/// @details  Based on a given MoveMap, this mover selects movable cyclic residues and flips their rings to an
/// idealized ring conformer.
class RingConformationMover: public moves::Mover {
public:  // Standard methods //////////////////////////////////////////////////
	/// @brief  Default constructor
	RingConformationMover();

	/// @brief  Copy constructor
	RingConformationMover( RingConformationMover const & object_to_copy );

	/// @brief  Constructor with MoveMap input option
	RingConformationMover( core::kinematics::MoveMapOP input_movemap );

	// Assignment operator
	RingConformationMover & operator=( RingConformationMover const & object_to_copy );

	// Destructor
	~RingConformationMover() override;


public: // Standard Rosetta methods ///////////////////////////////////////////
	// General methods
	/// @brief  Generate string representation of RingConformationMover for debugging purposes.
	void show( std::ostream & output=std::cout ) const override;


	// Mover methods
	/// @brief  Register options with the option system.
	static void register_options();

	/// @brief  Return the name of the Mover.
	std::string get_name() const override;

	static std::string mover_name();


	protocols::moves::MoverOP clone() const override;

	protocols::moves::MoverOP fresh_instance() const override;

	void parse_my_tag(
		TagCOP tag,
		basic::datacache::DataMap & data
	) override;

	static void provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

	/// @brief  Apply the corresponding move to <input_pose>.
	void apply( core::pose::Pose & input_pose ) override;


public:  // Citation Management ///////////////////////////////////////////////

	/// @brief Provide the citation.
	void provide_citation_info(basic::citation_manager::CitationCollectionList & ) const override;

public: // Accessors/Mutators /////////////////////////////////////////////////
	/// @brief  Get the current MoveMap, creating it if needed.
	core::kinematics::MoveMapCOP movemap( core::pose::Pose const & pose );

	/// @brief  Set the MoveMap.
	void movemap( core::kinematics::MoveMapOP new_movemap );

	/// @brief  Set the MoveMapFactory.
	void movemap_factory( core::select::movemap::MoveMapFactoryCOP new_movemap_factory );


	/// @brief  Get whether or not this Mover will sample all ring conformers, regardless of energy.
	bool sample_all_conformers() const { return sample_all_conformers_; }

	/// @brief  Set whether or not this Mover will sample all ring conformers, regardless of energy.
	void sample_all_conformers( bool setting ) { sample_all_conformers_ = setting; }


private:  // Private methods //////////////////////////////////////////////////
	// Set command-line options.  (Called by init())
	void set_commandline_options();

	// Initialize data members from arguments.
	void init();

	// Copy all data members from <object_to_copy_from> to <object_to_copy_to>.
	void copy_data( RingConformationMover & object_to_copy_to, RingConformationMover const & object_to_copy_from);

	// Setup list of movable cyclic residues from MoveMap.
	void setup_residue_list( core::pose::Pose const & pose );


private:  // Private data /////////////////////////////////////////////////////
	core::select::movemap::MoveMapFactoryCOP movemap_factory_;
	core::kinematics::MoveMapOP movemap_;
	utility::vector1< core::Size > residue_list_;  // list of movable cyclic residues by residue number
	bool locked_;  // Is this mover locked from the command line?
	bool sample_all_conformers_;  // Does this Mover sample both energy maxima and minima among ring conformers?

};  // class RingConformationMover

// Insertion operator (overloaded so that RingConformationMover can be "printed" in PyRosetta).
std::ostream & operator<<( std::ostream & output, RingConformationMover const & object_to_output );

}  // namespace simple_moves
}  // namespace protocols

#endif  // INCLUDED_protocols_simple_moves_RingConformationMover_HH
