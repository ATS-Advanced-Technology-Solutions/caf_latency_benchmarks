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

using serial_collect_a      = caf::atom_constant<caf::atom("collect")>;
using serial_collect_xtimes_a = caf::atom_constant<caf::atom("collect_xt")>;
using rc_init_a   = caf::atom_constant<caf::atom("rc_init")>;
using rec_collect_a  = caf::atom_constant<caf::atom("rec_coll")>;
using rec_collect_xtimes_a = caf::atom_constant<caf::atom("rec_collxt")>;
using send_collect_a = caf::atom_constant<caf::atom("send_coll")>;
using sys_collect_a = caf::atom_constant<caf::atom("sys_coll")>;
using benchmark_kill_a = caf::atom_constant<caf::atom("bench_kill")>;

caf::actor spawn_results_collector_actor(caf::actor_system & system, std::string filename);
