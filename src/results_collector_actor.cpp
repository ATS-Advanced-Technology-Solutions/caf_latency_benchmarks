/*****************************************************************************
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#include <fstream>

#include <caf/event_based_actor.hpp>

#include "perf_counters.hh"
#include "cross_times.hh"
#include "results_collector_actor.hh"
#include "benchmark_config.hh"

using namespace caf;
using namespace std;
using namespace _0lf;


class results_collector_actor : public event_based_actor {
public:
	results_collector_actor(actor_config& ac, string filename)
		: event_based_actor(ac)	{

		ifstream check_header(filename, ifstream::in);
		if (!check_header) {
			is_first_set_ = true;
		}
		else {
			is_first_set_ = false;
			check_header.close();
		}
		
		fstream_.open(filename, ofstream::app);
		if (fstream_.fail())
		    std::cerr << "Failed to open file " << filename << "\n";
	}

	virtual ~results_collector_actor()
	{
		fstream_.close();
	}

	const char* name() const override { return "results"; }

	behavior make_behavior() override
	{
		return{
			[&](rc_init_a){
				
				auto& cfg = benchmark_cfg();

				if (is_first_set_) {

					string header;

					if (cfg.general_.type_ == "PARALLEL")
						header = "number of actors;sender message rate (msg/s);percentage sender error;all actor latency (us);pipeline actor latency (us);receiver message rate (msg/s);percentage receiver error";
					else
						header = "number of actors;net latency (us);gross latency (us);message rate (msg/sec)";

					if (cfg.general_.tracking_type_ == "XTIMES")
						header += ";sender to pipeline latency (us); detached actor send latency (us);detached actor receive latency (us);message passing latency (us);message computation latency (us)";

					fstream_ << header << endl;
					fstream_.flush();

				}
				// writing number of actors
				fstream_ << cfg.general_.pipeline_actors_ << ";";
				delayed_send(this, 3s, sys_collect_a::value);
			},
			[&](serial_collect_a, double net_lat, double gross_lat, double msg_rate)
			{
				fstream_ << net_lat << ";" << gross_lat << ";" << msg_rate << endl;

				delayed_send(this, 3s, benchmark_kill_a::value);
			},
			[&](serial_collect_xtimes_a, double net_lat, double gross_lat, double msg_rate, unsigned int, unsigned int detached_send, unsigned int detached_receive, unsigned int message_passing_latency, unsigned int message_computation_latency )
			{

				fstream_ << net_lat << ";"
					<< gross_lat << ";"
					<< msg_rate << ";"
					<< detached_send << ";"
					<< detached_receive << ";"
					<< message_passing_latency << ";"
					<< message_computation_latency << endl;
				delayed_send(this, 3s, benchmark_kill_a::value);
			},
			[&](send_collect_a, double rate, double err){

				fstream_ << rate << ";" << err << ";";
			},
			[&](rec_collect_a, double overall_net_lat_from_rg, double overall_net_lat, double msg_rate, double err)
			{
			      fstream_ << overall_net_lat_from_rg << ";"
				       << overall_net_lat << ";"
				       << msg_rate << ";"
				       << err << endl;

				delayed_send(this, 3s, benchmark_kill_a::value);
			},
			[&](rec_collect_xtimes_a, double overall_net_lat_from_rg, double overall_net_lat, double msg_rate, double err, unsigned int first_latency, unsigned int detached_send, unsigned int detached_receive, unsigned int message_passing_latency, unsigned int message_computation_latency )
			{
			       
				fstream_ << overall_net_lat_from_rg << ";"
					<< overall_net_lat << ";"
					<< msg_rate << ";"
					<< err << ";"
					<< first_latency << ";"
					<< detached_send << ";"
					<< detached_receive << ";"
					<< message_passing_latency << ";"
					<< message_computation_latency << endl;
				delayed_send(this, 3s, benchmark_kill_a::value);
			},
			[&](sys_collect_a) {
				delayed_send(this, 2s, sys_collect_a::value);
			},
			[&](benchmark_kill_a) {
				fstream_.flush();
				exit(0);
			}
		};
	}
private:
    ofstream fstream_;
    bool is_first_set_;
};

actor spawn_results_collector_actor(actor_system & system, string filename)
{
	return system.spawn<results_collector_actor>(filename);
}
