/*****************************************************************************
*                                                                            *
* Copyright (C) 2017                                                         *
* ATS S.p.A - Milano                                                         *
* http://atscom.it - https://github.com/ATS-Advanced-Technology-Solutions    *
*                                                                            *
* Leveraging CAF - C++ Actor Framework < https://www.actor-framework.org/ >  *
*                                                                            *
*****************************************************************************/


#include "times.hh"
#include "date.h"

using namespace std;


namespace _0lf
{
    using std::to_string;

    //////////////////////////////////////////////
    // time helpers

    string to_string (const hr_tpoint& tp)
    {
        return "hrt<" + to_string (tp.time_since_epoch ().count ()) + ">";
    }


    string to_string (const tpoint& tp)
    {
        ostringstream oss;
        date::operator<< (oss, tp);
        return oss.str ();
    }

}    // end namespace _0lf

