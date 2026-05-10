//Last Modified At 2026/05/10
//@Version 1.0.1.0
#ifndef _STDEX_STRUCTURE_GRAPH_ALGORITHM_H_
#define _STDEX_STRUCTURE_GRAPH_ALGORITHM_H_ 1

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace stdex {

namespace algorithm {

namespace graph {

template <typename _Tp,typename _Graph=std::unordered_map<_Tp,std::unordered_set<_Tp>>>
bool find(_Graph graph,_Tp node) {
	if constexpr (std::is_same_v<decltype(graph.find(node)),bool>) {
		return graph.find(node);
	} else {
		return graph.find(node)!=graph.end();
	}
}

template <typename,typename=void>
struct is_range : std::false_type {};

template <typename _Tp>
struct is_range<_Tp,std::void_t<decltype(std::begin(std::declval<_Tp&>())),decltype(std::end(std::declval<_Tp&>()))>> : std::true_type {};

template <typename _Tp>
constexpr bool is_range_v=is_range<_Tp>::value;

template <typename _Graph>
decltype(auto) get(_Graph& graph) {
	if constexpr (is_range_v<_Graph>) {
		return graph;
	} else {
		return graph.get();
	}
}

template <typename _Tp,typename _Graph=std::unordered_map<_Tp,std::unordered_set<_Tp>>>
void kosaraju_dfs1(_Tp node,const _Graph& graph,std::unordered_set<_Tp> &visited,std::vector<_Tp> &order) {
	visited.insert(node);
	if (find(graph,node)) {
		for (auto &it:graph.at(node)) {
			if (!visited.count(it)) kosaraju_dfs1(it,graph,visited,order);
		}
	}
	order.push_back(node);
}

template <typename _Tp,typename _Graph=std::unordered_map<_Tp,std::unordered_set<_Tp>>>
void kosaraju_dfs2(_Tp node,const _Graph& rev_graph,std::unordered_set<_Tp> &visited,std::unordered_set<_Tp> &scc) {
 	visited.insert(node);
	scc.insert(node);
	if (find(rev_graph,node)) {
		for (auto &it:rev_graph.at(node)) {
 			if (!visited.count(it)) kosaraju_dfs2(it,rev_graph,visited,scc);
		}
	}
}

}

}

template <typename _Tp,typename _Graph=std::unordered_map<_Tp,std::unordered_set<_Tp>>>
std::vector<std::unordered_set<_Tp>> kosaraju(const _Graph& graph) {
	std::unordered_set<_Tp> all_nodes;
	for (auto &it:algorithm::graph::get(graph)) {
		all_nodes.insert(it.first);
		for (auto &jt:it.second) all_nodes.insert(jt);
	}
	std::unordered_map<_Tp,std::unordered_set<_Tp>> rev_graph;
	for (auto &it:algorithm::graph::get(graph)) {
		for (auto &jt:it.second) rev_graph[jt].insert(it.first);
	}
	std::unordered_set<_Tp> visited1;
	std::vector<_Tp> finish_order;
	for (auto &it:all_nodes) {
		if (!visited1.count(it)) algorithm::graph::kosaraju_dfs1<_Tp,_Graph>(it,graph,visited1,finish_order);
	}
	std::unordered_set<_Tp> visited2;
	std::vector<std::unordered_set<_Tp>> sccs;
	for (auto it=finish_order.rbegin();it!=finish_order.rend();it++) {
		if (!visited2.count(*it)) {
			std::unordered_set<_Tp> scc;
			algorithm::graph::kosaraju_dfs2<_Tp,_Graph>(*it,rev_graph,visited2,scc);
  			sccs.push_back(std::move(scc));
		}
	}
	return sccs;
}

template<typename _Tp,typename _Graph=std::unordered_map<_Tp,std::unordered_set<_Tp>>>
std::vector<std::vector<std::unordered_set<_Tp>>> kosaraju_plus(const _Graph& graph) {
	auto sccs=kosaraju<_Tp,_Graph>(graph);
	int n=sccs.size();
	std::unordered_map<_Tp,int> node2scc;
	for (int i=0;i<n;i++) {
		for (auto &it:sccs[i]) node2scc[it] = i;
	}
	std::vector<std::vector<int>> dag(n);
	for (auto &it:algorithm::graph::get(graph)) {
		for (auto &jt:it.second) {
			int u=node2scc[it.first];
			int v=node2scc[jt];
			if (u!=v) {
				dag[u].push_back(v);
				dag[v].push_back(u);
			}
		}
	}
	std::vector<int> comp_id(n,-1);
	int comp_cnt=0;
	for (int i=0;i<n;i++) {
		if (comp_id[i]==-1) {
			std::vector<int> stack={i};
			comp_id[i]=comp_cnt;
			for (std::size_t k=0;k<stack.size();k++) {
				int u=stack[k];
				for (int it:dag[u]) {
					if (comp_id[it]==-1) {
						comp_id[it]=comp_cnt;
						stack.push_back(it);
					}
				}
			}
			comp_cnt++;
		}
	}
	std::vector<std::vector<std::unordered_set<_Tp>>> components(comp_cnt);
	for (int i=0;i<n;i++) components[comp_id[i]].push_back(sccs[i]);
	return components;
}

template <typename _Tp,typename _Func,typename _Graph=std::unordered_map<_Tp,std::unordered_set<_Tp>>>
std::unordered_map<_Tp,std::unordered_set<_Tp>> inverse_topology_closure(const _Graph& graph,_Func func) {
	auto sccs=kosaraju<_Tp,_Graph>(graph);
	int n=sccs.size();
	std::unordered_map<_Tp,int> node2scc;
	for (int i=0;i<n;i++) {
		for (auto& it:sccs[i]) node2scc[it]=i;
	}
	std::vector<std::unordered_set<int>> dag(n);
	for (auto &it:algorithm::graph::get(graph)) {
		for (auto &jt:it.second) {
			int u=node2scc[it.first];
			int v=node2scc[jt];
			if (u!=v) dag[u].insert(v);
		}
	}
	std::vector<std::unordered_set<_Tp>> scc_result(n);
	for (int i=n-1;i>=0;i--) {
		auto& comp=sccs[i];
		auto& acc=scc_result[i];
		for (auto& it:comp) {
			auto elems=func(it);
			acc.insert(elems.begin(),elems.end());
		}
		for (int it:dag[i]) acc.insert(scc_result[it].begin(),scc_result[it].end());
	}
	std::unordered_map<_Tp,std::unordered_set<_Tp>> result;
	for (int i=0;i<n;i++) {
		for (auto& it:sccs[i]) result[it]=scc_result[i];
	}
	return result;
}

template <typename _Tp,typename _Graph=std::unordered_map<_Tp,std::unordered_set<_Tp>>>
struct tarjan {
	const _Graph &graph;
	std::unordered_set<_Tp> nodes;
	std::unordered_map<_Tp,int> index_map;
	std::unordered_map<_Tp,int> lowlink;
	std::stack<_Tp> stack;
	std::unordered_set<_Tp> on_stack;
	int index_counter;
	std::vector<std::unordered_set<_Tp>> sccs;

	tarjan(const _Graph &graph) : graph(graph) { }

	void strong_connect(_Tp v) {
		index_map[v]=index_counter;
		lowlink[v]=index_counter++;
		stack.push(v);
		on_stack.insert(v);
		if (algorithm::graph::find(graph,v)) {
			for (auto &it:algorithm::graph::get(graph,v)) {
				if (index_map.find(it)==index_map.end()) {
					strong_connect(it);
					lowlink[v]=std::min(lowlink[v],lowlink[it]);
				} else if (on_stack.count(it)) lowlink[v]=std::min(lowlink[v],index_map[it]);
			}
		}
		if (lowlink[v]==index_map[v]) {
			std::unordered_set<_Tp> scc;
			_Tp w;
			do {
				w=stack.top();
				stack.pop();
				on_stack.erase(w);
				scc.insert(w);
			} while (w!=v);
			sccs.push_back(scc);
		}
	}

	std::vector<std::unordered_set<_Tp>> run() {
		sccs.clear();
		for (auto &it:algorithm::graph::get(graph)) {
			nodes.insert(it.first);
			for (auto &jt:it.second) nodes.insert(jt);
		}
		index_counter=0;
		for (auto &it:nodes) {
			if (index_map.find(it)==index_map.end()) strong_connect(it);
		}
		return sccs;
	}
};

}

#endif