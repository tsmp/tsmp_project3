#pragma once

#ifndef LUABIND_BUILDING
#define USE_EASTL 1
#endif

#ifdef USE_EASTL

#include "EASTL/vector.h"
#include "EASTL/deque.h"
#include "EASTL/sort.h"
#include "EASTL/stack.h"
#include "EASTL/list.h"
#include "EASTL/set.h"
#include "EASTL/map.h"
#include "EASTL/string.h"

#include "EASTL/algorithm.h"

#endif // USE_EASTL


#ifdef _M_AMD64
#define M_DONTDEFERCLEAR_EXT
#endif

#define M_DONTDEFERCLEAR_EXT //. for mem-debug only

#ifdef M_NOSTDCONTAINERS_EXT

#define xr_list std::list
#define xr_deque std::deque
#define xr_stack std::stack
#define xr_set std::set
#define xr_multiset std::multiset
#define xr_map std::map
#define xr_multimap std::multimap
#define xr_string std::string

template <class T>
class xr_vector : public std::vector<T>
{
public:
	typedef size_t size_type;
	typedef T &reference;
	typedef const T &const_reference;

public:
	xr_vector() : std::vector<T>() {}
	xr_vector(size_t _count, const T &_value) : std::vector<T>(_count, _value) {}
	explicit xr_vector(size_t _count) : std::vector<T>(_count) {}
	void clear() { erase(begin(), end()); }
	void clear_and_free() { std::vector<T>::clear(); }
	void clear_not_free() { erase(begin(), end()); }
	ICF const_reference operator[](size_type _Pos) const
	{
		{
			VERIFY(_Pos < size());
		}
		return (*(begin() + _Pos));
	}
	ICF reference operator[](size_type _Pos)
	{
		{
			VERIFY(_Pos < size());
		}
		return (*(begin() + _Pos));
	}
};

template <>
class xr_vector<bool> : public std::vector<bool>
{
	typedef bool T;

public:
	xr_vector<T>() : std::vector<T>() {}
	xr_vector<T>(size_t _count, const T &_value) : std::vector<T>(_count, _value) {}
	explicit xr_vector<T>(size_t _count) : std::vector<T>(_count) {}
	u32 size() const { return (u32)std::vector<T>::size(); }
	void clear() { erase(begin(), end()); }
};

#else

template <typename T>
class AllocatorEA
{
public:
	using difference_type = ptrdiff_t;

	AllocatorEA(const char* pName = EASTL_NAME_VAL("custom allocator"))
	{
#if EASTL_NAME_ENABLED
		mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
	}

	~AllocatorEA() = default;
	AllocatorEA& operator=(const AllocatorEA& x) { return *this; }
	bool operator==(const AllocatorEA& x) { return true; }

	void* allocate(size_t n, int flags = 0)
	{
		return Memory.mem_alloc(n);
	}

	void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
	{
		R_ASSERT2(false, "aligned alloc not implemented");
		return nullptr;
	}

	void deallocate(void* p, size_t n)
	{
		Memory.mem_free(p);
	}

	const char* get_name() const
	{
#if EASTL_NAME_ENABLED
		return mpName;
#else
		return "custom allocator";
#endif
	}

	void set_name(const char* pName)
	{
#if EASTL_NAME_ENABLED
		mpName = pName;
#endif
	}

protected:
#if EASTL_NAME_ENABLED
	const char* mpName; // Debug name, used to track memory.
#endif
};

template <typename T>
class xalloc
{
public:
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using pointer = T *;
	using const_pointer = const T *;
	using reference = T &;
	using const_reference = const T &;
	using value_type = T;

	template <class Other>
	struct rebind
	{
		using other = xalloc<Other>;
	};

	pointer address(reference ref) const { return &ref; }
	const_pointer address(const_reference ref) const { return &ref; }

	xalloc() = default;
	xalloc(const xalloc<T> &) = default;

	template <class Other>
	xalloc(const xalloc<Other> &) {}

	template <class Other>
	xalloc &operator=(const xalloc<Other> &)
	{
		return *this;
	}
#pragma warning(push)
#pragma warning(disable : 4267)
	pointer allocate(const size_type n, const void *p = nullptr) const
	{
		return xr_alloc<T>(n);
	}
#pragma warning(pop)

