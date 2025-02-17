// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file
/// @brief

#ifndef INCLUDED_protocols_cryst_wallpaper_hh
#define INCLUDED_protocols_cryst_wallpaper_hh

#include <protocols/cryst/util.hh>


#include <core/types.hh>


#include <utility/vector1.hh>





#include <string>

#include <numeric/xyzMatrix.hh>
#include <numeric/xyzVector.hh>

#include <protocols/cryst/CheshireCell.hh> // AUTO IWYU For CheshireCell
#include <core/kinematics/RT.hh> // AUTO IWYU For RT



namespace protocols {
namespace cryst {

enum WallpaperGroupSetting {
	wgMONOCLINIC,  // a, b, angle move
	wgTETRAGONAL,  // a, b move, theta=90
	wgHEXAGONAL,   // a+b move, theta=60
	wgCUBIC,       // a+b move, theta=90
	wgNOSETTING
};

class WallpaperGroup {
private:
	std::string name_;
	WallpaperGroupSetting setting_;

	// cryst stuff
	numeric::xyzMatrix<core::Real> f2c_, c2f_;
	core::Real a_, b_, alpha_, V_;
	core::Size ncopies_;

	utility::vector1<core::kinematics::RT> symmops_;
	CheshireCell cc_;

public:
	WallpaperGroup();
	WallpaperGroup(std::string name_in);

	void
	set_wallpaper_group( std::string name_in);

	numeric::xyzMatrix<core::Real> const &f2c() const { return f2c_; };
	numeric::xyzMatrix<core::Real> const &c2f() const { return c2f_; };
	core::Real A() const { return a_; }
	core::Real B() const { return b_; }
	core::Real alpha() const { return alpha_; }
	core::Real volume() const { return V_; }

	WallpaperGroupSetting setting() const { return setting_; }
	utility::vector1<core::kinematics::RT> const &symmops() const { return symmops_; }
	core::kinematics::RT symmop(core::Size i) const { return symmops_[i]; }
	core::Size nsymmops() const { return symmops_.size(); }
	CheshireCell cheshire_cell() const { return cc_; }

	// grid spacing must be a multiple of this number
	core::Size minmult() const {
		if ( setting_ == wgMONOCLINIC )   return 4;
		if ( setting_ == wgCUBIC )        return 4;
		if ( setting_ == wgTETRAGONAL )   return 8;
		if ( setting_ == wgHEXAGONAL )    return 6;
		return 0;
	}

	// sets AND VALIDATES input parameters
	void set_parameters(core::Real a_in, core::Real b_in, core::Real alpha_in );

	std::string
	get_moveable_dofs() const {
		utility::vector1<core::kinematics::RT> rt;
		CheshireCell  cc;
		get_symmops (rt,cc);
		std::string retval;
		if ( cc.high[0]>cc.low[0] ) retval += "x ";
		if ( cc.high[1]>cc.low[1] ) retval += "y ";
		if ( cc.high[2]>cc.low[2] ) retval += "z ";
		return retval;
	}

	numeric::xyzVector<core::Size>
	get_nsubdivisions() {
		numeric::xyzVector<core::Size> retval(1,1,1);
		for ( int i=1; i<=(int)nsymmops(); ++i ) {
			numeric::xyzVector<core::Real> const &T = symmops_[i].get_translation();
			retval[0] = std::max( retval[0], denom( T[0] ) );
			retval[1] = std::max( retval[1], denom( T[1] ) );
		}
		return retval;
	}

	numeric::xyzVector<core::Size>
	get_trans_dofs() {
		if ( setting_ == wgMONOCLINIC ) {
			return numeric::xyzVector<core::Size>(1,2,0);
		} else if ( setting_ == wgCUBIC ) {
			return numeric::xyzVector<core::Size>(1,1,0);
		} else if ( setting_ == wgTETRAGONAL ) {
			return numeric::xyzVector<core::Size>(1,2,0);
		} else if ( setting_ == wgHEXAGONAL ) {
			return numeric::xyzVector<core::Size>(1,1,0);
		}

		return numeric::xyzVector<core::Size>(1,2,0);  // no warnings
	}

	core::Size
	get_nrot_dofs() {
		if ( setting_ == wgMONOCLINIC ) {
			return 1;
		}
		return 0;
	}

	// get 'CRYST1' name of WallpaperGroup
	std::string pdbname() const;

private:
	// get symmops
	void get_symmops(utility::vector1<core::kinematics::RT> &rt_out, CheshireCell &cc) const;
};


}
}

#endif
