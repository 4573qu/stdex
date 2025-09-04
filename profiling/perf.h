//Last Modified At 2025/09/04
//@Version 1.0.0.0
#ifndef _STD4573_PROFILING_PERF_H_
#define _STD4573_PROFILING_PERF_H_ 1

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#if __cplusplus >= 202002L
#include <format>
#include <source_location>
#else
#include <iomanip>
#include <sstream>
#endif

namespace stdex {

namespace perf {

enum TIME_UNIT {
	TU_NANOSECONDS,
	TU_MICROSECONDS,
	TU_MILLISECONDS,
	TU_SECONDS,
};

constexpr const char* TU_to_string(TIME_UNIT unit) {
	switch (unit) {
		case TU_NANOSECONDS:  	return "ns";
		case TU_MICROSECONDS: 	return "μs";
		case TU_MILLISECONDS: 	return "ms";
		case TU_SECONDS:		return "s";
		default:				return "unknown";
	}
}

class perf_timer {
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start_;
	std::chrono::time_point<std::chrono::high_resolution_clock> end_;
	bool running_=false;

private:
    template <TIME_UNIT _Unit>
	static double get_duration(std::chrono::time_point<std::chrono::high_resolution_clock> start,std::chrono::time_point<std::chrono::high_resolution_clock> end) {
		if constexpr (_Unit==TU_NANOSECONDS) {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
		} else if constexpr (_Unit==TU_MICROSECONDS) {
			return std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
		} else if constexpr (_Unit==TU_MILLISECONDS) {
			return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
		} else if constexpr (_Unit==TU_SECONDS) {
			return std::chrono::duration_cast<std::chrono::duration<double>>(end-start).count();
		} else {
			throw std::invalid_argument("Invalid TIME_UNIT");
		}
	}

public:
	void start() {
		start_=std::chrono::high_resolution_clock::now();
		running_=true;
	}

	void stop() {
		if (running_) {
			end_=std::chrono::high_resolution_clock::now();
			running_=false;
		}
	}

	template <TIME_UNIT _Unit=TU_MILLISECONDS>
	double elapsed() const {
		static_assert(_Unit==TU_NANOSECONDS || _Unit==TU_MICROSECONDS || _Unit==TU_MILLISECONDS || _Unit==TU_SECONDS,"Invalid TIME_UNIT specified");
		if (running_) {
			auto current=std::chrono::high_resolution_clock::now();
			return get_duration<_Unit>(start_,current);
		} else return get_duration<_Unit>(start_,end_);
	}

	void reset() {
		running_=false;
		start_=std::chrono::time_point<std::chrono::high_resolution_clock>{};
		end_=std::chrono::time_point<std::chrono::high_resolution_clock>{};
	}

	bool is_running() const {
		return running_;
	}

	std::string format(TIME_UNIT unit=TU_MILLISECONDS,int precision=6) const {
		double time=0.0;
		switch (unit) {
			case TU_NANOSECONDS:	time=elapsed<TU_NANOSECONDS>(); break;
			case TU_MICROSECONDS:	time=elapsed<TU_MICROSECONDS>(); break;
			case TU_MILLISECONDS:	time=elapsed<TU_MILLISECONDS>(); break;
			case TU_SECONDS:		time=elapsed<TU_SECONDS>(); break;
			default: 				throw std::invalid_argument("Invalid TIME_UNIT");
		}
#if __cplusplus >= 202002L
		return std::format("{:.{}f} {}",time,precision,TU_to_string(unit));
#else
		std::ostringstream ss;
		ss<<std::fixed<<std::setprecision(precision)<<time<<" "<<TU_to_string(unit);
		return ss.str();
#endif
	}
};

class scope_timer {
private:
	perf_timer timer_;
	std::string name_;
	TIME_UNIT unit_;
	std::ostream* output_;
#if __cplusplus >= 202002L
	std::source_location location_;
#endif

public:
#if __cplusplus >= 202002L
	scope_timer(std::string_view name="",TIME_UNIT unit=TU_MILLISECONDS,std::ostream* output=&std::cout,const std::source_location& location=std::source_location::current()) : name_(name) , unit_(unit) , output_(output) , location_(location) {
		timer_.start();
    }
#else
	scope_timer(std::string_view name="",TIME_UNIT unit=TU_MILLISECONDS,std::ostream* output=&std::cout) : name_(name) , unit_(unit) , output_(output) {
		timer_.start();
	}
#endif
    ~scope_timer() {
		timer_.stop();
		if (output_) {
#if __cplusplus >= 202002L
			(*output_)<<std::format("[{}] {}: {}\n",location_.function_name(),name_.empty()?"Elapsed":name_,timer_.format(unit_));
#else
			if (!name_.empty()) (*output_)<<"["<<name_<<"] Elapsed: "<<timer_.format(unit_)<<"\n";
			else (*output_)<<"Elapsed: "<<timer_.format(unit_)<<"\n";
#endif
		}
	}

