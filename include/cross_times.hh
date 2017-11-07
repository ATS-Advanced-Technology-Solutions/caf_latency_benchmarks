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

#include <vector>

#include "times.hh"

#include <caf/meta/type_name.hpp>



#include "cross_times_hops.hh"

namespace _0lf
{
	//! Cross latency time structure
    class cross_times
    {
    public:

        //! Constructor, defaults reserving 10 hops, specify expectd hops otherwise
        cross_times (size_t reserve = 10) noexcept : reserved_hops_ (reserve) { }

        //! Clears cross times
        void clear() noexcept
        {   origin_ts_ = {}; hops_.clear (); }

        //! Adds hop signature for current time (init if empty), version for atom_values
        template <typename  ... H>
        inline void add (H ... hop_sig)
        {
            if (hops_.empty ())
            {   hops_.reserve (reserved_hops_);	// default to avoid frequent realloc
                origin_ts_ = clock::now(); }
			append_hop(std::forward<H>(hop_sig)...);
        }

	private:
		template<typename ...H>
		void append_hop (H... hop_sig)
		{
			auto hr_now = hr_clock::now();
			if (hop::process_atom_ != hops_.back().component_)
			{
				auto sys_now = clock::now();
				auto d_hr = (hops_.back().hop_tp_ - hr_now) + (origin_ts_ - sys_now);
				for (auto& h : hops_)
					h.hop_tp_ -= d_hr;
				origin_ts_ = sys_now;
			}
			hops_.emplace_back (hop{std::move(hr_now), std::forward<H>(hop_sig)...});
		}
	public:

        //! Returns total latency of this `cross_times` object in nanoseconds
        inline auto latency () const noexcept
        {   if (hops_.size () < 2) return std::chrono::nanoseconds::zero();
            return std::chrono::duration_cast <std::chrono::nanoseconds> (hops_.rbegin ()->hop_tp_ - hops_.begin ()->hop_tp_); }

        //! Returns origin time point (i.e. first hop time point)
        inline tpoint origin() const noexcept
        { return origin_ts_; }

        //! Returns total latency of this cross times
        inline auto hops() const noexcept
        { return hops_.size (); }

        //! Returns given hop, throws if out of range
        inline const hop& at (size_t n) const
        { return hops_.at (n); }

        //! Returns true if contains some data
        inline operator bool () const noexcept { return hops () > 0; }

        //! Increments the hop durations with correspoding durations of another `cross_times` object
        /*!
        \throw std::logic_error 
        - `cross_times` with different hops count
        - `cross_times` with not comparable hops
        */
        cross_times& operator += (const cross_times& rhv);
      
        //! Returns a `cross_times` object which hop durations are the sum of given `cross_times` \see operator+=()
        cross_times operator + (const cross_times& rhv) const;
      
      
		//! Divides the duration of hops by the given value
        template <typename I>
            cross_times& operator /= (I value)
        {   static_assert (std::is_arithmetic<I>::value, "operator / on cross_times only accepts an arithmetic type divisor");
            for (unsigned int i = 1; i < hops_.size (); ++i)
                hops_[i].hop_tp_ = hops_[0].hop_tp_ + ((hops_[i].hop_tp_ - hops_[0].hop_tp_) / value);
            return *this;
		}

        //! Returns a `cross_times` object which hop durations diveded by the given value \see operator/=()
        template <typename I>
		    cross_times operator / (I value) const
        {   cross_times xt (*this); xt /= value; return xt; }

	private:
        size_t reserved_hops_ = {};
		tpoint origin_ts_ = {};
		std::vector<hop> hops_ = {};

        // cross latency structure inspector
        template <class Inspector> friend typename Inspector::result_type inspect (Inspector& f, cross_times& x)
        {   return f (caf::meta::type_name ("cross_times"), x.origin_ts_, x.hops_);   }

        friend std::string to_string (const cross_times& xt);
    };

    //! Returns a string representation of a `cross_times` object, eventually reduced to specified detail \see cross_times::reduce()
    std::string to_string (const cross_times& xt);

}    // end namespace _0lf

