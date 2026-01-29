//Last Modified At 2026/01/30
//@Version 1.0.0.0
#ifndef _STDEX_ALGORITHM_SOLVER_NLP_SOLVER_H_
#define _STDEX_ALGORITHM_SOLVER_NLP_SOLVER_H_ 1

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace stdex {

namespace nlp {

using scalar_func=std::function<double(const std::vector<double>&)>;

enum constraint_kind {
	CK_LESS_EQUAL,
	CK_EQUAL,
	CK_GREATER_EQUAL,
};

struct constraint {
	scalar_func func;
	constraint_kind kind=CK_LESS_EQUAL;
	double rhs=0.0;
	double tolerance=1e-8;
	std::string name;
};

class model {
	std::size_t n_vars_=0;
	scalar_func objective_;
	bool has_objective_=false;
	std::vector<double> low_, high_;
	bool has_low_=false,has_high_=false;
	std::vector<constraint> constraints_;

public:
	explicit model(std::size_t n_vars=0) : n_vars_(n_vars) { }

	std::size_t& n_vars() {
		return n_vars_;
	}
	void set_objective(scalar_func func) {
		objective_=std::move(func);
		has_objective_=true; }
	bool has_objective() const {
		return has_objective_;
	}
	const scalar_func& objective() const {
		if (!has_objective_) throw std::runtime_error("Objective not existed");
		return objective_;
	}

	void set_lower_bounds(std::vector<double> low) {
		if (low.size()!=n_vars_) throw std::invalid_argument("Lower bounds size mismatch");
		low_=std::move(low);
		has_low_=true;
	}
	void set_upper_bounds(std::vector<double> high) {
		if (high.size()!=n_vars_) throw std::invalid_argument("Upper bounds size mismatch");
		high_=std::move(high);
		has_high_=true;
	}

	bool has_lower_bounds() const { return has_low_; }
	bool has_upper_bounds() const { return has_high_; }
	const std::vector<double>& lower_bounds() const { return low_; }
	const std::vector<double>& upper_bounds() const { return high_; }

	void unbound_lower(std::size_t index) {
		if (index>=n_vars_) throw std::invalid_argument("Invalid index");
		low_[index]=-std::numeric_limits<double>::infinity();
	}
	void unbound_upper(std::size_t index) {
		if (index>=n_vars_) throw std::invalid_argument("Invalid index");
		high_[index]=std::numeric_limits<double>::infinity();
	}

	void add_constraint(constraint c) {
		constraints_.push_back(std::move(c));
	}

	void add_less_equal(scalar_func func,double rhs=0.0,std::string name={}) {
		constraint c;
		c.func=std::move(func);
		c.kind=CK_LESS_EQUAL;
		c.rhs=rhs;
		c.name=std::move(name);
		add_constraint(std::move(c));
	}
	void add_equal(scalar_func func,double rhs=0.0,double tolerance=1e-8,std::string name={}) {
		constraint c;
		c.func=std::move(func);
		c.kind=CK_EQUAL;
		c.rhs=rhs;
		c.tolerance=tolerance;
		c.name=std::move(name);
		add_constraint(std::move(c));
	}
	void add_equal(scalar_func func,double rhs=0.0,std::string name={}) {
		add_equal(func,rhs,1e-8,name);
	}
	void add_greater_equal(scalar_func func,double rhs=0.0,std::string name={}) {
		constraint c;
		c.func=std::move(func);
		c.kind=CK_GREATER_EQUAL;
		c.rhs=rhs;
		c.name=std::move(name);
		add_constraint(std::move(c));
	}
	const std::vector<constraint>& constraints() const { return constraints_; }
};

inline std::vector<double> clamp_to_bounds(std::vector<double> x,const model& m) {
	if (m.has_lower_bounds()) {
		const auto& low=m.lower_bounds();
		for (std::size_t i=0;i<x.size();i++) x[i]=std::max(x[i],low[i]);
	}
	if (m.has_upper_bounds()) {
		const auto& high=m.upper_bounds();
		for (std::size_t i=0;i<x.size();i++) x[i]=std::min(x[i],high[i]);
	}
	return x;
}

struct term_contribution {
	std::string name;
	double value=0.0;
	double violation=0.0;
	double contribution=0.0; // penalty/AL contribution for this constraint
};

struct solve_diagnostics {
	std::vector<term_contribution> constraint_terms;
};

struct solve_result {
	std::vector<double> x;
	double objective=std::numeric_limits<double>::infinity();      // final scalar minimized by handler
	double raw_objective=std::numeric_limits<double>::infinity();  // m.objective()(x)
	int iters=0;
	bool converged=false;
	solve_diagnostics diagnostics;
};

inline double constraint_violation(const constraint& c,double v) {
	switch (c.kind) {
		case CK_LESS_EQUAL: {
			return std::max<double>(0.0,v-c.rhs);
		}
		case CK_EQUAL: {
			double diff=std::abs(v-c.rhs);
			return std::max<double>(0.0,diff-c.tolerance);
		}
		case CK_GREATER_EQUAL: {
			return std::max<double>(0.0,c.rhs-v);
		}
	}
	return 0.0;
}

inline double constraint_residual_signed(const constraint& c,double v) {
	// residual r(x) desired:
	// - inequality: r(x) <= 0
	// - equality: r(x)  == 0
	switch (c.kind) {
		case CK_LESS_EQUAL:
		case CK_EQUAL: return v-c.rhs;
		case CK_GREATER_EQUAL: return c.rhs-v;
	}
	return 0.0;
}

inline double dot(const std::vector<double>& lhs,const std::vector<double>& rhs) {
	if (lhs.size()!=rhs.size()) throw std::invalid_argument("Dot vector size mismatch");
	double result=0.0;
	for (std::size_t i=0;i<lhs.size();i++) result+=lhs[i]*rhs[i];
	return result;
}
inline double norm2(const std::vector<double>& v) {
	return std::sqrt(dot(v,v));
}

inline std::vector<double> numerical_gradient(const scalar_func& func,const std::vector<double>& x,double eps=1e-4) {
	std::vector<double> result(x.size(),0.0);
	std::vector<double> xp=x,xm=x;
	for (std::size_t i=0;i<x.size();i++) {
		double h=eps*std::max<double>(1.0,std::abs(x[i]));
		xp[i]=x[i]+h;
		xm[i]=x[i]-h;
		double fp=func(xp);
		double fm=func(xm);
		result[i]=(fp-fm)/(2.0*h);
		xp[i]=x[i];
		xm[i]=x[i];
	}
	return result;
}

struct penalty_options {
	std::vector<double> mu_schedule{1.0,10.0,100.0};
	bool squared=true;
	double base_weight=1.0;
};

struct augmented_lagrangian_options {
	int outer_iters=6;
	double mu0=1.0;
	double mu_growth=10.0;
	double lambda_clip=1e6;
	bool verbose=false;
};

struct constraint_handler_options {
	enum handler_kind {
		CHO_HK_PENALTY,
		CHO_HK_AUGMENTED_LAGRANGIAN,
	} handler=CHO_HK_PENALTY;
	union {
		penalty_options penalty;
		augmented_lagrangian_options augmented_lagrangian;
	} option;
};

// penalty objective
inline double penalized_objective(const model& m,const std::vector<double>& x,const penalty_options& popt,double mu,solve_diagnostics* diagnostic) {
	double f=m.objective()(x);
	double pen=0.0;
	if (diagnostic) diagnostic->constraint_terms.clear();
	for (const auto& it:m.constraints()) {
		double v=it.func(x);
		double violation=constraint_violation(it,v);
		double term=popt.squared?(violation*violation):std::abs(violation);
		term*=(mu*popt.base_weight);
		pen+=term;
		if (diagnostic) {
			term_contribution t;
			t.name=it.name;
			t.value=v;
			t.violation=violation;
			t.contribution=term;
			diagnostic->constraint_terms.push_back(std::move(t));
		}
	}
	return f+pen;
}

// augmented lagrangian objective
inline double augmented_lagrangian_objective(const model& m,const std::vector<double>& x,const std::vector<double>& lambda,double mu,solve_diagnostics* diagnostic) {
	double f=m.objective()(x);
	double add=0.0;
	if (diagnostic) diagnostic->constraint_terms.clear();
	const auto& cs=m.constraints();
	for (std::size_t i=0;i<cs.size();i++) {
		const auto& c=cs[i];
		double v=c.func(x);
		double term=0.0;
		double violation=0.0;
		if (c.kind==CK_EQUAL) {
			double diff=v-c.rhs;
			double dead=std::max<double>(0.0,std::abs(diff)-c.tolerance);
			term=lambda[i]*diff+(mu/2.0)*dead*dead;
			violation=dead;
		} else {
			double r=constraint_residual_signed(c,v); // want r<=0
			double rp=std::max<double>(0.0,r);
			term=lambda[i]*r+(mu/2.0)*rp*rp;
			violation=rp;
		}
		add+=term;
		if (diagnostic) {
			term_contribution t;
			t.name=c.name;
			t.value=v;
			t.violation=violation;
			t.contribution=term;
			diagnostic->constraint_terms.push_back(std::move(t));
		}
 	}
	return f+add;
}

class algorithm {
public:
	virtual ~algorithm()=default;
	// minimize scalar F(x) under bounds of model (algorithm should clamp)
	virtual solve_result minimize(const model& m,const scalar_func& func,std::vector<double> x0) const=0;
};

struct nelder_mead_options {
	int max_iter=800;
	double init_step=0.5;
	double alpha=1.0;
	double gamma=2.0;
	double rho=0.5;
	double sigma=0.5;
	double f_tolerance=1e-10;
	bool verbose=false;
};

class nelder_mead final : public algorithm {
	nelder_mead_options option_;

public:
	explicit nelder_mead(nelder_mead_options option={}) : option_(option) { }

	solve_result minimize(const model& m,const scalar_func& func,std::vector<double> x0) const override {
		solve_result result;
		x0=clamp_to_bounds(std::move(x0),m);
		const std::size_t n=x0.size();
		std::vector<std::vector<double>> simplex(n+1,x0);
		for (std::size_t i=0;i<n;i++) {
			simplex[i+1][i]+=option_.init_step;
			simplex[i+1]=clamp_to_bounds(std::move(simplex[i+1]),m);
		}
		std::vector<double> f_val(n+1);
		for (std::size_t i=0;i<n+1;i++) f_val[i]=func(simplex[i]);
		auto sort_simplex=[&](){
			std::vector<std::size_t> index(n+1);
			std::iota(index.begin(),index.end(),0);
			std::sort(index.begin(),index.end(),[&](std::size_t lhs,std::size_t rhs){
				return f_val[lhs]<f_val[rhs];
			});
			std::vector<std::vector<double>> s2(n+1);
			std::vector<double> f2(n+1);
			for (std::size_t k=0;k<n+1;k++) {
				s2[k]=simplex[index[k]];
				f2[k]=f_val[index[k]];
			}
			simplex.swap(s2);
			f_val.swap(f2);
		};
		sort_simplex();
		for (int it=0;it<option_.max_iter;it++) {
			if (option_.verbose) std::cerr<<"[nm] it="<<it<<" best="<<f_val[0]<<" worst="<<f_val[n]<<std::endl;
			if (std::abs(f_val[n]-f_val[0])<option_.f_tolerance) {
				result.converged=true;
				result.iters=it;
				break;
			}
			std::vector<double> c(n,0.0);
			for (std::size_t i=0;i<n;i++) {
				for (std::size_t j=0;j<n;j++) c[j]+=simplex[i][j];
			}
			for (std::size_t j=0;j<n;j++) c[j]/=(double)n;
			std::vector<double> xr(n);
			for (std::size_t j=0;j<n;j++) xr[j]=c[j]+option_.alpha*(c[j]-simplex[n][j]);
			xr=clamp_to_bounds(std::move(xr),m);
			double fr=func(xr);
			if (fr<f_val[0]) {
				std::vector<double> xe(n);
				for (std::size_t j=0;j<n;j++) xe[j]=c[j]+option_.gamma*(xr[j]-c[j]);
				xe=clamp_to_bounds(std::move(xe),m);
				double fe=func(xe);
				if (fe<fr) {
					simplex[n]=std::move(xe);
					f_val[n]=fe;
				} else {
					simplex[n]=std::move(xr);
					f_val[n]=fr;
				}
			} else if (fr<f_val[n-1]) {
				simplex[n]=std::move(xr);
				f_val[n]=fr;
			} else {
				std::vector<double> xc(n);
				if (fr<f_val[n]) {
					for (std::size_t j=0;j<n;j++) xc[j]=c[j]+option_.rho*(xr[j]-c[j]);
				} else {
					for (std::size_t j=0;j<n;j++) xc[j]=c[j]+option_.rho*(simplex[n][j]-c[j]);
				}
				xc=clamp_to_bounds(std::move(xc),m);
				double fc=func(xc);
				if (fc<f_val[n]) {
					simplex[n]=std::move(xc);
					f_val[n]=fc;
				} else {
					for (std::size_t i=1;i<n+1;i++) {
						for (std::size_t j=0;j<n;j++) simplex[i][j]=simplex[0][j]+option_.sigma*(simplex[i][j]-simplex[0][j]);
						simplex[i]=clamp_to_bounds(std::move(simplex[i]),m);
						f_val[i]=func(simplex[i]);
					}
				}
			}
			sort_simplex();
			result.iters=it+1;
		}
		result.x=simplex[0];
		result.objective=func(result.x);
		return result;
	}
};

struct lbfgs_options {
	int max_iter=300;
	int m=10;             // history size
	double grad_eps=1e-4;
	double grad_tolerance=1e-6;
	double step0=1.0;
	double c1=1e-4;         // Armijo
	double step_shrink=0.5;
	int max_line_search=20;
	bool verbose=false;
};

class lbfgs final : public algorithm {
	lbfgs_options option_;

public:
	explicit lbfgs(lbfgs_options option={}) : option_(option) { }

	solve_result minimize(const model& m,const scalar_func& func,std::vector<double> x0) const override {
		solve_result result;
		std::vector<double> x=clamp_to_bounds(std::move(x0),m);
		double fx=func(x);
		std::vector<double> g=numerical_gradient(func,x,option_.grad_eps);
		std::vector<std::vector<double>> s_hist,y_hist;
		std::vector<double> rho_hist;
		auto two_loop=[&](const std::vector<double>& grad)->std::vector<double>{
			std::vector<double> q=grad;
			int k=(int)s_hist.size();
			std::vector<double> alpha(k);
			for (int i=k-1;i>=0;i--) {
				alpha[i]=rho_hist[i]*dot(s_hist[i],q);
				for (std::size_t j=0;j<q.size();j++) q[j]-=alpha[i]*y_hist[i][j];
			}
			double gamma0=1.0;
			if (k>0) {
				double sy=dot(s_hist[k-1],y_hist[k-1]);
				double yy=dot(y_hist[k-1],y_hist[k-1]);
				if (yy>0) gamma0=sy/yy;
			}
			for (auto& it:q) it*=gamma0;
			for (int i=0;i<k;i++) {
				double beta=rho_hist[i]*dot(y_hist[i],q);
				for (std::size_t j=0;j<q.size();j++) q[j]+=s_hist[i][j]*(alpha[i]-beta);
			}
			for (auto& it:q) it=-it;
			return q;
		};
		for (int it=0;it<option_.max_iter;it++) {
			double gn=norm2(g);
			if (option_.verbose) std::cerr<<"[lbfgs] it="<<it<<" f="<<fx<<" |g|="<<gn<<std::endl;
			if (gn<option_.grad_tolerance) {
				result.converged=true;
				result.iters=it;
				break;
			}
			std::vector<double> p=two_loop(g);
			double step=option_.step0;
			bool accepted=false;
			for (int ls=0;ls<option_.max_line_search;ls++) {
				std::vector<double> xn=x;
				for (std::size_t i=0;i<xn.size();i++) xn[i]+=step*p[i];
				xn=clamp_to_bounds(std::move(xn),m);
				double fn=func(xn);
				if (fn<=fx+option_.c1*step*dot(g,p)) {
					std::vector<double> g2=numerical_gradient(func,xn,option_.grad_eps);
					std::vector<double> svec(x.size()),yvec(x.size());
					for (std::size_t i=0;i<x.size();i++) {
						svec[i]=xn[i]-x[i];
						yvec[i]=g2[i]-g[i];
					}
					double sy=dot(svec,yvec);
					if (std::abs(sy)>1e-12) {
						double rho=1.0/sy;
						if ((int)s_hist.size()==option_.m) {
							s_hist.erase(s_hist.begin());
							y_hist.erase(y_hist.begin());
							rho_hist.erase(rho_hist.begin());
						}
						s_hist.push_back(std::move(svec));
						y_hist.push_back(std::move(yvec));
						rho_hist.push_back(rho);
					}
					x=std::move(xn);
					fx=fn;
					g=std::move(g2);
					accepted=true;
					break;
				}
				step*=option_.step_shrink;
			}
			if (!accepted) {
				result.converged=false;
				result.iters=it+1;
				break;
			}
			result.iters=it+1;
		}
		result.x=x;
		result.objective=fx;
		return result;
	}
};

// ---------------- cma_es (diagonal covariance for compactness) ----------------
struct cma_es_options {
	int max_iter=300;
	int lambda=0;     // 0 => auto
	int mu=0;         // 0 => lambda/2
	double sigma0=0.3;
	bool verbose=false;
	unsigned seed=random_device{}();
};

class cma_es final : public algorithm {
	cma_es_options option_;

public:
	explicit cma_es(cma_es_options option={}) : option_(option) { }
	solve_result minimize(const model& m,const scalar_func& func,std::vector<double> x0) const override {
		solve_result result;
		x0=clamp_to_bounds(std::move(x0),m);
		const int n=(int)x0.size();
		std::mt19937 rng(option_.seed);
		std::normal_distribution<double> nd(0.0,1.0);
		int lambda=option_.lambda>0?option_.lambda:(4+(int)std::floor(3*std::log((double)n)));
		int mu=option_.mu>0?option_.mu:(lambda/2);
		std::vector<double> w(mu);
		for (int i=0;i<mu;i++) w[i]=std::log((double)(mu+0.5))-std::log((double)(i+1));
		double wsum=std::accumulate(w.begin(),w.end(),0.0);
		for (auto& it:w) it/=wsum;
		double mueff=0.0;
		for (auto& it:w) mueff+=it*it;
		mueff=1.0/mueff;
		double cc=(4.0+mueff/n)/(n+4.0+2.0*mueff/n);
		double cs=(mueff+2.0)/(n+mueff+5.0);
		double c1=2.0/((n+1.3)*(n+1.3)+mueff);
		double cmu=std::min(1.0-c1,2.0*(mueff-2.0+1.0/mueff)/((n+2.0)*(n+2.0)+mueff));
		double damps=1.0+2.0*std::max(0.0,std::sqrt((mueff-1.0)/(n+1.0))-1.0)+cs;
		std::vector<double> mean=x0;
		double sigma=option_.sigma0;
		std::vector<double> Cdiag(n,1.0);
		std::vector<double> pc(n, 0.0),ps(n,0.0);
		double chi_n=std::sqrt((double)n)*(1.0-1.0/(4.0*n)+1.0/(21.0*n*n));
		struct indiv {
			std::vector<double> x;
			std::vector<double> z;
			double f;
		};
		double best_f=func(mean);
		std::vector<double> best_x=mean;
		auto sample=[&](){
			std::vector<double> z(n),y(n),x(n);
			for (int i=0;i<n;i++) z[i]=nd(rng);
			for (int i=0;i<n;i++) y[i]=std::sqrt(Cdiag[i])*z[i];
			for (int i=0;i<n;i++) x[i]=mean[i]+sigma*y[i];
			x=clamp_to_bounds(std::move(x),m);
			return std::make_pair(x,z);
		};
		for (int it=0;it<option_.max_iter;it++) {
			std::vector<indiv> pop;
			pop.reserve(lambda);
			for (int k=0;k<lambda;k++) {
				auto [x,z]=sample();
				double fx=func(x);
				pop.push_back({std::move(x),std::move(z),fx});
			}
			std::nth_element(pop.begin(),pop.begin()+mu,pop.end(),[](const indiv& lhs,const indiv& lhs){
				return lhs.f<rhs.f;
			});
			std::sort(pop.begin(),pop.begin()+mu,[](const indiv& lhs,const indiv& lhs){
				return lhs.f<rhs.f;
			});
			if (pop[0].f<best_f) {
				best_f=pop[0].f;
				best_x=pop[0].x;
			}
			std::vector<double> new_mean(n,0.0);
			for (int i=0;i<mu;i++) {
				for (int j=0;j<n;j++) new_mean[j]+=w[i]*pop[i].x[j];
			}
			std::vector<double> zmean(n,0.0);
			for (int i=0;i<mu;i++) {
				for (int j=0;j<n;j++) zmean[j]+=w[i]*pop[i].z[j];
			}
			for (int j=0;j<n;j++) ps[j]=(1.0-cs)*ps[j]+std::sqrt(cs*(2.0-cs)*mueff)*zmean[j];
			double ps_norm=norm2(ps);
			bool hsig=ps_norm/std::sqrt(1.0-std::pow(1.0-cs,2.0*(it+1)))/chi_n<(1.4+2.0/(n+1.0));
			for (int j=0;j<n;j++) pc[j]=(1.0-cc)*pc[j]+(hsig?1.0:0.0)*std::sqrt(cc*(2.0-cc)*mueff)*(new_mean[j]-mean[j])/std::max<double>(1e-12,sigma);
			for (int j=0;j<n;j++) {
				double rank_one=pc[j]*pc[j];
				double rank_mu=0.0;
				for (int i=0;i<mu;i++) {
					double dx=(pop[i].x[j]-mean[j])/std::max<double>(1e-12,sigma);
					rank_mu+=w[i]*dx*dx;
				}
				Cdiag[j]=(1.0-c1-cmu)*Cdiag[j]+c1*rank_one+cmu*rank_mu;
				Cdiag[j]=std::max<double>(1e-18,Cdiag[j]);
			}
			sigma*=std::exp((cs/damps)*(ps_norm/chi_n-1.0));
			mean=std::move(new_mean);
			if (option_.verbose) std::cerr<< "[cma_es] it="<<it<<" best="<<best_f<<" sigma="<<sigma<<std::endl;
			result.iters=it+1;
		}
		result.x=best_x;
		result.objective=best_f;
		result.converged=false;
		return result;
	}   
};

class function_adapter final : public algorithm {
public:
	using func_type=std::function<solve_result(const model&,const scalar_func&,std::vector<double>)>;

private:
	func_type func_;

public:
	explicit function_adapter(func_type func) : func_(std::move(func)) {
		if (!func_) throw std::invalid_argument("Invalid null func");
	}

	solve_result minimize(const model& m,const scalar_func& func,std::vector<double> x0) const override {
		return func_(m,func,std::move(x0));
	}
};

class solver {
	std::shared_ptr<algorithm> algorithm_;
	constraint_handler_options handler_;

private:
	solve_result solve_penalty(const model& m,std::vector<double> x0) const {
		solve_result result;
		std::vector<double> x=std::move(x0);
		for (double mu:handler_.penalty.mu_schedule) {
			auto func=[&](const std::vector<double>& xx){
				return penalized_objective(m,xx,handler_.penalty,mu,nullptr);
			};
			result=algorithm_->minimize(m,func,x);
			result.x=clamp_to_bounds(std::move(result.x),m);
			result.objective=penalized_objective(m,result.x,handler_.penalty,mu,&last.diagnostics);
			result.raw_objective=m.objective()(result.x);
			x=result.x;
		}
		return result;
	}
	solve_result solve_augmented_lagrangian(const model& m,std::vector<double> x0) const {
		solve_result result;
		std::vector<double> x=std::move(x0);
		const auto& cs=m.constraints();
		std::vector<double> lambda(cs.size(),0.0);
		double mu=handler_.augmented_lagrangian.mu0;
		for (int outer=0;outer<handler_.augmented_lagrangian.outer_iters;outer++) {
			auto func=[&](const std::vector<double>& xx){
				return augmented_lagrangian_objective(m,xx,lambda,mu,nullptr);
			};
			result=algorithm_->minimize(m,func,x);
			result.x=clamp_to_bounds(std::move(result.x),m);
			for (std::size_t i=0;i<cs.size();i++) {
				const auto& c=cs[i];
				double v=c.func(result.x);
				if (c.kind==CK_EQUAL) {
					double diff=v - c.rhs;
					lambda[i]=std::clamp(lambda[i]+mu*diff,-handler_.augmented_lagrangian.lambda_clip,handler_.augmented_lagrangian.lambda_clip);
				} else {
					double r=constraint_residual_signed(c,v); // want r<=0
					lambda[i]=std::max<double>(0.0,lambda[i]+mu*r);
					lambda[i]=std::min(lambda[i],handler_.augmented_lagrangian.lambda_clip);
				}
			}
			result.objective=augmented_lagrangian_objective(m,result.x,lambda,mu,&result.diagnostics);
			result.raw_objective=m.objective()(result.x);
			if (handler_.augmented_lagrangian.verbose) {
				double vmax=0.0;
				for (const auto& it:cs) vmax=std::max(vmax,constraint_violation(it,it.func(result.x)));
				std::cerr<<"[al] outer="<<outer<<" mu="<<mu<<" raw="<<result.raw_objective<<" max_viol="<<vmax<<std::endl;
			}
			x=result.x;
			mu*=handler_.augmented_lagrangian.mu_growth;
		}
	return result;
	}

public:
	solver() : algorithm_(std::make_shared<nelder_mead>()) { }
	solver(algorithm& algorithm) : algorithm_(std::make_shared<algorithm>(algorithm)) { }

	void set_algorithm(algorithm& algorithm,_Args&&... args) {
		algorithm_=std::make_shared<algorithm>(algorithm);
	}
	void set_algorithm(std::shared_ptr<algorithm> algorithm) {
		if (!algorithm) throw std::invalid_argument("Invalid null algorithm");
		algorithm_=std::move(algorithm);
	}
   	constraint_handler_options& constraint_handler() {
		return handler_;
	}
	solve_result solve(const model& m,std::vector<double> x0) const {
		if (!m.has_objective()) throw std::invalid_argument("Model objective not set");
		if (x0.size()!=m.n_vars()) throw std::invalid_argument("x0 size mismatch");
		x0=clamp_to_bounds(std::move(x0),m);
		if (handler_.handler==CHO_HK_PENALTY) return solve_penalty(m,std::move(x0));	
		return solve_augmented_lagrangian(m,std::move(x0));
	}
};

enum goal_priority {
	GP_LOW=1,
	GP_MEDIUM=2,
	GP_HIGH=3,
	GP_CRITICAL=4,
};

enum goal_kind {
	GK_EQUAL,
	GK_RANGE,
	GK_UPPER,
	GK_LOWER,
	GK_MINIMIZE,
	GK_MAXIMIZE,
};

struct metric {
	std::string name;
	scalar_func func;
};

struct goal_specification {
	std::string metric_name;
	goal_kind kind=GK_RANGE;
	goal_priority priority=GP_MEDIUM;
	double target=0.0;
	double low=-std::numeric_limits<double>::infinity();
	double high=std::numeric_limits<double>::infinity();
	double tolerance=1e-8;
	// for minimize/maximize
	double preference_weight=1.0;
	// optional per-goal scale override
	double scale=0.0; // 0 => use compile_options.metric_scale or 1
};

struct goal_contribution {
	std::string metric_name;
	goal_kind kind;
	goal_priority priority;
	double value=0.0;
	double penalty=0.0; // in weighted_sum objective sense
};

struct compile_options {
	std::function<double(priority)> priority_to_weight=[](priority p){
		switch (p) {
			case GP_LOW: return 1.0;
			case GP_MEDIUM: return 10.0;
			case GP_HIGH: return 100.0;
			case GP_CRITICAL: return 1000.0;
		}
		return 10.0;
	};
	bool squared_penalty=true;
	std::unordered_map<std::string,double> metric_scale;
	double scale_eps=1e-9;
	// For constrained compilation: which priorities become "hard constraints"?
	goal_priority hard_min_priority=GP_HIGH;
};

inline double square(double x) {
	return x*x;
}

class goal_model {
	std::size_t n_vars_=0;
	std::unordered_map<std::string,metric> metrics_;
	std::vector<goal_specification> goals_;

private:
	double scale_for(const compile_options& option,const goal_specification& g,const std::string& metric_name) const {
		if (g.scale>0.0) return std::max(option.scale_eps,g.scale);
		auto it=option.metric_scale.find(metric_name);
		if (it!=opt.metric_scale.end()) return std::max(option.scale_eps,it->second);
		return 1.0;
	}
    
public:
	explicit goal_model(std::size_t n_vars) : n_vars_(n_vars) { }

	std::size_t& n_vars() {
		return n_vars_;
	}
	void add_metric(std::string name,scalar_func func) {
		if (metrics_.count(name)) throw std::invalid_argument("Metric name exists");
		metrics_[name]=metric{std::move(name),std::move(func)};
	}
	void add_goal(goal_specification g) {
		if (!metrics_.count(g.metric_name)) throw std::runtime_error("Unknown metric name");
		goals_.push_back(std::move(g));
	}
	void goal_equal(std::string metric_name,double target,double tolerance,goal_priority priority) {
		goal_specification g;
		g.metric_name=std::move(metric_name);
		g.kind=GK_EQUAL;
		g.target=target;
		g.tolerance=tolerance;
		g.priority=priority;
		add_goal(std::move(g));
	}
	void goal_range(std::string metric_name,double low,double high,goal_priority priority) {
		goal_specification g;
		g.metric_name=std::move(metric_name);
		g.kind=GK_RANGE;
		g.low=low;
		g.high=high;
		g.priority=priority;
		add_goal(std::move(g));
	}
	void goal_upper(std::string metric_name,double high,goal_priority priority) {
		goal_specification g;
		g.metric_name=std::move(metric_name);
		g.kind=GK_UPPER;
		g.high=high;
		g.priority=priority;
		add_goal(std::move(g));
	}
	void goal_lower(std::string metric_name,double low,goal_priority priority) {
		goal_specification g;
		g.metric_name=std::move(metric_name);
		g.kind=GK_LOWER;
		g.low=low;
		g.priority=priority;
		add_goal(std::move(g));
	}
	void prefer_minimize(std::string metric_name,goal_priority priority,double weight=1.0) {
		goal_specification g;
		g.metric_name=std::move(metric_name);
		g.kind=GK_MINIMIZE;
		g.priority=priority;
		g.preference_weight=weight;
		add_goal(std::move(g));
	}
	void prefer_maximize(std::string metric_name,goal_priority priority,real weight=1.0) {
		goal_specification g;
		g.metric_name=std::move(metric_name);
		g.kind=GK_MAXIMIZE;
		g.priority=priority;
		g.preference_weight=weight;
		add_goal(std::move(g));
	}

	std::vector<goal_contribution> evaluate(const compile_options& option,const std::vector<double>& x) const {
		std::vector<goal_contribution> result;
		result.reserve(goals_.size());
		for (const auto& it:goals_) {
			double v=metrics_.at(it.metric_name).func(x);
			double w=option.priority_to_weight(it.priority);
			double s=scale_for(option,it,it.metric_name);
			auto pen=[&](double e)->double{
				double en=e/s;
				double p=option.squared_penalty?square(en):std::abs(en);
				return w*p;
			};
			double p_val=0.0;
			switch (it.kind) {
				case GK_EQUAL: p_val=pen(std::max<double>(0.0,std::abs(v-it.target)-it.tolerance));break;
				case GK_RANGE: p_val=pen(std::max<double>(0.0,it.low-v))+pen(std::max<double>(0.0,v-it.high));break;
				case GK_UPPER: p_val=pen(std::max<double>(0.0,v-it.high));break;
				case GK_LOWER: p_val=pen(std::max<double>(0.0,it.low-v));break;
				case GK_MINIMIZE: double en=v/s;p_val=w*it.preference_weight*(option.squared_penalty?square(en):std::abs(en));break;
				case GK_MAXIMIZE: double en=(-v)/s;p_val=w*it.preference_weight*(option.squared_penalty?square(en):std::abs(en));break;
			}
			result.push_back(goal_contribution{it.metric_name,it.kind,it.priority,v,p_val});
		}
		return result;
	}

	// 1) weighted-sum compilation: everything becomes objective (loss-only)
	model compile_weighted_sum(const compile_options& option,const std::vector<double>& var_low={},const std::vector<double>& var_high={}) const {
		model m(n_vars_);
		if (!var_low.empty()) m.set_lower_bounds(var_low);
		if (!var_high.empty()) m.set_upper_bounds(var_high);
		m.set_objective([=](const std::vector<double>& x)->double{
			double L=0.0;
			auto contribution=evaluate(option,x);
			for (const auto& it:contribution) L+=it.penalty;
			return L;
		});
		return m;
	}
	// 2) constrained compilation:
	//    - goals with prio >= hard_min_priority are compiled into model.constraints()
	//    - the rest become objective penalties/preferences (weighted)
	//
	// This lets you use AL in nlp::solver for "hard-ish" constraints.
	model compile_constrained_model(const compile_options& option,const std::vector<double>& var_low={},const std::vector<double>& var_high={}) const {
		model m(n_vars_);
		if (!var_low.empty()) m.set_lower_bounds(var_low);
		if (!var_high.empty()) m.set_upper_bounds(var_high);
		// objective for soft goals only
		m.set_objective([=](const std::vector<double>& x)->double{
			double L=0.0;
			for (const auto& it:goals_) {
				if (static_cast<int>(it.priority)>=static_cast<int>(option.hard_min_priority)) continue;
				double v=metrics_.at(it.metric_name).func(x);
				double w=option.priority_to_weight(it.priority);
				double s=scale_for(option,it,it.metric_name);
				auto pen=[&](double e)->double{
					double en=e/s;
					double p=option.squared_penalty?square(en):std::abs(en);
					return w*p;
				};
				switch (it.kind) {
					case GK_EQUAL: L+=pen(std::max<double>(0.0,std::abs(v-it.target)-it.tolerance));break;
					case GK_RANGE: L+=pen(std::max<double>(0.0,it.low-v))+pen(std::max<double>(0.0,v-it.high));break;
					case GK_UPPER: L+=pen(std::max<double>(0.0,v-it.high));break;
					case GK_LOWER: L+=pen(std::max<double>(0.0,it.low-v));break;
					case GK_MINIMIZE: L+=w*it.preference_weight*(option.squared_penalty?square((v/s)):std::abs(v/s));break;
					case GK_MAXIMIZE: L+=w*it.preference_weight*(option.squared_penalty?square((-v/s)):std::abs(-v/s));break;
				}
			}
			return L;
		});
		// hard goals -> constraints
		for (const auto& it: goals_) {
			if (static_cast<int>(it.priority)<static_cast<int>(option.hard_min_priority)) continue;
			const auto& func=metrics_.at(g.metric_name).func;
			std::string name=g.metric_name;
			switch (it.kind) {
				case GK_EQUAL: m.add_equal(func,it.target,it.tolerance,name+":equal");break;
				case GK_RANGE: m.add_greater_equal(func,it.low,name+":range_low");m.add_less_equal(func,it.high,name+":range_high");break;
				case GK_UPPER: m.add_less_equal(func,it.high,name+":upper");break;// fn(x) <= hi
				case GK_LOWER: m.add_greater_equal(func,it.low,name+":lower");break;// fn(x) >= lo
				case GK_MINIMIZE:
				case GK_MAXIMIZE: break;// preferences are not constraints; ignore as hard constraint
			}
		}
		return m;
	}
	// 3) lexicographic solve (multi-stage): critical -> high -> medium -> low.
	// It works for both compilation modes, you choose by passing a function pointer.
	template <class _Func>
	solve_result solve_lexicographic(const solver& solver,const compile_options& option,const std::vector<double>& x0,const std::vector<double>& var_low,const std::vector<double>& var_high,_Func compiler) const {
		auto solve_at_or_above=[&](goal_priority min_priority,std::vector<double> start){
			goal_model sub(n_vars_);
			sub.metrics_=metrics_;
			for (const auto& it:goals_) {
				if (static_cast<int>(it.priority)>=static_cast<int>(min_priority)) sub.goals_.push_back(it);
			}
			model m=(sub.*compiler)(option,var_low,var_high);
			return solver.solve(m,std::move(start));
		};
		solve_result result=solve_at_or_above(GP_CRITICAL,x0);
		result=solve_at_or_above(GP_HIGH,result.x);
		result=solve_at_or_above(GP_MEDIUM,result.x);
		result=solve_at_or_above(GP_LOW,result.x);
		return result;
	}
};

}

namespace algorithm {

using nlp_solver=nlp::solver;

}

}

#endif