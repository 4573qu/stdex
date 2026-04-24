//Last Modified At 2026/04/23
//@Version 1.2.0.0

#include <type_traits>

namespace stdex {
	namespace algorithm { }
	namespace bitwise { }
	namespace container { }
	namespace crypto { }
	namespace integrity { }
	namespace machine {
		namespace assembler { }
	}
	namespace math { }
	namespace memory { }
	namespace meta {
		namespace core { }
	}
	namespace nlp { }
	namespace perf { }
	namespace profiling { }
	namespace structure { }
	namespace syntax { }
	namespace type { }
	namespace utility { }
	namespace vision { }
}

#ifdef _STDEX_HEADER_NAME
namespace _STDEX_HEADER_NAME {
#else
namespace std {
#endif

#define _STDEX_SAFE_ALIAS(ALIAS) \
using namespace stdex::##ALIAS;

	_STDEX_SAFE_ALIAS(algorithm)
	_STDEX_SAFE_ALIAS(bitwise)
	_STDEX_SAFE_ALIAS(container)
	_STDEX_SAFE_ALIAS(crypto)
	_STDEX_SAFE_ALIAS(integrity)
	_STDEX_SAFE_ALIAS(machine)
	_STDEX_SAFE_ALIAS(machine::assembler)
	_STDEX_SAFE_ALIAS(math)
	_STDEX_SAFE_ALIAS(memory)
	_STDEX_SAFE_ALIAS(meta::core)
	_STDEX_SAFE_ALIAS(nlp)
	_STDEX_SAFE_ALIAS(perf)
	_STDEX_SAFE_ALIAS(profiling)
	_STDEX_SAFE_ALIAS(structure)
	_STDEX_SAFE_ALIAS(syntax)
	_STDEX_SAFE_ALIAS(type)
	_STDEX_SAFE_ALIAS(utility)
	_STDEX_SAFE_ALIAS(vision)

#undef _STDEX_SAFE_ALIAS

}