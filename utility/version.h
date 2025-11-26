//Last Modified At 2025/11/26
//@Version 1.0.0.0
#ifndef _STDEX_UTILITY_VERSION_H_
#define _STDEX_UTILITY_VERSION_H_ 1

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace stdex {

namespace utility {

struct version {
	uint32_t major_;
	uint32_t minor_;
	uint32_t patch_;
	uint32_t build_;
	std::string to_string() {
		char result[40];
		snprintf(result,sizeof(result),"%d.%02d.%03d.%03d",major_,minor_,patch_,build_);
		return std::string(result);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true) {
		if (initialize) major_=minor_=patch_=build_=0;
		std::vector<std::string> parts;
		std::size_t start=0;
		std::size_t end=s.find('.');
		while (end!=std::string::npos) {
			parts.push_back(s.substr(start,end-start));
			start=end+1;
			end=s.find('.',start);
		}
		parts.push_back(s.substr(start));
		if (strict) {
			if (parts.size()!=4) return 0;
			if (parts[0].empty() || parts[0].size()<1 || parts[1].size()<2 || parts[2].size()<3 || parts[3].size()<3) return 0;
		} else {
			if (parts.size()<4) return 0;
		}
		for (const auto& it:parts) {
			if (it.empty() || !all_of(it.begin(),it.end(),[](int8_t num){
				return std::isdigit(num);
			})) return 0;
		}
		std::size_t length=0;
		auto safe_convert=[&length](const std::string& s,bool& success)->uint32_t {
			if (s.empty()) return 0;
			char* end;
			errno=0;
			unsigned long long val=std::strtoull(s.c_str(),&end,10);
			if (*end!='\0' || errno==ERANGE || val>UINT32_MAX) {
				success=false;
				return 0;
			}
			length+=s.size();
			return static_cast<uint32_t>(val);
		};
		bool success=true;
		major_=safe_convert(parts[0],success);
		length++;
		minor_=safe_convert(parts[1],success);
		length++;
		patch_=safe_convert(parts[2],success);
		length++;
		build_=safe_convert(parts[3],success);
		if (!success) return 0;
		return length;
	}
	bool operator ==(const version& other) const {
		if (major_!=other.major_) return false;
		if (minor_!=other.minor_) return false;
		if (patch_!=other.patch_) return false;
		return build_==other.build_;
	}
	bool operator !=(const version& other) const {
		return !(*this == other);
	}
	bool operator <(const version& other) const {
		if (major_!=other.major_) return major_<other.major_;
		if (minor_!=other.minor_) return minor_<other.minor_;
		if (patch_!=other.patch_) return patch_<other.patch_;
		return build_<other.build_;
	}
	bool operator >(const version& other) const {
		return (other<*this);
	}
	bool operator <=(const version& other) const {
		return !(other<*this);
	}
	bool operator >=(const version& other) const {
		return !(*this<other);
	}
};

}

}

#endif