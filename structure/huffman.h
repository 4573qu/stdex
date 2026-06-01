//Last Modified At 2026/05/29
//@Version 3.2.0.0
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

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

namespace stdex {

namespace structure {

template <typename _Tp,typename _Freq=std::size_t,std::size_t _K=2,typename _Compare=std::greater<_Freq>>
class huffman {
	static_assert(_K>=2,"K must be at least 2.");

public:
	struct node {
		_Tp symbol{};
		_Freq frequency{};
		std::vector<std::shared_ptr<node>> kids;

		node() = default;
		node(const _Tp& symbol, _Freq freq) : symbol(symbol) , frequency(freq) { }
		explicit node(std::vector<std::shared_ptr<node>> childrens) : symbol{} , kids(std::move(childrens)) {
			if (kids.empty() || kids.size()>_K) throw std::invalid_argument("Kids size must be in [1,_K]");
			frequency={};
			for (auto& it:kids) {
				if (!it) throw std::invalid_argument("Null child node");
				frequency+=it->frequency;

			}
		}
		bool is_leaf() const noexcept { return kids.empty(); }
		std::size_t child_count() const noexcept { return kids.size(); }

		const std::shared_ptr<node>& child(std::size_t i) const {
			if (i>=kids.size()) throw std::out_of_range("Child index out of range");
			return kids[i];
		}
		const std::shared_ptr<node>& operator [](std::size_t i) const {
			return child(i);
		}
	};

private:
	template <typename _Up,typename=void>
	struct is_hashable : std::false_type {};
	template <typename _Up>
	struct is_hashable<_Up,std::void_t<decltype(std::hash<_Up>{}(std::declval<_Up>()))>> : std::true_type {};

	template <typename _Up,typename=void>
	struct is_freq_like : std::false_type {};
	template <typename _Up>
	struct is_freq_like<_Up,std::void_t<decltype(std::declval<_Up>()+std::declval<_Up>()),decltype(std::declval<_Up&>()+=std::declval<_Up>())>> : std::true_type {};

	template <typename _Up,typename=void>
	struct is_string_like : std::false_type {};
	template <typename _Up>
	struct is_string_like<_Up,std::void_t<typename _Up::value_type,decltype(std::declval<_Up>().push_back(std::declval<typename _Up::value_type>())),decltype(std::declval<_Up>().size()),decltype(std::declval<_Up>().empty()),decltype(std::declval<_Up>().begin()),decltype(std::declval<_Up>().end()),decltype(std::declval<_Up>().insert(std::declval<_Up>().end(),std::declval<_Up>().begin(),std::declval<_Up>().end()))>> : std::true_type {};

	template <typename _Func,typename=void>
	struct is_leaf_visitor : std::false_type {};
	template <typename _Func>
	struct is_leaf_visitor<_Func,std::void_t<decltype(std::declval<_Func>()(std::declval<const _Tp&>(),std::declval<_Freq>(),std::declval<std::size_t>()))>> : std::true_type {};

	static_assert(is_freq_like<_Freq>::value,"_Freq must support + and += operators.");

	static constexpr char keycodes_[16]={
		'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
	};
	//static_assert(_K<=16,"K must not exceed 16 (keycodes_ has 16 entries).");
	template <typename _Ch>
	static std::vector<_Ch> make_default_codes_() {
		if constexpr (_K<=16) {
			return std::vector<_Ch>(keycodes_,keycodes_+_K);
		} else {
			static_assert(_K<=16,"_K>16 with no default codebook, provide codes explicitly");
			return {};
		}
	}

	std::shared_ptr<node> root_;

	template <typename _Str>
	static void build_codes_impl(const std::shared_ptr<node>& curr,_Str prefix,std::unordered_map<_Tp,_Str>& table,const std::vector<typename _Str::value_type>& codes) {
		if (!curr) return;
		if (curr->is_leaf()) {
			table[curr->symbol]=prefix.empty()?_Str(1,codes[0]):prefix;
			return;
		}
		for (std::size_t i=0;i<curr->kids.size();i++) {
			_Str next=prefix;
			next.push_back(codes[i]);
			build_codes_impl(curr->kids[i],next,table,codes);
		}
	}

