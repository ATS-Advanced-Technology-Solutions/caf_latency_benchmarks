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

#include <limits.h>

#include <caf/actor_system_config.hpp>

//! Benchmark configuration
class benchmark_config : public caf::actor_system_config
{
    int    argc__{0};
    char** argv__{nullptr};
    std::string   ini_file_;

public:
    
    //! General informations of all benchmark's types
    struct general {
	
	//! Benchmark type: currently, the allowed values are serial and parallel
	std::string type_;
	/// Payload type:: currently, the allowed values are none and complex
	std::string payload_type_;
	/// Tracking type:: currently, the allowed values are latency and cross times
	std::string tracking_type_;
	
	//! Actors in the benchmark's pipeline
	unsigned int pipeline_actors_;
	
	//! This parameter indicates the benchmark's duration (in seconds)
	std::chrono::seconds duration_;
	
	//! Folder path in which store the results
	std::string result_path_;
	
	//! Extension of the output file
	std::string extension_;
	
	//! File name in which store the results
	std::string output_file_;
	
	//! Composition of path + file name + 'test type' + extension
	std::string result_file_;
	
	//! To spawn the x-th actor in detached mode (x belongs to [0, pipeline_actors_-1] interval, otherwise no detached actor will be spawned);
	unsigned int detached_actor_;
	
	//! Defines the message route
	bool forth_and_back_;
    };
    
    //! Information about the parallel benchmark type
    struct parallel {
	//! Frequency of messages introduced into benchmark's pipeline
	double rate_;
	
	//! Number of chains in 1 to N configuration
	int chains_number_;
	
	//! Message sending order to chains' heads (0 = random, 1 = Round Robin)
	int sending_order_;
	
	parallel() :
	    rate_(100.0),
	    chains_number_(1),
	    sending_order_(1) {}
    };
    
    struct payload {
	/// The length of the string into a complex (or crosstime) message
	unsigned int string_length_;
	
	/// The size of the vector<char> into a complex (or crosstime) message.
	unsigned int vector_size_;

	//! Copy complex message
	bool copy_msg_;
	
	//! Batch size (to obtain "square wave" in output)
	unsigned int batch_size_;
	
	//! Actor work time (microseconds)
	std::chrono::microseconds actor_computing_time_;
	
	payload() :
	    string_length_(16),
	    vector_size_(500),
	    copy_msg_(false),
	    batch_size_(0),
	    actor_computing_time_(0) {}
	
    };
    
    benchmark_config(const std::string& cfg_ini_file, int argc, char** argv);

    //! Parse che input. Always call before reading the data!
    benchmark_config& parse();
    
    //! Returns true if the configuration is consistent
    bool is_valid() const;

    
    // return configuration file path
    std::string path () const
    {
	std::string path = folder();
        if (*rbegin (path) != '/' && *rbegin (path) != '\\') 
            path += '/';
        path += ini_file_.empty () ? std::string ("component.ini") : ini_file_;
        return path;
    }


    // return configuration file folder, based on command line (default is "config")
    std::string folder () const
    {
        return argc__ < 2 || *argv__[1] == '-' ? 
            "config" :  argv__[1];
    }

    
    general general_;
    parallel parallel_;
    payload payload_;

    static benchmark_config* instance_;
    
private:
    struct general_internal {
	std::string type_str_;
	std::string payload_type_str_;
	std::string tracking_type_str_;
	unsigned int payload_string_lenght_;
	unsigned int payload_vector_size_;
	unsigned int pipeline_actors_;
	unsigned int duration_;
	std::string result_path_;
	std::string extension_;
	std::string output_file_;
	unsigned int detached_actor_;
	unsigned int forth_and_back_;
	
	//default values
	general_internal()
	    : type_str_("SERIAL"),
	      payload_type_str_("NONE"),
	      tracking_type_str_("LATENCY"),
	      pipeline_actors_(2),
	      duration_ (10),
	      result_path_("./trace/"),
	      extension_("txt"),
	      output_file_("benchmark_result"),
	      detached_actor_(UINT_MAX),
	      forth_and_back_(false)
	{}
    };
    
    struct payload_internal {
	unsigned int string_length_;
	unsigned int vector_size_;
	unsigned int copy_msg_;
	unsigned int batch_size_;
	unsigned int actor_computing_time_;
	
	payload_internal() :
	    string_length_(16),
	    vector_size_(500),
	    copy_msg_(0),
	    batch_size_(0),
	    actor_computing_time_(0)
	{}
    };
   
    general_internal general_internal_;
    payload_internal payload_internal_;
    
    static void convert_gen_internal_to_external(general_internal& internal_struct, general& external_struct);
    static void convert_payl_internal_to_external(payload_internal& internal_struct, payload& payload_struct);
    
    //! Construct output filename as path+filename+description+extension
    static void construct_filename(general&);

};

static inline benchmark_config& benchmark_cfg() {
    if (!benchmark_config::instance_)
	throw std::logic_error ("no configuration instance found");
    return *benchmark_config::instance_;
}
