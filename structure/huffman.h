//Last Modified At 2025/11/06
//@Version 1.1.0.0
#ifndef _STDEX_STRUCTURE_HUFFMAN_H_
#define _STDEX_STRUCTURE_HUFFMAN_H_ 1

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <memory>
#include <numeric>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace stdex {

namespace structure {

template <typename _Tp,typename _Freq=std::size_t,typename _Compare=std::greater<_Freq>>
class huffman {
public:
	struct node {
		_Tp symbol_{};
		_Freq frequency_;
		std::shared_ptr<node> left_;
		std::shared_ptr<node> right_;
		node()=default;
		node(const _Tp& symbol,_Freq freq) : symbol_(symbol) , frequency_(freq) { }
		node(std::shared_ptr<node> left,std::shared_ptr<node> right) : symbol_{} , frequency_(left->frequency_+right->frequency_) , left_(std::move(left)) , right_(std::move(right)) { }
		bool is_leaf() const noexcept { return !left_ && !right_; }
	};

private:
	template <typename _Str>
	void build_codes(std::shared_ptr<node> root,const _Str& prefix,std::unordered_map<_Tp,_Str>& table) {
		if (!root) return;
		if (root->is_leaf()) {
			table[root->symbol_]=prefix.empty()?_Str("0"):prefix;
			return;
		}
		_Str left_string=prefix,right_string=prefix;
		left_string.push_back('0');
		right_string.push_back('1');
		build_codes(root->left_,left_string,table);
		build_codes(root->right_,right_string,table);
	}

	void serialize_node(const std::shared_ptr<node>& root,std::ostream& out) {
		if (!root) return;
		if (root->is_leaf()) {
			out.put('L');
			out.write(reinterpret_cast<const char*>(&root->symbol_),sizeof(_Tp));
		} else {
			out.put('I');
			//out.put('(');
			serialize_node(root->left_,out);
			//out.put(',');
			serialize_node(root->right_,out);
			//out.put(')');
		}
	}

	template <typename _Str>
	std::shared_ptr<node> deserialize_node(std::istream& in) {
		int c=in.get();
		if (c==EOF) throw std::runtime_error("Unexpected EOF in Huffman tree");
		typename _Str::value_type tag=static_cast<typename _Str::value_type>(c);
		if (_Str(tag)==_Str('L')) {
			_Tp sym{};
			in.read(reinterpret_cast<typename _Str::value_type*>(&sym),sizeof(sym));
			if (!in) throw std::runtime_error("Truncated symbol data");
			return std::make_shared<node>(sym,0);
		} else if (_Str(tag)==_Str('I')) {
			auto left=deserialize_node<_Str>(in);
			auto right=deserialize_node<_Str>(in);
			return std::make_shared<node>(left,right);
		} else throw std::runtime_error("Invalid tree tag");
	}
	
	template <typename _Func>
	void for_each(const std::shared_ptr<node>& root,_Func func,int index) const {
		if (!root) return;
		if (root->is_leaf()) {
			func(root->symbol_,root->frequency_,index);
			return;
		}
		for_each<_Func>(root->left_,func,index+1);
		for_each<_Func>(root->right_,func,index+1);
	}

public:
	std::shared_ptr<node> root_;	

	void build(const std::vector<_Freq>& frequencies,const std::vector<_Tp>& symbols) {
		if (frequencies.size()!=symbols.size()) throw std::invalid_argument("Frequency size and symbol size mismatch");
		std::priority_queue<std::shared_ptr<node>,std::vector<std::shared_ptr<node>>,_Compare> pq;
		for (std::size_t i=0;i<frequencies.size();i++){
			if (frequencies[i]>0) pq.push(std::make_shared<node>(symbols[i],frequencies[i]));
		}
		if (pq.empty()) throw std::runtime_error("Empty frequency table");
		while (pq.size()>1) {
			auto left=pq.top();
			pq.pop();
			auto right=pq.top();
			pq.pop();
			pq.push(std::make_shared<node>(left,right));
		}
		root_=pq.top();
	}
	
	/*[[discarded]]
	static std::vector<std::size_t> get_package_length(const std::vector<_Freq>& frequencies,int max_depth) {
		/*struct package {
			_Freq freq_;
			int count_;
			std::vector<int> items_;
			package() { }
			package(_Freq freq,int count,int index) : freq_(freq) , count_(count) {
				if (index>=0) items_.push_back(index);
			}
		};
		auto merge_package=[](const package &lhs,const package &rhs) {
			package result;
			result.freq_=lhs.freq_+rhs.freq_;
			result.count_=lhs.count_+rhs.count_;
			result.items_.reserve(lhs.items_.size()+rhs.items_.size());
			result.items_.insert(result.items_.end(),lhs.items_.begin(),lhs.items_.end());
			result.items_.insert(result.items_.end(),rhs.items_.begin(),rhs.items_.end());
			return result;
		};
		However, I give up with PackageMerge for the reason that it is too hard to learn about
	}*/