	static void serialize_node_impl(const std::shared_ptr<node>& curr,std::ostream& out) {
		static_assert(std::is_trivially_copyable<_Tp>::value, "_Tp must be trivially copyable for binary serialization to work.");
		if (!curr) return;
		if (curr->is_leaf()) {
			out.put('L');
			out.write(reinterpret_cast<const char*>(&curr->symbol),sizeof(_Tp));
		} else {
			out.put('I');
			uint8_t cnt=static_cast<uint8_t>(curr->kids.size());
			out.put(static_cast<char>(cnt));
			for (const auto& it:curr->kids) serialize_node_impl(it,out);
		}
	}

	static std::shared_ptr<node> deserialize_node_impl(std::istream& in) {
		int c=in.get();
		if (c==EOF) throw std::runtime_error("Unexpected EOF");
		if (static_cast<char>(c)=='L') {
			_Tp sym{};
			in.read(reinterpret_cast<char*>(&sym),sizeof(_Tp));
			if (!in) throw std::runtime_error("truncated symbol data");
			return std::make_shared<node>(sym,_Freq{});
		} else if (static_cast<char>(c)=='I') {
			int cnt_c=in.get();
			if (cnt_c==EOF) throw std::runtime_error("Unexpected EOF reading child count");
			std::size_t cnt=static_cast<std::size_t>(static_cast<uint8_t>(cnt_c));
			if (cnt==0 || cnt>_K) throw std::runtime_error("Invalid child count in huffman tree");
			std::vector<std::shared_ptr<node>> kids;
			kids.reserve(cnt);
			for (std::size_t i=0;i<cnt;i++) kids.push_back(deserialize_node_impl(in));
			return std::make_shared<node>(std::move(kids));
		}
		throw std::runtime_error("Invalid node tag in huffman tree");
	}

	template <typename _Func>
	static void for_each_impl(const std::shared_ptr<node>& curr,_Func& func,std::size_t depth) {
		if (!curr) return;
		if (curr->is_leaf()) {
			func(curr->symbol,curr->frequency,depth);
			return;
		}
		for (const auto& it:curr->kids) for_each_impl(it,func,depth+1);
	}

	static void collect_depths_impl(const std::shared_ptr<node>& curr,std::vector<std::size_t>& depths,std::size_t depth) {
		if (!curr) return;
		if (curr->is_leaf()) { 
			depths.push_back(depth);
			return;
		}
		for (const auto& it:curr->kids) collect_depths_impl(it,depths,depth+1);
	}

	static std::shared_ptr<node> build_canonical_impl(const std::vector<std::size_t>& sorted_lengths,const std::vector<std::shared_ptr<node>>& sorted_leaves) {
		if (sorted_leaves.empty()) return nullptr;
		if (sorted_leaves.size()==1) return sorted_leaves[0];
		std::size_t max_d=sorted_lengths.back();
		std::vector<std::vector<std::shared_ptr<node>>> by_depth(max_d+1);
		for (std::size_t i=0;i<sorted_leaves.size();i++) by_depth[sorted_lengths[i]].push_back(sorted_leaves[i]);
		for (std::size_t d=max_d;d>=1;d--) {
			auto& level=by_depth[d];
			std::size_t i=0;
			while (i+_K<=level.size()) {
				std::vector<std::shared_ptr<node>> kids;
				kids.reserve(_K);
				for (std::size_t j=0;j<_K;j++) kids.push_back(level[i+j]);
				i+=_K;
				by_depth[d-1].push_back(std::make_shared<node>(std::move(kids)));
			}
			while (i<level.size()) {
				by_depth[d-1].push_back(level[i]);
				i++;
			}
		}
		if (by_depth[0].size()!=1) throw std::runtime_error("Length assignment does not yield a valid _K-ary huffman tree");
		return by_depth[0][0];
	}

public:
	static constexpr std::size_t arity=_K;

	huffman()=default;
	explicit huffman(std::shared_ptr<node> root) : root_(std::move(root)) { }
	~huffman()=default;
	huffman(const huffman&)=default;
	huffman(huffman&&) noexcept=default;
	huffman& operator =(const huffman&)=default;
	huffman& operator =(huffman&&) noexcept=default;

