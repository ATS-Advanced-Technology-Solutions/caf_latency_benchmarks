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

#include "times.hh"

namespace _0lf
{
    //! Single latency probe template
    /*!
      Usage:
      \code
          latency <milliseconds> my_probe;            // create and start the probe with given duration type (latency<> defaults to nanoseconds)
          cout << "intermediate: " << my_probe();     // use intermediate value if required
          cout << "restart: " << my_probe.restart();  // restart count using intermediate value if required
          my_probe.stop();                            // stop probe when finished
          cout << "latency: " << my_probe();          // use final value (same as intermediate)
          my_probe.start();                           // restart probe if required
      \endcode
    */
    template <typename D = std::chrono::nanoseconds>
    class latency
    {
    public:
        //! operator() result type
        using result_type = D;

        //! Constructor, starts latency count
        inline latency (bool start_now = true) noexcept { if (start_now) start_ = hr_clock::now (); };

        //! (re)starts counting, returning latency counted so far
    	inline D restart() noexcept { auto so_far = operator()(); start_ = (stop_ != hr_tpoint {} ? move(stop_) : hr_clock::now ()); stop_ = {}; return so_far; }

        //! Stop counting
    	inline latency& stop() noexcept { if (stop_ != hr_tpoint{}) stop_ = hr_clock::now(); return *this; }

        //! Gets latency (takes an intermediate if stop() not called)
        inline D operator()() const noexcept { return start_ == hr_tpoint {} ? D{} : std::chrono::duration_cast<D>((stop_ != hr_tpoint {} ? stop_ : hr_clock::now ()) - start_); }

    private:
        hr_tpoint start_ = {},
	    stop_ = {};
    };


    //! Average value template
    /*!
      Usage:
      \code
          average_value <size_t> my_avg;                      // create the probe with given type (e.g. a duration, a size_t, a double value...)
          my_avg.tick(msg_size)                               // tick a value 
          cout << "average size: " << my_avg() << "bytes";    // use average value so far
          cout << "ticks so far: " << my_avg.ticks();         // use counted ticks so far
          my_avg.reset();                                     // reset average if required
      \endcode
    */
    template <typename V>
    class average_value
    {
    public:

        //! operator() result type
        using result_type = V;

        //! Constructor
        inline average_value () noexcept = default;

        //! Counts the given value
        inline average_value& tick (V value) noexcept { sum_ += value; num_samples_++; return *this; };

        //! Returns number of ticks so far
        inline uint64_t ticks () noexcept { return num_samples_; }

        //! Resets counter
        inline void reset () noexcept { sum_ = {}; num_samples_ = { 0 }; }

        //! Gets average value from creation or last reset() call
        inline V operator()() const noexcept { if (!num_samples_) return V {}; return static_cast<V>(sum_ / num_samples_); }

    private:
        V sum_ {};  // default constructs to 0 for given type
        uint64_t num_samples_ { 0 };
    };


    // if required, adjust a mobile window sampling array circular index to current time
    template <typename P, int N, bool return_period = false, class A, typename TP, typename I>
    inline double adjust_circular_array (
					 A& data_array,      // the data array being adjusted
					 TP& origin_tp,      // the sampling origin, will be updated to current time if required
					 I& absolute_idx     // the current absolute index, will be updated if required
					 ) noexcept
    {
	if (origin_tp == TP {}) // no data yet, initialise and return
	    {
		origin_tp = TP::clock::now (); 
		return N - 1;	// always compute average on a minimum of N-1 periods, to provide ramp-up
	    }

	auto absolute_elapsed = TP::clock::now() - origin_tp;


	auto new_absolute_idx = static_cast<I>(   // new absolute index according to current time
					       std::chrono::duration_cast<P>(absolute_elapsed).count ());

	double period = return_period ?
	    (N - 1 +				// periods to compute average range between N-1 and N
	     std::chrono::duration_cast 
	     <std::chrono::duration <double, typename P::period>>	// cast to P duration type represented as double
	     (absolute_elapsed % P(1)).count() ) :	// module to 1 P duration 
	    0.0;	// no return period required

	if (absolute_idx == new_absolute_idx) 
	    return period;  // no change, do nothing, just return new period

	for (auto end_idx = new_absolute_idx % N;;)   // starting from next absolute index ...
	    {
		auto zero_idx = ++absolute_idx % N;       // ... using % to loop only once ...
		data_array [zero_idx] = {};               // ... empty array element ...

		if (zero_idx == end_idx) 
		    break;
	    }                                             // ... up to new absolute index included

	absolute_idx = new_absolute_idx;

	return period;
    }


    //! Mobile window average value template
    /*!
      Usage:
      \code
          mobile_average_value <double, seconds, 10> my_avg;  // create the probe with given type (double here) and window (10 seconds here)
          my_avg.tick (current_cats_per_second);              // tick a value
          cout << "average speed: " << my_avg() << "cats/s";  // use average value so far
          cout << "ticks so far: " << my_avg.ticks();         // use counted ticks so far
          cout << "mobile window: " << my_avg.window();       // use sampling mobile window size (a duration)
          my_avg.reset();                                     // reset average if required
      \endcode
    */
    template <typename V,
	      typename P = std::chrono::seconds,
	      int N = 5>
    class mobile_average_value
    {
    public:
        //! operator() result type
        using result_type = V;

