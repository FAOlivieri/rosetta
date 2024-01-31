// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/bootcamp/BootcampMover.cc
/// @brief Bootcamp mover
/// @author FAOlivieri (federicoaolivieri@gmail.com)

// Unit headers
#include <protocols/bootcamp/BootcampMover.hh>
#include <protocols/bootcamp/BootcampMoverCreator.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/moves/MonteCarlo.fwd.hh>
#include <protocols/moves/PyMOLMover.hh>
#include <protocols/bootcamp/fold_tree_from_ss.hh> 
#include <protocols/jd2/JobDistributor.hh>

// Core headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/pack_rotamers.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/optimization/MinimizerOptions.hh>
#include <core/optimization/AtomTreeMinimizer.hh>
#include <core/scoring/ScoreType.hh>
#include <core/pose/variant_util.hh>


// Basic/Utility headers
#include <basic/Tracer.hh>
#include <utility/tag/Tag.hh>
#include <utility/pointer/memory.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <utility/pointer/owning_ptr.hh>
#include <numeric/random/random.hh>


// XSD Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <protocols/moves/mover_schemas.hh>

// Citation Manager
#include <utility/vector1.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>

static basic::Tracer TR( "protocols.bootcamp.BootcampMover" );

namespace protocols {
namespace bootcamp {

	/////////////////////
	/// Constructors  ///
	/////////////////////

/// @brief Default constructor
BootcampMover::BootcampMover():
	protocols::moves::Mover( BootcampMover::mover_name() )
{

}

////////////////////////////////////////////////////////////////////////////////
/// @brief Destructor (important for properly forward-declaring smart-pointer members)
BootcampMover::~BootcampMover(){}

////////////////////////////////////////////////////////////////////////////////
	/// Mover Methods ///
	/////////////////////

/// @brief Apply the mover
void
BootcampMover::apply( core::pose::Pose& mypose){

	//leftover from before. leaving it here just in case
	//std::cout << "You entered: " << filenames[ 1 ] << " as the PDB file to be read" << std::endl;		
	//core::pose::PoseOP mypose = core::import_pose::pose_from_file( filenames[1] ); 
	core::scoring::ScoreFunctionOP sfxn = core::scoring::get_score_function();
	
	sfxn->set_weight(core::scoring::linear_chainbreak, 1);  //foldtree
	core::pose::correctly_add_cutpoint_variants(mypose);
	core::kinematics::FoldTree fold_tree = protocols::bootcamp::fold_tree_from_ss(mypose);
	TR << "check_fold_tree: "<<fold_tree.check_fold_tree()<<std::endl;
	mypose.fold_tree(fold_tree);

	protocols::moves::MonteCarloOP monteCarlo(new protocols::moves::MonteCarlo(mypose, *sfxn, 0.8));
	//protocols::moves::PyMOLObserverOP the_observer = protocols::moves::AddPyMOLObserver( mypose, true, 0 );    
	//the_observer->pymol().apply( mypose);
	core::kinematics::MoveMap mm;
	mm.set_bb( true );
	mm.set_chi( true );

	core::optimization::MinimizerOptions min_opts( "lbfgs_armijo_atol", 0.01, true );
	core::optimization::AtomTreeMinimizer atm;
	core::pose::Pose copy_pose = mypose;

	core::Size number_of_iterations = 100;
	core::Size number_of_accepted_changes = 0;
	core::Real temp = 0.5;
	core::Real acum_score = 0;

	for (core::Size i = 0; i <= number_of_iterations; i++) {
		double uniform_random_number= numeric::random::uniform();
		core::Size poseSize =  mypose.size();


		core::Size randres = uniform_random_number * poseSize + 1 ;

		core::Real pert1 = numeric::random::gaussian() * temp ;
		core::Real pert2 = numeric::random::gaussian() * temp ;
		core::Real orig_phi = mypose.phi( randres );
		core::Real orig_psi = mypose.psi( randres );
		mypose.set_phi( randres, orig_phi + pert1 );
		mypose.set_psi( randres, orig_psi + pert2 );

		core::pack::task::PackerTaskOP repack_task = core::pack::task::TaskFactory::create_packer_task( mypose );
		repack_task->restrict_to_repacking();
		core::pack::pack_rotamers( mypose, *sfxn, repack_task );

		//atm.run( copy_pose, mm, *sfxn, min_opts );
		//*mypose = copy_pose;

		core::Real newScore = sfxn->score( mypose );
		bool accepted = monteCarlo->boltzmann(mypose);

		core::Real currScore = sfxn->score( mypose );
		TR << "Iteration number "<<i<<". new pose score: " << newScore << ". current score: " << currScore << std::endl;

		if (accepted){
			number_of_accepted_changes++;
			acum_score+=currScore;
		}

		if (i % 100 == 0 ){
			float success_rate = float(number_of_accepted_changes)/float(number_of_iterations) ;
			float avg_score=acum_score/float(number_of_accepted_changes);
			TR << "Proportion of accepted changes: " << success_rate << std::endl;  //temp=0.5  0.43 w/o foldtree ||||  0.68 with foldtree
			TR << "Average score of accepted poses: " << avg_score << std::endl;

		}

	}
    
	

}

////////////////////////////////////////////////////////////////////////////////
/// @brief Show the contents of the Mover
void
BootcampMover::show(std::ostream & output) const
{
	protocols::moves::Mover::show(output);
}

////////////////////////////////////////////////////////////////////////////////
	/// Rosetta Scripts Support ///
	///////////////////////////////

/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
void
BootcampMover::parse_my_tag(
	utility::tag::TagCOP ,
	basic::datacache::DataMap&
) {

}
void BootcampMover::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{

	using namespace utility::tag;
	AttributeList attlist;

	//here you should write code to describe the XML Schema for the class.  If it has only attributes, simply fill the probided AttributeList.

	protocols::moves::xsd_type_definition_w_attributes( xsd, mover_name(), "Bootcamp mover", attlist );
}


////////////////////////////////////////////////////////////////////////////////
/// @brief required in the context of the parser/scripting scheme
protocols::moves::MoverOP
BootcampMover::fresh_instance() const
{
	return utility::pointer::make_shared< BootcampMover >();
}

/// @brief required in the context of the parser/scripting scheme
protocols::moves::MoverOP
BootcampMover::clone() const
{
	return utility::pointer::make_shared< BootcampMover >( *this );
}

std::string BootcampMover::get_name() const {
	return mover_name();
}

std::string BootcampMover::mover_name() {
	return "BootcampMover";
}



/////////////// Creator ///////////////

protocols::moves::MoverOP
BootcampMoverCreator::create_mover() const
{
	return utility::pointer::make_shared< BootcampMover >();
}

std::string
BootcampMoverCreator::keyname() const
{
	return BootcampMover::mover_name();
}

void BootcampMoverCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	BootcampMover::provide_xml_schema( xsd );
}

/// @brief This mover is unpublished.  It returns FAOlivieri as its author.
void
BootcampMover::provide_citation_info(basic::citation_manager::CitationCollectionList & citations ) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"BootcampMover", basic::citation_manager::CitedModuleType::Mover,
		"FAOlivieri",
		"TODO: institution",
		"federicoaolivieri@gmail.com",
		"Wrote the BootcampMover."
		)
	);
}


////////////////////////////////////////////////////////////////////////////////
	/// private methods ///
	///////////////////////


std::ostream &
operator<<( std::ostream & os, BootcampMover const & mover )
{
	mover.show(os);
	return os;
}


} //bootcamp
} //protocols
