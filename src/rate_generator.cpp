/*****************************************************************************
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#include <random>

#include <caf/blocking_actor.hpp>

#include "cross_times.hh"
#include "results_collector_actor.hh"
#include "benchmark_actor.hh"
#include "executor_actor.hh"
#include "benchmark_config.hh"


using namespace caf;
using namespace _0lf;
using namespace std;
using namespace chrono;


//! rate_generator actor
/*!
	blocking_actor which sends messages at a given message rate to a given receiver actor.
*/
class rate_generator : public blocking_actor
{
public:
		//! construct a rate_generator producing specified `msg_rate` for a `bench_duration` time.
		explicit rate_generator(actor_config& act,			//!< actor configuration
			vector<actor> receivers,				//!< Receiver handler
			actor results_collector 				//!< Results collector handler
			)
		: blocking_actor(act),
		receivers_(receivers),
		results_collector_(results_collector)
	{
		auto& cfg = benchmark_cfg();

		msg_rate_ = cfg.parallel_.rate_;
		round_robin_ = cfg.parallel_.sending_order_;
		payload_type_ = cfg.general_.payload_type_;
		complex_msg_string_lengh_ = cfg.payload_.string_length_;
		complex_msg_vector_size_ = cfg.payload_.vector_size_;
		bench_duration_ = cfg.general_.duration_;
		
	}

	//! return actor name ("rate_gen")
	const char* name() const override { return "rate_gen"; }

	void act() override
	{
		// todo: move these parameters to configuration
		

		unsigned int msg_counter_ = 0;
		int index;

		int vec_size = receivers_.size();

		default_random_engine generator;
		uniform_int_distribution<int> distribution(0, vec_size-1);
		
		hr_tpoint start_tp_;
		latency<nanoseconds> real_msg_duration_(false);
		mobile_average_ticks<> avg_rate_;

		// approximate number of hops in case of benchmark with cross times
		unsigned int hops_count = benchmark_cfg().payload_.actor_computing_time_.count() == 0 ?
			benchmark_cfg().general_.pipeline_actors_ :
			benchmark_cfg().general_.pipeline_actors_ * 2;

		if (benchmark_cfg().general_.forth_and_back_)
			hops_count *= 2;

		hops_count += 1; // rate generator hop

		auto batch_size = benchmark_cfg().payload_.batch_size_;

		start_tp_ = hr_clock::now();
		auto end_tp = start_tp_ + bench_duration_;
		if (bench_duration_.count() == 0)
			end_tp = hr_tpoint::max();
		
		if (round_robin_) {
			index = 0;
		}
		else {
			index = distribution(generator);
		}

		real_msg_duration_.restart();
		auto now_tp = hr_clock::now();
		
		while (end_tp > now_tp)
		{
			if (payload_type_ == "COMPLEX")
			{
				complex_msg c_msg = complex_msg(complex_msg_vector_size_);
				string c_str(complex_msg_string_lengh_, ' ');
				if (benchmark_cfg().general_.tracking_type_ == "XTIMES")
				{
					auto tmp_counter = 0u;
					do // message will be sent at least once
					{
						latency<> latency_from_rg;
						latency<> latency(false);
						cross_times xt(hops_count);
						xt.add(hop::forth, atom("sender"), hop::in);
						send(receivers_[index], timed_cmsg_a::value, make_message(latency, latency_from_rg, xt), move(c_str), move(c_msg));
						tmp_counter++;

						// index update
						if ( round_robin_ )
						{
							index = (index + 1) % vec_size;
						}
						else
						{
							index = distribution ( generator );
						}

						avg_rate_.tick();
					} while ( tmp_counter < batch_size );

					// update msg_counter
					msg_counter_ += tmp_counter;
				}
				else
				{
					latency<> latency_from_rg;
					latency<> latency ( false );
					send(receivers_[index], timed_cmsg_a::value, make_message(latency, latency_from_rg), move(c_str), move(c_msg));
					++msg_counter_;

					// index update
					if ( round_robin_ )
					{
						index = (index + 1) % vec_size;
					}
					else
					{
						index = distribution ( generator );
					}
					avg_rate_.tick();
				}
			}
			else
			{
				latency<> latency_from_rt;
				latency<> latency ( false );
				if (benchmark_cfg().general_.tracking_type_ == "XTIMES")
				{
					cross_times xt(hops_count);
					xt.add(hop::forth, atom("sender"), hop::in);
					send(receivers_[index], timed_msg_a::value, make_message(latency, latency_from_rt, xt));
				}
				else
				{
					send(receivers_[index], timed_msg_a::value, make_message(latency, latency_from_rt));
				}
				++msg_counter_;
				avg_rate_.tick();

				// index update
				if ( round_robin_ )
				{
					index = (index+1) % vec_size;
				}
				else 
				{
					index = distribution(generator);
				}
			}

			auto step = duration_cast<milliseconds>(1s) * (msg_counter_ / msg_rate_);
			auto next_send_tp = start_tp_ + duration_cast<nanoseconds>(step);
			now_tp = hr_clock::now();
			auto sleep_interval = next_send_tp - now_tp;

			if (sleep_interval.count() > 0)
			{
				this_thread::sleep_for(sleep_interval);
				now_tp += sleep_interval;
			}
		}

		real_msg_duration_.stop();

		auto elapsed_time = real_msg_duration_().count();
		double rate = msg_counter_ / (real_msg_duration_().count() / double(duration_cast<nanoseconds>(1s).count()));
		double perc_error = ( elapsed_time / double(1e9 * msg_counter_ / msg_rate_) - 1. ) * 100.0;

		send(results_collector_, send_collect_a::value, rate, perc_error);

		this_thread::sleep_for(2s);
	}

private:
	double msg_rate_;
	int round_robin_;
    std::string payload_type_;
	unsigned int complex_msg_string_lengh_;
	unsigned int complex_msg_vector_size_;
	std::chrono::seconds bench_duration_;
	vector<actor> receivers_;
	actor results_collector_;
};

void spawn_rate_generator(actor_system& system, vector<actor> heads, actor results_collector) {
	system.spawn<rate_generator>(heads, results_collector);
}
