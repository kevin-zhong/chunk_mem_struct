#ifndef  _CHK_MEM_MGR_H
#define _CHK_MEM_MGR_H

#include "chk_base.h"
#include "chk_list.h"
#include "chk_mem_mgr.h"

namespace  comm { namespace  chk
{

#define chk_align(d, a)     (((d) + ((a) - 1)) & ~((a) - 1))
#define chk_align_def(d)   chk_align(d, CHK_SIZE(1))
#define chk_align_capaicty(d)  chk_align(d, CHK_CAPACITY(1))

/*
* chk mem manager (24 bytes)
*/
struct  ChkMemMgr
{
        uint32_t  _total_used;
        uint32_t  _total_free;
        uint16_t  _total_capacity;//max 64M

        uint16_t  _free_0:4;//max=(16*8 = 128)
        uint16_t  _free_1:5;//max=(32*8 = 256)
        uint16_t  _free_2:7;//max=(128*8= 1024)
        ChkSList  _free_list[3];

        ChkMemMgr()
        {
                bzero(this, sizeof(ChkMemMgr));

                _free_0 = free_size_preset[0];
                _free_1 = free_size_preset[1];
                _free_2 = free_size_preset[2];
                
                for (size_t i = 0; i < ARRAY_SIZE(_free_list); ++i)
                {
                        chk_slist_init(_free_list + i);
                }
        }

        ChkPtr alloc(int size, ChkIndex* chk_index);
        void  free(ChkPtr chk_ptr, ChkIndex* chk_index);

        static void preset_free_sizes(int *presets);

private:
        static  int  free_size_preset[3];
        DEBUG_LOG_DECL(_logger);
};

inline std::ostream & operator<<(std::ostream& stream, const ChkMemMgr& chk_mem_mgr)
{
        stream << "chk_memmgr{this=" << &chk_mem_mgr 
                        << ", capacity=" << CHK_CAPACITY(chk_mem_mgr._total_capacity)
                        << ", used=" << CHK_SIZE(chk_mem_mgr._total_used)
                        << ", free=" << CHK_SIZE(chk_mem_mgr._total_free)
                        << ", free_size=" << chk_mem_mgr._free_0 
                                << "|" << chk_mem_mgr._free_1 << "|" << chk_mem_mgr._free_2 
                                << "}";
        return  stream;
}

}
}

#endif

