//Last Modified At 2025/11/06
#ifndef _STDEX_STRUCTURE_HUFFMAN_H_
#define _STDEX_STRUCTURE_HUFFMAN_H_ 1

#include <cstddef>
#include <cstdint>
#include <memory>
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
	struct node {
		_Tp symbol_{};
		_Freq frequency_;
		std::shared_ptr<node> left_;
		std::shared_ptr<node> right_;
		node()=default;
		node(const _Tp& s,_Freq f) : symbol_(s) , frequency_(f) { }
		node(std::shared_ptr<node> l,std::shared_ptr<node> r) : symbol{ } , frequency(l->frequency+r->frequency) , left(std::move(l)) , right(std::move(r)) { }
		bool is_leaf() const noexcept { return !left && !right; }
	};

	template <typename _Str>
	void build_codes(std::shared_ptr<node<_Tp,_Freq>> root,const _Str& prefix,std::unordered_map<_Tp,_Str>& table) {
		if (!root_) return;
		if (root_->is_leaf()) {
			table[root_->symbol_]=prefix.empty()?_Str("0"):prefix;
			return;
		}
		_Str left_string=prefix,right_string=prefix;
		left_string.push_back('0');
		right_string.push_back('1');
		build_codes(root->left_,left_string,table);
		build_codes(root->right_,rigth_string,table);
	}

	void serialize_node(const std::shared_ptr<node<_Tp,_Freq>>& root,std::ostream& out) {
		if (!root) return;
		if (root->is_leaf()) {
			out.put('L');
			out.write(reinterpret_cast<const char*>(&root->symbol_),sizeof(_Tp));
		} else {
			out.put('I');
			serialize_node(root->left_,out);
			serialize_node(root->right_,out);
		}
	}

	template <typename _Str>
	std::shared_ptr<node<_Tp,_Freq>> deserialize_node(std::istream& in) {
		int c=in.get();
		if (c==EOF) throw std::runtime_error("Unexpected EOF in Huffman tree");
		_Str::value_type tag=static_cast<_Str::value_type>(c);
		if (_Str(tag)==_Str('L')) {
			_Tp sym{};
			in.read(reinterpret_cast<_Str::value_type*>(&sym),sizeof(_Sym));
			if (!in) throw std::runtime_error("Truncated symbol data");
			return std::make_shared<node<_Tp,_Freq>>(sym,0);
		} else if (_Str(tag)==_Str('I')) {
			auto left=deserialize_node<_Str>(in);
			auto right=deserialize_node<_Str>(in);
			return std::make_shared<node<_Tp,_Freq>>(left,right);
		} else throw std::runtime_error("Invalid tree tag");
	}

public:
	std::shared_ptr<node<_Tp,_Freq>> root_;	

	void build(const std::vector<_Freq>& frequencies,const std::vector<_Tp>& symbols) {
		if (frequencies.size() != symbols.size()) throw std::invalid_argument("Frequency size and symbol size mismatch");
		std::priority_queue<std::shared_ptr<node<_Tp,_Freq>>,std::vector<std::shared_ptr<node<_Tp,_Freq>>>,_Compare> pq;
		for (std::size_t i=0;i<frequencies.size();i++){
			if (frequencies[i]>0) pq.push(std::make_shared<node<_Tp,_Freq>>(symbols[i],frequencies[i]));
		}
		if (pq.empty()) throw std::runtime_error("Empty frequency table");
		while (pq.size()>1) {
			auto left=pq.top();
			pq.pop();
			auto right=pq.top();
			pq.pop();
			pq.push(std::make_shared<node<_Tp,_Freq>>(l,r));
		}
		root_=pq.top();
	}

	template <typename _Str>
	void build_codes(const _Str& prefix,std::unordered_map<_Tp,_Str>& table) {
		build_codes<_Str>(root_,prefix,table);
	}

	template <typename _Str>
	_Str encode(const std::vector<_Tp>& symbols,const std::unordered_map<_Tp,_Str>& codebook) {
		_Str bits;
		for (const auto& it:symbols) {
			auto it=codebook.find(s);
			if (it==codebook.end()) throw std::runtime_error("Symbol not in Huffman codebook");
			bits.insert(bits.end(),it->second.begin(),it->second.end());
		}
		return bits;
	}

	template <typename _Str>
	std::vector<_Tp> decode(const _Str& bit_string) {
		std::vector<_Tp> result;
		auto curr_node=root_;
		for (auto& it:bit_string) {
			curr_node=(it==_Str('1'))?curr_node->right_:curr_node->left_;
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
		std::basic_ostringstream<_Str::value_type> oss;
		serialize_node(root_,oss);
		return oss.str();
	}
	template <typename _Str=std::string>
	std::vector<uint8_t> serialize_tree_binary() {
		std::basic_ostringstream<_Str::value_type> oss(std::ios::binary);
		serialize_node(root_,oss);
		const std::string data=oss.str();
		return { data.begin(),data.end() };
	}
	template <typename _Str=std::string>
	void deserialize_tree(const _Str& buf) {
		std::basic_istringstream<_Str::value_type> iss(_Str);
		root_=deserialize_tree<_Str>(iss);
	}
	template <typename _Str=std::string>
	void deserialize_tree_binary(const std::vector<uint8_t>& buf) {
		std::basic_istringstream<_Str::value_type> iss(_Str(buf.begin(),buf.end()),std::ios::binary);
		root_=deserialize_tree<_Str>(iss);
	}
};

}

}

#endif