	//when the tree is so much complicated,it may have some bug while generating length
	//for example:generate code-length table in DEFLATE btype=2 while building an exe.
	static std::vector<std::size_t> get_shortest_length(const std::vector<_Freq>& frequencies,int max_depth) {
		const int n=static_cast<int>(frequencies.size());
		if (n==0) return {};
		if (n==1) return {1};
		if (max_depth<=0) throw std::invalid_argument("Invalid max_depth");
		auto solve=[&](int h,std::vector<std::size_t> emptys,std::vector<_Freq> freq)->std::vector<std::size_t>{
			int m=freq.size();
			int empty=emptys[h];
			auto get_cost=[&](std::size_t target)->std::size_t{
				return std::pow(2,h-target)-1;
			};
			std::vector<std::pair<int,int>> indexes;
			for (int i=0;i<m;i++) indexes.push_back({freq[i],i});
			struct index_cmp {
				bool operator ()(const std::pair<int,int>& lhs,const std::pair<int,int>& rhs) {
					_Compare comp;
					return comp(rhs.first,lhs.first);
				}
			};
			std::sort(indexes.begin(),indexes.end(),index_cmp());
			long long best_sum=LLONG_MAX;
			std::vector<std::size_t> best_height(m,h);
			std::vector<std::vector<long long>> dp(m+1,std::vector<long long>(empty+1,LLONG_MAX));
			std::vector<std::vector<std::size_t>> choice(m+1,std::vector<std::size_t>(empty+1,-1));
			dp[0][0]=0;
			for (int i=0;i<m;i++) {
				int index=indexes[i].second;
				for (int j=0;j<=empty;j++) {
					if (dp[i][j]==LLONG_MAX) continue;
					for (int target=1;target<=h;target++) {
						int cost=get_cost(target);
						if (j+cost<=empty) {
							long long new_val=dp[i][j]+(long long)target*freq[index];
							if (new_val<dp[i+1][j+cost]) {
								dp[i+1][j+cost]=new_val;
								choice[i+1][j+cost]=target;
							}
						}
					}
				}
			}
			if (dp[m][empty]!=LLONG_MAX) {
				best_sum=dp[m][empty];
				std::vector<std::size_t> height(m,h);
				int curr_empty=empty;
				for (int i=m-1;i>=0;i--) {
					int index=indexes[i].second;
					for (int target=1;target<=h;target++) {
						int cost=get_cost(target);
						if (curr_empty>=cost && dp[i][curr_empty-cost]+(long long)target*freq[index]==dp[i+1][curr_empty]) {
							height[index]=target;
							curr_empty-=cost;
							break;
						}
					}
				}
				best_height=height;
			}
			return best_height;
		};
		//h=max_depth emptys=[0..h-1]:0 [h]:2^max_depth-n freq=frequencies
		std::vector<std::size_t> emptys(max_depth+1,0);
		emptys[max_depth]=std::pow(2,max_depth)-n;
		return solve(max_depth,emptys,frequencies);
	}
	
	static std::vector<std::size_t> get_fastest_length(const std::vector<_Freq>& frequencies,int max_depth) {
		const int n=static_cast<int>(frequencies.size());
		if (n==0) return {};
		if (n==1) return {1};
		if (max_depth<=0) throw std::invalid_argument("Invalid max_depth");
		std::size_t depth=1;
		while ((1ULL<<depth)<n && depth<max_depth) depth++;
		std::size_t total=1ULL<<depth;
		std::size_t extra=total-n;
		std::vector<std::size_t> counts(depth+1,0);
		counts[depth]=total;
		for (std::size_t d=depth;d>1 && extra>0;d--) {
			std::size_t can_fold=std::min(extra,counts[d]/2);
			counts[d]-=can_fold*2;
			counts[d-1]+=can_fold;
			extra-=can_fold;
		}
		std::vector<std::size_t> lengths;
		lengths.reserve(n);
		for (std::size_t d=1;d<=depth;d++) {
			for (std::size_t i=0;i<counts[d];i++) lengths.push_back(d);
        }
		if (lengths.size()>n) lengths.resize(n);
		return lengths;
	}
	