        //! Sampling period mobile window size
        constexpr auto window() noexcept
        {   return P{N}; }

        //! Constructor
        inline mobile_average_value () = default;

        //! Counts the given value
        inline mobile_average_value& tick (V value) noexcept
        {
            adjust_circular_array<P, N> (values_, origin_tp_, absolute_idx_);  // adjust to current time
            auto& pv = values_[absolute_idx_ % N]; // take current slot
            pv.first += value; pv.second++;
            return *this; }

        //! Returns number of ticks so far
        inline uint64_t ticks () noexcept
        {   
            uint64_t num_samples { 0 };
            for (auto pv : values_) num_samples += pv.second;
            return num_samples; }

        //! Resets counter
        inline void reset () noexcept  {   origin_tp_ = {};  absolute_idx_ = { 0 }; }

        //! Gets average value over last sampling period
        inline V operator()() noexcept
        {
            adjust_circular_array <P, N> (values_, origin_tp_, absolute_idx_);  // adjust to current time
            V sum {}; uint64_t num_samples { 0 };
            for (auto pv : values_) // sum values and samples
		{   sum += pv.first; num_samples += pv.second; }
            if (!num_samples) return V {};  // return average, or 0 value if no samples
            return static_cast<V>(sum / num_samples); }

    private: 
        using period_value = std::pair<V, uint64_t>;     // sum of values and number of samples for one period
        std::array<period_value, N> values_ {};          // period array
        size_t absolute_idx_ = { 0 };                    // absolute index of current sample
        hr_tpoint origin_tp_ {};                         // origin time of period array
    };


    //! Mobile window average ticks per period template
    /*!
      Usage:
      \code
          mobile_average_ticks <seconds, 30> my_avg;          // create the probe with given type (double here) and window (30 seconds here, default 5 seconds)
          my_avg.tick();                                      // tick a value
          cout << "average rate: " << my_avg() << "msg/s";    // use average ticks per period so far
          cout << "mobile window: " << my_avg.window();       // use sampling mobile window size (a duration)
          my_avg.reset();                                     // reset average if required
      \endcode
    */
    template <typename P = std::chrono::seconds,
	      int N = 5>
    class mobile_average_ticks
    {
    public:

        //! Sampling period mobile window size
        constexpr auto window() noexcept
        {   return P{N}; }

        //! Constructor
        inline mobile_average_ticks () = default;

        //! Count the given value
        inline mobile_average_ticks& tick () noexcept
        {
            adjust_circular_array <P, N> (ticks_, origin_tp_, absolute_idx_);  // adjust to current time
            ticks_[absolute_idx_ % N]++; // increment ticks of slot
            return *this;
        }

        //! Resets counter
        inline void reset () noexcept { origin_tp_ = {};  absolute_idx_ = { 0 }; }

        //! Gets average ticks per declare period unit over last sampling window
        inline double operator()() noexcept
        {
            auto periods = adjust_circular_array <P, N, true> (ticks_, origin_tp_, absolute_idx_);  // adjust to current time
            uint64_t tick_sum { 0 };
            for (auto t : ticks_) tick_sum += t; // sum values and samples
            return tick_sum / periods;
        }

    private:
        std::array<uint64_t, N> ticks_ {};  // period array
        size_t absolute_idx_ = { 0 };       // absolute index of current sample
        hr_tpoint origin_tp_ {};            // origin time of period array
    };


    //! Mobile window average rate template
    /*!
      Usage:
      \code
          mobile_average_rate <double, seconds, 10> my_avg;	// create the probe with given type (double here) and window (10 seconds here)
          my_avg.tick (current_cats);							// tick a value
          cout << "average speed: " << my_avg() << "cats/s";	// use average value so far
          cout << "ticks so far: " << my_avg.ticks();			// use counted ticks so far
          cout << "mobile window: " << my_avg.window();		// use sampling mobile window size (a duration)
          my_avg.reset();										// reset average if required
      \endcode
    */
    template <typename V,
	      typename P = std::chrono::seconds,
	      int N = 5>
    class mobile_average_rate
    {
    public:
	//! operator() result type
	using result_type = V;

	//! Sampling period mobile window size
	constexpr auto window() noexcept
	{
	    return P{ N };
	}

	//! Constructor
	inline mobile_average_rate() noexcept = default;

	//! Counts the given value
	inline mobile_average_rate& tick(V value) noexcept
	{
	    adjust_circular_array<P, N>(values_, origin_tp_, absolute_idx_);  // adjust to current time
	    auto& pv = values_[absolute_idx_ % N]; // take current slot
	    pv += value;
	    return *this;
	}

	//! Resets counter
	inline void reset() noexcept { origin_tp_ = {};  absolute_idx_ = { 0 }; }

	//! Gets average value per period unit. Return value may not be of type V
	inline auto operator()() noexcept
	{
	    double periods = adjust_circular_array <P, N, true>(values_, origin_tp_, absolute_idx_);  // adjust to current time
	    V sum{};
	    for (auto pv : values_) sum += pv;// sum values and samples
	    return sum / periods;
	}

    private:
	std::array<V, N> values_{};				        // period array
	size_t absolute_idx_ = { 0 };					// absolute index of current sample
	hr_tpoint origin_tp_{};                         // origin time of period array
    };

}    // end namespace _0lf
