// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/jd3/pose_outputters/SilentFilePoseOutputterCreator.hh
/// @brief  Declaration of the %SilentFilePoseOutputterCreator class
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com), Andy Watkins (amw579@stanford.edu)


#ifndef INCLUDED_protocols_jd3_pose_outputters_SilentFilePoseOutputterCreator_HH
#define INCLUDED_protocols_jd3_pose_outputters_SilentFilePoseOutputterCreator_HH

//unit headers
#include <protocols/jd3/pose_outputters/PoseOutputterCreator.hh>

// Package headers
#include <protocols/jd3/pose_outputters/PoseOutputter.fwd.hh>

// utility headers
#include <utility/tag/XMLSchemaGeneration.fwd.hh>

// C++ headers
#include <string>

#ifdef    SERIALIZATION
// Cereal headers
#include <cereal/types/polymorphic.fwd.hpp>
#endif // SERIALIZATION

namespace protocols {
namespace jd3 {
namespace pose_outputters {

class SilentFilePoseOutputterCreator : public PoseOutputterCreator
{
public:
	PoseOutputterOP create_outputter() const override;
	std::string keyname() const override;
	void provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const override;
	void list_options_read( utility::options::OptionKeyList & read_options ) const override;
	bool outputter_specified_by_command_line() const override;
#ifdef    SERIALIZATION
public:
	template< class Archive > void save( Archive & arc ) const;
	template< class Archive > void load( Archive & arc );
#endif // SERIALIZATION

};

} // namespace pose_outputters
} // namespace jd3
} // namespace protocols

#ifdef    SERIALIZATION
CEREAL_FORCE_DYNAMIC_INIT( protocols_jd3_pose_outputters_SilentFilePoseOutputterCreator )
#endif // SERIALIZATION


#endif //INCLUDED_protocols_jd3_pose_outputters_PoseOutputter_HH
