#include "chk_mem_mgr.h"
#include <algorithm>

namespace  comm { namespace  chk
{

DEBUG_LOG_IMPL(ChkMemMgr, _logger);

int  ChkMemMgr::free_size_preset[3];

void ChkMemMgr::preset_free_sizes(int *presets)
{
        int max_presets[3] = {127, 255, 1023};

        for (int i = 0; i < 3; ++i)
        {
                free_size_preset[i] = SET_CHK_SIZE(chk_align_def(
                                std::min(max_presets[i], presets[i])));
                
                DEBUG_LOG("preset free size[" << i << "]=" << free_size_preset[i]);
        }
}

ChkPtr ChkMemMgr::alloc(int size, ChkIndex* chk_index)
{
        size = SET_CHK_SIZE(size);
        
        int  free_size[3] = {_free_0, _free_1, _free_2};
        
        for (size_t i = 0; i < ARRAY_SIZE(free_size); ++i)
        {
                if (size <= free_size[i] && !chk_slist_empty(&_free_list[i]))
                {
                        ChkAddrChk addr_chk = chk_slist_pop(&_free_list[i], chk_index);
                        ChkPtr chk_ptr = chk_addrchk2ptr(&addr_chk, CHK_SIZE(size));
                        int  rest_size = free_size[i] - size;

                        DEBUG_LOG("target size=" << CHK_SIZE(size) 
                                        << ", alloc size=" << CHK_SIZE(free_size[i])
                                        << ", rest=" << CHK_SIZE(rest_size)
                                        << ", chk_ptr=" << chk_ptr);

                        if (rest_size > 0)
                        {
                                ChkPtr chk_ptr_rest = chk_ptr;
                                chk_ptr_rest.add(CHK_SIZE(size));
                                chk_ptr_rest._size = rest_size;

                                free(chk_ptr_rest, chk_index);
                        }
                        return  chk_ptr;
                }
        }

        DEBUG_LOG("cant alloc from mem pool");
        return  CHK_NULL;
}

void  ChkMemMgr::free(ChkPtr chk_ptr, ChkIndex* chk_index)
{
        ChkAddrChk addr_chk;
        int  left_size = (int)chk_ptr._size;

        int  free_size[3] = {_free_0, _free_1, _free_2};
        assert(_free_0);
        
        while (left_size >= free_size[0])
        {
                for (int i = 2; i >= 0; --i)
                {
                        if (left_size >= free_size[i])
                        {
                                chk_ptr._size = free_size[i];
                                addr_chk = chk_ptr2addrchk(chk_index, chk_ptr);
                               
                                bzero(addr_chk.addr, CHK_SIZE(chk_ptr._size));
                                
                                chk_slist_push(&addr_chk, &_free_list[i]);

                                left_size -= free_size[i];
                                DEBUG_LOG("free ptr add=" << chk_ptr 
                                                << ", left size=" << CHK_SIZE(left_size));

                                chk_ptr._offset += CHK_SIZE_2_OFFSET(free_size[i]);
                                break;
                        }
                }
        }
}

}
}