	void deallocate(pointer p, const size_type) const
	{
		xr_free(p);
	}

	void deallocate(void *p, const size_type) const { xr_free(p); }

	void construct(pointer p, const T &_Val) { new (p) T(_Val); }

	void destroy(pointer p) { p->~T(); }

	size_type max_size() const
	{
		const auto count = std::numeric_limits<size_type>::max() / sizeof(T);
		return 0 < count ? count : 1;
	}
};

struct xr_allocator
{
	template <typename T>
	struct helper
	{
		typedef xalloc<T> result;
	};

	static void *alloc(const u32 &n) { return xr_malloc((u32)n); }
	template <typename T>
	static void dealloc(T *&p) { xr_free(p); }
};

template <class _Ty, class _Other>
inline bool operator==(const xalloc<_Ty> &, const xalloc<_Other> &) { return (true); }

template <class _Ty, class _Other>
inline bool operator!=(const xalloc<_Ty> &, const xalloc<_Other> &) { return (false); }

namespace std
{
	template <class _Tp1, class _Tp2>
	inline xalloc<_Tp2> &__stl_alloc_rebind(xalloc<_Tp1> &__a, const _Tp2 *) { return (xalloc<_Tp2> &)(__a); }
	template <class _Tp1, class _Tp2>
	inline xalloc<_Tp2> __stl_alloc_create(xalloc<_Tp1> &, const _Tp2 *) { return xalloc<_Tp2>(); }
}; // namespace std

// vector

#ifdef USE_EASTL

template <typename T, typename allocator = AllocatorEA<T>>
class xr_vector : public eastl::vector<T, allocator>
{
private:
	typedef eastl::vector<T, allocator> inherited;

#else

template <typename T, typename allocator = xalloc<T>>
class xr_vector : public std::vector<T, allocator>
{
private:
	typedef std::vector<T, allocator> inherited;

#endif	

public:
	typedef allocator allocator_type;

public:
	xr_vector() : inherited() {}
	xr_vector(size_t _count, const T &_value) : inherited(_count, _value) {}
	explicit xr_vector(size_t _count) : inherited(_count) {}
	u32 size() const { return xr_narrow_cast<u32>(inherited::size()); }

	void clear_and_free() { inherited::clear(); }
	void clear_not_free() { erase(begin(), end()); }

	void clear_and_reserve()
	{
		if (capacity() <= (size() + size() / 4))
			clear_not_free();
		else
		{
			u32 old = size();
			clear_and_free();
			reserve(old);
		}
	}

#ifdef M_DONTDEFERCLEAR_EXT
	void clear()
	{
		clear_and_free();
	}
#else
	void clear()
	{
		clear_not_free();
	}
#endif

