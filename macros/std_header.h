//Last Modified At 2025/10/10
//@Version 1.0.0.0
#ifndef _STDEX_MACROS_STD_HEADER_H_
#define _STDEX_UTILITY_MATCH_H_ 1

namespace stdex {
	namespace algorithm { }
	namespace bitwise {	}
	namespace machine {
		namespace assembler { }
	}
	namespace math { }
	namespace memory { }
	namespace meta {
		namespace core { }
	}
	namespace perf { }
	namespace profiling { }
	namespace structure { }
	namespace syntax { }
	namespace type { }
	namespace utility {	}
	namespace vision { }
}

#ifdef _STDEX_HEADER_NAME
namespace _STDEX_HEADER_NAME {
#else
namespace std {
#endif

	using namespace stdex::algorithm;
	using namespace stdex::bitwise;
	using namespace stdex::machine;
	using namespace stdex::machine::assembler;
	using namespace stdex::math;
	using namespace stdex::memory;
	using namespace stdex::meta::core;
	using namespace stdex::perf;
	using namespace stdex::profiling;
	using namespace stdex::structure;
	using namespace stdex::syntax;
	using namespace stdex::type;
	using namespace stdex::utility;
	using namespace stdex::vision;

}

#endif