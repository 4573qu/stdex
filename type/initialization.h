//Last Modified At 2025/10/31
//@Version 1.2.0.0
#ifndef _STDEX_TYPE_INITIALIZATION_H_
#define _STDEX_TYPE_INITIALIZATION_H_ 1

#include <fstream>
#include <iostream>
#include <sstream>

namespace stdex {

namespace type {

class initialization {
public:
	using section=std::map<std::string,std::string>;

private:
	std::map<std::string,section> sections_;

	void process_line(std::string_view line,std::string& current_section) {
		line = trim(line);
		if (line.empty() || line[0]==';' || line[0]=='#') return;
		if (line.front()=='[' && line.back()==']') {
			current_section=std::string(line.substr(1,line.size()-2));
			return;
		}
		if (auto eq_pos=line.find('=');eq_pos!=std::string_view::npos) {
			auto key=trim(line.substr(0,eq_pos));
			auto val=trim(line.substr(eq_pos+1));
			if (!current_section.empty() && !key.empty()) sections_[current_section][std::string(key)]=std::string(val);
		}
	}
	static std::string_view trim(std::string_view str) {
		auto start=str.find_first_not_of(" \t");
		auto end=str.find_last_not_of(" \t");
		return (start!=std::string_view::npos)?str.substr(start,end-start+1):"";
	}

public:
	bool load(std::ifstream& is) {
		if (!is.is_open()) return false;
		std::string aContent((std::istreambuf_iterator<char>(is)),std::istreambuf_iterator<char>());
		std::istringstream iss(aContent);
		if (!iss.good()) return false;
		sections_.clear();
		std::string curr_section;
		std::string line;
		while (std::getline(iss,line)) process_line(line,curr_section);
		return true;
	}
	bool save(std::ofstream& os) const {
		if (!os.is_open()) return false;
		std::ostringstream oss;
		for (const auto& [section_name,kv_pairs]:sections_) {
			oss<<"["<<section_name<<"]\n";
			for (const auto& [key,value]:kv_pairs) oss<<key<<"="<<value<<"\n";
			oss<<"\n";
		}
		os.write(oss.str().c_str(),oss.str().size());
		return true;
	}
	std::optional<std::string> get(const std::string& section,const std::string& key) const {
		if (auto it=sections_.find(section);it!=sections_.end()) {
			if (auto kv_it=it->second.find(key);kv_it!=it->second.end()) return kv_it->second;
		}
		return std::nullopt;
	}
	template <typename _Tp>
	std::optional<_Tp> get_as(const std::string& section,const std::string& key) const {
		if (auto val=get(section,key)) {
			_Tp result;
			std::istringstream iss(*val);
			if (iss>>result) return result;
		}
		return std::nullopt;
	}
	template <typename _Tp>
	bool get(const std::string& section,const std::string& key,_Tp& value) const {
		if (auto val=get(section,key)) {
			std::istringstream iss(*val);
			if (iss>>value) return true;
		}
		return false;
	}
	void set(const std::string& section,const std::string& key,std::string value) {
		sections_[section][key]=std::move(value);
	}
	template <typename _Tp>
	void set(const std::string& section,const std::string& key,_Tp value) {
		std::ostringstream oss;
		oss<<value;
		set(section,key,oss.str());
	}
	void clear(const std::string& section) {
		sections_.erase(section);
	}
	void clear(const std::string& section,const std::string& key) {
		sections_[section].erase(key);
	}
};

using ini=initialization;

}

}

#endif