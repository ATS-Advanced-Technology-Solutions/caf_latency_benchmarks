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
#include "cross_times.hh"
#include "executor_actor.hh"
#include "topology_builder.hh"
#include "benchmark_actor.hh"
#include "results_collector_actor.hh"
#include "benchmark_status.hh"
#include "rate_generator.hh"

using namespace caf;
using namespace _0lf;
using namespace std;
using namespace chrono;

/// executor_actor is a event_based_actor which sends messages at a given message rate to a given receiver actor.
class executor_actor : public event_based_actor {
public:
	executor_actor(actor_config& ac)
		: event_based_actor(ac),
		msg_counter_(0),
		rep_counter_(0),
		benchmark_duration_(0s),
		update_time_(1s),
		payload_type_("NONE"),
		is_crosstime_msg_(false)
	{}

	~executor_actor()
	{ /*aout(this) << "executor dtor\n";*/	}

	const char* name() const override { return "executor"; }

	using res_time_unit = chrono::microseconds;

	void analyze_cross_times ( const cross_times& xtimes, unsigned int& _first_latency, unsigned int& _detached_send, unsigned int& _detached_receive, unsigned int& _message_passing_latency, unsigned int& _message_computation_latency ) const
	{
		// xtimes
	    average_value<res_time_unit> detached_receive;
	    average_value<res_time_unit> detached_send;
	    average_value<res_time_unit> message_passing_latency;
	    average_value<res_time_unit> message_computation_latency;

		bool is_detached = false;
		bool previous_is_detached = false;
		for ( unsigned h = 1; h < xtimes.hops (); ++h )
		{
			auto duration = chrono::duration_cast<res_time_unit>(xtimes.at ( h ).hop_tp_ - xtimes.at ( h - 1 ).hop_tp_);
			auto& hop = xtimes.at ( h );

			is_detached = to_string ( hop.actor_ ).compare ( 0, 3, "DET" ) == 0;

			if ( is_detached && hop.step_ == hop::in )
			{
				detached_receive.tick ( duration );
			}
			else if ( previous_is_detached && hop.step_ == hop::in )
			{
				detached_send.tick ( duration );
			}
			else if ( h == 1 )
			{
				_first_latency = duration.count ();
			}
			else
			{
				if ( hop.step_ == hop::out )
				{
					message_computation_latency.tick ( duration );
				}
				else
				{
					message_passing_latency.tick ( duration );
				}
			}

			previous_is_detached = is_detached;
		}

		_detached_send					= detached_send ().count ();
		_detached_receive				= detached_receive ().count ();
		_message_passing_latency		= message_passing_latency ().count ();
		_message_computation_latency	= message_computation_latency ().count ();
	}

