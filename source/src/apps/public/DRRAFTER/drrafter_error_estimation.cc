// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file src/apps/public/DRRAFTER/drrafter_error_estimation.cc
/// @brief Calculate error estimates for a set of DRRAFTER models
/// @author Kalli Kappel

// libRosetta headers
#include <core/types.hh>
#include <core/chemical/ChemicalManager.hh>
#include <core/scoring/rms_util.hh>
#include <basic/options/option.hh>
#include <basic/options/option_macros.hh>
#include <protocols/viewer/viewers.hh>
#include <core/pose/Pose.hh>
#include <core/pose/init_id_map.hh>
#include <core/conformation/Residue.hh>
#include <core/pose/extra_pose_info_util.hh>
#include <core/init/init.hh>
#include <core/id/AtomID.hh>
#include <core/import_pose/pose_stream/PoseInputStream.hh>
#include <core/import_pose/pose_stream/PDBPoseInputStream.hh>
#include <core/import_pose/pose_stream/SilentFilePoseInputStream.hh>
#include <core/pose/PDBInfo.hh>
#include <utility/vector1.hh>

// C++ headers
#include <iostream>
#include <string>

#include <basic/Tracer.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <utility/excn/Exceptions.hh>

#include <utility/stream_util.hh> // AUTO IWYU For operator<<

OPT_KEY( Boolean, rmsd_nosuper )
OPT_KEY( Boolean, protein_align )
OPT_KEY( Boolean, rna_rmsd )
OPT_KEY( Boolean, per_residue_convergence )
OPT_KEY( IntegerVector, rmsd_res )
OPT_KEY( IntegerVector, align_residues )
OPT_KEY( FileVector, sgroup1 )
OPT_KEY( FileVector, sgroup2 )

static basic::Tracer TR( "apps.public.DRRAFTER.drrafter_error_estimation" );

///////////////////////////////////////////////////////////////////////////////
core::Real
mean( utility::vector1< core::Real > const & real_vector )
{
	using namespace core;

	Real avg( 0.0 );
	for ( Size i = 1; i <= real_vector.size(); ++i ) {
		avg += real_vector[ i ];
	}
	avg /= real_vector.size();
	return avg;
}

///////////////////////////////////////////////////////////////////////////////
core::Real
estimate_min_rmsd( core::Real const & mean_pairwise_rmsd ) {

	return 0.6249* mean_pairwise_rmsd + 0.2768;

}

///////////////////////////////////////////////////////////////////////////////
core::Real
estimate_mean_rmsd( core::Real const & mean_pairwise_rmsd ) {

	return 0.9660* mean_pairwise_rmsd + 0.1665;
}

///////////////////////////////////////////////////////////////////////////////
core::Real
estimate_rmsd_of_median_struct( core::Real const & mean_pairwise_rmsd ) {

	return 0.8199* mean_pairwise_rmsd + 0.2995;
}

///////////////////////////////////////////////////////////////////////////////
std::string
get_median_structure( utility::vector1< std::string > all_pose_tags,
	std::map< std::string, std::map< std::string, core::Real > > pairwise_rmsds )
{
	using namespace core;

	std::map< std::string, Real > mean_pairwise_rmsd_per_struct;
	for ( Size i = 1; i <= all_pose_tags.size(); ++i ) {
		utility::vector1< Real > pairwise_rmsds_this_struct;
		for ( Size j = 1; j <= all_pose_tags.size(); ++j ) {
			if ( i == j ) continue;
			pairwise_rmsds_this_struct.push_back( pairwise_rmsds[ all_pose_tags[i]][ all_pose_tags[j] ] );
		}
		TR.Debug << "pairwise rmsds " << all_pose_tags[ i ] << " " << pairwise_rmsds_this_struct << std::endl;
		Real mean_pairwise_rmsd_this_struct = mean( pairwise_rmsds_this_struct );
		mean_pairwise_rmsd_per_struct[ all_pose_tags[i] ] = mean_pairwise_rmsd_this_struct;
	}

	/// which one has the minimum RMSD?
	Real min_rmsd = mean_pairwise_rmsd_per_struct[ all_pose_tags[ 1 ] ];
	std::string median_struct;
	for ( Size i = 1; i <= mean_pairwise_rmsd_per_struct.size(); ++i ) {
		if ( i == 1 || mean_pairwise_rmsd_per_struct[ all_pose_tags[ i ] ] < min_rmsd ) {
			min_rmsd = mean_pairwise_rmsd_per_struct[ all_pose_tags[ i ] ];
			median_struct = all_pose_tags[ i ];
		}
	}

	TR.Debug << "Median structure: " << median_struct << std::endl;
	TR.Debug << "(with average rmsd of): " << min_rmsd << std::endl;

	return median_struct;
}

