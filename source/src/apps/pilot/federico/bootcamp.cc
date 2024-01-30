// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

#include <iostream>
#include <core/pose/Pose.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <devel/init.hh> 
#include <core/import_pose/import_pose.hh>
#include <utility/pointer/owning_ptr.hh>

#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <basic/Tracer.hh>
#include <numeric/random/random.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/moves/MonteCarlo.fwd.hh>
#include <protocols/moves/PyMOLMover.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/pack_rotamers.hh>
#include <core/pack/task/TaskFactory.hh>
static basic::Tracer TR( "apps.pilot.federico.bootcamp" );


int main(int argc, char ** argv ) {
    

	std::cout << "Hello World!" << std::endl;
    devel::init( argc, argv );
    utility::vector1< std::string > filenames = basic::options::option[ basic::options::OptionKeys::in::file::s ].value();
    if ( filenames.size() > 0 ) {
    std::cout << "You entered: " << filenames[ 1 ] << " as the PDB file to be read" << std::endl;
    } else {
        std::cout << "You didn't provide a PDB file with the -in::file::s option" << std::endl;
        return 1;
    }   

    core::pose::PoseOP mypose = core::import_pose::pose_from_file( filenames[1] );
    core::scoring::ScoreFunctionOP sfxn = core::scoring::get_score_function();
    protocols::moves::MonteCarloOP monteCarlo(new protocols::moves::MonteCarlo(*mypose, *sfxn, 0.8));
    protocols::moves::PyMOLObserverOP the_observer = protocols::moves::AddPyMOLObserver( *mypose, true, 0 );    
    the_observer->pymol().apply( *mypose);
    core::kinematics::MoveMap mm;
    mm.set_bb( true );
    mm.set_chi( true );

    core::optimization::MinimizerOptions min_opts( "lbfgs_armijo_atol", 0.01, true );
    core::optimization::AtomTreeMinimizer atm;
    core::pose::Pose copy_pose = *mypose;


    for (int i = 0; i < 25; i++) {
        double uniform_random_number= numeric::random::uniform();
        core::Size poseSize =  mypose->size();


        core::Size randres = uniform_random_number * poseSize + 1 ;
        core::Real pert1 = numeric::random::gaussian() ;
        core::Real pert2 = numeric::random::gaussian() ;
        core::Real orig_phi = mypose->phi( randres );
        core::Real orig_psi = mypose->psi( randres );
        mypose->set_phi( randres, orig_phi + pert1 );
        mypose->set_psi( randres, orig_psi + pert2 );

        core::pack::task::PackerTaskOP repack_task = core::pack::task::TaskFactory::create_packer_task( *mypose );
        repack_task->restrict_to_repacking();
        core::pack::pack_rotamers( *mypose, *sfxn, repack_task );

        atm.run( *copy_pose, mm, *sfxn, min_opts );
        *mypose = copy_pose;

        core::Real newScore = sfxn->score( *mypose );
        TR << "new score: " << newScore << std::endl;
        monteCarlo->boltzmann(*mypose);
        core::Real currScore = sfxn->score( *mypose );
        TR << "current score: " << currScore << std::endl;
        core::Real score = sfxn->score( *mypose );
        TR << score << std::endl;

    }
	return 0;
} 