	bool operator ==(const huffman& other) const noexcept {
		return root_==other.root_;
	}
	bool operator !=(const huffman& other) const noexcept {
		return !(*this==other);
	}
	
	const std::shared_ptr<node>& root() const noexcept { return root_; }
	bool empty() const noexcept { return !root_; }
	void clear() noexcept { root_.reset(); }
	explicit operator bool() const noexcept { return static_cast<bool>(root_); }

	void build(const std::vector<_Tp>& symbols,const std::vector<_Freq>& frequencies) {
		if (symbols.size()!=frequencies.size()) throw std::invalid_argument("Symbol and frequency size mismatch");
		if (symbols.empty()) throw std::invalid_argument("Empty symbol table");
		struct comp_ {
			bool operator ()(const std::shared_ptr<node>& lhs,const std::shared_ptr<node>& rhs) const {
				_Compare cmp;
				return cmp(rhs->frequency,lhs->frequency);
			}
		};
		std::priority_queue<std::shared_ptr<node>,std::vector<std::shared_ptr<node>>,comp_> pq;
		for (std::size_t i=0;i<symbols.size();i++) pq.push(std::make_shared<node>(symbols[i],frequencies[i]));
		if (pq.size()==1) {
			root_=pq.top();
			return;
		}
		std::size_t n=pq.size();
		std::size_t padding=(((_K-1)-(n-1)%(_K-1))%(_K-1));
		for (std::size_t i=0;i<padding;i++) pq.push(std::make_shared<node>(_Tp{},_Freq{}));
		while (pq.size()>1) {
			std::vector<std::shared_ptr<node>> kids;
			kids.reserve(_K);
			std::size_t take=std::min(pq.size(),_K);
			for (std::size_t i=0;i<take;i++) {
				kids.push_back(pq.top());
				pq.pop();
			}
			pq.push(std::make_shared<node>(std::move(kids)));
		}
		root_=pq.top();
	}

	void build(const std::unordered_map<_Tp,_Freq>& freq_map) {
		std::vector<_Tp> syms;
		std::vector<_Freq> freqs;
		syms.reserve(freq_map.size());
		freqs.reserve(freq_map.size());
		for (const auto& it:freq_map) {
			syms.push_back(it.first);
			freqs.push_back(it.second);
		}
		build(syms,freqs);
	}

	void build_from_sequence(const std::vector<_Tp>& sequence) {
		std::unordered_map<_Tp,_Freq> freq_map;
		for (const auto& it:sequence) freq_map[it]+=static_cast<_Freq>(1);
		build(freq_map);
	}

