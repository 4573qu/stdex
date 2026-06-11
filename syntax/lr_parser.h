//Last Modified At 2026/06/11
//@Version 1.2.0.0
#ifndef _STDEX_SYNTAX_LR_PARSER_H_
#define _STDEX_SYNTAX_LR_PARSER_H_ 1

#include <cstddef>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "parser.h"//At Least 3.4

namespace stdex {
	
namespace syntax {

template <typename _Tp,typename _SentenceEnum=int>
class lalr_parser : public parser<_Tp,_SentenceEnum> {
	using base=parser<_Tp,_SentenceEnum>;
	using unit_type=typename base::unit_type;
	using lr_node=typename base::lr_node;
	struct lalr_item {
		uintptr_t index;
		std::unordered_set<_Tp> lookaheads;
	};
	std::unordered_map<lr_node*,std::vector<lalr_item>> lalr_items_;
public:
	lalr_parser(std::initializer_list<std::variant<_Tp,std::vector<unit_type>,std::map<_Tp,bool>>> init_list) : parser<_Tp,_SentenceEnum>(init_list) { }
private:
	std::unordered_set<_Tp> compute_first(const std::vector<_Tp>& sequence) {
		std::unordered_set<_Tp> result;
		for (auto& it:sequence) {
			for (auto& jt:base::first_set[it]) {
				if (jt!=base::epsilon) result.insert(jt);
			}
			if (base::first_set[it].find(base::epsilon)==base::first_set[it].end()) return result;
		}
		result.insert(base::epsilon);
		return result;
	}
	void propagate(lr_node* start) {
		for (auto& it:base::lr_node_list) {
			for (uintptr_t i=0;i<it->unit_list.size();i++) {
				lalr_item temp{i,{}};
				lalr_items_[it].push_back(temp);
			}
		}
		std::queue<std::pair<lr_node*,int>> q;
		for (int i=0;i<start->unit_list.size();i++) {
			if (start->unit_list[i].left_op==base::start && start->unit_list[i].dot==0) {
				lalr_items_[start][i].lookaheads.insert(base::eof);
				q.push(std::make_pair(start,i));
			}
		}
		while (q.size()) {
			auto current=q.front();
			q.pop();
			unit_type& item=current.first->unit_list[current.second];
			auto& lookaheads=lalr_items_[current.first][current.second].lookaheads;
			if (item.dot<item.right_ops.size()) {
				_Tp ptr=item.right_ops[item.dot];
				std::vector<_Tp> beta;
				beta.insert(beta.end(),item.right_ops.begin()+item.dot+1,item.right_ops.end());
				if (base::ptrs[ptr]) {
					auto firsts=compute_first(beta);
					if (firsts.find(base::epsilon)!=firsts.end()) firsts.insert(lookaheads.begin(),lookaheads.end());
					for (int i=0;i<current.first->unit_list.size();i++) {
						if (current.first->unit_list[i].left_op==ptr && current.first->unit_list[i].dot==0) {
							if (firsts.find(base::epsilon)!=firsts.end()) q.push(std::make_pair(current.first,i));
							else {
								uintptr_t size=lalr_items_[current.first][i].lookaheads.size();
								lalr_items_[current.first][i].lookaheads.insert(firsts.begin(),firsts.end());
								if (lalr_items_[current.first][i].lookaheads.size()>size) q.push(std::make_pair(current.first,i));
							}
						}
					}
				}
				if (current.first->edges.count(ptr) && current.first->edges[ptr]) {
					unit_type temp;
					temp.left_op=item.left_op;
					temp.right_ops=item.right_ops;
					temp.dot=item.dot+1;
					for (int i=0;i<current.first->edges[ptr]->unit_list.size();i++) {
						if (current.first->edges[ptr]->unit_list[i]==temp) {
							//auto next_firsts=compute_first(beta);
							uintptr_t size=lalr_items_[current.first->edges[ptr]][i].lookaheads.size();
							lalr_items_[current.first->edges[ptr]][i].lookaheads.insert(lookaheads.begin(),lookaheads.end());
							if (lalr_items_[current.first->edges[ptr]][i].lookaheads.size()>size) q.push(std::make_pair(current.first->edges[ptr],i));
						}
					}
				}
			}
		}
	}
	void construct_table() override {
		auto it=base::units_by_lhs_.find(base::start);
		std::vector<unit_type*> start_units;
		if (it!=base::units_by_lhs.end()) {
			for (auto& jt:it->second) {
				if (jt->right_ops.size() && jt->right_ops[jt->right_ops.size()-1]==base::eof) start_units.push_back(const_cast<unit_type*>(jt));
			}
		}
		for (auto& it:base::lr_node_list) {
			for (auto jt:it->edges) {
				base::lr_sheet[std::make_pair(jt.first,it->id)].next.lr_ptr=new std::shared_ptr<lr_node>(std::make_shared<lr_node>(*jt.second));
				if (!base::ptrs[jt.first]) base::lr_sheet[std::make_pair(jt.first,it->id)].type=ST_SHIFT;
			}
		}
		for (auto& it:base::lr_node_list) {
			uintptr_t i=0;
			for (auto& jt:lalr_items_[it]) {
				if (it->unit_list[jt.index].dot==it->unit_list[jt.index].right_ops.size()) {
					unit_type temp_unit;
					temp_unit.left_op=it->unit_list[jt.index].left_op;
					temp_unit.right_ops=it->unit_list[jt.index].right_ops;
					temp_unit.dot=-1;
					temp_unit.id=it->unit_list[jt.index].id;
					for (auto kt:base::ptrs) {
						_Tp current_ptr=kt.first;
						if (jt.lookaheads.find(current_ptr)!=jt.lookaheads.end()) {
							if (base::lr_sheet[std::make_pair(current_ptr,it->id)].next.lr_ptr && base::lr_sheet[std::make_pair(current_ptr,it->id)].type!=ST_ERROR) {
								if (base::ptrs[current_ptr]) throw std::logic_error("Conflict GOTO and REDUCTION at production "+std::to_string(i)+"("+it->unit_list[jt.index].to_string()+") with GT("+std::to_string(it->id)+","+std::to_string(current_ptr)+")");
								else {
									if (base::lr_sheet[std::make_pair(current_ptr,it->id)].type==ST_SHIFT) throw std::logic_error("Conflict SHIFT and REDUCTION at production "+std::to_string(i)+"("+it->unit_list[jt.index].to_string()+") with SHIFT("+std::to_string(it->id)+","+std::to_string(current_ptr)+")");
									else throw std::logic_error("Conflict REDUCTION and REDUCTION at production "+std::to_string(i)+"("+it->unit_list[jt.index].to_string()+") with REDUCTION("+std::to_string(it->id)+","+std::to_string(current_ptr)+")");
								}
							}
							base::lr_sheet[std::make_pair(current_ptr,it->id)].type=ST_REDUCTION;
						}
						if (!base::lr_sheet[std::make_pair(current_ptr,it->id)].next.unit_ptr) base::lr_sheet[std::make_pair(current_ptr,it->id)].next.unit_ptr=new std::shared_ptr<unit_type>(std::make_shared<unit_type>(temp_unit));
					}
				}
				i++;
			}
			if (it->unit_list.size()==1 && it->unit_list[0].right_ops.size() && it->unit_list[0].right_ops[it->unit_list[0].right_ops.size()-1]==base::eof) {
				unit_type temp_unit=unit_list[0];
				temp_unit->dot=-1;
				for (auto& jt:start_units) {
					if (*jt==temp_unit) {
						for (auto& kt:lr_node_list) {
							for (auto& lt:kt->edges) {
								if (lt.second->id==it->id) lr_sheet[std::make_pair(base::eof,kt->id)].type=ST_ACCEPT;
							}
						}
					}
				}
			}
		}
    }

public:
	void generate_parser(bool auto_ptr=true) override {
		uintptr_t node_amount=0;
		if (auto_ptr) {
			base::ptrs.clear();
			for (auto it:base::units) {
				base::ptrs[it.left_op]|=true;
				for (auto jt:it.right_ops) base::ptrs[jt]|=false;
			}
		}
		base::generate_initialize();
		auto it=base::units_by_lhs.find(base::start);
		if (it==base::units_by_lhs.end()) return;
		std::vector<unit_type> start_units;
		for (auto jt:it->second) start_units.push_back(*jt);
		auto I0=base::generate_lr_node(start_units,node_amount);
		base::calculate_first();
		lalr_items_.clear();
		propagate(I0);
#ifdef _STDEX_OUTPUT_PARSER
		auto lr_sort=[](lr_node* lhs,lr_node* rhs){
			return lhs->id<rhs->id;
		};
		std::set<lr_node*,decltype(lr_sort)> lr_output(lr_sort);
		for (auto& it:base::lr_node_list) lr_output.insert(it);
		for (auto& it:lr_output) {
			_STDEX_OUTPUT_PARSER<<it->id<<":"<<std::endl;
			_STDEX_OUTPUT_PARSER<<"  units:"<<std::endl;
			for (int i=0;i<it->unit_list.size();i++) {
				_STDEX_OUTPUT_PARSER<<"    "<<it->unit_list[i].left_op<<"->";
				for (int j=0;j<it->unit_list[i].right_ops.size();j++) {
					if (j==it->unit_list[i].dot) _STDEX_OUTPUT_PARSER<<"· ";
					_STDEX_OUTPUT_PARSER<<it->unit_list[i].right_ops[j]<<" ";
				}
				if (it->unit_list[i].dot==it->unit_list[i].right_ops.size()) _STDEX_OUTPUT_PARSER<<"· ";
				_STDEX_OUTPUT_PARSER<<"{";
				std::string temp_lookahead="";
				for (auto jt:lalr_items_[it][i].lookaheads) temp_lookahead+=std::to_string((int)jt)+",";
				if (temp_lookahead.size()) temp_lookahead.pop_back();
				_STDEX_OUTPUT_PARSER<<temp_lookahead<<"}"<<std::endl;
			}
			_STDEX_OUTPUT_PARSER<<"  edges:"<<std::endl;
			for (auto jt:it->edges) _STDEX_OUTPUT_PARSER<<"    "<<jt.first<<"->"<<jt.second->id<<std::endl;
			_STDEX_OUTPUT_PARSER<<std::endl;
		}
#endif
		construct_table();
#ifdef _STDEX_OUTPUT_PARSER
		_STDEX_OUTPUT_PARSER<<"\n";
		for (auto& it:base::lr_sheet) {
			std::vector<std::string> get_type={"e","r","s","a"};
			intptr_t id=-1;
			if (it.second.type==ST_SHIFT || (it.second.type==ST_ERROR && it.second.next.lr_ptr)) id=(*it.second.next.lr_ptr)->id;
			else if (it.second.type_==ST_REDUCTION) {
				for (uintptr_t i=0;i<base::units.size();i++) {
					if (base::units[i]==**it.second.next.unit_ptr) id=i;
				}
			}
			_STDEX_OUTPUT_PARSER<<it.first.second<<"-"<<it.first.first<<"->"<<get_type[(int)it.second.type]<<id<<std::endl;
		}
#endif
	}
};

template <typename _Tp,typename _SentenceEnum=int,uintptr_t _K=1>
class lr_parser : public parser<_Tp,_SentenceEnum> {
	using base=parser<_Tp,_SentenceEnum>;
	using unit_type=typename base::unit_type;
	using lr_node=typename base::lr_node;
	struct lrk_node : lr_node {
		std::map<unit_type*,std::set<std::vector<Tp>> lookaheads;
		lrk_node(uintptr_t id) : lr_node(id) {
			table=&lrk_items;
		}
		bool operator ==(const lrk_node& other) {
			if (*static_cast<lr_node*>(this)!=*static_cast<lr_node*>(const_cast<lrk_node*>(&other))) return false;
			for (int i=0;i<unit_list_.size();i++) {
				if (lookaheads[&unit_list[i]]!=other.lookaheads[&other.unit_list[i]]) return false;
			}
			return true;
		}
	};
public:
	lr_parser(std::initializer_list<std::variant<_Tp,std::vector<unit_type>,std::map<_Tp,bool>>> init_list) : parser<_Tp,_SentenceEnum>(init_list) { }
private:
	lrk_node* generate_lrk_node(lrk_node* prev_node,std::vector<int> starts,uintptr_t& node_amount) {
		lrk_node* curr_node=new lr_node(node_amount++);
		std::unordered_set<unit_type*> temp_set;
		if (!prev_node) {
			for (auto& it:starts) {
				curr_node->unit_list.push_back(units[i]);
				if (units_[i].left_op==base::start) curr_node->lookaheads[&curr_node->unit_list[curr_node->unit_list.size()-1]].insert({base::eof});
				curr_node->unit_list[curr_node->unit_list.size()-1].dot=0;
				temp_set.insert(&curr_node->unit_list[curr_node->unit_list.size()-1]);
			}
		} else {
			for (auto& it:starts) {
				curr_node->unit_list.push_back(prev_node->unit_list[i]);
				curr_node->lookaheads[&curr_node->unit_list[curr_node->unit_list.size()-1]]=prev_node->lookaheads[&prev_node->unit_list[i]];
				curr_node->unit_list[curr_node->unit_list.size()-1].dot++;
				temp_set.insert(&curr_node->unit_list[curr_node->unit_list.size()-1]);
			}
		}

//use closure to get units
//SECOND propagate to every unit of curr_node
//思路是对的，核心是改闭包
/*
闭包部分简述参考：
一个产生式X，他的lookahead记为LX，是否可以认为，由他闭包构造的DFA节点的所有新增产生式，都只有两种情况：第一种是和LX有关的，第二种是LX无关的。在LR(0)中，我针对每一个符号构造了一个闭包，意味着遇到这个符号可以闭包展开哪些产生式；现在对于LR(k)，我可以同样保留这个设计，但每一个闭包项额外记录：产生它所携带的lookahead和他是否和传入的LX有关。
然后，我这样生成下一个lr节点时这样做：
继承上一个节点来的所有产生式和对应的lookahead，然后挪动点的位置；
构造一个map<token,lookahead>，将每个点后token的lookahead合并成LX（比如说两个产生式挪完点之后都是S，那么就把这两个产生式的lookahead合并成S对应的LX）；最后，调用闭包，直接把闭包产生的产生式全部加入这个项目集。这样是不是就可以了？
除此之外的一个问题是：你这个问题问得特别深入，已经不只是“LR(k) 的定义是什么”，而是在琢磨**怎么构造 LR(k) 项目集族**，以及怎么在程序里落地实现。等于是你想把 **LR(0) 那套“点移动+闭包”的节点构造流程**推广到带 lookahead 的版本。我们一点一点梳理。

---

## 1. 背景回顾：LR(0) 和 LR(1) 的差异

- **LR(0)** 项目：只存放“点”的位置：  
  \[(A -> α · β)\]  
  闭包时：如果点后面是非终结符 \(B\)，就把 \(B\) 的所有产生式 \((B -> · \gamma)\) 加进来。

- **LR(1)** 项目：除了点，还记录一个 **lookahead 符号**：  
  \[(A -> α · β, a)\]  
  闭包规则：如果点后面是非终结符 \(B\)，就把 \(B\) 的产生式全加进去，但要给它们分配正确的 lookahead 集合。这个集合是：  
  \[\text{First}_1(β a)\]  
  也就是说，点后串 \(β\) 跟已有的 lookahead 符号 \(a\) 合并推导，看未来 1 个符号可能是什么。

- **LR(k)**：完全类似，不过 lookahead 由单个符号变成了长度 \(\leq k\) 的串：  
  \[(A -> α · β, u),\quad u \in \Sigma^{\le k}\]  
  闭包时给新产生式分配的 lookahead 集合是：  
  \[\text{First}_k(β u)\]  
  这里的 Firstk 就是推导未来**最多 \(k\) 个前缀终结符串**。

---

## 2. 回答你设想的“继承+合并+闭包”法

你提的三个步骤：
1. **继承所有产生式及 lookahead，移动点。**
2. **收集所有点后的 token → 合并 lookahead 进一个 map[token → LX]。**
3. **对这个 map 做闭包，把对应的产生式加进来。**

→ 方向是对的！这就是 LR(k) 标准算法的程序员实践形态。不同的只是：
- 你手里要维护的不再是单一符号的 lookahead，而是「长度 ≤ k 的串」。
- lookahead 合并，不是简单 union，而是要通过 **Firstk** 函数来计算准确结果（避免多余分支）。

---

## 3. 关键点：改造闭包（closure）函数

### 原版(LR(0))闭包逻辑：
- 遇到点后符号是(B)：把(B->·γ)全部加进来，不管三七二十一。

### LR(1)/LR(k) 闭包逻辑（你需要改造的地方）：
- 遇到点后符号是(B)：  
  - 先取lookahead集合(u)（这个项目携带的）。  
  - 计算(Firstk(βu))，其中(β) 是点后紧随的串。  
  - 把产生式((B->·γ,v))加进来lookahead(v)=上面的集合。  

注意：“Firstk” 是下一个重点：  
- 定义：(Firstk(α))=从串(α) 出发能推导出来的**所有**终结符前缀，长度≤(k)。  
- 如果(α)能推出一些以ε开头的串，那lookahead也可能继续“吃”后续串。这个就是LR(1)中ε与Follow的结合点。

---

## 4. 改造你的流程图

所以你的“map<token,lookahead>”方案可以继续用，但要有个修正：

- **不是 token → lookahead**，而是 **非终结符 → lookahead 集合**。  
  因为点后面可能跟的是非终结符 (B)，此时闭包要去展开 (B)。而 lookahead 的集合就是 (Firstk(βu))。  
- 这个集合可能很大（尤其 (k>1) 时会爆炸），所以实际实现里要么压缩存储，要么乖乖回到 LR(1) 就收工（这就是为什么 LR(>1) 很少用）。

---

## 5. 总结成简单操作规约

**闭包 for LR(k):**
- 输入：项目集 (I)。  
- 初始化：把(I)拷贝到结果集(J)。  
- While 可添加新项目：  
  对于每个((A->α·Bβ,u)∈J)：  
  - 计算(L=Firstk(βu))。  
  - For 每个(B->γ)：  
    - 加入((B->·γ,v))对每个(v∈L)。  

→ 直到没有新项目。

---

## 6. 轻松一句话版

你说得对，**LR(k)闭包就是LR(0)闭包那套，只不过每次展开时“顺手带条尾巴”，尾巴是lookahead集合，而这条尾巴需要用Firstk算出来。**  
这样既能保留你原来“移动点+构造闭包”的风格，又能精确计算LR(k)的附加信息。  

---
*/	

		
		for (auto& it:temp_set) curr_node->unit_list_.push_back(*it);
		std::sort(curr_node->unit_list_.begin(),curr_node->unit_list_.end());



		auto [it,inserted]=lr_node_list_.insert(curr_node);
		if (!inserted) {
			delete curr_node;
			node_amount--;
			return *it;
		}
		std::unordered_map<_Tp,std::vector<int>> temp_map;
		for (int i=0;i<curr_node->unit_list_.size();i++) {
			if (curr_node->unit_list_[i].dot_<curr_node->unit_list_[i].right_ops_.size()) temp_map[curr_node->unit_list_[i].right_ops_[curr_node->unit_list_[i].dot_]].push_back(i);
		}
		for (auto& it:temp_map) curr_node->edges_[it.first]=generate_lr_node(curr_node,it.second,node_amount);
		return curr_node;
	}
	//FOR GLR change construct_table->GLR_BASIC AND MAKE class GLR { GLR_BASIC } to avoid invalid function using.
};

}

}

#endif