#ifndef  _CHK_MEMALIGN_H_20120414
#define _CHK_MEMALIGN_H_20120414

#include <stdlib.h>
#include <assert.h>
#include "utils/logger.h"
#include "utils/shortcuts_util.h"
#include <list>
#include <vector>
#ifdef  _THREADS
#include <pthread.h>
#endif

namespace  comm { namespace  chk
{

typedef  void (*monitor_func)(int conf_chunks, int used_chunks, int free_chunks);

class  ChkMemalign
{
public:
        static ChkMemalign* instance()
        {
                static  ChkMemalign chk_memalign;
                return &chk_memalign;
        }

        void  init(int conf_chunks, monitor_func monitor);
        void* alloc();
        void dellocate(void* ptr);

private:
        void  refill_chunks(int chunk_num);
        ChkMemalign() : _conf_chunks(0)
        {
        }

private:
        int  _conf_chunks;
        int  _used_chunks;
        monitor_func  _monitor;
        std::vector<void*>  _free_chunks;

#ifdef _THREADS
        pthread_mutex_t  _mutex;
#endif

        DEBUG_LOG_DECL(_logger);
};


class  ChkDefaultMemAlloc
{
public:
        static ChkDefaultMemAlloc* instance()
        {
                static  ChkDefaultMemAlloc chk_def_mem_allc;
                return &chk_def_mem_allc;
        }
        void  init(bool realloc_fix);

        void* alloc(size_t size);
        void* reallocate(void* ptr, size_t org_size, size_t size);
        void dellocate(void* ptr, size_t size);

        ChkDefaultMemAlloc() : _alloc_size(0)
        {
        }

        size_t   _alloc_size;
        bool     _realloc_fix;

#ifdef _THREADS
        pthread_mutex_t  _mutex;
#endif

        DEBUG_LOG_DECL(_logger);        
};

}
}

#endif