	void build_shortest_limited(const std::vector<_Freq>& frequencies,const std::vector<_Tp>& symbols,int max_depth) {
		if (frequencies.size()!=symbols.size()) throw std::invalid_argument("Frequency size and symbol size mismatch");
		const int n=static_cast<int>(frequencies.size());
		if (n==0) throw std::invalid_argument("Invalid size");
		if (n==1) {
			root_=std::make_shared<node>(symbols[0],frequencies[0]);
			return;
		}
		if (max_depth<=0) throw std::invalid_argument("Invalid max_depth");
		auto length=get_fastest_length(frequencies,max_depth);
		//std::reverse(length.begin(),length.end());
		std::sort(length.begin(),length.end());
		std::vector<std::shared_ptr<node>> nodes;
		for (int i=0;i<n;i++) nodes.push_back(std::make_shared<node>(symbols[i],frequencies[i]));
		std::stable_sort(nodes.begin(),nodes.end(),[](const std::shared_ptr<node>& lhs,const std::shared_ptr<node>& rhs){
			return lhs->frequency_<rhs->frequency_;
		});
		root_=std::make_shared<node>();
		std::function<void(std::shared_ptr<node>&,int,int&)> write_kid=[&](std::shared_ptr<node>& curr,int depth,int& index)->void{
			bool right=false;
			while (index<n) {
				if (length[index]==depth+1) {
					if (!right) {
						curr->left_=nodes[index];
						curr->frequency_+=nodes[index]->frequency_;
						index++;
						right=true;
					} else {
						curr->right_=nodes[index];
						curr->frequency_+=nodes[index]->frequency_;
						index++;
						return;
					}
				} else {
					if (!right) {
						curr->left_=std::make_shared<node>();
						write_kid(curr->left_,depth+1,index);
						curr->frequency_+=curr->left_->frequency_;
						right=true;
					} else {
						curr->right_=std::make_shared<node>();
						write_kid(curr->right_,depth+1,index);
						curr->frequency_+=curr->right_->frequency_;
						return;
					}
				}
			}
		};
		int index=0;
		write_kid(root_,0,index);
	}

	template <typename _Str>
	void build_codes(const _Str& prefix,std::unordered_map<_Tp,_Str>& table) {
		build_codes<_Str>(root_,prefix,table);
	}

	std::vector<std::size_t> get_depths() {
		if (root_->is_leaf()) return {1};
		std::vector<std::size_t> result;
		get_depths(result,root_,0);
		return result;
	}

	void get_depths(std::vector<std::size_t>& depths,std::shared_ptr<node> node,std::size_t depth) {
		if (node->is_leaf()) depths.push_back(depth);
		else {
			get_depths(depths,node->left_,depth+1);
			get_depths(depths,node->right_,depth+1);
		}
	}

	template <typename _Str>
	_Str encode(const std::vector<_Tp>& symbols,const std::unordered_map<_Tp,_Str>& codebook) {
		_Str bits;
		for (const auto& it:symbols) {
			auto jt=codebook.find(it);
			if (jt==codebook.end()) throw std::runtime_error("Symbol not in Huffman codebook");
			bits.insert(bits.end(),jt->second.begin(),jt->second.end());
		}
		return bits;
	}

	template <typename _Str>
	std::vector<_Tp> decode(const _Str& bit_string) {
		std::vector<_Tp> result;
		auto curr_node=root_;
		for (auto& it:bit_string) {
			curr_node=(it=='1')?curr_node->right_:curr_node->left_;
			if (curr_node->is_leaf()) {
				result.push_back(curr_node->symbol_);
				curr_node=root_;
			}
		}
		return result;
	}

	template <typename _Str>
	_Str serialize(const std::unordered_map<_Tp,_Str>& codes) {
		std::ostringstream oss;
		oss<<std::hex<<std::setfill('0');
		for (const auto& it:codes) {
			const _Tp& sym=it.first;
			const _Str& bits=it.second;
			oss<<static_cast<uint64_t>(sym)<<':'<<bits<< '\n';
		}
		return oss.str();
	}

	template <typename _Str=std::string>
	_Str serialize_tree() {
		std::basic_ostringstream<typename _Str::value_type> oss;
		serialize_node(root_,oss);
		return oss.str();
	}
	template <typename _Str=std::string>
	std::vector<uint8_t> serialize_tree_binary() {
		std::basic_ostringstream<typename _Str::value_type> oss(std::ios::binary);
		serialize_node(root_,oss);
		const std::string data=oss.str();
		return { data.begin(),data.end() };
	}
	template <typename _Str=std::string>
	void deserialize_tree(const _Str& buf) {
		std::basic_istringstream<typename _Str::value_type> iss(_Str);
		root_=deserialize_tree<_Str>(iss);
	}
	template <typename _Str=std::string>
	void deserialize_tree_binary(const std::vector<uint8_t>& buf) {
		std::basic_istringstream<typename _Str::value_type> iss(_Str(buf.begin(),buf.end()),std::ios::binary);
		root_=deserialize_tree<_Str>(iss);
	}
	
	template <typename _Func>
	void for_each(_Func func) const {
		int index=0;
		for_each<_Func>(root_,func,index);
	}
};

}

}

#endif