///////////////////////////////////////////////////////////////////////////////
void
check_all_poses_have_the_same_sequence( utility::vector1< core::pose::Pose > const & all_poses ) {

	using namespace core;

	std::string sequence_1 = all_poses[ 1 ].sequence();

	for ( core::Size i=2; i <= all_poses.size(); ++i ) {
		std::string sequence = all_poses[ i ].sequence();
		if ( sequence != sequence_1 ) {
			utility_exit_with_message( "All structures must have exactly the same sequence (and be in the same order!). Please check your input structures!" );
		}

	}
}
///////////////////////////////////////////////////////////////////////////////
core::id::AtomID_Map< core::id::AtomID >
setup_align_atom_map( core::pose::Pose const & pose ) {

	using namespace core;
	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	core::id::AtomID_Map< id::AtomID > align_atom_map;
	utility::vector1< core::Size > super_residues;

	if ( !option[ rmsd_nosuper ]() ) {
		if ( option[ protein_align ]() ) {
			for ( core::Size i = 1; i<= pose.total_residue(); ++i ) {
				if ( pose.residue( i).is_protein() ) {
					super_residues.push_back( i );
				}
			}
		} else {
			super_residues = option[ align_residues ]();
		}
	}

	if ( !option[ rmsd_nosuper ]() ) {
		core::pose::initialize_atomid_map( align_atom_map, pose, id::AtomID::BOGUS_ATOM_ID() );
		for ( core::Size i = 1; i <= super_residues.size(); ++i ) {
			id::AtomID const id1( pose.residue( super_residues[ i ] ).atom_index("CA"), super_residues[ i ]);
			id::AtomID const id2( pose.residue( super_residues[ i ] ).atom_index("CA"), super_residues[ i ]);
			align_atom_map[ id1 ] = id2;
		}
	}

	return align_atom_map;
}

///////////////////////////////////////////////////////////////////////////////
std::map<core::id::AtomID, core::id::AtomID>
setup_rmsd_atom_map( core::pose::Pose const & pose, utility::vector1< core::Size > rmsd_residues ) {

	using namespace core;
	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	std::map<core::id::AtomID, core::id::AtomID> atom_map_rms;

	// RNA heavy atom map
	for ( core::Size i = 1; i<= rmsd_residues.size(); ++i ) {
		core::conformation::Residue const & rsd = pose.residue( rmsd_residues[i] );

		for ( Size j = 1; j<=rsd.nheavyatoms(); ++j ) {
			std::string name( rsd.atom_name( j ) );
			if ( rsd.is_virtual(j) ) continue;
			id::AtomID const id1( rsd.atom_index( name ), rmsd_residues[ i ] );
			atom_map_rms[ id1 ] = id1;
		}
	}

	return atom_map_rms;
}