	const_reference operator[](size_type _Pos) const
	{
		{
			VERIFY(_Pos < size());
		}
		return (*(begin() + _Pos));
	}
	reference operator[](size_type _Pos)
	{
		{
			VERIFY(_Pos < size());
		}
		return (*(begin() + _Pos));
	}
};

// vector<bool>

#ifdef USE_EASTL

template <>
class xr_vector<bool, AllocatorEA<bool>> : public eastl::vector<bool, AllocatorEA<bool>>
{
private:
	typedef eastl::vector<bool, AllocatorEA<bool>> inherited;

#else

template <>
class xr_vector<bool, xalloc<bool>> : public std::vector<bool, xalloc<bool>>
{
private:
	typedef std::vector<bool, xalloc<bool>> inherited;

#endif

public:
	u32 size() const { return xr_narrow_cast<u32>(inherited::size()); }
	void clear() { erase(begin(), end()); }
};

template <typename allocator>
class xr_vector<bool, allocator> : 
#ifdef USE_EASTL
	public eastl::vector<bool, allocator>
#else
	public std::vector<bool, allocator>
#endif
{
private:
#ifdef USE_EASTL
	typedef eastl::vector<bool, allocator> inherited;
#else
	typedef std::vector<bool, allocator> inherited;
#endif

public:
	u32 size() const { return xr_narrow_cast<u32>(inherited::size()); }
	void clear() { erase(begin(), end()); }
};

#ifdef USE_EASTL

// deque
template <typename T, typename allocator = AllocatorEA<T>>
using xr_deque = eastl::deque<T, allocator>;

// stack
template <typename T, class C = xr_deque<T>>
using xr_stack = eastl::stack<T, C>;

// list
template <typename T, typename allocator = AllocatorEA<T>>
using xr_list = eastl::list<T, allocator>;

// set
template <typename K, class P = eastl::less<K>, typename allocator = AllocatorEA<K>>
using xr_set = eastl::set<K, P, allocator>;

// multiset
template <typename K, class P = eastl::less<K>, typename allocator = AllocatorEA<K>>
using xr_multiset = eastl::multiset<K, P, allocator>;

// map
template <typename K, class V, class P = eastl::less<K>, typename allocator = AllocatorEA<K>>
using xr_map = eastl::map<K, V, P, allocator>;

// multimap
template <typename K, class V, class P = eastl::less<K>, typename allocator = AllocatorEA<K>>
using xr_multimap = eastl::multimap<K, V, P, allocator>;

// string(char)
typedef eastl::basic_string<char, AllocatorEA<char>> xr_string;

#else

// deque
template <typename T, typename allocator = xalloc<T>>
using xr_deque = std::deque<T, allocator>;

// stack
template <typename T, class C = xr_deque<T>>
using xr_stack = std::stack<T, C>;

// list
template <typename T, typename allocator = xalloc<T>>
using xr_list = std::list<T, allocator>;

// set
template <typename K, class P = std::less<K>, typename allocator = xalloc<K>>
using xr_set = std::set<K, P, allocator>;

// multiset
template <typename K, class P = std::less<K>, typename allocator = xalloc<K>>
using xr_multiset = std::multiset<K, P, allocator>;

// map
template <typename K, class V, class P = std::less<K>, typename allocator = xalloc<std::pair<const K, V>>>
using xr_map = std::map<K, V, P, allocator>;

// multimap
template <typename K, class V, class P = std::less<K>, typename allocator = xalloc<std::pair<const K, V>>>
using xr_multimap = std::multimap<K, V, P, allocator>;

// string(char)
typedef std::basic_string<char, std::char_traits<char>, xalloc<char>> xr_string;

#endif // USE_EASTL

#endif // M_NOSTDCONTAINERS_EXT

#ifdef USE_EASTL
#define mk_pair eastl::make_pair
using eastl::swap;
using eastl::pair;
using eastl::sort;
using eastl::find;
using eastl::reverse;
using eastl::remove_if;
using eastl::remove;
using eastl::find_if;
using eastl::count_if;
using eastl::stable_sort;
using eastl::lower_bound;
using eastl::upper_bound;
using eastl::unique;
using eastl::set_difference;
using eastl::max;
using eastl::min;
using eastl::push_heap;
using eastl::pop_heap;
using eastl::advance;
using eastl::distance;
using eastl::copy;
using eastl::less;
using eastl::greater;
using eastl::to_string;
using eastl::for_each;
using eastl::fill_n;
using eastl::fill;
using eastl::lexicographical_compare;
using eastl::binary_search;
using eastl::max_element;
using eastl::min_element;
#else
#define mk_pair std::make_pair
using std::swap;
using std::pair;
using std::sort;
using std::find;
using std::reverse;
using std::remove_if;
using std::remove;
using std::find_if;
using std::count_if;
using std::stable_sort;
using std::lower_bound;
using std::upper_bound;
using std::unique;
using std::set_difference;
using std::max;
using std::min;
using std::push_heap;
using std::pop_heap;
using std::advance;
using std::distance;
using std::copy;
using std::less;
using std::greater;
using std::to_string;
using std::for_each;
using std::fill_n;
using std::fill;
using std::lexicographical_compare;
using std::binary_search;
using std::max_element;
using std::min_element;
#endif

struct pred_str
{
	IC bool operator()(const char* x, const char* y) const { return xr_strcmp(x, y) < 0; }
};

struct pred_stri
{
	IC bool operator()(const char* x, const char* y) const { return stricmp(x, y) < 0; }
};

// STL extensions
#define DEF_VECTOR(N, T)    \
	typedef xr_vector<T> N; \
	typedef N::iterator N##_it;
#define DEF_LIST(N, T)    \
	typedef xr_list<T> N; \
	typedef N::iterator N##_it;
#define DEF_DEQUE(N, T)    \
	typedef xr_deque<T> N; \
	typedef N::iterator N##_it;
#define DEF_MAP(N, K, T)    \
	typedef xr_map<K, T> N; \
	typedef N::iterator N##_it;

#define DEFINE_DEQUE(T, N, I) \
	typedef xr_deque<T> N;    \
	typedef N::iterator I;
#define DEFINE_LIST(T, N, I) \
	typedef xr_list<T> N;    \
	typedef N::iterator I;
#define DEFINE_VECTOR(T, N, I) \
	typedef xr_vector<T> N;    \
	typedef N::iterator I;
#define DEFINE_MAP(K, T, N, I) \
	typedef xr_map<K, T> N;    \
	typedef N::iterator I;
#define DEFINE_MAP_PRED(K, T, N, I, P) \
	typedef xr_map<K, T, P> N;         \
	typedef N::iterator I;
#define DEFINE_MMAP(K, T, N, I)  \
	typedef xr_multimap<K, T> N; \
	typedef N::iterator I;
#define DEFINE_SVECTOR(T, C, N, I) \
	typedef svector<T, C> N;       \
	typedef N::iterator I;
#define DEFINE_SET(T, N, I) \
	typedef xr_set<T> N;    \
	typedef N::iterator I;
#define DEFINE_SET_PRED(T, N, I, P) \
	typedef xr_set<T, P> N;         \
	typedef N::iterator I;
#define DEFINE_STACK(T, N) typedef xr_stack<T> N;

#include "FixedVector.h"

// auxilary definition
DEFINE_VECTOR(bool, boolVec, boolIt);
DEFINE_VECTOR(BOOL, BOOLVec, BOOLIt);
DEFINE_VECTOR(BOOL *, LPBOOLVec, LPBOOLIt);
DEFINE_VECTOR(Frect, FrectVec, FrectIt);
DEFINE_VECTOR(Irect, IrectVec, IrectIt);
DEFINE_VECTOR(Fplane, PlaneVec, PlaneIt);
DEFINE_VECTOR(Fvector2, Fvector2Vec, Fvector2It);
DEFINE_VECTOR(Fvector, FvectorVec, FvectorIt);
DEFINE_VECTOR(Fvector *, LPFvectorVec, LPFvectorIt);
DEFINE_VECTOR(Fcolor, FcolorVec, FcolorIt);
DEFINE_VECTOR(Fcolor *, LPFcolorVec, LPFcolorIt);
DEFINE_VECTOR(LPSTR, LPSTRVec, LPSTRIt);
DEFINE_VECTOR(LPCSTR, LPCSTRVec, LPCSTRIt);
//DEFINE_VECTOR(string64,string64Vec,string64It);
DEFINE_VECTOR(xr_string, SStringVec, SStringVecIt);

DEFINE_VECTOR(s8, S8Vec, S8It);
DEFINE_VECTOR(s8 *, LPS8Vec, LPS8It);
DEFINE_VECTOR(s16, S16Vec, S16It);
DEFINE_VECTOR(s16 *, LPS16Vec, LPS16It);
DEFINE_VECTOR(s32, S32Vec, S32It);
DEFINE_VECTOR(s32 *, LPS32Vec, LPS32It);
DEFINE_VECTOR(u8, U8Vec, U8It);
DEFINE_VECTOR(u8 *, LPU8Vec, LPU8It);
DEFINE_VECTOR(u16, U16Vec, U16It);
DEFINE_VECTOR(u16 *, LPU16Vec, LPU16It);
DEFINE_VECTOR(u32, U32Vec, U32It);
DEFINE_VECTOR(u32 *, LPU32Vec, LPU32It);
DEFINE_VECTOR(float, FloatVec, FloatIt);
DEFINE_VECTOR(float *, LPFloatVec, LPFloatIt);
DEFINE_VECTOR(int, IntVec, IntIt);
DEFINE_VECTOR(int *, LPIntVec, LPIntIt);