	static std::vector<std::size_t> get_package_merge_lengths(const std::vector<_Freq>& frequencies,std::size_t max_depth) {
		const std::size_t n=frequencies.size();
		if (n==0) return {};
		if (n==1) return {1};
		if (max_depth==0) throw std::invalid_argument("Max_depth cannot be negative than 1");
		std::size_t min_cap=_K,min_d=1;
		while (min_cap<n) {
			min_cap*=_K;
			min_d++;
		}
		if (max_depth<min_d) throw std::invalid_argument("Max_depth too small for given number of symbols");
		std::size_t padding=(((_K-1)-(n-1)%(_K-1))%(_K-1));
		std::size_t n_eff=n+padding;
		std::size_t width=_K*(n_eff-1)/(_K-1);
		struct pm_item_ {
			_Freq weight_;
			int orig_idx_;
			std::size_t pool_idx_;
			std::vector<std::size_t> children_;
		};
		std::vector<pm_item_> pool;
		pool.reserve(n_eff*(max_depth+1)*4);
		struct orig_item_ {
			_Freq weight_;
			int orig_idx_;
		};
		std::vector<orig_item_> sorted_orig;
		sorted_orig.reserve(n_eff);
		for (std::size_t i=0;i<padding;i++) sorted_orig.push_back({_Freq{},-1});
		std::vector<std::size_t> order(n);
		std::iota(order.begin(),order.end(),0);
		std::stable_sort(order.begin(),order.end(),[&](std::size_t lhs,std::size_t rhs){
			_Compare cmp;
			return cmp(frequencies[rhs],frequencies[lhs]);
		});
		for (std::size_t i=0;i<n;i++) sorted_orig.push_back({frequencies[order[i]],static_cast<int>(order[i])});
		std::vector<std::size_t> curr_list;
		curr_list.reserve(width);
		auto make_leaves=[&]()->std::vector<std::size_t>{
			std::vector<std::size_t> v;
			v.reserve(n_eff);
			for (std::size_t i=0;i<n_eff;i++) {
				pool.push_back({sorted_orig[i].weight_,sorted_orig[i].orig_idx_,pool.size(),{}});
				v.push_back(pool.size()-1);
			}
			return v;
		};
		curr_list=make_leaves();
		for (std::size_t iter=0;iter<max_depth-1;iter++) {
			std::vector<std::size_t> pkgs;
			pkgs.reserve(curr_list.size()/_K);
			for (std::size_t i=0;i+_K<=curr_list.size();i+=_K) {
				_Freq w{};
				std::vector<std::size_t> children;
				children.reserve(_K);
				for (std::size_t j=0;j<_K;j++) {
					w+=pool[curr_list[i+j]].weight_;
					children.push_back(curr_list[i+j]);
				}
				pool.push_back({w,-1,pool.size(),std::move(children)});
				pkgs.push_back(pool.size()-1);
			}
			std::vector<std::size_t> new_leaves=make_leaves();
			std::vector<std::size_t> merged;
			merged.reserve(pkgs.size()+n_eff);
			std::size_t pi=0,li=0;
			while (pi<pkgs.size() && li<n_eff) {
				if (pool[pkgs[pi]].weight_<=pool[new_leaves[li]].weight_) merged.push_back(pkgs[pi++]);
				else merged.push_back(new_leaves[li++]);
			}
			while (pi<pkgs.size()) merged.push_back(pkgs[pi++]);
			while (li<n_eff) merged.push_back(new_leaves[li++]);
			if (merged.size()>width) merged.resize(width);
			curr_list=std::move(merged);
		}
		std::vector<std::size_t> lengths(n,0);
		std::function<void(std::size_t)> traverse=[&](std::size_t idx){
			const pm_item_& item=pool[idx];
			if (item.children_.empty()) {
				if (item.orig_idx_>=0) lengths[static_cast<std::size_t>(item.orig_idx_)]++;
			} else {
				for (std::size_t it:item.children_) traverse(it);
			}
		};
		for (std::size_t it:curr_list) traverse(it);
		return lengths;
	}

	static std::vector<std::size_t> get_fast_lengths(const std::vector<_Freq>& frequencies,std::size_t max_depth) {
		std::size_t n=frequencies.size();
		if (n==0) return {};
		if (n==1) return {1};
		if (max_depth==0) throw std::invalid_argument("Max_depth cannot be negative than 1");
		std::size_t depth=1,capacity=_K;
		while (capacity<n && depth<max_depth) {
			depth++;
			capacity*=_K;
		}
		if (capacity<n) throw std::invalid_argument("Max_depth too small for given n");
		std::vector<std::size_t> counts(depth+1,0);
		counts[depth]=capacity;
		std::size_t extra=capacity-n;
		for (std::size_t d=depth;d>1 && extra>0;d--) {
			std::size_t can=std::min(extra,counts[d]/_K);
			counts[d]-=can*_K;
			counts[d-1]+=can;
			extra-=can;
		}
		std::vector<std::size_t> length_sorted;
		length_sorted.reserve(n);
		for (std::size_t d=1;d<=depth;d++) {
			for (std::size_t i=0;i<counts[d];i++) length_sorted.push_back(d);
		}
		length_sorted.resize(n);
		std::vector<std::size_t> order(n);
		std::iota(order.begin(),order.end(),0);
		std::stable_sort(order.begin(),order.end(),[&](std::size_t lhs,std::size_t rhs){
			_Compare cmp;
			return cmp(frequencies[lhs],frequencies[rhs]);
		});
		std::vector<std::size_t> result(n);
		for (std::size_t i=0;i<n;i++) result[order[i]]=length_sorted[i];
		return result;
	}

