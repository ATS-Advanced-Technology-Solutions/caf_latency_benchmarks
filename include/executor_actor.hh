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

#include "perf_counters.hh"

CAF_ALLOW_UNSAFE_MESSAGE_TYPE(_0lf::latency<>);

using exec_init_a = caf::atom_constant<caf::atom("init")>;
using sysconfig_a = caf::atom_constant<caf::atom("sysconfig")>;
using system_building_a = caf::atom_constant<caf::atom("sysbuild")>;

using serial_benchmark_a = caf::atom_constant<caf::atom("s_bench")>;
using serial_a = caf::atom_constant<caf::atom("serial")>;
using serial_update_metrics_a = caf::atom_constant<caf::atom("met_serial")>;
using serial_kill_a = caf::atom_constant<caf::atom("ser_kill")>;
using parallel_a = caf::atom_constant<caf::atom("parallel")>;

using parallel_benchmark_a = caf::atom_constant<caf::atom("p_bench")>;
using parallel_update_metrics_a = caf::atom_constant<caf::atom("met_parall")>;
using parallel_kill_a = caf::atom_constant<caf::atom("par_kill")>;
using print_something_a = caf::atom_constant<caf::atom("pippo")>;

using end_a = caf::atom_constant<caf::atom ("end")>;

// spawns and return the benchmark executor actor using system configuration
caf::actor spawn_executor(caf::actor_system & system);
