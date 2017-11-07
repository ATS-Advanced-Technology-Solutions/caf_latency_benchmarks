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
#include <caf/atom.hpp>

#include "perf_counters.hh"
#include "cross_times.hh"
#include "benchmark_actor.hh"
#include "executor_actor.hh"
#include "benchmark_config.hh"

using namespace std;
using namespace caf;
using namespace _0lf;

latency<> chain_latency;

class benchmark_actor : public event_based_actor
{
public:
	benchmark_actor(actor_config& ac)
		: event_based_actor(ac), come_back_(false), is_crosstime_msg_(false)
	{
	}

	~benchmark_actor() { /*aout(this) << "benchmark_actor dtor\n";*/ }

	const char* name() const override { return name_.c_str(); };

	void do_work(chrono::microseconds working_time, string& c_str, complex_msg&)
	{
		std::hash<std::string> hash;
		auto start_pt = hr_clock::now();
		do
		{			
			c_str[0] = static_cast<char>(hash(c_str));
		} while (hr_clock::now() - start_pt < working_time);
	}

	void send_timed_msg(message& time, string* c_str = nullptr, complex_msg* c_msg = nullptr)
	{
		// 0: latency
		// 1: latency_from_rg
		// 2: cross_times
		if ( prev_ == actor () )
		{
			time.get_mutable_as<latency<>>(0).restart();
		}

		if (is_crosstime_msg_)
		{
			time.get_mutable_as<cross_times>(2).add(hop::forth, name_a_, hop::in);

			if (c_msg != nullptr/* && c_str != nullptr*/)
			{
				auto working_time = benchmark_cfg().payload_.actor_computing_time_;
				if (working_time.count() > 0)
				{
					do_work(working_time, *c_str, *c_msg);
					time.get_mutable_as<cross_times>(2).add(hop::forth, name_a_, hop::out);
				}
			}
		}

		if (next_ != actor())
		{
			if (c_msg == nullptr/* && c_str == nullptr*/)
			{
				send(next_, timed_msg_a::value, move(time));
			}
			else
			{
			    if (benchmark_cfg().payload_.copy_msg_)
				{
					send(next_, timed_cmsg_a::value, move(time), *c_str, *c_msg);
				}
				else
				{
					send(next_, timed_cmsg_a::value, move(time), move(*c_str), move(*c_msg));
				}
			}
		}
		else
		{
			if (!come_back_)
			{
				time.get_mutable_as<latency<>>(0).stop();
				time.get_mutable_as<latency<>>(1).stop();
				send(executor_, parallel_a::value, move(time));
			}
			else
			{
				if (c_msg == nullptr)
				{
					send(prev_, timed_msg_back_a::value, move(time));
				}
				else
				{
					if (benchmark_cfg().payload_.copy_msg_)
					{
						send(prev_, timed_cmsg_back_a::value, move(time), *c_str, *c_msg);
					}
					else
					{
						send(prev_, timed_cmsg_back_a::value, move(time), move(*c_str), move(*c_msg));
					}
				}
			}
		}
	}

	void send_timed_back_msg(message& time, string* c_str = nullptr, complex_msg* c_msg = nullptr)
	{
		// 0: latency
		// 1: latency_from_rg
		// 2: cross_times
		if (is_crosstime_msg_)
		{
			time.get_mutable_as<cross_times>(2).add(hop::back, name_a_, hop::in);

			if (c_msg != nullptr/* && c_str != nullptr*/)
			{
				auto working_time = benchmark_cfg().payload_.actor_computing_time_;
				if (working_time.count() > 0)
				{
					do_work(working_time, *c_str, *c_msg);
					time.get_mutable_as<cross_times>(2).add(hop::back, name_a_, hop::out);
				}
			}
		}

		if (prev_ != actor())
		{
			if (c_msg == nullptr)
			{
				send(prev_, timed_msg_back_a::value, move(time));
			}
			else
			{
				if (benchmark_cfg().payload_.copy_msg_)
				{
					send(prev_, timed_cmsg_back_a::value, move(time), *c_str, *c_msg);
				}
				else
				{
					send(prev_, timed_cmsg_back_a::value, move(time), move(*c_str), move(*c_msg));
				}
			}
		}
		else
		{
			time.get_mutable_as<latency<>>(0).stop();
			time.get_mutable_as<latency<>>(1).stop();
			send(executor_, parallel_a::value, move(time));
		}
	}

	behavior make_behavior() override
	{
		return
		{
		[&](init_bench_a, actor prev, actor next, actor ex, unsigned int pipeline_actor_id) -> result<unit_t>
		{
			auto& cfg = benchmark_cfg();	

			stringstream ss;
			if (getf(is_detached_flag))
			{
				ss << "DET_";
			}
			ss << "ba" << pipeline_actor_id;
			name_ = ss.str();
			name_a_ = atom_from_string(name_.substr(0, 10));
			
			prev_ = prev;
			next_ = next;
			executor_ = ex;
			come_back_ = cfg.general_.forth_and_back_;
			is_crosstime_msg_ = cfg.general_.tracking_type_ == "XTIMES";
			return unit_t{};
		},
		[&](weightless_a)
		{
			if (next_ != actor())
			{
				send(next_, weightless_a::value);
			}
			else {
				if (!come_back_) {
					chain_latency.stop();
					send(executor_, serial_a::value);
				}
				else
					send(prev_,weightless_back_a::value);
			}
		},
		[&](weightless_back_a) {
			if (prev_ != actor()) {
				send(prev_, weightless_back_a::value);
			}
			else {
				chain_latency.stop();
				send(executor_, serial_a::value);
			}
		},
		[&](timed_msg_a, message& timed)
		{
			send_timed_msg(timed);
		},
		[&](timed_cmsg_a, message& timed, string& c_str, complex_msg& c_msg)
		{
			send_timed_msg(timed, &c_str, &c_msg);
		},
		[&](timed_msg_back_a, message& timed)
		{
			send_timed_back_msg(timed);
		},
		[&](timed_cmsg_back_a, message& timed, string& c_str, complex_msg& c_msg)
		{
			send_timed_back_msg(timed, &c_str, &c_msg);
		},
		[&](kill_a)
		{
			if (next_ != actor())
				send(next_, kill_a::value);

			prev_ = actor();
			next_ = actor();
			executor_ = actor();
			quit();
		}
		};
	}
private:
	actor next_ = {};
	actor prev_ = {};
	actor executor_ = {};
	bool come_back_;
	bool is_crosstime_msg_;
	static int msg_counter_;
	string name_;
	atom_value name_a_{};
};

// executor spawner
actor spawn_benchmark_actor(actor_system & system, bool detach)
{
	if (!detach)
		return system.spawn<benchmark_actor>();
	else
		return system.spawn<benchmark_actor, detached>();
}
