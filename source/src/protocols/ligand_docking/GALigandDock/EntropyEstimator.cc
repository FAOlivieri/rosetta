// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/ligand_docking/EntropyEstimator.cc
///
/// @brief
/// @author Hahnbeom Park and Frank DiMaio

#include <protocols/ligand_docking/GALigandDock/EntropyEstimator.hh>

#include <core/scoring/ScoreFunction.hh>
#include <core/chemical/AA.hh>
#include <core/chemical/ResidueType.hh>
#include <core/conformation/Residue.hh>
#include <core/conformation/Conformation.hh>
#include <core/pose/Pose.hh>
#include <core/pose/util.hh>
#include <protocols/minimization_packing/MinMover.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/kinematics/MoveMap.hh>

#include <ObjexxFCL/format.hh>
#include <basic/Tracer.hh>
#include <numeric/random/random.hh>
#include <cmath>
#include <utility/vector1.hh>

namespace protocols {
namespace ligand_docking {
namespace ga_ligand_dock {

static basic::Tracer TR( "protocols.ligand_docking.GALigandDock.EntropyEstimator" );

using namespace ObjexxFCL::format;

EntropyEstimator::EntropyEstimator( core::scoring::ScoreFunctionOP sfxn,
	core::pose::Pose const & pose,
	utility::vector1< core::Size > const &ligids,
	std::string method
) :
	method_(method),
	ligids_( ligids ),
	sfxn_( sfxn ),
	run_apostate_( false ), // by default free-ligand simulation only
	run_holostate_( false ),
	P_randomize_( 0.2 ),
	maxpert_( 120.0 ),
	minimize_( true ),
	sample_every_iter_( 10 ),
	temp_i_( 300.0 ),
	temp_f_( 300.0 ),
	niter_( 3000 ),
	iter_collect_begin_( 500 ),
	wRG_( 1.0 ),
	wtors_( 1.0 ),
	weighted_( true ),
	jumpid_( 0 )
{
	get_chi_weight( pose );

	// get ligand jump id
	utility::vector1< core::kinematics::Edge > jumps = pose.fold_tree().get_jump_edges();
	for ( auto j : jumps ) {
		core::Size up = j.start(), down = j.stop();
		if ( std::find( ligids_.begin(), ligids_.end(), up) == ligids_.end()
				&& std::find( ligids_.begin(), ligids_.end(), down) != ligids_.end()
				) {
			runtime_assert(jumpid_==0);
			jumpid_ = j.label();
		}
	}
	runtime_assert(jumpid_!=0);
}


void
EntropyEstimator::get_chi_weight( core::pose::Pose const &pose_ref ) {
	// return the weight for a torsion based on # atoms moved by that torsion
	auto chiWeight = [this](core::pose::Pose const &p0, core::pose::Pose const &p1) -> core::Real {
		core::Size nchange_upon_chi = 0, nheavy= 0;
		for ( core::Size i=1; i<=ligids_.size(); ++i ) {
		nheavy += p0.residue(ligids_[i]).nheavyatoms();
		for ( core::Size jatm = 1; jatm <= p0.residue(i).nheavyatoms(); ++jatm ) {
		core::Vector const &xyz_ref = p0.residue(ligids_[i]).xyz(jatm);
		core::Vector const &xyz     = p1.residue(ligids_[i]).xyz(jatm);
		core::Real d2 = xyz.distance_squared(xyz_ref);
		if ( d2 > 1.0 ) nchange_upon_chi++;
		}
		}
		nchange_upon_chi = std::min( nchange_upon_chi, nheavy-nchange_upon_chi);
		return std::log(core::Real(nchange_upon_chi+1));
		};

	for ( core::Size i=1; i<= ligids_.size(); ++i ) {
		if ( pose_ref.residue(ligids_[i]).is_protein() ) {
			if ( !pose_ref.residue(ligids_[i]).is_lower_terminus() ) {
				chimapping_.push_back( std::make_pair(ligids_[i],-1) ); // phi
				ligpose_chimapping_.push_back( std::make_pair(i,-1) );
			}
			if ( !pose_ref.residue(ligids_[i]).is_upper_terminus() ) {
				chimapping_.push_back( std::make_pair(ligids_[i],-2) ); // psi
				chimapping_.push_back( std::make_pair(ligids_[i],-3) ); //omega
				ligpose_chimapping_.push_back( std::make_pair(i,-2) );
				ligpose_chimapping_.push_back( std::make_pair(i,-3) );
			}
			core::Size nchi = pose_ref.residue(ligids_[i]).nchi();
			for ( core::Size j=1; j<=nchi; ++j ) {
				chimapping_.push_back( std::make_pair(ligids_[i],j) ); //sidechain chis
				ligpose_chimapping_.push_back( std::make_pair(i, j) );
			}
		} else {
			core::Size nchi = pose_ref.residue(ligids_[i]).nchi();
			for ( core::Size j=1; j<=nchi; ++j ) {
				chimapping_.push_back( std::make_pair(ligids_[i],j) ); //chis
				ligpose_chimapping_.push_back( std::make_pair(i, j) );
			}
		}
	}

	core::Size nchis = chimapping_.size();

	if ( !weighted_ ) {
		chiweights_ = utility::vector1<core::Real> ( nchis, 1.0 );
		return;
	}

	for ( auto chi : chimapping_ ) {
		core::pose::Pose pose = pose_ref;
		if ( chi.second == -1 ) {
			core::Real phi = pose.phi(chi.first)+180.0;
			pose.set_phi(chi.first, phi);
		} else if ( chi.second == -2 ) {
			core::Real psi = pose.psi(chi.first)+180.0;
			pose.set_psi(chi.first, psi);
		} else if ( chi.second == -3 ) {
			core::Real omega = pose.omega(chi.first)+180.0;
			pose.set_omega(chi.first, omega);
		} else {
			core::Real ichi = pose.residue(chi.first).chi(chi.second)+180.0;
			pose.set_chi( chi.second, chi.first, ichi );
		}
		chiweights_.push_back( chiWeight(pose, pose_ref) );
	}
}

core::Real
EntropyEstimator::MCEntropy( core::pose::Pose const &pose ) const {
	std::string est_target( "ligand" );
	if ( run_apostate_ ) est_target += ",apo-receptor";
	if ( run_holostate_ ) est_target += ",complex";

	TR << "Running dG estimation on " << est_target
		<< "; from " << niter_ << " MC simulation..." << std::endl;

	utility::vector1< core::Size > flexscs;
	utility::vector1< std::pair< core::Size, core::Size > > chidefs;

	// ligand-only simulation
	core::pose::PoseOP ligpose( new core::pose::Pose );
	ligpose->append_residue_by_jump( pose.residue(ligids_[1]), 0 );
	for ( core::Size i=2; i<=ligids_.size(); ++i ) {
		ligpose->append_residue_by_bond( pose.residue(ligids_[i]) );
	}
	core::pose::addVirtualResAsRoot(*ligpose);
	ligpose->conformation().detect_disulfides();

	utility::vector1< ChiInfo > chitrj = setup_trj( pose, flexscs, chidefs );

	// Below is acturally free energy; log(10.0) is just constant offset
	core::Real dGrg_lig = 2.92*std::log(ligpose->residue(1).type().mass()-log(10.0));

	// Ligand-only: pass empty vectors as flexsc & chidefs
	core::Real Stors_lig( 0.0 ), Stors_rec( 0.0 ), Stors_complex( 0.0 );
	Stors_lig = estimate_Stors( *ligpose, chitrj, utility::vector1<core::Size>(),
		utility::vector1< std::pair< core::Size, core::Size > >(),
		true, false );

	// holo-state receptor simulation
	// I haven't tested if this works...
	if ( run_holostate_ ) {
		TR << "Running holo-state simulation " << std::endl;
		chitrj.resize( 1 ); // keep the first one only
		core::pose::PoseOP pose_work( new core::pose::Pose );
		*pose_work = pose;
		Stors_complex = estimate_Stors( *pose_work, chitrj, flexscs, chidefs, true, true );
	}

	// apo-state receptor simulation
	if ( run_apostate_ ) {
		TR << "Running ligand-free simulation " << std::endl;
		core::pose::PoseOP pose_work( new core::pose::Pose );
		*pose_work = pose;

		for ( auto ligid = ligids_.rbegin(); ligid != ligids_.rend(); ++ligid ) {
			pose_work->delete_residue_slow( *ligid );
		}

		chitrj.resize( 1 ); // keep the first one only
		chitrj[1].ligchis.resize( 0 ); // no ligand chi
		Stors_rec = estimate_Stors( *pose_work, chitrj, flexscs, chidefs, false, true );
	}

	core::Real RT = 0.00198588*temp_f_;
	core::Real Ssum = 0.5*RT*wtors_*(Stors_lig + Stors_rec + Stors_complex) + wRG_*dGrg_lig;

	TR << "Estimated entropic contributions to freenergy: " << std::endl;
	TR << "From LigandRG: " << wRG_*dGrg_lig << std::endl;
	TR << "From LigandTors: " << 0.5*RT*wtors_*Stors_lig << std::endl;
	TR << "From ReceptorTors: " << 0.5*RT*wtors_*Stors_rec << std::endl;
	TR << "From ComplexTors: " << 0.5*RT*wtors_*Stors_complex << std::endl;

	return Ssum; // free energy in kcal/mol
}

core::Real
EntropyEstimator::SimpleEntropy( core::pose::Pose const &pose ) const {
	TR << "Using simple entropy estimation method: S = wchi*nchi " << std::endl;
	core::Real Stot(0.0);
	core::Size nchi(0);
	core::Real w_nchi(0.4); // eventually move to class variables
	for ( auto ligid:ligids_ ) {
		nchi += pose.residue(ligid).nchi();
		for ( core::Size ichi=1; ichi<=pose.residue(ligid).nchi(); ++ichi ) {
			if ( pose.residue(ligid).type().is_proton_chi(ichi) ) {
				nchi -=1;
			}
		}
	}

	TR << "Entropy from rotors: " << w_nchi*nchi << std::endl;
	Stot = w_nchi*nchi;
	return Stot;
}

core::Real
EntropyEstimator::apply( core::pose::Pose const &pose ) const {
	TR << "Entropy method is " << method_ << std::endl;
	if ( method_ == "MCEntropy" ) return MCEntropy(pose);
	else if ( method_ == "Simple" ) return SimpleEntropy(pose);
	else return 0.0;
}

core::Real
EntropyEstimator::analyze_trajectory(
	utility::vector1< ChiInfo > const &chitrj,
	core::pose::Pose const &pose,
	utility::vector1< std::pair< core::Size, core::Size > > const &chidefs,
	core::Real const Emin,
	core::Real const RT,
	bool const run_on_ligand,
	bool const run_on_receptor
) const {
	if ( chitrj.size() < 10 ) return 0.0;

	core::Size const nchi( chitrj[1].ligchis.size() );
	core::Size ncount( 0 );

	// Ligand part
	utility::vector1< utility::vector1< core::Real > > count_per_chibin;
	std::map< core::Size, utility::vector1< core::Size > > rotbin_at_res;
	utility::vector1< utility::vector1< core::Real > > chivals( nchi );

	core::Size const nbin(6);
	core::Real const binang( 360.0/nbin );

	if ( run_on_ligand ) {
		runtime_assert( chimapping_.size() == nchi );
		count_per_chibin.resize( nchi );

		for ( core::Size ichi = 1; ichi <= nchi; ++ichi ) {
			//bin into 6; 60 deg
			count_per_chibin[ichi] = utility::vector1< core::Real >( nbin, 0.0 );
		}

		// receptor
		for ( core::Size idef = 1; idef <= chidefs.size(); ++idef ) {
			core::Size const &ires= chidefs[idef].first;
			utility::vector1< core::Size > const &chis = chitrj[1].recchis.at(ires);

			auto it = rotbin_at_res.find(ires);
			if ( it == rotbin_at_res.end() ) {
				core::Size nbins = std::pow( 3, chis.size() );
				rotbin_at_res[ires] = utility::vector1< core::Size >( nbins, 0 );
			}
		}
	}

	for ( core::Size itrj = 1; itrj <= chitrj.size(); ++itrj ) {
		ChiInfo const &trj = chitrj[itrj];
		core::Real Boltz = exp(-(trj.E - Emin)/RT);
		if ( Boltz < 0.001 ) continue; //drop if dE ~ 2.8

		ncount++;

		// ligand; will skip here if no ligand exist (i.e. nchi == 0)
		for ( core::Size ichi = 1; ichi <= nchi; ++ichi ) {
			utility::vector1< core::Real > &frac = count_per_chibin[ichi];

			core::Real chi = trj.ligchis[ichi];
			if ( chi < 0 ) chi += 360.0;
			if ( chi > 360.0 ) chi -= 360.0;
			// 30~90 goes to 2
			core::Size ibin = std::floor((chi+0.5*binang)/binang)+1;
			if ( ibin == nbin+1 ) ibin = 1;
			if ( ibin > nbin or ibin == 0 ) continue; //why happening?
			chivals[ichi].push_back(chi); //detailed
			frac[ibin] += 1.0;
		}

		// receptor
		if ( run_on_receptor ) {
			for ( auto it = trj.recchis.begin(); it != trj.recchis.end(); ++it ) {
				core::Size const &ires = it->first;
				utility::vector1< core::Real > const &chis = trj.recchis.at(ires);
				core::Size rotid = chis2rotid( chis );
				if ( rotid > rotbin_at_res[ires].size() || rotid < 1 ) {
					TR.Debug << "WARNING! " << ires << " " << rotid << ":" << rotbin_at_res[ires].size() << " ? "
						<< chis.size() << " " << chis[1] << " " << chis[2] << std::endl;
					continue;
				}
				rotbin_at_res[ires][rotid]++;
			}
		}
	}

	// ligand; will skip here if no ligand exist (i.e. nchi == 0)
	core::Real Ssum( 0.0 );
	TR << "Estimate per-torsion contribution to Stors" << std::endl;
	for ( core::Size ichi = 1; ichi <= nchi; ++ichi ) {
		auto chiidx = (run_on_ligand && !run_on_receptor) ? ligpose_chimapping_[ichi] : chimapping_[ichi];

		if ( chiidx.second >0 && pose.residue_type(chiidx.first).is_proton_chi( chiidx.second ) ) {
			TR << "Res " << chiidx.first << "Chi " << chiidx.second << ", skip" << std::endl;
		} else {
			utility::vector1< core::Real > Pbins( nbin, 0.0 );
			for ( core::Size ibin = 1; ibin <= nbin; ++ibin ) {
				Pbins[ibin] = count_per_chibin[ichi][ibin]/ncount;
			}
			Pbins.push_back( Pbins[1] ); // periodic

			core::Real Schi( 0.0 );
			utility::vector1< core::Real > Pcorrected( nbin, 0.0 );
			Pcorrected[1] = Pbins[nbin]+Pbins[1]+Pbins[2];
			for ( core::Size ibin = 2; ibin <= nbin; ++ibin ) {
				Pcorrected[ibin] = Pbins[ibin-1]+Pbins[ibin]+Pbins[ibin+1];
			}
			core::Real Psum( 0.0 ), PlogP( 0.0 );
			for ( core::Size ibin = 1; ibin <= nbin; ++ibin ) Psum += Pcorrected[ibin];
			for ( core::Size ibin = 1; ibin <= nbin; ++ibin ) Pcorrected[ibin] /= Psum;
			for ( core::Size ibin = 1; ibin <= nbin; ++ibin ) {
				PlogP -= Pcorrected[ibin]*std::log(Pcorrected[ibin]+1.e-6);
			}
			Schi = PlogP*chiweights_[ichi];

			TR << "Chi " << ichi
				<< " " << std::setw(4) << chiweights_[ichi] << " |";
			for ( core::Size ibin = 1; ibin <= nbin; ++ibin ) {
				TR << " " << F(5,1,count_per_chibin[ichi][ibin]*100.0/ncount);
			}
			TR << ", Schi/0.5RT: " << F(8,3,Schi) << std::endl;

			Ssum += Schi;
		}
	}

	// receptor part; will skip here if rotbin_at_res is empty
	for ( auto it = rotbin_at_res.begin(); it != rotbin_at_res.end(); ++it ) {
		core::Size const &ires = it->first;
		utility::vector1< core::Real > const &counts = it->second;
		core::Real nrots = counts.size();

		TR << "ROT " << I(3,int(ires)) << " " << pose.residue(ires).name().c_str() << " " << I(3,int(nrots));
		core::Real S( 0.0 );
		for ( core::Size irot = 1; irot <= nrots; ++irot ) {
			if ( counts[irot] > 0 ) {
				core::Real P = counts[irot]/ncount;
				TR << " " << I(2,int(irot)) << " " << F(5,1,P*100.0);
				S -= P*log(P);
			}
		}
		TR << std::endl;
		Ssum += S;
	}

	return Ssum;
}

core::Real
EntropyEstimator::estimate_Stors(
	core::pose::Pose pose,
	utility::vector1< ChiInfo > &chitrj,
	utility::vector1< core::Size > const &flexscs,
	utility::vector1< std::pair< core::Size, core::Size > > const &chidefs,
	bool const run_on_ligand,
	bool const run_on_receptor
) const {
	ChiInfo currentchi = chitrj[1];
	core::Size const nligchi( currentchi.ligchis.size() );

	// mm for minmover
	core::kinematics::MoveMapOP mm( new core::kinematics::MoveMap );
	mm->set_bb( false ); mm->set_chi( false ); mm->set_jump( false );
	if ( run_on_ligand && !run_on_receptor ) { // ligand-only
		mm->set_bb( true );
		mm->set_chi( true );
	} else if ( run_on_receptor ) {
		for ( auto res : flexscs ) {
			mm->set_chi( res, true );
		}
		if ( run_on_ligand ) {
			mm->set_jump( jumpid_, true );
			for ( auto res : ligids_ ) {
				mm->set_bb( res, true );
				mm->set_chi( res, true );
			}
		}
	}

	// minmover
	protocols::minimization_packing::MinMoverOP min_mover =
		protocols::minimization_packing::MinMoverOP
		( new protocols::minimization_packing::MinMover( mm, sfxn_, "linmin", 0.001, true ) );
	//min_mover->max_iter( 30 );  // fd: linmin is always just 1 step...

	// skip MC if there is only "rigid" ligand
	if ( run_on_ligand && run_on_receptor && nligchi == 0 && ligids_.size() == 1 && flexscs.size() == 0 ) {
		return 0.0;
	} else if ( run_on_ligand && !run_on_receptor && nligchi == 0 && ligids_.size() == 1 ) {
		return 0.0;
	} else if ( !run_on_ligand && run_on_receptor && flexscs.size() == 0 ) {
		return 0.0;
	} else if ( !run_on_ligand && !run_on_receptor ) {
		return 0.0;
	}

	// MC initialize
	core::Size const nsamples( niter_/sample_every_iter_ );
	utility::vector1< core::Real > Etrj( nsamples, 0.0 );

	core::Real Eprv = sfxn_->score( pose );

	core::pose::Pose pose_min = pose;
	core::Real Emin( 1e6 );
	core::Real RT(0.0);

	for ( core::Size it = 1; it <= niter_; ++it ) {
		core::Real const temp( get_temperature(it) );
		RT = 0.00198588*temp;
		core::pose::Pose pose_prv( pose );

		bool pertlig=true;
		perturb( pose, nligchi, flexscs, pertlig );

		if ( minimize_ ) {
			min_mover->apply( pose );
		}

		core::Real E = sfxn_->score( pose );

		core::Real Boltz = exp(-(E-Eprv)/RT);
		core::Real rannum( numeric::random::rg().uniform() );
		if ( Boltz > rannum ) { // accept
			Eprv = E;
			currentchi.E = E;

			if ( pertlig ) {
				update_chis( pose, currentchi.ligchis, true ); //this pose should be ligand only pose
			} else {
				update_flexscs( pose, chidefs, currentchi.recchis );
			}
			if ( E < Emin ) {
				Emin = E;
				pose_min = pose;
			}
		} else {
			pose = pose_prv;
		}

		if ( it%sample_every_iter_ == 0 && it > iter_collect_begin_ ) {
			chitrj.push_back( currentchi );
		}
	}

	// analyze Nchi rotable
	core::Real Stors = analyze_trajectory(
		chitrj, pose, chidefs, Emin, RT,
		run_on_ligand, run_on_receptor );

	return Stors;
}

void
EntropyEstimator::perturb(
	core::pose::Pose &pose,
	core::Size const nligchi,
	utility::vector1< core::Size > const &flexscs,
	bool &pert_ligand
) const
{
	// gz: if there is nothing to perturb, this function
	// shoudn't be called. throw exception
	runtime_assert( nligchi >= 1 || flexscs.size() >= 1 );
	// ligand part
	if ( nligchi >= 1 && flexscs.size() >= 1 ) {
		core::Real Plig = core::Real(nligchi)/core::Real(nligchi+2*flexscs.size()); // assume average 2 chis per aa
		pert_ligand = (numeric::random::rg().uniform() < Plig ? true : false );
	} else if ( flexscs.size() >= 1 ) {
		pert_ligand = false;
	} //otherwise pert ligand

	std::pair<int,int> chiidx;
	if ( pert_ligand ) {
		int ichi = numeric::random::rg().random_range( 1, nligchi );
		//chi = pose.residue(ligid).chi(ichi);
		chiidx = ligpose_chimapping_[ichi];
	} else {
		int ires = numeric::random::rg().random_range( 1, flexscs.size() );
		int ichi = numeric::random::rg().random_range( 1, pose.residue( flexscs[ires] ).nchi() );
		chiidx = std::make_pair((int)flexscs[ires], ichi);
	}

	core::Real chi = 0.0;
	if ( chiidx.second == -1 ) {
		chi = pose.phi(chiidx.first);
	} else if ( chiidx.second == -2 ) {
		chi = pose.psi(chiidx.first);
	} else if ( chiidx.second == -3 ) {
		chi = pose.omega(chiidx.first);
	} else {
		chi = pose.chi(chiidx.second, chiidx.first);
	}

	if ( numeric::random::rg().uniform() < P_randomize_ ) {
		// complete randomization
		chi = 360.0 * numeric::random::rg().uniform();
	} else {
		core::Real delta = maxpert_*(1.0 - 2.0*numeric::random::rg().uniform());
		chi += delta;
		if ( chi > 360.0 ) chi -= 360.0;
		if ( chi < 0.0 ) chi += 360.0;
	}

	// make it within range
	if ( chiidx.second == -1 ) {
		pose.set_phi(chiidx.first, chi);
	} else if ( chiidx.second == -2 ) {
		pose.set_psi(chiidx.first, chi);
	} else if ( chiidx.second == -3 ) {
		pose.set_omega(chiidx.first, chi);
	} else {
		pose.set_chi(chiidx.second, chiidx.first, chi);
	}
}

utility::vector1< ChiInfo >
EntropyEstimator::setup_trj(
	core::pose::Pose const &pose,
	utility::vector1< core::Size > &flexscs,
	utility::vector1< std::pair< core::Size, core::Size > > &chidefs
) const {
	utility::vector1< ChiInfo > chitrj;

	// dof setup
	core::Size const nchi( chimapping_.size() );

	// get flexsc info
	if ( ligids_.size() > 1 || ligids_[1] != 1 ) {
		flexscs = get_contacting_reslist( pose, chidefs );
	}

	// setup template chitrj
	ChiInfo currentchi;
	currentchi.ligchis.resize( nchi );
	for ( core::Size ichidef = 1; ichidef <= chidefs.size(); ++ichidef ) {
		core::Size ires = chidefs[ichidef].first;
		core::Size ichi = chidefs[ichidef].second;
		auto it = currentchi.recchis.find( ires );
		if ( it == currentchi.recchis.end() ) currentchi.recchis[ires] = utility::vector1< core::Real >();
		it = currentchi.recchis.find( ires );
		if ( it->second.size() < ichi ) currentchi.recchis[ires].resize( ichi );
	}

	// initialize trj info
	update_chis( pose, currentchi.ligchis, false ); //this pose should be rec+ligand
	if ( currentchi.recchis.size() > 0 ) {
		update_flexscs( pose, chidefs, currentchi.recchis );
	}

	chitrj.push_back( currentchi );
	return chitrj;
}

utility::vector1< core::Size >
EntropyEstimator::get_contacting_reslist(
	core::pose::Pose const &pose,
	utility::vector1< std::pair< core::Size, core::Size > > &chidefs
) const {
	core::Real const D2_COARSE( 225.0 );
	core::Real const D2_FINE( 4.0*4.0 );

	utility::vector1< core::Size > flexscs;

	//core::Vector const &ligcom = pose.residue(ligid_).nbr_atom_xyz();
	utility::vector1< core::Vector > ligCOMs;
	for ( auto ligid : ligids_ ) {
		ligCOMs.push_back ( pose.residue(ligid).nbr_atom_xyz() );
	}
	utility::vector1< bool > is_close( pose.size(), false );
	for ( core::Size ires = 1; ires <= pose.size(); ++ires ) {
		if ( !pose.residue(ires).is_protein() ) continue;
		if ( pose.residue(ires).aa() == core::chemical::aa_gly ||
				pose.residue(ires).aa() == core::chemical::aa_ala ||
				pose.residue(ires).aa() == core::chemical::aa_pro ) continue;

		core::Vector const & resnbr = pose.residue(ires).nbr_atom_xyz();

		core::Real d2min = D2_COARSE+1;
		for ( auto com : ligCOMs ) {
			d2min = std::min( d2min, com.distance_squared( resnbr ) );
		}
		if ( d2min < D2_COARSE ) is_close[ires] = true;
	}

	for ( core::Size ires = 1; ires <= pose.size(); ++ires ) {
		if ( !is_close[ires] ) continue;
		if ( std::find( ligids_.begin(), ligids_.end(), ires ) != ligids_.end() ) continue;

		bool i_is_contacting = false;
		for ( core::Size iatm = 1; iatm <= pose.residue(ires).nheavyatoms(); ++iatm ) {
			if ( pose.residue(ires).atom_is_backbone(iatm) ) continue;
			core::Vector const &xyz_i = pose.residue(ires).xyz(iatm);

			for ( auto ligid : ligids_ ) {
				for ( core::Size jatm = 1; jatm <= pose.residue(ligid).nheavyatoms(); ++jatm ) {
					core::Vector const &xyz_j = pose.residue(ligid).xyz(jatm);

					core::Real d2 = xyz_i.distance_squared( xyz_j );
					if ( d2 < D2_FINE ) {
						i_is_contacting = true;
						break;
					}
					if ( i_is_contacting ) break;
				}
				if ( i_is_contacting ) break;
			}
			if ( i_is_contacting ) break;
		} // iatm

		if ( i_is_contacting ) {
			flexscs.push_back( ires );
			for ( core::Size ichi = 1; ichi <= pose.residue(ires).nchi(); ++ichi ) {
				if ( !pose.residue(ires).type().is_proton_chi(ichi)  ) {
					chidefs.push_back( std::make_pair( ires, ichi ) );
					TR.Debug << "chidef: " << chidefs.size() << " " << ires << " " << ichi <<std::endl;
				}
			}
		}
	} // ires

	return flexscs;
}

core::Real
EntropyEstimator::get_temperature( core::Size const it ) const
{
	core::Size nhalf( niter_/2 );
	if ( it > nhalf ) return temp_f_;

	core::Real f = core::Real(it)/core::Real(nhalf);
	return temp_i_ + f*(temp_f_-temp_i_);
}

void
EntropyEstimator::update_chis(
	core::pose::Pose const &pose,
	utility::vector1< core::Real > &chis,
	bool const& ligonly
) const {
	runtime_assert ( chis.size() == chimapping_.size() );
	if ( ligonly ) {
		for ( core::Size i=1; i<=chis.size(); ++i ) {
			if ( ligpose_chimapping_[i].second == -1 ) {
				chis[i] = pose.phi(ligpose_chimapping_[i].first);
			} else if ( ligpose_chimapping_[i].second == -2 ) {
				chis[i] = pose.psi(ligpose_chimapping_[i].first);
			} else if ( ligpose_chimapping_[i].second == -3 ) {
				chis[i] = pose.omega(ligpose_chimapping_[i].first);
			} else {
				chis[i] = pose.chi(ligpose_chimapping_[i].second, ligpose_chimapping_[i].first);
			}
		}
	} else {
		for ( core::Size i=1; i<=chis.size(); ++i ) {
			if ( chimapping_[i].second == -1 ) {
				chis[i] = pose.phi(chimapping_[i].first);
			} else if ( chimapping_[i].second == -2 ) {
				chis[i] = pose.psi(chimapping_[i].first);
			} else if ( chimapping_[i].second == -3 ) {
				chis[i] = pose.omega(chimapping_[i].first);
			} else {
				chis[i] = pose.chi(chimapping_[i].second, chimapping_[i].first);
			}
		}
	}
}

void
EntropyEstimator::update_flexscs( core::pose::Pose const &pose,
	utility::vector1< std::pair< core::Size, core::Size > > const &chidefs,
	std::map< core::Size, utility::vector1< core::Real > > &chis
) const
{
	for ( core::Size ichidef = 1; ichidef <= chidefs.size(); ++ichidef ) {
		core::Size ires = chidefs[ichidef].first;
		core::Size ichi = chidefs[ichidef].second;
		chis[ires][ichi] = pose.residue(ires).chi(ichi);
	}
}


// Unused... need to be replaced to rotlib functionality...
core::Size
EntropyEstimator::chis2rotid( utility::vector1< core::Real > const &chis ) const
{
	// no consideration on symmetric cases (D/E/F)
	core::Size rotid( 1 );
	for ( core::Size ichi = 1; ichi <= chis.size(); ++ichi ) {
		core::Real chi = chis[ichi];
		// why is D.chi2 having weird numbers sometimes?
		//if( chi < 0 ) chi += 360.0*(floor(-chi/360.0)+1);
		//if( chi > 360 ) chi -= 360.0*floor(chi/360.0);

		if ( chi < 0 ) chi += 360.0;
		if ( chi > 360 ) chi -= 360.0;
		int k = floor(int(chi/120.0));
		rotid += std::pow( 3, chis.size()-ichi )*k;
	}
	return rotid;
}

} //namespace GALigandDock
}
}
