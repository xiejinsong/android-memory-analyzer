#include <tr1/unordered_map>
#include "dlmalloc.h"
#include "HeapInfo.h"
#include "ChunkInfo.h"


template <class _Tp>
class SpecialAllocator
{
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef _Tp*       pointer;
    typedef const _Tp* const_pointer;
    typedef _Tp&       reference;
    typedef const _Tp& const_reference;
    typedef _Tp        value_type;

    template<typename _Tp1>
        struct rebind
        { typedef SpecialAllocator<_Tp1> other; };

    SpecialAllocator() throw() { }

    SpecialAllocator(const SpecialAllocator&) throw() { }

    template<typename _Tp1>
        SpecialAllocator(const SpecialAllocator<_Tp1>&) throw() { }

    ~SpecialAllocator() throw() { }

    pointer
        address(reference __x) const { return &__x; }

    const_pointer
        address(const_reference __x) const { return &__x; }

    // NB: __n is permitted to be 0.  The C++ standard says nothing
    // about what the return value is when __n == 0.
    pointer
        allocate(size_type __n, const void* = 0);

    // __p is not permitted to be a null pointer.
    void
        deallocate(pointer __p, size_type);
        

    size_type
        max_size() const throw() 
        { return size_t(-1) / sizeof(_Tp); }

    // _GLIBCXX_RESOLVE_LIB_DEFECTS
    // 402. wrong new expression in [some_] allocator::construct
    // void 
    //     construct(pointer __p, const _Tp& __val);

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    template<typename... _Args>
        void
        construct(pointer __p, _Args&&... __args);
#endif

    void 
        destroy(pointer __p) { __p->~_Tp(); }
};


typedef std::tr1::unordered_map<void* /*the chunk pointer*/ 
,ChunkInfo /* the meaningful value */
,std::tr1::hash<void *>
,std::equal_to<void *>
,SpecialAllocator<std::pair<const void*,ChunkInfo> >
> HeapInfoMap;

struct HeapInfo::HeapInfoImpl
{
    mspace m_data;
    HeapInfoMap m_infoMap; 
};
template <class _Tp>
typename SpecialAllocator<_Tp>::pointer
SpecialAllocator<_Tp>::allocate(size_type __n, const void* )
{ 
    if (__builtin_expect(__n > this->max_size(), false))
        std::__throw_bad_alloc();

    return static_cast<_Tp*>(mspace_malloc(HeapInfo::m_impl->m_data,__n * sizeof(_Tp)));
}

template <class _Tp>
    void
        SpecialAllocator<_Tp>::deallocate(typename SpecialAllocator<_Tp>::pointer __p, size_type)
        { mspace_free(HeapInfo::m_impl->m_data,__p); }
#ifdef __GXX_EXPERIMENTAL_CXX0X__
template<class _Tp>
template<typename... _Args>
void
SpecialAllocator<_Tp>::construct(SpecialAllocator<_Tp>::pointer __p, _Args&&... __args)
{ ::new((void *)__p) _Tp(std::forward<_Args>(__args)...); }
#endif

HeapInfo::HeapInfoImpl * HeapInfo::m_impl = NULL;



void HeapInfo::init(int dataSize)
{
    mspace space = create_mspace(dataSize,1); 
    void * storage = mspace_malloc(space,sizeof(HeapInfoImpl));
    m_impl = new (storage)HeapInfoImpl; 
    m_impl->m_data = space;
}


void HeapInfo::registerChunkInfo(const void * dataPointer,ChunkInfo const &info)
{
    m_impl->m_infoMap.insert(std::pair<void *,ChunkInfo>((void*)dataPointer,info));
}

void HeapInfo::unregisterChunkInfo(const void *dataPointer)
{
    m_impl->m_infoMap.erase((void*)dataPointer);
}

ChunkInfo  const * HeapInfo::getChunkInfo(const void *dataPointer)
{
    HeapInfoMap::iterator i = m_impl->m_infoMap.find((void*)dataPointer);
    if(i == m_impl->m_infoMap.end())
    {
        return NULL;
    }
    return &i->second;
}

