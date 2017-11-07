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


#include <sstream>

#include "cross_times.hh"


using namespace std;
using namespace std::chrono;
using namespace caf;
using namespace _0lf;


namespace _0lf
{

    atom_value hop::process_atom_ = {};

    cross_times& cross_times::operator+=(const cross_times& rhv)
    {
        if (hops_.empty ()) 
        {
            *this = rhv;
        }

        // size check
        if (hops () != rhv.hops ()) 
        {
            throw std::logic_error ("cannot apply operator+= between cross_times with different sizes");
        }

        // comparable hops check
        for (unsigned int i = 0; i < hops_.size (); ++i) 
        {
            if (hops_[i] != rhv.hops_[i]) 
            {
                throw std::logic_error ("cannot apply operator+= between hops that are not comparable");
            }
        }

        for (unsigned int i = 1; i < hops_.size (); ++i) 
        {
            hops_[i].hop_tp_ += rhv.hops_[i].hop_tp_ - rhv.hops_[0].hop_tp_;
        }

        this->origin () = tpoint ();

        return *this;
    }

    cross_times cross_times::operator+(const cross_times& rhv) const 
    {
        cross_times xt (*this);
        xt += rhv;
        return xt;	//RVO
    }
  
  
  string to_string (const cross_times& xt)
  {
    ostringstream oss;
    
    oss << "latency=" << xt.latency().count() << "ns";
    
    if (!xt.hops_.empty ())
    {
	    hr_tpoint last_tp = {};
	    int n = 0;
	
	    for (auto h : xt.hops_) 
        {
            oss << " " << ++n << "=(";
            if (h.route_ != hop::no_route) oss << (h.route_ == hop::forth ? "forth" : "back");
            if (h.component_ != atom_value {})
            {   if (h.route_ != hop::no_route)   oss << ".";
            oss << to_string (h.component_); }
            if (h.actor_ != atom_value {})  oss << "." << to_string (h.actor_);
            if (h.step_ == hop::in) oss << ".in @ ";
            else if (h.step_ == hop::out) oss << ".out @ ";
            else oss << "." << h.step_ << " @ ";
            if (last_tp == hr_tpoint {}) oss << to_string (xt.origin_ts_) << ")";
            else oss << "+" << duration_cast<nanoseconds>(h.hop_tp_ - last_tp).count () << "ns)";
	    
            last_tp = h.hop_tp_;
        }
    }
    
    return oss.str ();
    }

}    // end namespace _0lf