	behavior make_behavior() override
	{
		return{
			/// exec init lambda
			/**
				some detailed info
			*/
			[&](exec_init_a, actor results_collector, actor topo_builder)
			{
				auto& cfg = benchmark_cfg();

				results_collector_		= results_collector;
				topo_builder_			= topo_builder;
				chains_number_			= cfg.parallel_.chains_number_;
				benchmark_duration_		= cfg.general_.duration_;
				msg_rate_				= cfg.parallel_.rate_;
				is_parallel_benchmark_	= cfg.general_.type_ == "PARALLEL";
				payload_type_			= cfg.general_.payload_type_;

				if (cfg.general_.forth_and_back_)
					nsteps_ = cfg.general_.pipeline_actors_ * 2 - 2;
				else
					nsteps_ = cfg.general_.pipeline_actors_ - 1;

				is_crosstime_msg_ = cfg.general_.tracking_type_ == "XTIMES";
			},
			[&](sysconfig_a)
			{
			    if (heads_.size() == chains_number_) {
					if (is_parallel_benchmark_)
						send(this, parallel_benchmark_a::value);
					else
						send(this, serial_benchmark_a::value);
				}
				else {
					send(topo_builder_, make_linear_chain_a::value);
				}
			},
			[&](system_building_a, actor a) {
				heads_.emplace_back(a);
				send(this, sysconfig_a::value);
			},
			[&](serial_benchmark_a)
			{
			    auto& cfg = benchmark_cfg();
				if (cfg.general_.payload_type_ == "COMPLEX" ||
					cfg.general_.tracking_type_ == "XTIMES" ) {
					return;
				}
				

				total_latency_.restart();

				send(this, serial_a::value);


				delayed_send(this, update_time_, serial_update_metrics_a::value);
				if (benchmark_duration_.count() > 0) {
					delayed_send(this, benchmark_duration_, serial_kill_a::value);
				}
			},
			[&](serial_a)
			{
				if (rep_counter_ > 0)
				{
					// print latency per message
					current_rate_.tick();
					overall_net_latency_.tick(chain_latency());
					mobile_current_latency_.tick(chain_latency());
				}

				// sending message to start new latency mesurement
				rep_counter_++;
				chain_latency.restart();
				
				send(heads_[0], weightless_a::value);
			},
			[&](serial_update_metrics_a)
			{
				delayed_send(this, update_time_, serial_update_metrics_a::value);
			},
			[&](serial_kill_a)
			{
				total_latency_.stop();

				auto elapsed_time = total_latency_().count();

				// multiplying by 1/1000 in order to obtain microseconds
				double overall_net_lat = overall_net_latency_().count() / nsteps_ * 1.e-3;
				double gross_lat = elapsed_time / (nsteps_*rep_counter_) * 1.e-3;
				double overall_rate = rep_counter_ / (elapsed_time / duration_cast<nanoseconds>(1s).count());

				if (is_crosstime_msg_)
				{
					unsigned int first_latency = 0;
					unsigned int detached_send = 0;
					unsigned int detached_receive = 0;
					unsigned int message_passing_latency = 0;
					unsigned int message_computation_latency = 0;

					analyze_cross_times ( ct_overall_net_latency_ (), first_latency, detached_send, detached_receive, message_passing_latency, message_computation_latency );


					send(results_collector_, serial_collect_xtimes_a::value, 
						  overall_net_lat, 
						  gross_lat, 
						  overall_rate, 
						  first_latency, 
						  detached_send, 
						  detached_receive, 
						  message_passing_latency, 
						  message_computation_latency );
				}
				else
				{
					send(results_collector_, serial_collect_a::value, overall_net_lat, gross_lat, overall_rate);

				}
					
				// kill benchmark chain
				send(heads_[0], kill_a::value);

				// kill topology_builder
				send(topo_builder_, topo_kill_a::value);
				topo_builder_ = actor();

				results_collector_ = actor();

                delayed_send (this, 2s, end_a::value);
            },
			[&](parallel_a, message& msg)
			{
				const latency<>& orig = msg.get_as<latency<>>(0);
				const latency<>& orig_from_rg = msg.get_as<latency<>>(1);

				current_rate_.tick();
				overall_net_latency_.tick(orig());
				overall_net_latency_from_rg_.tick(orig_from_rg());
				mobile_current_latency_.tick(orig());
				++msg_counter_;

				// cross_times
				if (is_crosstime_msg_) {
				  ct_overall_net_latency_.tick(msg.get_as<cross_times>(2));
				}
			},
			[&](parallel_kill_a)
			{
				total_latency_.stop();

				auto elapsed_time = total_latency_().count();

				//std::cout << "overall_net_latency_().count()         = " << overall_net_latency_().count() << std::endl;
				//std::cout << "overall_net_latency_from_rg_().count() = " << overall_net_latency_from_rg_().count() << std::endl;

				std::cout << "Total average latency (ns): " << overall_net_latency_from_rg_().count() << "\n" << std::flush;

				// multiplying by 1/1000 in order to obtain microseconds
				double overall_net_lat	= overall_net_latency_().count() / nsteps_ * 1.e-3;
				double overall_net_lat_from_rg = overall_net_latency_from_rg_().count() / (nsteps_ + 1) * 1.e-3;
				double msg_rate			= msg_counter_ / (elapsed_time / double(duration_cast<nanoseconds>(1s).count()));
				double perc_err			= elapsed_time * 100.0 / double(1e9 * msg_counter_ / msg_rate_) - 100.0;

				if (is_crosstime_msg_)
				{
					unsigned int detached_send = 0;
					unsigned int detached_receive = 0;
					unsigned int message_passing_latency = 0;
					unsigned int message_computation_latency = 0;
					unsigned int first_latency = 0;

					analyze_cross_times ( ct_overall_net_latency_ (), first_latency, detached_send, detached_receive, message_passing_latency, message_computation_latency );


					send(results_collector_, rec_collect_xtimes_a::value,
						  overall_net_lat_from_rg,
						  overall_net_lat,
						  msg_rate,
						  perc_err,
						  first_latency,
						  detached_send,
						  detached_receive,
						  message_passing_latency,
						  message_computation_latency);
				}
				else
				{
					send(results_collector_, rec_collect_a::value,
						overall_net_lat_from_rg,
						overall_net_lat,
						msg_rate,
						perc_err);
				}

				for(size_t i=0; i < heads_.size(); ++i)
				    send(heads_[i], kill_a::value);
				heads_.clear();

				send(topo_builder_, topo_kill_a::value);
				topo_builder_ = actor();

				results_collector_ = actor();

                delayed_send (this, 2s, end_a::value);
			},
            [&](end_a)
            {
                quit();
            },
			[&](parallel_update_metrics_a)
			{
				unsigned int detached_send = 0;
				unsigned int detached_receive = 0;
				unsigned int message_passing_latency = 0;
				unsigned int message_computation_latency = 0;
				unsigned int first_latency = 0;

				analyze_cross_times(ct_overall_net_latency_(), first_latency, detached_send, detached_receive, message_passing_latency, message_computation_latency);


				delayed_send(this, update_time_, parallel_update_metrics_a::value);
			},
			[&](parallel_benchmark_a)
			{

				total_latency_.restart();

				spawn_rate_generator(home_system(), heads_, results_collector_);
				delayed_send(this, update_time_, parallel_update_metrics_a::value);
				if(benchmark_duration_.count() > 0)
					delayed_send(this, benchmark_duration_, parallel_kill_a::value);
			}
		};
	}

private:
	latency<> total_latency_;
	average_value<nanoseconds> overall_net_latency_;
	average_value<nanoseconds> overall_net_latency_from_rg_;
	average_value<cross_times> ct_overall_net_latency_;
	mobile_average_value<latency<>::result_type> mobile_current_latency_;
	mobile_average_ticks<> current_rate_;
	  
	actor topo_builder_, results_collector_;
	vector<actor> heads_;
	bool is_parallel_benchmark_;
	unsigned int msg_counter_, nsteps_;
	uint64_t rep_counter_;
        size_t chains_number_;
	double msg_rate_;
	std::chrono::duration<int64_t> benchmark_duration_, update_time_;
        std::string payload_type_;
	bool is_crosstime_msg_;

	string name_;	
};

// executor spawner
actor spawn_executor(actor_system & system)
{
	return system.spawn<executor_actor>();
}
