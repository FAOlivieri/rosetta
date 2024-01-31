// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

#include <iostream>
#include <basic/Tracer.hh>
#include <devel/init.hh>

#include <protocols/bootcamp/BootcampMover.hh>
#include <protocols/bootcamp/BootcampMoverCreator.hh>

static basic::Tracer TR( "apps.pilot.federico.bootcamp" );


int main(int argc, char ** argv ) {
    devel::init(argc, argv);

    //Just copying stuff from fixbb. Throw *stuff* at the wall and see what sticks

    using core::pack::task::operation::TaskOperationCOP;
    core::pack::task::TaskFactoryOP main_task_factory( new core::pack::task::TaskFactory );
    main_task_factory->push_back( utility::pointer::make_shared< core::pack::task::operation::InitializeFromCommandline >() );
    main_task_factory->push_back( utility::pointer::make_shared< core::pack::task::operation::ReadResfile >() );

    protocols::moves::BootcampMoverOP bc_mover( new protocols::moves::BootcampMover );

    protocols::jd2::JobDistributor::get_instance()->go(bc_mover);

	return 0;
} 
