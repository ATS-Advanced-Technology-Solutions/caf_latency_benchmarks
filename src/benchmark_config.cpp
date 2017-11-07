/*****************************************************************************
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#include "benchmark_config.hh"

#include <caf/io/middleman.hpp>

using namespace std;

benchmark_config* benchmark_config::instance_ = nullptr;


benchmark_config::benchmark_config(const std::string& cfg_ini_file, int argc, char** argv)
{
    argc__    = argc;
    argv__    = argv;
    ini_file_ = cfg_ini_file;
    if (!instance_) instance_ = this;


    // benchmark type
    static const std::string serial{"SERIAL"};
    static const std::string parallel{"PARALLEL"};
    static std::string el_type          = std::string("Benchmark type: ") 
	+ "\n" + std::string (55, ' ') + '[' + std::string (1, serial.c_str()[0]) + ']' + std::string (&serial.c_str()[1]) + " (" + "serial" + ')' 
	+ "\n" + std::string (55, ' ') + '[' + std::string (1, parallel.c_str()[0]) + ']' + std::string (&parallel.c_str()[1]) + " (" + "parallel" + ')'
	+ " (default is serial)";
	
    // payload type
    static const std::string none{"NONE"};
    static const std::string complex{"COMPLEX"};
    static std::string el_payload_type      = std::string("Payload type: ") 
	+ "\n" + std::string (55, ' ') + '[' + std::string (1, none.c_str()[0]) + ']' + std::string (&none.c_str()[1]) + " (" + "none" + ')' 
	+ "\n" + std::string (55, ' ') + '[' + std::string (1, complex.c_str()[0]) + ']' + std::string (&complex.c_str()[1]) + " (" + "complex" + ')'
	+ " (default is \"NONE\")";

    // tracking type
    static const std::string latency{"LATENCY"};
    static const std::string xtimes{"XTIMES"};
    static std::string el_tracking_type    = std::string("Tracking type: ") 
	+ "\n" + std::string (55, ' ') + '[' + std::string (1, latency.c_str()[0]) + ']' + std::string (&latency.c_str()[1]) + " (" + "latency" + ')' 
	+ "\n" + std::string (55, ' ') + '[' + std::string (1, xtimes.c_str()[0]) + ']' + std::string (&xtimes.c_str()[1]) + " (" + "cross times" + ')'
	+ " (default is \"LATENCY\")";

    
    opt_group{ custom_options_, "benchmark" }
    .add(general_internal_.type_str_,			"benchmark_type,t",	el_type.c_str())
	 .add(general_internal_.payload_type_str_,	"payload_type,P",	el_payload_type.c_str())
	 .add(general_internal_.tracking_type_str_,	"tracking_type,K",	el_tracking_type.c_str())
	 .add(general_internal_.pipeline_actors_,	"pipeline_actors,N",	"Number of actors in the benchmark's pipeline (default is 2)")
	 .add(general_internal_.duration_,		"duration,d",		"Benchmark's duration (seconds), 0 for endless benchmark (default is 10s)")
	 .add(general_internal_.result_path_,		"result_path",		"Folder path in which store output files (default is ./trace/)")
	 .add(general_internal_.extension_,		"extension",		"Extension of the output file (default is csv)")
	 .add(general_internal_.output_file_,		"output_file",		"File name in which store the results (_serial or _parallel will be added depending on the benchmark type)")
	 .add(general_internal_.detached_actor_,	"detach",		"Actor chain number, included in [0,N-1] interval, to spawn in detached mode (default = no actor detached)")
	 .add(general_internal_.forth_and_back_,	"forth_and_back",	"Defines message route (default is 0)");
    
    opt_group{ custom_options_, "parallel" }
    .add(parallel_.rate_,		"rate,r",		"Frequency of messages introduced into the benchmark's pipeline (messages per second, default is 100.0)")
	 .add(parallel_.chains_number_,	"chains_number",	"Number of chains to built in 1 to N configuration (default is 1)")
	 .add(parallel_.sending_order_,	"sending_order",	"Message sending order to chains' heads (0 = random, 1 = Round Robin = default)");
    
    opt_group{ custom_options_, "payload" }
    .add(payload_internal_.string_length_,		"string_length",	 "The length of the string into a complex message (default is 16)")
	 .add(payload_internal_.vector_size_,		"vector_size",		 "The size of the vector<char> into a complex message (default is 500)")
	 .add(payload_internal_.copy_msg_,		"copy_msg",		 "Flag for copy complex messages instead of moving them (default is 0)")
	 .add(payload_internal_.batch_size_,		"batch_size",		 "Batch size in order to obtain a square wave in output (default is 0)")
	 .add(payload_internal_.actor_computing_time_,	"actor_computing_time",	 "Actor work time (microseconds) (default is 0)");
    
	parse();
	load<caf::io::middleman>();
}

void benchmark_config::convert_gen_internal_to_external(general_internal& internal_struct, general& external_struct)
{
    // general parameters
    external_struct.type_                  = internal_struct.type_str_;
    external_struct.payload_type_          = internal_struct.payload_type_str_;
    external_struct.tracking_type_         = internal_struct.tracking_type_str_;
    external_struct.pipeline_actors_       = internal_struct.pipeline_actors_;
    external_struct.duration_              = std::chrono::seconds(internal_struct.duration_);
    external_struct.result_path_           = internal_struct.result_path_;
    external_struct.extension_             = internal_struct.extension_;
    external_struct.output_file_           = internal_struct.output_file_;
    external_struct.detached_actor_        = internal_struct.detached_actor_;
    external_struct.forth_and_back_        = internal_struct.forth_and_back_ > 0;
}

void benchmark_config::convert_payl_internal_to_external(payload_internal& internal_str, payload& payload_str) {
	// payload parameters
	payload_str.string_length_	      = internal_str.string_length_;
	payload_str.vector_size_	      = internal_str.vector_size_;
	payload_str.copy_msg_		      = internal_str.copy_msg_ > 0;
	payload_str.batch_size_		      = internal_str.batch_size_;
	payload_str.actor_computing_time_ = std::chrono::microseconds(internal_str.actor_computing_time_);
}

void benchmark_config::construct_filename(general& external_struct) {

	auto filename = external_struct.output_file_
	    + "_"
	    + external_struct.type_;

	//checking dot before extension
	if (external_struct.extension_[0] != '.')
		filename += '.' + external_struct.extension_;
	else
		filename += external_struct.extension_;

	// checking backslash at the end of path
	auto size_path = external_struct.result_path_.size();
	auto path = external_struct.result_path_;

	if (path[size_path-1] != '/' && path[size_path - 1] != '\\')
		path += "/";

	external_struct.result_file_ = path + filename;
}

benchmark_config& benchmark_config::parse()
{
    actor_system_config::parse (argc__, argv__, path().c_str());
    convert_gen_internal_to_external(general_internal_, general_);
    convert_payl_internal_to_external(payload_internal_, payload_);
    construct_filename(general_);
    return *this;
}

bool benchmark_config::is_valid() const
{
	return general_.pipeline_actors_ > 1;
}
