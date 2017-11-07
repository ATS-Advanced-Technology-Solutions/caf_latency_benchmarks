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
#include "benchmark_config.hh"

void spawn_rate_generator(caf::actor_system& system,	//! Actor system
	std::vector<caf::actor> heads,						//! Container with heads
	caf::actor results_collector						//! Results collector handle
	);