///////////////////////////////////////////////////////////////////////////////
utility::vector1< std::map< core::id::AtomID, core::id::AtomID > >
setup_per_residue_rmsd_atom_map( core::pose::Pose const & pose, utility::vector1< core::Size > rmsd_residues ) {

	using namespace core;
	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	utility::vector1< std::map< core::id::AtomID, core::id::AtomID > > per_residue_atom_map_rms;

	for ( core::Size i = 1; i<=rmsd_residues.size(); ++i ) {
		std::map< core::id::AtomID, core::id::AtomID > this_residue_atom_map_rms;

		core::conformation::Residue const & rsd = pose.residue( rmsd_residues[i] );

		for ( Size j = 1; j<=rsd.nheavyatoms(); ++j ) {
			std::string name( rsd.atom_name( j ) );
			if ( rsd.is_virtual( j ) ) continue;
			id::AtomID const id1( rsd.atom_index( name ), rmsd_residues[ i ] );
			this_residue_atom_map_rms[ id1 ] = id1;
		}
		per_residue_atom_map_rms.push_back( this_residue_atom_map_rms );
	}


	return per_residue_atom_map_rms;
}
///////////////////////////////////////////////////////////////////////////////
void
drrafter_error_estimation()
{
	using namespace core;
	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	using namespace core::chemical;
	using namespace core::scoring;
	using namespace core::import_pose::pose_stream;

	if ( option[ in::file::s ].user() && ( option[ sgroup1 ].user() || option[ sgroup2 ].user() ) ) {
		TR << "Error: please provide EITHER -s or -sgroup1 and -sgroup2." << std::endl;
		exit( 0 );
	}

	if ( option[ sgroup1 ].user() && !option[ sgroup2 ].user() ) {
		TR << "Error: please provide BOTH sgroup1 AND sgroup2, or instead provide -s." << std::endl;
		exit( 0 );
	}

	ResidueTypeSetCOP rsd_set;
	rsd_set = core::chemical::ChemicalManager::get_instance()->residue_type_set( FA_STANDARD );

	///////////////////////////////////////////////
	// input stream
	///////////////////////////////////////////////
	PoseInputStreamOP input, input_group1, input_group2;
	if ( option[ in::file::silent ].user() ) {
		if ( option[ in::file::tags ].user() ) {
			input = utility::pointer::make_shared< SilentFilePoseInputStream >(
				option[ in::file::silent ](),
				option[ in::file::tags ]()
			);
		} else {
			input = utility::pointer::make_shared< SilentFilePoseInputStream >( option[ in::file::silent ]() );
		}
	} else if ( option[ in::file::s ].user() ) {
		input = PoseInputStreamOP( new PDBPoseInputStream( option[ in::file::s ]() ) );
	} else {
		input_group1 = PoseInputStreamOP( new PDBPoseInputStream( option[ sgroup1 ]() ) );
		input_group2 = PoseInputStreamOP( new PDBPoseInputStream( option[ sgroup2 ]() ) );
	}

	///////////////////////////////////////////////
	// load all the poses
	///////////////////////////////////////////////

	pose::Pose pose;

	utility::vector1< pose::Pose > all_poses;
	utility::vector1< pose::Pose > all_poses_group1;
	utility::vector1< pose::Pose > all_poses_group2;
	utility::vector1< std::string > all_pose_tags;
	utility::vector1< std::string > all_pose_tags_group1;
	utility::vector1< std::string > all_pose_tags_group2;

	if ( option[ in::file::s ].user() ) {
		while ( input->has_another_pose() ) {

			input->fill_pose( pose, *rsd_set );

			// tag
			std::string tag = tag_from_pose( pose );

			all_poses.push_back( pose );
			all_pose_tags.push_back( tag );
		}
	} else if ( option[ sgroup1 ].user() ) {
		while ( input_group1->has_another_pose() ) {

			input_group1->fill_pose( pose, *rsd_set );

			std::string tag = tag_from_pose( pose );

			all_poses_group1.push_back( pose );
			all_pose_tags_group1.push_back( tag );
		}

		while ( input_group2->has_another_pose() ) {

			input_group2->fill_pose( pose, *rsd_set );

			std::string tag = tag_from_pose( pose );

			all_poses_group2.push_back( pose );
			all_pose_tags_group2.push_back( tag );
		}
	}

	if ( all_poses_group1.size() > 1 and all_poses.size() > 1 ) return;

	for ( core::Size i = 1; i <= all_poses_group1.size(); ++i ) {
		all_poses.push_back( all_poses_group1[ i ] );
		all_pose_tags.push_back( all_pose_tags_group1[ i ] );
	}

	for ( core::Size i = 1; i <= all_poses_group2.size(); ++i ) {
		all_poses.push_back( all_poses_group2[ i ] );
		all_pose_tags.push_back( all_pose_tags_group2[ i ] );
	}

	///////////////////////////////////////////////
	// check that all poses have the same sequence
	///////////////////////////////////////////////

	check_all_poses_have_the_same_sequence( all_poses );

	///////////////////////////////////////////////
	// set up the alignment and RMSD atom maps
	///////////////////////////////////////////////

	utility::vector1< core::Size > rmsd_residues;

	if ( option[ rna_rmsd ]() ) {
		for ( core::Size i =1; i<= pose.size(); ++i ) {
			if ( pose.residue( i ).is_RNA() ) {
				rmsd_residues.push_back( i );
			}
		}
	} else {
		rmsd_residues = option[ rmsd_res ]();
	}

	core::id::AtomID_Map< id::AtomID > align_atom_map;
	align_atom_map = setup_align_atom_map( all_poses[1] );
	std::map<core::id::AtomID, core::id::AtomID> atom_map_rms;
	atom_map_rms = setup_rmsd_atom_map( all_poses[1], rmsd_residues );

	// per residue RMSD map
	utility::vector1< std::map< core::id::AtomID, core::id::AtomID > > per_residue_atom_map_rms;
	if ( option[ per_residue_convergence ]() ) {
		per_residue_atom_map_rms = setup_per_residue_rmsd_atom_map( all_poses[1], rmsd_residues );
	}

	///////////////////////////////////////////////
	// calculate all the pairwise RMSDs
	///////////////////////////////////////////////

	std::map< std::string, std::map< std::string, Real > > pairwise_rmsds;
	utility::vector1< Real > pairwise_rmsds_list;
	utility::vector1< utility::vector1< Real > > per_residue_pairwise_rmsds_lists;

	std::map< std::string, std::map< std::string, Real > > pairwise_rmsds_groups;
	utility::vector1< Real > pairwise_rmsds_list_groups;
	utility::vector1< utility::vector1< Real > > per_residue_pairwise_rmsds_lists_groups;

	if ( option[ per_residue_convergence ]() ) {
		for ( core::Size i = 1; i <= per_residue_atom_map_rms.size(); ++i ) {
			utility::vector1< Real > empty_rmsd_list;
			per_residue_pairwise_rmsds_lists.push_back( empty_rmsd_list );
			per_residue_pairwise_rmsds_lists_groups.push_back( empty_rmsd_list );
		}
	}

	for ( core::Size i = 1; i <= all_pose_tags.size(); ++i ) {
		std::string tag1 = all_pose_tags[ i ];
		for ( core::Size j = i+1; j <= all_pose_tags.size(); ++j ) {
			std::string tag2 = all_pose_tags[ j ];

			// get the RMSD
			Real align_rmsd( 0.0 );
			Real rmsd;
			if ( !option[ rmsd_nosuper ]() ) {
				align_rmsd = superimpose_pose( all_poses[i], all_poses[j], align_atom_map );
			}

			rmsd = rms_at_corresponding_atoms_no_super( all_poses[i], all_poses[j], atom_map_rms );
			TR.Debug << "Align " << i << ", " << j << " " << align_rmsd << std::endl;
			TR.Debug << "RMSD " << tag1 << ", " << tag2 << " " << rmsd << std::endl;

			pairwise_rmsds[ tag1 ][ tag2 ] = rmsd;
			pairwise_rmsds[ tag2 ][ tag1 ] = rmsd;
			pairwise_rmsds_list.push_back( rmsd );

			// get the per residue RMSDs
			if ( option[ per_residue_convergence ]() ) {
				for ( core::Size r=1; r<=per_residue_atom_map_rms.size(); ++r ) {
					Real residue_rmsd( 0.0 );
					residue_rmsd = rms_at_corresponding_atoms_no_super( all_poses[i], all_poses[j], per_residue_atom_map_rms[r] );
					//std::cout << i << " " << j << " " << r << " " << residue_rmsd << std::endl;
					per_residue_pairwise_rmsds_lists[ r ].push_back( residue_rmsd );
				}
			}

		}
	}

	///////////////////////////////////////////////
	// calculate all the pairwise RMSDs between groups, if specified by user
	///////////////////////////////////////////////

	for ( core::Size i = 1; i <= all_pose_tags_group1.size(); ++i ) {
		std::string tag1 = all_pose_tags_group1[ i ];
		for ( core::Size j = 1; j <= all_pose_tags_group2.size(); ++j ) {
			std::string tag2 = all_pose_tags_group2[ j ];

			// get the RMSD
			Real align_rmsd( 0.0 );
			Real rmsd;
			if ( !option[ rmsd_nosuper ]() ) {
				align_rmsd = superimpose_pose( all_poses_group1[i], all_poses_group2[j], align_atom_map );
			}

			rmsd = rms_at_corresponding_atoms_no_super( all_poses_group1[i], all_poses_group2[j], atom_map_rms );
			TR.Debug << "Align " << i << ", " << j << " " << align_rmsd << std::endl;
			TR.Debug << "RMSD " << tag1 << ", " << tag2 << " " << rmsd << std::endl;

			pairwise_rmsds_groups[ tag1 ][ tag2 ] = rmsd;
			pairwise_rmsds_groups[ tag2 ][ tag1 ] = rmsd;
			pairwise_rmsds_list_groups.push_back( rmsd );

			// get the per residue RMSDs
			if ( option[ per_residue_convergence ]() ) {
				for ( core::Size r=1; r<=per_residue_atom_map_rms.size(); ++r ) {
					Real residue_rmsd( 0.0 );
					residue_rmsd = rms_at_corresponding_atoms_no_super( all_poses_group1[i], all_poses_group2[j], per_residue_atom_map_rms[r] );
					//std::cout << i << " " << j << " " << r << " " << residue_rmsd << std::endl;
					per_residue_pairwise_rmsds_lists_groups[ r ].push_back( residue_rmsd );
				}
			}

		}
	}

	///////////////////////////////////////////////
	// get convergence RMSD
	///////////////////////////////////////////////

	Real mean_pairwise_rmsd = mean( pairwise_rmsds_list );
	TR.Debug << "Mean pairwise rmsd: " << mean_pairwise_rmsd << std::endl;

	Real mean_pairwise_rmsd_groups = mean( pairwise_rmsds_list_groups );

	utility::vector1< Real > mean_per_residue_rmsds;
	if ( option[ per_residue_convergence ]() ) {
		for ( core::Size r=1; r<=per_residue_pairwise_rmsds_lists.size(); ++r ) {
			mean_per_residue_rmsds.push_back( mean( per_residue_pairwise_rmsds_lists[ r ] ) );
		}
	}

	utility::vector1< Real > mean_per_residue_rmsds_groups;
	if ( option[ per_residue_convergence ]() ) {
		for ( core::Size r=1; r<=per_residue_pairwise_rmsds_lists_groups.size(); ++r ) {
			mean_per_residue_rmsds_groups.push_back( mean( per_residue_pairwise_rmsds_lists_groups[ r ] ) );
		}
	}

	///////////////////////////////////////////////
	// get error estimates from convergence RMSD
	///////////////////////////////////////////////

	Real estimated_min_rmsd, estimated_mean_rmsd, estimated_rmsd_median;

	estimated_min_rmsd = estimate_min_rmsd( mean_pairwise_rmsd );
	estimated_mean_rmsd = estimate_mean_rmsd( mean_pairwise_rmsd );
	estimated_rmsd_median = estimate_rmsd_of_median_struct( mean_pairwise_rmsd );

	///////////////////////////////////////////////
	// get the median structure
	///////////////////////////////////////////////

	std::string median_struct = get_median_structure( all_pose_tags, pairwise_rmsds );

	///////////////////////////////////////////////
	// print out all results
	///////////////////////////////////////////////

	TR << "#############################################" << std::endl;
	TR << "Mean pairwise RMSD (convergence): " << mean_pairwise_rmsd << std::endl;
	if ( option[ sgroup1 ].user() ) {
		TR << "Group vs group mean pairwise RMSD (convergence): " << mean_pairwise_rmsd_groups << std::endl;
	}
	TR << "Estimated minimum RMSD: " << estimated_min_rmsd << std::endl;
	TR << "Estimated mean RMSD: " << estimated_mean_rmsd << std::endl;
	TR << "Estimated RMSD of median structure: " << estimated_rmsd_median << std::endl;
	TR << "Median structure: " << median_struct << std::endl;
	if ( option[ per_residue_convergence ]() ) {
		TR << "Mean pairwise RMSDs (convergence) per residue: " << std::endl;
		core::pose::PDBInfoOP pdbinfo = all_poses[1].pdb_info();
		for ( core::Size i=1; i<= mean_per_residue_rmsds.size(); ++i ) {
			TR << "Residue " << pdbinfo->number( rmsd_residues[i] ) << " : " << mean_per_residue_rmsds[i] << std::endl;
		}
		if ( option[ sgroup1 ].user() ) {
			TR << "Group vs group mean pairwise RMSDs (convergence) per residue: " << std::endl;
			for ( core::Size i=1; i<= mean_per_residue_rmsds_groups.size(); ++i ) {
				TR << "Group vs group Residue " << pdbinfo->number( rmsd_residues[i] ) << " : " << mean_per_residue_rmsds_groups[i] << std::endl;
			}
		}
	}
	TR << "#############################################" << std::endl;

}


