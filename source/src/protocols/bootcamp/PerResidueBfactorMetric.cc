// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/bootcamp/PerResidueBfactorMetric.cc
/// @brief Calculates per residue B factor metric
/// @author FAOlivieri (federicoaolivieri@gmail.com)

// Unit headers
#include <protocols/bootcamp/PerResidueBfactorMetric.hh>
#include <protocols/bootcamp/PerResidueBfactorMetricCreator.hh>

// Core headers
#include <core/simple_metrics/PerResidueRealMetric.hh>
#include <core/simple_metrics/util.hh>

#include <core/select/residue_selector/ResidueSelector.hh>
#include <core/select/residue_selector/util.hh>
#include <core/select/util.hh>
#include <core/pose/Pose.hh>
#include <core/pose/PDBInfo.hh>

#include <core/conformation/Residue.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <basic/datacache/DataMap.hh>
#include <utility/tag/Tag.hh>
#include <utility/string_util.hh>
#include <utility/pointer/memory.hh>


// XSD Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>
#include <basic/citation_manager/CitationCollection.hh>

#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/types/polymorphic.hpp>
#endif // SERIALIZATION

static basic::Tracer TR( "protocols.bootcamp.PerResidueBfactorMetric" );


namespace protocols {
namespace bootcamp {

using namespace core::select;
using namespace core::select::residue_selector;

	/////////////////////
	/// Constructors  ///
	/////////////////////

/// @brief Default constructor
PerResidueBfactorMetric::PerResidueBfactorMetric():
	core::simple_metrics::PerResidueRealMetric()
{}

////////////////////////////////////////////////////////////////////////////////
/// @brief Destructor (important for properly forward-declaring smart-pointer members)
PerResidueBfactorMetric::~PerResidueBfactorMetric(){}

core::simple_metrics::SimpleMetricOP
PerResidueBfactorMetric::clone() const {
	return utility::pointer::make_shared< PerResidueBfactorMetric >( *this );
}

std::string
PerResidueBfactorMetric::name() const {
	return name_static();
}

std::string
PerResidueBfactorMetric::name_static() {
	return "PerResidueBfactorMetric";

}
std::string
PerResidueBfactorMetric::metric() const {

	return "pr_bfactor";
}

void
PerResidueBfactorMetric::parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & datamap)
{

	SimpleMetric::parse_base_tag( tag );
	PerResidueRealMetric::parse_per_residue_tag( tag, datamap );


	if (tag->hasOption("atom_type")){
		atom_type_ = tag->getOption<std::string>("atom_type", "CA");
	}
}

void
PerResidueBfactorMetric::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) {
	using namespace utility::tag;
	using namespace core::select::residue_selector;

	AttributeList attlist;
	attlist + XMLSchemaAttribute::required_attribute("atom_type", xs_string, "atom type");

	//attributes_for_parse_residue_selector( attlist, "residue_selector",
	//	"Selector specifying residues." );

	core::simple_metrics::xsd_per_residue_real_metric_type_definition_w_attributes(xsd, name_static(),
		"Calculates per residue B factor metric", attlist);
}

std::map< core::Size, core::Real >
PerResidueBfactorMetric::calculate(const core::pose::Pose & pose) const {
	utility::vector1< core::Size > selection1 = selection_positions(get_selector()->apply(pose));
	
	std::map< core::Size, core::Real> results;

	core::Real acum =0.0;
	for (core::Size res=1; res <= selection1.size();++res){
		acum=0;
		for (core::Size atom=1; atom <= pose.residue(res).natoms(); ++atom){
			TR << pose.residue(res).atom_type(atom).name()<< std::endl;
			if (pose.residue(res).atom_type(atom).name()==atom_type_){//this is just not a good way to write this
				
				acum=pose.pdb_info()->bfactor(res,atom); 

			}
		}
		results.insert({res,acum});
	}
	return results;

}

/// @brief This simple metric is unpublished.  It returns FAOlivieri as its author.
void
PerResidueBfactorMetric::provide_citation_info( basic::citation_manager::CitationCollectionList & citations ) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"PerResidueBfactorMetric", basic::citation_manager::CitedModuleType::SimpleMetric,
		"FAOlivieri",
		"TODO: institution",
		"federicoaolivieri@gmail.com",
		"Wrote the PerResidueBfactorMetric."
		)
	);
}

void
PerResidueBfactorMetricCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const {
	PerResidueBfactorMetric::provide_xml_schema( xsd );
}

std::string
PerResidueBfactorMetricCreator::keyname() const {
	return PerResidueBfactorMetric::name_static();
}

core::simple_metrics::SimpleMetricOP
PerResidueBfactorMetricCreator::create_simple_metric() const {
	return utility::pointer::make_shared< PerResidueBfactorMetric >();
}

void PerResidueBfactorMetric::set_atom_type(std::string  new_atom_type){
	atom_type_=new_atom_type;
}
std::string PerResidueBfactorMetric::get_atom_type(){
	return atom_type_;
}


} //bootcamp
} //protocols


#ifdef    SERIALIZATION



template< class Archive >
void
protocols::bootcamp::PerResidueBfactorMetric::save( Archive & arc ) const {
	arc( cereal::base_class< core::simple_metrics::PerResidueRealMetric>( this ) );
	//arc( CEREAL_NVP( output_as_pdb_nums_ ) );

}

template< class Archive >
void
protocols::bootcamp::PerResidueBfactorMetric::load( Archive & arc ) {
	arc( cereal::base_class< core::simple_metrics::PerResidueRealMetric >( this ) );
	//arc( output_as_pdb_nums_ );


}

SAVE_AND_LOAD_SERIALIZABLE( protocols::bootcamp::PerResidueBfactorMetric );
CEREAL_REGISTER_TYPE( protocols::bootcamp::PerResidueBfactorMetric )

CEREAL_REGISTER_DYNAMIC_INIT( protocols_bootcamp_PerResidueBfactorMetric )
#endif // SERIALIZATION




