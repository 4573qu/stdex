#ifndef _GLIBCXX_DISJOINTSET
#define _GLIBCXX_DISJOINTSET 1

#pragma GCC system_header

#include <bits/stl_algobase.h>
#if __cplusplus > 201703L
#  include <bits/stl_algo.h> // For remove and remove_if
#endif // C++20
#include <bits/allocator.h>
#include <bits/stl_construct.h>
#include <bits/stl_uninitialized.h>
#include <bits/stl_disjoint_set.h>
#include <bits/range_access.h>

#ifndef _GLIBCXX_EXPORT_TEMPLATE
# include <bits/disjoint_set.tcc>
#endif

#ifdef _GLIBCXX_DEBUG
# include <debug/disjoint_set>
#endif

#ifdef _GLIBCXX_PROFILE
# include <profile/disjoint_set>
#endif

#if __cplusplus >= 201703L
namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
  namespace pmr {
    template<typename _Tp> class polymorphic_allocator;
    template<typename _Tp>
      using disjoint_set = std::disjoint_set<_Tp, polymorphic_allocator<_Tp>>;
  } // namespace pmr
# ifdef _GLIBCXX_DEBUG
  namespace _GLIBCXX_STD_C::pmr {
    template<typename _Tp>
      using disjoint_set
	= _GLIBCXX_STD_C::disjoint_set<_Tp, std::pmr::polymorphic_allocator<_Tp>>;
  } // namespace _GLIBCXX_STD_C::pmr
# endif
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std
#endif // C++17

#if __cplusplus > 201703L
namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

#define __cpp_lib_erase_if 201900L

  template<typename _Tp, typename _Alloc, typename _Predicate>
    inline typename disjoint_set<_Tp, _Alloc>::size_type
    erase_if(disjoint_set<_Tp, _Alloc>& __cont, _Predicate __pred)
    {
      const auto __osz = __cont.size();
      __cont.erase(std::remove_if(__cont.begin(), __cont.end(), __pred),
		   __cont.end());
      return __osz - __cont.size();
    }

  template<typename _Tp, typename _Alloc, typename _Up>
    inline typename disjoint_set<_Tp, _Alloc>::size_type
    erase(disjoint_set<_Tp, _Alloc>& __cont, const _Up& __value)
    {
      const auto __osz = __cont.size();
      __cont.erase(std::remove(__cont.begin(), __cont.end(), __value),
		   __cont.end());
      return __osz - __cont.size();
    }
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std
#endif // C++20

#endif /* _GLIBCXX_DISJOINTSET */