///////////////////////////////////////////////////////////////
void*
my_main( void* )
{
	drrafter_error_estimation();

	protocols::viewer::clear_conformation_viewers();
	exit( 0 );
}


///////////////////////////////////////////////////////////////////////////////
int
main( int argc, char * argv [] )
{
	try {
		using namespace basic::options;
		using namespace core::scoring;
		using namespace basic::options::OptionKeys;

		option.add_relevant( in::file::s );
		option.add_relevant( in::file::silent );
		option.add_relevant( in::file::tags );
		NEW_OPT( sgroup1, "structures group 1, e.g. built into half map 1", "");
		NEW_OPT( sgroup2, "structures group 2, e.g. built into half map 2", "");
		NEW_OPT( rmsd_nosuper, "Calculate rmsd without superposition first", false);
		NEW_OPT( rna_rmsd, "Calculate rmsd over rna", true);
		NEW_OPT( protein_align, "align structures over protein residues", true);
		NEW_OPT( per_residue_convergence, "get the convergence per residue", false);
		NEW_OPT( rmsd_res, "residues to calculate rmsd for", 1);
		NEW_OPT( align_residues, "residues to align over, default all", 1);

		////////////////////////////////////////////////////////////////////////////
		// setup
		////////////////////////////////////////////////////////////////////////////
		core::init::init(argc, argv);

		////////////////////////////////////////////////////////////////////////////
		// end of setup
		////////////////////////////////////////////////////////////////////////////
		protocols::viewer::viewer_main( my_main );
	} catch (utility::excn::Exception const & e ) {
		e.display();
		return -1;
	}
}
