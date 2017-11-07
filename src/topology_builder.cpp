/*****************************************************************************
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#include <caf/event_based_actor.hpp>

#include "perf_counters.hh"
#include "executor_actor.hh"
#include "topology_builder.hh"
#include "benchmark_actor.hh"
#include "benchmark_config.hh"

using namespace std;
using namespace caf;
using namespace _0lf;

class topology_builder : public event_based_actor
{
public:
	explicit topology_builder(actor_config& ac)
		: event_based_actor(ac)
	{}

	behavior make_behavior() override
	{
		return
		{
			[&](init_a, actor executor)
			{
				auto& cfg = benchmark_cfg();

				executor_ = executor;
				N_ = cfg.general_.pipeline_actors_;
				counter_ = N_;
				detached_actor_ = cfg.general_.detached_actor_;
			},
			[&](make_linear_chain_a)
			{
				vector<actor> bench_actors;

				// spawn benchmark actors
				for (unsigned int i = 0; i < N_; ++i) {
				    actor new_benchmark_actor = spawn_benchmark_actor(home_system(), i == (unsigned int)detached_actor_);
				    bench_actors.emplace_back(new_benchmark_actor);
				}
				for (unsigned int i = 0; i < N_; i++) {

					actor previous {};
					actor next {};

					if (i > 0)
						previous = bench_actors[i - 1];

					if (i < N_ - 1)
						next = bench_actors[i + 1];

					request(bench_actors[i], infinite, init_bench_a::value,
						previous, next, executor_, i).then(
							[=](unit_t) mutable {
							counter_--;
							if (counter_ == 0) {
								counter_ = N_;
								send(executor_, system_building_a::value, bench_actors[0]);
								bench_actors.clear();
							}
					});
				}
			},
			[&](topo_kill_a)
			{
				executor_ = actor();
				quit();
			}
		};
	}

private:
	// number of actors
	unsigned int N_;
	unsigned int detached_actor_;
	int counter_;
	actor executor_;
};

actor spawn_topology_builder(actor_system & system)
{
	return system.spawn<topology_builder>();
}