	void build_from_lengths(const std::vector<_Tp>& symbols,const std::vector<_Freq>& frequencies,const std::vector<std::size_t>& lengths) {
		if (symbols.size()!=frequencies.size()) throw std::invalid_argument("Symbol and frequency size mismatch");
		if (symbols.size()!=lengths.size()) throw std::invalid_argument("Symbol and length size mismatch");
		const std::size_t n=symbols.size();
		if (n==0) {
			root_=nullptr;
			return;
		}
		if (n==1) {
			root_=std::make_shared<node>(symbols[0],frequencies[0]);
			return;
		}
		std::vector<std::size_t> order(n);
		std::iota(order.begin(),order.end(),0);
		std::stable_sort(order.begin(),order.end(),[&](std::size_t lhs,std::size_t rhs){
			if (lengths[lhs]!=lengths[rhs]) return lengths[lhs]<lengths[rhs];
			_Compare cmp;
			return cmp(frequencies[lhs],frequencies[rhs]);
		});
		std::vector<std::size_t> sl(n);
		std::vector<std::shared_ptr<node>> sn(n);
		for (std::size_t i=0;i<n;i++) {
			sl[i]=lengths[order[i]];
			sn[i]=std::make_shared<node>(symbols[order[i]],frequencies[order[i]]);
		}
		root_=build_canonical_impl(sl,sn);
	}

	void build_length_limited(const std::vector<_Tp>& symbols,const std::vector<_Freq>& frequencies,std::size_t max_depth) {
		if (symbols.size()!=frequencies.size()) throw std::invalid_argument("Symbol and frequency size mismatch");
		if (symbols.empty()) {
			root_=nullptr;
			return;
		}
		if (symbols.size()==1) {
			root_=std::make_shared<node>(symbols[0],frequencies[0]);
			return;
		}
		auto lengths=get_package_merge_lengths(frequencies,max_depth);
		build_from_lengths(symbols,frequencies,lengths);
	}