	const perf_timer& get_timer() const {
        return timer_;
    }
};

class benchmark {
private:
	static std::map<std::string,double> calculate_statistics(const std::vector<double>& times,TIME_UNIT unit) {
		if (times.empty()) return {};
		double min=times[0];
		double max=times[0];
		double sum=0.0;
		for (double time:times) {
			if (time<min) min=time;
			if (time>max) max=time;
			sum+=time;
		}
		double avg=sum/times.size();
		double variance=0.0;
		for (double time:times) variance+=(time-avg)*(time-avg);
		variance/=times.size();
		double stddev=std::sqrt(variance);
        std::map<std::string,double> stats;
		stats["min"]=min;
		stats["max"]=max;
		stats["avg"]=avg;
		stats["stddev"]=stddev;
		stats["total"]=sum;
		stats["runs"]=times.size();
		return stats;
	}

public:    
    static std::map<std::string,double> run_benchmark(const std::function<void()>& func,int iterations=100,TIME_UNIT unit=TU_MILLISECONDS) {
		if (iterations<=0) throw std::invalid_argument("Iterations must be positive");
		std::vector<double> times;
		times.reserve(iterations);
		perf_timer timer;
		for (int i=0;i<iterations;i++) {
			timer.start();
			func();
			timer.stop();
			switch (unit) {
				case TU_NANOSECONDS:  	times.push_back(timer.elapsed<TU_NANOSECONDS>()); break;
				case TU_MICROSECONDS: 	times.push_back(timer.elapsed<TU_MICROSECONDS>()); break;
				case TU_MILLISECONDS: 	times.push_back(timer.elapsed<TU_MILLISECONDS>()); break;
				case TU_SECONDS:      	times.push_back(timer.elapsed<TU_SECONDS>()); break;
				default: 				throw std::invalid_argument("Invalid TIME_UNIT");
			}
		}
		return calculate_statistics(times,unit);
	}
};

template <TIME_UNIT _Unit=TU_MILLISECONDS,typename _Func,typename... _Args>
auto measure(_Func&& func,_Args&&... args) {
	static_assert(_Unit==TU_NANOSECONDS || _Unit==TU_MICROSECONDS || _Unit==TU_MILLISECONDS || _Unit==TU_SECONDS,"Invalid TIME_UNIT specified");
	perf_timer timer;
	timer.start();
	if constexpr (std::is_void_v<std::invoke_result_t<_Func,_Args...>>) {
		std::invoke(std::forward<_Func>(func),std::forward<_Args>(args)...);
		timer.stop();
		return timer.elapsed<_Unit>();
	} else {
		auto result=std::invoke(std::forward<_Func>(func),std::forward<_Args>(args)...);
		timer.stop();
		return std::make_pair(result,timer.elapsed<_Unit>());
	}
}

#define _STDEX_PERF_TIMER(unit) stdex::perf::scope_timer STDEX_PERF_TIMER_##__LINE__("",unit)
#define _STDEX_NAMED_PERF_TIMER(name, unit) stdex::perf::scope_timer PERF_TIMER_##__LINE__(name,unit)

}

}

#endif