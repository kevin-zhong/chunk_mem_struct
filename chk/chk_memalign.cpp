#include "chk_memalign.h"
#include "chk_base.h"

namespace  comm { namespace  chk
{

static void* _allocate_func(size_t size)
{
        assert(size <= CHK_MAX_CHKSIZE);
        void* p = ChkMemalign::instance()->alloc();
        if (p)
                bzero(p, CHK_MAX_CHKSIZE);
        return p;
}

static void  _dellocate_func(void* ptr, size_t size)
{
        assert(size <= CHK_MAX_CHKSIZE);
        ChkMemalign::instance()->dellocate(ptr);
}

static void* _realloc_func(void* ptr, size_t org_size, size_t size)
{
        assert(size <= CHK_MAX_CHKSIZE);
        
        if (ptr == NULL)
                return _allocate_func(size);
        else
                return ptr;
}

static void* __memalign_sys(size_t  size)
{
        void *p;
        int err = posix_memalign(&p, CHK_CAPACITY(1), size);
        if (err)
                p = NULL;
        if (p)
                bzero(p, size);
        return p;
}

DEBUG_LOG_IMPL(ChkMemalign, _logger);

void  ChkMemalign::init(int conf_chunks, monitor_func monitor)
{
        if (_conf_chunks > 0)
        {
                DEBUG_LOG("already inited, _conf_chunks=" << _conf_chunks);
                return;
        }
        _conf_chunks = conf_chunks;
        _used_chunks = 0;
        _monitor = monitor;

        ChkMetadata::_allocate = _allocate_func;
        ChkMetadata::_dellocate = _dellocate_func;
        ChkMetadata::_reallocate = _realloc_func;

        ChkIndex::UNCOMPRESS_SPLIT = true;

#ifdef _THREADS
        ASSERT(pthread_mutex_init(&_mutex, NULL) == 0);
#endif

        DEBUG_LOG("pre init chunk num=" << _conf_chunks 
                                << ", chunk size=" << CHK_MAX_CHKSIZE);

        assert(_conf_chunks >= 8);
        _free_chunks.reserve(_conf_chunks * 3 / 2);
        refill_chunks(_conf_chunks);
}


void  ChkMemalign::refill_chunks(int chunk_num)
{
        char* init_mem = (char*)__memalign_sys(CHK_MAX_CHKSIZE * chunk_num);
        assert(init_mem);

        for (int i = 0; i < chunk_num; ++i)
        {
                _free_chunks.push_back(init_mem);
                init_mem += CHK_MAX_CHKSIZE;
        }
}

void* ChkMemalign::alloc()
{
#ifdef _THREADS
        ASSERT(pthread_mutex_lock(&_mutex) == 0);
#endif
        if (_free_chunks.empty())
        {
                DEBUG_LOG("pre init chunks used out, refill more chunks now");
                refill_chunks(_conf_chunks >> 2);
        }

        void* new_chunk = _free_chunks.back();
        _free_chunks.pop_back();
        ++_used_chunks;
        size_t  free_size = _free_chunks.size();

        if (_monitor)
                _monitor(_conf_chunks, _used_chunks, free_size);

#ifdef _THREADS
        pthread_mutex_unlock(&_mutex);
#endif
        
        DEBUG_LOG("alloc chunk_" << new_chunk
                        << ", used chunks=" << _used_chunks
                        << ", free chunks=" << free_size);
        return  new_chunk;
}

void ChkMemalign::dellocate(void* ptr)
{
#ifdef _THREADS
        ASSERT(pthread_mutex_lock(&_mutex) == 0);
#endif
        _free_chunks.push_back(ptr);
        --_used_chunks;
        size_t  free_size = _free_chunks.size();
#ifdef _THREADS
        pthread_mutex_unlock(&_mutex);
#endif

        DEBUG_LOG("dellocate chunk_" << ptr 
                        << ", used chunks=" << _used_chunks 
                        << ", free chunks=" << free_size);
}


static void* _def_allocate_func(size_t size)
{
        return ChkDefaultMemAlloc::instance()->alloc(size);
}

static void  _def_dellocate_func(void* ptr, size_t size)
{
        ChkDefaultMemAlloc::instance()->dellocate(ptr, size);
}

static void* _def_realloc_func(void* ptr, size_t org_size, size_t size)
{
        return ChkDefaultMemAlloc::instance()->reallocate(ptr, org_size, size);
}


DEBUG_LOG_IMPL(ChkDefaultMemAlloc, _logger);

void  ChkDefaultMemAlloc::init(bool realloc_fix)
{
        if (_alloc_size > 0)
        {
                DEBUG_LOG("already inited, _alloc_size=" << _alloc_size);
                return;
        }
        
        ChkMetadata::_unalign_allocate = _def_allocate_func;
        ChkMetadata::_unalign_dellocate = _def_dellocate_func;
        ChkMetadata::_unalign_reallocate = _def_realloc_func;

        _realloc_fix = realloc_fix;
        DEBUG_LOG("_realloc_fix=" << _realloc_fix);

#ifdef _THREADS
        ASSERT(pthread_mutex_init(&_mutex, NULL) == 0);
#endif 
}


void* ChkDefaultMemAlloc::alloc(size_t size)
{
#ifdef _THREADS
        ASSERT(pthread_mutex_lock(&_mutex) == 0);
#endif

        void* alloc_ptr = ::malloc(size);
        assert(alloc_ptr);
        _alloc_size += size;

#ifdef _THREADS
        pthread_mutex_unlock(&_mutex);
#endif

        DEBUG_LOG("after alloc, used mem=" << _alloc_size);
        return  alloc_ptr;
}

void* ChkDefaultMemAlloc::reallocate(void* ptr, size_t org_size, size_t size)
{
#ifdef _THREADS
        ASSERT(pthread_mutex_lock(&_mutex) == 0);
#endif
        void* new_ptr = NULL;
        if (_realloc_fix)
        {
                if (size)
                {
                        new_ptr = ::malloc(size);
                        assert(new_ptr);
                        ::memcpy(new_ptr, ptr, org_size<size?org_size: size);
                }
                ::free(ptr);
        }
        else {
                new_ptr = ::realloc(ptr, size);
        }
        _alloc_size += size;
        _alloc_size -= org_size;
#ifdef _THREADS
        pthread_mutex_unlock(&_mutex);
#endif

        DEBUG_LOG("after reallocate, used mem=" << _alloc_size 
                        << ", org size=" << org_size << ", new size=" << size
                        << ", org ptr=" << ptr << ", new ptr=" << new_ptr);
        return  new_ptr;
}

void ChkDefaultMemAlloc::dellocate(void* ptr, size_t size)
{
#ifdef _THREADS
        ASSERT(pthread_mutex_lock(&_mutex) == 0);
#endif

        ::free(ptr);
        _alloc_size -= size;

#ifdef _THREADS
        pthread_mutex_unlock(&_mutex);
#endif

        DEBUG_LOG("after dellocate, used mem=" << _alloc_size);
}


}
}

