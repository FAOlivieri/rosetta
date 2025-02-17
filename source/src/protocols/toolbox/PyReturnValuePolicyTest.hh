// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/toolbox/PyReturnValuePolicyTest.hh
/// @brief A few functions test how PyRosetta handle boost ReturnValuePolicy
/// @author Sergey Lyskov

#ifndef INCLUDED_protocols_toolbox_PyReturnValuePolicyTest_hh
#define INCLUDED_protocols_toolbox_PyReturnValuePolicyTest_hh

#include <core/pose/Pose.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>

#include <utility/pointer/owning_ptr.hh>

#include <core/types.hh>

namespace protocols {
namespace toolbox {

class DummyClass;

typedef utility::pointer::shared_ptr< DummyClass > DummyClassOP;
typedef utility::pointer::shared_ptr< DummyClass const > DummyClassCOP;

//typedef utility::pointer::access_ptr< DummyClass > DummyClassAP;
//typedef utility::pointer::access_ptr< DummyClass const > DummyClassCAP;


class DummyClass : public utility::VirtualBase {
public:
	DummyClass() {};
	DummyClass(int) {};
	~DummyClass() override = default;

	DummyClass(DummyClass const &) = default;

	DummyClass & operator=( DummyClass const & ) { return *this; };

	static DummyClass * create() { return new DummyClass(); };

	virtual DummyClassOP clone() const { return utility::pointer::make_shared< DummyClass >(); };

	void test() {};

private:
};

inline DummyClassOP PyReturnValuePolicyTest_DummyClassOP(void) { return DummyClassOP( DummyClass::create() ); }
inline DummyClassCOP PyReturnValuePolicyTest_DummyClassCOP(void) { return DummyClassCOP( DummyClass::create() ); }
//inline DummyClassAP PyReturnValuePolicyTest_DummyClassAP(void)  { return DummyClass::create(); };
//inline DummyClassCAP PyReturnValuePolicyTest_DummyClassCAP(void)  { return DummyClass::create(); };


class SF_Replica;

typedef utility::pointer::shared_ptr< SF_Replica > SF_ReplicaOP;
typedef utility::pointer::shared_ptr< SF_Replica const > SF_ReplicaCOP;

//typedef utility::pointer::access_ptr< SF_Replica > SF_ReplicaAP;
//typedef utility::pointer::access_ptr< SF_Replica const > SF_ReplicaCAP;


// SF_Replica
class SF_Replica : public utility::VirtualBase
{
public:

	SF_Replica() {};

	~SF_Replica() override = default;

	SF_Replica &
	operator=( SF_Replica const & ) { return *this; };

	SF_Replica( SF_Replica const & ) = default;

	virtual SF_ReplicaOP clone() const { return nullptr; };

	virtual core::Real operator ()( core::pose::Pose &) const { return 0.0; };

private:
	//int some_private_int_;
};

inline SF_ReplicaOP PyReturnValuePolicyTest_SF_ReplicaOP(void) { return utility::pointer::make_shared< SF_Replica >(); }
inline SF_ReplicaCOP PyReturnValuePolicyTest_SF_ReplicaCOP(void) { return utility::pointer::make_shared< SF_Replica >(); }
//inline SF_ReplicaAP PyReturnValuePolicyTest_SF_ReplicaAP(void)  { return new SF_Replica; };
//inline SF_ReplicaCAP PyReturnValuePolicyTest_SF_ReplicaCAP(void)  { return new SF_Replica; };


inline core::pose::PoseOP PyReturnValuePolicyTest_PoseOP(void) { return utility::pointer::make_shared< core::pose::Pose >(); }
inline core::pose::PoseCOP PyReturnValuePolicyTest_PoseCOP(void) { return utility::pointer::make_shared< core::pose::Pose >(); }
//inline core::pose::PoseAP PyReturnValuePolicyTest_PoseAP(void)  { return new core::pose::Pose(); };
//inline core::pose::PoseCAP PyReturnValuePolicyTest_PoseCAP(void)  { return new core::pose::Pose(); };

inline core::scoring::ScoreFunctionOP PyReturnValuePolicyTest_ScoreFunctionOP(void) { return core::scoring::get_score_function_legacy( core::scoring::PRE_TALARIS_2013_STANDARD_WTS ); }
inline core::scoring::ScoreFunctionCOP PyReturnValuePolicyTest_ScoreFunctionCOP(void) { return core::scoring::get_score_function_legacy( core::scoring::PRE_TALARIS_2013_STANDARD_WTS ); }
inline core::scoring::ScoreFunctionCOP PyReturnValuePolicyTest_ScoreFunctionCOP2(void) { return utility::pointer::make_shared< core::scoring::ScoreFunction >(); }

inline void take_pose_op(core::pose::PoseOP const &) {}

// test for out-of-bounds access handling
void out_of_bounds_memory_access();




// PyRosetta inheritance tests
namespace py_inheritance_test {
class Base : public utility::VirtualBase {
public:
	Base() : data_(0) {};
	~Base() override = default;

	virtual std::string who_am_i() { data_++; return "Base"; }
private:
	int data_;
};


class O_1A : public Base {
public:
	std::string who_am_i() override { return "B1A"; }
};

class O_1B : public Base {
public:
	std::string who_am_i() override { return "O_1B"; }
};



class O_2A : public O_1A {
public:
	std::string who_am_i() override { return "O_2A"; }
};

class O_2B : public O_1B {
public:
	std::string who_am_i() override { return "O_2B"; }
};



class O_2A2B : public O_2A, public O_2B {
public:
	std::string who_am_i() override { return "O_2A2B"; }
};


inline std::string take_Base_reference(Base &r) { return r.who_am_i(); }
inline std::string take_Base_pointer  (Base *p) { return p->who_am_i(); }
inline std::string take_Base_SP       (utility::pointer::shared_ptr<Base> &s) { return s->who_am_i(); }

inline std::string take_O_1A_reference(O_1A &r) { return r.who_am_i(); }
inline std::string take_O_1A_pointer  (O_1A *p) { return p->who_am_i(); }
inline std::string take_O_1A_SP       (utility::pointer::shared_ptr<O_1A> &s) { return s->who_am_i(); }

inline std::string take_O_2A_reference(O_2A &r) { return r.who_am_i(); }
inline std::string take_O_2A_pointer  (O_2A *p) { return p->who_am_i(); }
inline std::string take_O_2A_SP       (utility::pointer::shared_ptr<O_2A> &s) { return s->who_am_i(); }



} //namespace py_inheritance_test


} //toolbox
} //protocols

#endif  // INCLUDED_protocols_toolbox_PyReturnValuePolicyTest_hh
