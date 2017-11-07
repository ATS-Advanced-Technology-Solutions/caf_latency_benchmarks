/*****************************************************************************
* 0LF                                                                        *
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#pragma once

#include <caf/atom.hpp>


namespace _0lf
{
    // forward declarations

    class cross_times;

    std::string to_string (const cross_times& xt);

    //! Cross latency time hop structure
    struct hop
    {
		//! Route types
        enum route : uint32_t
        {
        	no_route    = 0, //!< No indication on route
            forth       = 1, //!< Hop belongs to the forth path
            back        = 2  //!< Hop belongs to the back path
        };

        //! Hop step
        enum step  : uint32_t
        {
        	in      = 0,               //!< entering actor 
            out     = uint32_t(-1),    //!< exiting actor 
            no_step = 0                //!< default, same a step::in
        };

        //! Default constructor (empty hop)
        hop () = default;

        //! Constructor for `cross_times` emplace calls (with route) \see cross_times::add()
        hop (hr_tpoint&& tp, route r, caf::atom_value a, step hs = no_step) :
            hop_tp_ (std::forward<hr_tpoint> (tp)),
            route_ (r),
            component_ (process_atom_), 
            actor_ (a), 
            step_ (hs) { }

        //! constructor for cross_times emplace calls (no route) \see cross_times::add()
        hop (hr_tpoint&& tp, caf::atom_value a, step hs = no_step) :
            hop_tp_ (std::forward <hr_tpoint> (tp)),
            route_ (no_route),
            component_ (process_atom_),
            actor_ (a), 
            step_ (hs) { }

		//! Checks if two hops are of same type (all parameters except time)
		bool operator==(const hop& rhv) const {
			return route_ == rhv.route_ &&
			       component_ == rhv.component_ &&
			       actor_ == rhv.actor_ &&
			       step_ == rhv.step_;
		}

		//! Checks if two hops are of different types (all parameters except time)
		bool operator!=(const hop& rhv) const {
			return !operator==(rhv);
		}

		//! Hop timestamp (high resolution)
		hr_tpoint hop_tp_ = {};

		//! Route flag
		route route_ = { no_route };

		//! Component (process) identifier
		caf::atom_value component_ = {};
		
		//! Actor identifier
		caf::atom_value actor_ = {};

		//! Hop step
		step step_ = { no_step };

		//! Process name, initialized by _0lf::_save_command_line()
        static caf::atom_value process_atom_;
    };


    // hop structure inspector
    template <class Inspector> typename Inspector::result_type inspect (Inspector& f, hop& x)
    {
        return f (caf::meta::type_name ("hop"), x.hop_tp_, reinterpret_cast<uint32_t&>(x.route_), x.component_, x.actor_, reinterpret_cast<uint32_t&>(x.step_));
    }

}    // end namespace _0lf
