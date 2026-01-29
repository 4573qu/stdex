//Last Modified At 2026/01/28
//@Version 1.2.0.0
#ifndef _STDEX_UTILITY_VERSION_H_
#define _STDEX_UTILITY_VERSION_H_ 1

#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

namespace stdex {

namespace utility {

template <typename _Tp=uint32_t>
struct version {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");

	_Tp major;
	_Tp minor;
	_Tp patch;
	_Tp build;
	version()=default;
	version(_Tp major, uint32_t minor, uint32_t patch, uint32_t build) : major(major) , minor(minor) , patch(patch) , build(build) { }
	std::string to_string() {
		constexpr double log10_2=0.30102999566398119521373889472449;
		constexpr int digits=static_cast<int>(sizeof(_Tp)*CHAR_BIT*log10_2)+2;
		char result[digits*4];
		snprintf(result,sizeof(result),"%d.%02d.%03d.%03d",major,minor,patch,build);
		return std::string(result);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true) const {
		if (initialize) major=minor=patch=build=0;
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
		auto safe_convert=[&length](const std::string& s,bool& success)->uint32_t{
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
		major=safe_convert(parts[0],success);
		length++;
		minor=safe_convert(parts[1],success);
		length++;
		patch=safe_convert(parts[2],success);
		length++;
		build=safe_convert(parts[3],success);
		if (!success) return 0;
		return length;
	}
	bool operator ==(const version& other) const {
		if (major!=other.major) return false;
		if (minor!=other.minor) return false;
		if (patch!=other.patch) return false;
		return build==other.build;
	}
	bool operator !=(const version& other) const {
		return !(*this==other);
	}
	bool operator <(const version& other) const {
		if (major!=other.major) return major<other.major;
		if (minor!=other.minor) return minor<other.minor;
		if (patch!=other.patch) return patch<other.patch;
		return build<other.build;
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