/*****************************************************************************
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#include <caf/actor_system_config.hpp>
#include <caf/scoped_actor.hpp>
#include <caf/io/middleman.hpp>

#include "benchmark_config.hh"
#include "results_collector_actor.hh"
#include "executor_actor.hh"
#include "topology_builder.hh"

using namespace std;
using namespace caf;

int main(int argc, char** argv)
{
    benchmark_config benchmark_config("caf_latency_benchmarks.ini", argc, argv);
    if (!benchmark_config.is_valid())
    {
	    std::cout << "Invalid configuration:" << std::endl;
	    std::cout << "  Number of pipeline actors must be greater than 1" << std::endl;
	    return -1;
    }
    
    actor_system system { benchmark_config };

    if (benchmark_config.cli_helptext_printed) exit(0);
    
    // spawning main actors
    scoped_actor self{ system };
    auto results_collector = spawn_results_collector_actor(system, benchmark_config.general_.result_file_);
    auto executor = spawn_executor(system);
    auto topo_builder = spawn_topology_builder(system);
    
    self->send(topo_builder, init_a::value, executor);
    self->send(results_collector, rc_init_a::value);
    self->send(executor, exec_init_a::value, results_collector, topo_builder);
    
    self->send(executor, sysconfig_a::value);
    
    return 0;
}
