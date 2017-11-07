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

using init_a  = caf::atom_constant<caf::atom("init")>;
using make_linear_chain_a = caf::atom_constant<caf::atom("chain_b")>;
using topo_kill_a = caf::atom_constant<caf::atom("kill")>;

caf::actor spawn_topology_builder(caf::actor_system& system);