	template <typename _Str=std::string,typename=std::enable_if_t<is_string_like<_Str>::value>>
	std::unordered_map<_Tp,_Str> build_codebook(const std::vector<typename _Str::value_type>& codes=make_default_codes_<typename _Str::value_type>()) const {
		if (!root_) throw std::runtime_error("Huffman tree is empty");
		if (codes.size()<_K) throw std::invalid_argument("Codes vector size less than _K");
		std::unordered_map<_Tp,_Str> table;
		build_codes_impl<_Str>(root_,_Str{},table,codes);
		return table;
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	template<typename _Str=std::string> requires is_string_like<_Str>::value
	std::unordered_map<_Tp,_Str> build_codebook(const std::vector<typename _Str::value_type>& codes=make_default_codes_<typename _Str::value_type>()) const=delete;
#endif

	template<typename _Str=std::string,typename=std::enable_if_t<is_string_like<_Str>::value>>
	_Str encode(const std::vector<_Tp>& symbols,const std::vector<typename _Str::value_type>& codes=make_default_codes_<typename _Str::value_type>()) const {
		auto table=build_codebook<_Str>(codes);
		_Str result;
		for (const auto& it:symbols) {
			auto jt=table.find(it);
			if (jt==table.end()) throw std::runtime_error("Symbol not in huffman codebook");
			result.insert(result.end(),jt->second.begin(),jt->second.end());
		}
		return result;
	}

	template<typename _Str=std::string,typename=std::enable_if_t<is_string_like<_Str>::value>>
	std::vector<_Tp> decode(const _Str& encoded,const std::vector<typename _Str::value_type>& codes=make_default_codes_<typename _Str::value_type>()) const {
		if (!root_) throw std::runtime_error("Huffman tree is empty");
		if (codes.size()<_K) throw std::invalid_argument("Codes vector size less than _K");
		std::vector<_Tp> result;
		auto curr=root_;
		for (std::size_t i=0;i<encoded.size();) {
			if (curr->is_leaf()) {
				result.push_back(curr->symbol);
				curr=root_;
				continue;
			}
			int idx=-1;
			for (std::size_t j=0;j<codes.size();j++) {
				if (static_cast<typename _Str::value_type>(codes[j])==encoded[i]) {
					idx=static_cast<int>(j);
					break;
				}
			}
			if (idx<0) throw std::invalid_argument("Invalid character at position "+std::to_string(i));
			if (static_cast<std::size_t>(idx)>=curr->kids.size()) throw std::invalid_argument("Code leads to non-existent child");
			curr=curr->kids[static_cast<std::size_t>(idx)];
			i++;
			if (curr->is_leaf()) {
				result.push_back(curr->symbol);
				curr=root_;
			}
		}
		if (curr!=root_) throw std::runtime_error("Incomplete code sequence at end of input");
		return result;
	}

	template<typename _Str=std::string,typename=std::enable_if_t<is_string_like<_Str>::value>>
	static std::unordered_map<_Tp,_Str> canonical_codebook_from_lengths(const std::vector<_Tp>& symbols,const std::vector<std::size_t>& lengths,const std::vector<typename _Str::value_type>& codes=make_default_codes_<typename _Str::value_type>()) {
		if (symbols.size()!=lengths.size()) throw std::invalid_argument("Symbol and length size mismatch");
		if (codes.size()<_K) throw std::invalid_argument("Codes vector size less than _K");
		const std::size_t n=symbols.size();
		if (n==0) return {};
		std::vector<std::size_t> order(n);
		std::iota(order.begin(),order.end(),0);
		std::stable_sort(order.begin(),order.end(),[&](std::size_t lhs,std::size_t rhs){
			return lengths[lhs]<lengths[rhs];
		});
		std::unordered_map<_Tp,_Str> table;
		std::vector<std::size_t> code_val(_K,0);
		std::size_t cur_len=lengths[order[0]];
		std::vector<std::size_t> digits(cur_len,0);
		auto digits_to_str=[&](const std::vector<std::size_t>& digs)->_Str{
			_Str s;
			for (std::size_t it:digs) s.push_back(codes[it]);
			return s;
		};
		auto increment=[&](std::vector<std::size_t>& digs){
			for (int p=static_cast<int>(digs.size())-1;p>=0;p--) {
				digs[p]++;
				if (digs[p]<_K) return;
				digs[p]=0;
			}
		};
		for (std::size_t i=0;i<n;i++) {
			std::size_t len=lengths[order[i]];
			if (len>cur_len) {
				while (cur_len<len) {
					digits.push_back(0);
					cur_len++;
				}
			}
			table[symbols[order[i]]]=digits_to_str(digits);
			increment(digits);
		}
		return table;
	}

	std::string serialize_tree() const {
		std::ostringstream oss(std::ios::binary);
		serialize_node_impl(root_,oss);
		return oss.str();
	}

	std::vector<uint8_t> serialize_tree_binary() const {
		std::ostringstream oss(std::ios::binary);
		serialize_node_impl(root_,oss);
		const auto s=oss.str();
		return std::vector<uint8_t>(s.begin(),s.end());
	}

	template<typename _Str=std::string,typename=std::enable_if_t<is_string_like<_Str>::value>>
	_Str serialize_codebook(const std::unordered_map<_Tp,_Str>& codebook) const {
		std::basic_ostringstream<typename _Str::value_type> oss;
		oss<<std::hex<<std::setfill('0');
		for (const auto& it:codebook) oss<<static_cast<uint64_t>(it.first)<<':'<<it.second<<'\n';
		return oss.str();
	}

	void deserialize_tree(const std::string& buf) {
		std::istringstream iss(buf,std::ios::binary);
		root_=deserialize_node_impl(iss);
	}

	void deserialize_tree_binary(const std::vector<uint8_t>& buf) {
		std::string s(buf.begin(),buf.end());
		std::istringstream iss(s,std::ios::binary);
		root_=deserialize_node_impl(iss);
	}

	template<typename _Func,typename=std::enable_if_t<is_leaf_visitor<_Func>::value>>
	void for_each(_Func func) const {
		for_each_impl(root_,func,0);
	}

	std::vector<std::size_t> depths() const {
		if (!root_) return {};
		if (root_->is_leaf()) return {0};
		std::vector<std::size_t> result;
		collect_depths_impl(root_,result,0);
		return result;
	}

	std::unordered_map<_Tp,std::size_t> code_lengths() const {
		std::unordered_map<_Tp,std::size_t> result;
		for_each_impl(root_,[&](const _Tp& sym,_Freq,std::size_t depth){
			result[sym]=depth;
		},0);
		return result;
	}

	std::size_t max_depth() const {
		auto ds=depths();
		if (ds.empty()) return 0;
		return *std::max_element(ds.begin(),ds.end());
	}

	std::size_t leaf_count() const {
		std::size_t cnt=0;
		for_each_impl(root_,[&](const _Tp&,_Freq,std::size_t){
			cnt++;
		},0);
		return cnt;
	}

	_Freq weighted_path_length() const {
		_Freq result{};
		for_each_impl(root_,[&](const _Tp&,_Freq freq,std::size_t depth){
			for (std::size_t i=0;i<depth;i++) result+=freq;
		},0);
		return result;
	}

	double entropy() const {
		double total=0.0;
		for_each_impl(root_,[&](const _Tp&,_Freq freq,std::size_t){
			total+=static_cast<double>(freq);
		},0);
		if (total<=0.0) return 0.0;
		double h=0.0;
		for_each_impl(root_,[&](const _Tp&,_Freq freq,std::size_t){
			double p=static_cast<double>(freq)/total;
			if (p>0.0) h-=p*std::log2(p);
		},0);
		return h;
	}

	double average_code_length() const {
		double total=0.0,weighted=0.0;
		for_each_impl(root_,[&](const _Tp&,_Freq freq,std::size_t depth){
			double f=static_cast<double>(freq);
			total+=f;
			weighted+=f*static_cast<double>(depth);
		},0);
		if (total<=0.0) return 0.0;
		return weighted/total;
	}

	double coding_efficiency() const {
		double h=entropy();
		double acl=average_code_length();
		if (acl<=0.0) return 0.0;
		return h/acl;
	}

	double kraft_sum() const {
		double sum=0.0;
		for_each_impl(root_,[&](const _Tp&,_Freq,std::size_t depth){
			double val=1.0;
			for (std::size_t i=0;i<depth;i++) val/=static_cast<double>(_K);
			sum+=val;
		},0);
		return sum;
	}

	std::vector<std::tuple<_Tp,_Freq,std::size_t>> leaf_info() const {
		std::vector<std::tuple<_Tp,_Freq,std::size_t>> result;
		for_each_impl(root_,[&](const _Tp& sym,_Freq freq,std::size_t depth){
			result.emplace_back(sym,freq,depth);
		},0);
		return result;
	}

	bool has_sibling_property() const {
		if (!root_) return true;
		std::vector<_Freq> all_freqs;
		std::function<void(const std::shared_ptr<node>&)> collect=[&](const std::shared_ptr<node>& curr) {
			if (!curr) return;
			all_freqs.push_back(curr->frequency);
			for (const auto& it:curr->kids) collect(it);
		};
		collect(root_);
		std::sort(all_freqs.begin(),all_freqs.end(),[](const _Freq& lhs,const _Freq& rhs){
			_Compare cmp;
			return cmp(rhs,lhs);
		});
		for (std::size_t i=0;i+1<all_freqs.size();i+=2) {
			_Compare cmp;
			if (cmp(all_freqs[i+1],all_freqs[i])) return false;
		}
		return true;
	}

#ifdef _STDEX_HUFFMAN_DEBUG
	std::string to_dot() const {
		std::ostringstream oss;
		oss<<"digraph huffman {\n";
		oss<<"\tnode [shape=circle];\n";
		std::size_t id=0;
		std::function<void(const std::shared_ptr<node>&,std::size_t)> dump=[&](
			const std::shared_ptr<node>& curr,
			std::size_t parent_id
		) {
			if (!curr) return;
			std::size_t my_id=id++;
			if (curr->is_leaf()) {
				oss<<"\t\""<<my_id<<"\" [label=\""
					<<curr->symbol<<'/'
					<<curr->frequency
					<<"\",shape=box];\n";
			} else {
				oss<<"\t\""<<my_id<<"\" [label=\""
					<<curr->frequency<<"\"];\n";
			}
			if (my_id!=parent_id)
				oss<<"\t\""<<parent_id<<"\" -> \""<<my_id<<"\";\n";
			for (const auto& k : curr->kids)
				dump(k,my_id);
		};
		if (root_) dump(root_,0);
		oss<<"}\n";
		return oss.str();
	}
#endif
};

}

}

#endif