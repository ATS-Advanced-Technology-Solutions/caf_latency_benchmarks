/*****************************************************************************
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#pragma once

#include <caf/actor.hpp>

using init_bench_a = caf::atom_constant<caf::atom("init_b")>;

using init_detached_a = caf::atom_constant<caf::atom("init_d")>;
using detached_msg_a = caf::atom_constant<caf::atom("d_msg")>;

using weightless_a = caf::atom_constant<caf::atom("weightless")>;
using weightless_back_a = caf::atom_constant<caf::atom("weigh_back")>;

using timed_msg_a = caf::atom_constant<caf::atom("t_msg")>;
using timed_msg_back_a = caf::atom_constant<caf::atom("t_msg_bk")>;

using timed_cmsg_a = caf::atom_constant<caf::atom("t_cmsg")>;
using timed_cmsg_back_a = caf::atom_constant<caf::atom("t_cmsg_bk")>;

using bench_lat_back_a = caf::atom_constant<caf::atom("bench_back")>;

using complex_bench_lat_a = caf::atom_constant<caf::atom("cplex_bl")>;
using complex_bench_lat_back_a = caf::atom_constant<caf::atom("cplex_blbk")>;

using kill_a = caf::atom_constant<caf::atom("kill")>;

//! Struct for complex messages
/*!
	A struct for complex message is composed by:
	- a fixed memory occupation (500byte),
	- a configurable size vector of char.
*/
struct complex_msg {
	
	complex_msg() = delete;

	//! Create a complex message
	complex_msg(
		size_t vector_size //!< size of dynamically allocated memory
	)
		: d01_(1.2),
		d02_(2.3),
		i01_(0),
		c01_('l'),
		i02_(27),
		d03_(3.4),
		i03_(42),
		c02_('f'),
		vector_(vector_size, 0)
	{
	}

	// inspector
	template <class Inspector>
	friend typename Inspector::result_type inspect(Inspector& f, complex_msg& x)
	{
		return f(caf::meta::type_name("complex_msg"),
			x.d01_, x.d02_, x.i01_, x.c01_, x.i02_, x.d03_, x.i03_, x.c02_, x.buffer_, x.vector_);
	};

private:
	// fixed size (~500 byte)
	double d01_;            // 8   (8)
	double d02_;            // 8   (16) 
	int i01_;               // 4   (20)
	char c01_;              // 1   (~21)
	int i02_;               // 4   (~25)
	double d03_;            // 8   (~33)
	int i03_;               // 4   (~37)
	char c02_;              // 1   (~38)
	char buffer_[462] = {}; // 462 (~500)
	
	std::vector<char> vector_;	//!< dynamically (and configurable) size
};

caf::actor spawn_benchmark_actor(caf::actor_system & system, bool detach);
