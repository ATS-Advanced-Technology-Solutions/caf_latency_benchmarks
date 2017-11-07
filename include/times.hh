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

#include <chrono>
#include <string>

#include <caf/allowed_unsafe_message_type.hpp>


namespace _0lf
{
    //////////////////////////////////////////////////////
    // timestamping types

    //! Standard clock to be used for absolute times
    using clock = std::chrono::system_clock;

    //! Standard clock duration (nanoseconds on an 64 bit integer)
    using tduration = std::chrono::duration<int64_t, std::nano>;

    //! Standard UTC time point to be used for absolute times, ns resolution
    using tpoint = std::chrono::time_point <clock, tduration>;

    //! High resolution clock to be used for latency, relative times
    using hr_clock = std::chrono::steady_clock;

    //! High resolution relative time point to be used for latency, relative times
    using hr_tpoint = std::chrono::time_point <hr_clock>;

    // to_string overrides

    std::string to_string (const tpoint& tp);

    std::string to_string (const hr_tpoint& tp);


}    // end namespace _0lf

CAF_ALLOW_UNSAFE_MESSAGE_TYPE (_0lf::hr_tpoint);

