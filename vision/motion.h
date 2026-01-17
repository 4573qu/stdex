//Last Modified At 2026/01/17
//@Version 1.0.1.0
#ifndef _STDEX_VISION_MOTION_H_
#define _STDEX_VISION_MOTION_H_ 1

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace stdex {

namespace vision {

struct motion_scalar {
	double position{0.0};
	double velocity{0.0};
	double acceleration{0.0};
};

struct motion_state {
	std::vector<motion_scalar> scalars;
	double time{0.0};

	motion_state()=default;
	explicit motion_state(std::size_t channels) : scalars(channels,{0.0,0.0,0.0}) { }
	static motion_state single(motion_scalar s,double time=0.0) {
		motion_state result(1);
		result.scalars[0]=s;
		result.time=time;
		return result;
	}
};

enum time_behavior {
	TB_FREE,
	TB_CLAMP,
	TB_LOOP,
	TB_OSCILLATE,
};

inline double map_time(double t,double duration,time_behavior tb) noexcept {
	if (tb==TB_FREE || !(duration>0.0)) return t;
	if (tb==TB_CLAMP) {
		if (t<0.0) return 0.0;
		if (t>duration) return duration;
		return t;
	}
	if (tb==TB_LOOP) {
		double m=std::fmod(t,duration);
		if (m<0.0) m+=duration;
		return m;
	}
	if (tb==TB_OSCILLATE) {
		const double p=2.0*duration;
		double m=std::fmod(t,p);
		if (m<0.0) m+=p;
		if (m<=duration) return m;
		return p-m;
	}
	return 0.0;
}

using prototype_func=std::function<motion_scalar(double)>;

class motion {
public:
	using eval_func=std::function<motion_state(double)>;
	using per_channel_func=std::function<motion_scalar(std::size_t,double)>;

private:
	eval_func func_{};
	std::size_t channels_{0};
	double duration_{0.0};
	time_behavior behavior_{TB_FREE};

	static motion_state ensure(motion_state s,std::size_t ch,double mapped_t) {
		if (s.scalars.size()!=ch) s.scalars.resize(ch,{0.0,0.0,0.0});
		s.time=mapped_t;
		return s;
	}

public:
	motion()=default;
	motion(eval_func func,std::size_t channels,double duration=0.0,time_behavior behavior=TB_FREE) : func_(std::move(func)) , channels_(channels) , duration_(duration) , behavior_(behavior) {
		if (channels_<=0) throw std::invalid_argument("motion channels must be positive");
	}

	bool empty() const noexcept { return !func_; }

	std::size_t channels() const noexcept { return channels_; }
	double& duration() noexcept { return duration_; }
	const double& duration() const noexcept { return duration_; }
	time_behavior& behavior() noexcept { return behavior_; }
	time_behavior behavior() const noexcept { return behavior_; }

	motion with_duration(double d) const {
		motion r=*this;
		r.duration_=d;
		return r;
	}
	motion with_time_behavior(time_behavior tb) const noexcept {
		motion r=*this;
		r.behavior_=tb;
		return r;
	}
	double map_time(double t) const noexcept {
		return stdex::vision::map_time(t,duration_,behavior_);
	}
	double progress(double t) const noexcept {
		if (!(duration_>0.0)) return 0.0;
		double tt=map_time(t);
		if (tt<=0.0) return 0.0;
		if (tt>=duration_) return 1.0;
		return tt/duration_;
	}
	bool finished(double t) const noexcept {
		if (behavior_==TB_LOOP || behavior_==TB_OSCILLATE) return false;
		if (!(duration_>0.0)) return false;
		return t>=duration_;
	}
	motion_state eval(double t) const {
		const double tt=map_time(t);
		if (!func_) return ensure(motion_state(),channels_,tt);
		return ensure(func_(tt),channels_,tt);
	}
	std::vector<motion_state> sample_states(double t0,double t1,std::size_t n) const {
		std::vector<motion_state> result;
		result.reserve(n+1);
		if (n==0) return {eval(t0)};
		const double dt=(t1-t0)/static_cast<double>(n);
		for (std::size_t i=0;i<=n;i++) result.push_back(eval(t0+dt*static_cast<double>(i)));
		return result;
	}
	std::vector<double> sample_positions(std::size_t channel,double t0,double t1,std::size_t n) const {
		if (channel>=channels_) throw std::out_of_range("channel out of range");
		std::vector<double> result;
		result.reserve(n+1);
		if (n==0) return {eval(t0).scalars[channel].position};
		const double dt=(t1-t0)/static_cast<double>(n);
		for (std::size_t i=0;i<=n;i++) result.push_back(eval(t0+dt*static_cast<double>(i)).scalars[channel].position);
		return result;
	}
	motion shift(double dt) const {
		motion base=*this;
		return motion([base,dt](double t)->motion_state{
			return base.eval(t-dt);
		},channels_,duration_,behavior_);
	}
	motion time_scale(double k) const {
		if (k<=0.0) throw std::invalid_argument("time_scale requires positive k");
		motion base=*this;
		const double new_duration=(duration_>0.0)?(duration_/k):duration_;
		return motion([base,k](double t)->motion_state{
			motion_state s=base.eval(k*t);
			for (std::size_t i=0;i<s.scalars.size();i++) {
				s.scalars[i].velocity*=k;
				s.scalars[i].acceleration*=k*k;
			}
			return s;
		},channels_,new_duration,behavior_);
	}
	motion value_offset(const double& offset) const {
		std::vector<double> offsets(channels_);
		for (int i=0;i<channels_;i++) offsets[i]=offset;
		return value_offset(offsets);
	}
	motion value_offset(const std::vector<double>& offsets) const {
		if (offsets.size()!=channels_) throw std::invalid_argument("value_offset size mismatch");
		motion base=*this;
		return motion([base,offsets](double t)->motion_state{
			motion_state s=base.eval(t);
			for (std::size_t i=0;i<s.scalars.size();i++) s.scalars[i].position+=offsets[i];
			return s;
		},channels_,duration_,behavior_);
	}
	motion value_scale(const double& scale) const {
		std::vector<double> scales(channels_);
		for (int i=0;i<channels_;i++) scales[i]=scale;
		return value_offset(scales);
	}
	motion value_scale(const std::vector<double>& scales) const {
		if (scales.size()!=channels_) throw std::invalid_argument("value_scale size mismatch");
		motion base=*this;
		return motion([base,scales](double t)->motion_state{
			motion_state s=base.eval(t);
			for (std::size_t i=0;i<s.scalars.size();i++) {
				s.scalars[i].position*=scales[i];
				s.scalars[i].velocity*=scales[i];
				s.scalars[i].acceleration*=scales[i];
			}
			return s;
		},channels_,duration_,behavior_);
	}
	motion add(const motion& other) const {
		if (other.channels()!=channels_) throw std::invalid_argument("add requires same channels");
		motion ma=*this;
		motion mb=other;
		return motion([ma,mb](double t)->motion_state{
			motion_state sa=ma.eval(t);
			motion_state sb=mb.eval(t);
			for (std::size_t i=0;i<sa.scalars.size();i++) {
				sa.scalars[i].position+=sb.scalars[i].position;
				sa.scalars[i].velocity+=sb.scalars[i].velocity;
				sa.scalars[i].acceleration+=sb.scalars[i].acceleration;
			}
			return sa;
		},channels_,duration_,behavior_);
	}

	bool check_finite(double t0,double t1,double step) const {
		if (step<=0.0) throw std::invalid_argument("step must be positive");
		if (t1<t0) std::swap(t0,t1);
		for (double t=t0;t<=t1+step*0.5;t+=step) {
			motion_state s=eval(t);
			for (auto it:s.scalars) {
				if (!std::isfinite(it.position)) return false;
				if (!std::isfinite(it.velocity)) return false;
				if (!std::isfinite(it.acceleration)) return false;
			}
		}
		return true;
	}
	struct continuity_issue {
		double t;
		std::vector<double> left;
		std::vector<double> right;
	};
	std::vector<continuity_issue> check_continuity(const std::vector<double>& times,double eps) const {
		if (eps<0.0) throw std::invalid_argument("eps cannot be negative");
		std::vector<continuity_issue> issues;
		const double h=1e-7;
		for (double it:times) {
			motion_state l=eval(it-h);
			motion_state r=eval(it+h);
			bool bad=false;
			for (std::size_t i=0;i<channels_;i++) {
				if (fabs(l.scalars[i].position-r.scalars[i].position)>eps) {
					bad=true;
					break;
				}
			}
			if (bad) {
				continuity_issue issue;
				issue.t=it;
				for (std::size_t i=0;i<channels_;i++) {
					issue.left.push_back(l.scalars[i].position);
					issue.right.push_back(r.scalars[i].position);
				}
				issues.push_back(issue);
			}
		}
		return issues;
	}

	static prototype_func proto_constant(double c) {
		return [c](double)->motion_scalar{
			return motion_scalar{c,0.0,0.0};
		};
	}
	static prototype_func proto_linear(double x0,double v) {
		return [x0,v](double t)->motion_scalar{
			return motion_scalar{x0+v*t,v,0.0};
		};
	}
	static prototype_func proto_constant_accel(double x0,double v0,double a) {
		return [x0,v0,a](double t)->motion_scalar{
			return motion_scalar{x0+v0*t+0.5*a*t*t,v0+a*t,a};
		};
	}

	static motion make_1d(const prototype_func& proto,double duration=0.0,time_behavior tb=TB_FREE) {
		return motion([proto](double t)->motion_state{
			const motion_scalar k=proto(t);
			return motion_state::single(k);
		},1,duration,tb);
	}
	static motion make_nd(std::size_t channels,const per_channel_func& func,double duration=0.0,time_behavior tb=TB_FREE) {
		if (channels<=0) throw std::invalid_argument("make_nd channels must be positive");
		return motion([channels,func](double t)->motion_state{
			motion_state s(channels);
			for (std::size_t i=0;i<channels;i++) {
				const motion_scalar k=func(i,t);
				s.scalars[i]=k;
			}
			return s;
		},channels,duration,tb);
	}

	static motion pack_channels(const std::vector<motion>& channels,double duration=0.0,time_behavior tb=TB_FREE) {
		if (channels.empty()) throw std::invalid_argument("channels cannot be empty");
		for (const auto& it:channels) {
			if (it.channels()!=1) throw std::invalid_argument("each element must be 1D motion");
		}
		const std::size_t n=channels.size();
		double curr_duration=duration;
		if (!(curr_duration>=0.0)) {
			double mx=0.0;
			bool any=false;
			for (const auto& it:channels) {
				if (it.duration()>0.0) {
					mx=std::max(mx,it.duration());
					any=true;
				}
			}
			curr_duration=any?mx:0.0;
		}
		return motion([channels,n](double t)->motion_state{
			motion_state s(n);
			for (std::size_t i=0;i<n;i++) {
				motion_state si=channels[i].eval(t);
				s.scalars[i]=si.scalars[0];
			}
			return s;
		},n,curr_duration,tb);
	}
	static motion set_channel(const motion& base,std::size_t index,const motion& channel) {
		if (base.channels()<=0) throw std::invalid_argument("invalid base");
		if (channel.channels()!=1) throw std::invalid_argument("source must be 1D");
		if (index>=base.channels()) throw std::out_of_range("index out of range");
		return motion([base,channel,index](double t)->motion_state{
			motion_state s=base.eval(t);
			motion_state c=channel.eval(t);
			s.scalars[index]=c.scalars[0];
			return s;
		},base.channels(),base.duration(),base.behavior());
	}

	void set_channel(std::size_t index,const motion& channel) {
		*this=set_channel(*this,index,channel);
	}

	class piecewise_builder;
	static piecewise_builder piecewise();
};

class motion::piecewise_builder {
	struct segment {
		double start_;
		motion m_;
	};
	std::vector<segment> segments_;
	double duration_{0.0};
	time_behavior behavior_{TB_FREE};

public:
	piecewise_builder()=default;

	piecewise_builder& with_duration(double d) noexcept {
		duration_=d;
		return *this;
	}
	piecewise_builder& with_time_behavior(time_behavior tb) noexcept {
		behavior_=tb;
		return *this;
	}
	piecewise_builder& add(const motion& m,double start_time) {
		segments_.push_back(segment{start_time,m});
		return *this;
	}
	motion build(bool require_sorted=true) const {
		if (segments_.empty()) return motion();
		auto segments=segments_;
		if (require_sorted) {
			for (std::size_t i=1;i<segments.size();i++) {
				if (segments[i].start_<segments[i-1].start_) throw std::invalid_argument("segments must be sorted");
			}
		} else std::sort(segments.begin(),segments.end(),[](const segment& a,const segment& b){ return a.start_<b.start_; });
		std::size_t ch=segments[0].m_.channels();
		for (const auto& it:segments) {
			if (it.m_.channels()!=ch) throw std::invalid_argument("channel mismatch");
		}
		double curr_duration=duration_;
		if (!(curr_duration>0.0)) {
			const auto& last=segments.back();
			if (last.m_.duration()>0.0) curr_duration=last.start_+last.m_.duration();
		}
		return motion([segments](double t)->motion_state{
			std::size_t index=0;
			for (std::size_t i=1;i<segments.size();i++) {
				if (segments[i].start_<=t) index=i;
				else break;
			}
			const auto& it=segments[index];
			return it.m_.eval(t-it.start_);
		},ch,curr_duration,behavior_);
	}
};

inline motion::piecewise_builder motion::piecewise() { 
	return piecewise_builder(); 
}

}

}

#endif