#ifndef  _CHK_BASE_H
#define _CHK_BASE_H

#include <stdlib.h>
#include <assert.h>
#include "utils/logger.h"
#include "utils/shortcuts_util.h"

namespace  comm { namespace  chk
{

#define CHK_MAGIC  0x128fcd6e
#define CHK_PADDING 0x208f2d75

#define CHK_SIZE(a) ((a)<<3)
#define SET_CHK_SIZE(a) ((a)>>3)
#define CHK_SIZE_VALID(a) (((a) & 7) == 0)

#define CHK_OFFSET(a) ((a)<<2)
#define SET_CHK_OFFSET(a) ((a)>>2)
#define CHK_OFFSET_VALID(a) (((a) & 3) == 0)

#define CHK_SIZE_2_OFFSET(a) ((a)<<1)
#define CHK_OFFSET_2_SIZE(a) ((a)>>1)

#define CHK_CAPACITY(a) ((a)<<10)
#define SET_CHK_CAPACITY(a) ((a)>>10)

// 4K
#define CHK_MAX_CONTINUS_LEN CHK_SIZE(((1<<9) - 1))

// 1..11..1
#define  CHK_MAX_CHK_NUM  ((1<<11) - 1)

// 4 bytes
//_chk_no*_offset = 32M
struct  ChkPtr
{
        //index, max=2047
        uint32_t   _chk_no:11;
        //°´4¶ÔÆë max_cksize=16k(0<_offset<4096)
        uint32_t   _offset:12;

        uint32_t   _size:9;

        bool  operator == (const ChkPtr& rh) const
        {
                return  _chk_no == rh._chk_no && _offset == rh._offset;
        }
        bool  operator != (const ChkPtr& rh) const
        {
                return !(*this == rh);
        }

        void add(size_t offset)
        {
                _offset += SET_CHK_OFFSET(offset);
        }
        void sub(size_t offset)
        {
                _offset -= SET_CHK_OFFSET(offset);
        }

        bool is_null() const
        {
                return  _chk_no == CHK_MAX_CHK_NUM;
        }
        void set_null()
        {
                _chk_no = CHK_MAX_CHK_NUM;
        }
};

inline std::ostream & operator<<(std::ostream& stream, const ChkPtr& chk_ptr)
{
        stream << "chkptr{no=" << chk_ptr._chk_no 
                        << ", offset=" << CHK_OFFSET(chk_ptr._offset) 
                        << ", size=" << CHK_SIZE(chk_ptr._size) << "}";
        return  stream;
}


extern const ChkPtr CHK_NULL;


enum CHK_MEM_ALLOC_TYPE
{
        CHK_MEM_ALLOC_MAX = 1,
        CHK_MEM_ALLOC_NEED = 2
};

// 16 bytes
struct  ChkMetadata
{
        uint32_t  _magic;
        
        uint32_t  _capacity:5;//max 31
        uint32_t  _padding:3;
        uint32_t  _used:12;
        uint32_t  _virgin:12;

        uint32_t   _no:11;
        uint32_t   _alloc_capacity:16;
        uint32_t   _padding2:5;
        
        uint32_t   _padding3;

        bool  can_enlarge();
        ChkPtr alloc(size_t size);
        //recv ret 0, else ret -1
        int  free(ChkPtr chk_ptr);

        //if success, will ret new chk
        static ChkMetadata*  shrink(ChkMetadata* old);

        //if old = NULL, then == alloc
        static ChkMetadata*  enlarge(ChkMetadata* old, CHK_MEM_ALLOC_TYPE alloc_type);
        static void  destruct(ChkMetadata* old);

        static size_t  chk_mem_sizes[6];

        int capacity()
        {
                return  CHK_CAPACITY(_capacity);
        }

        typedef  void* (*allocate_func)(size_t size);
        typedef  void  (*dellocate_func)(void* ptr, size_t size);
        typedef  void* (*realloc_func)(void* ptr, size_t org_size, size_t size);
        
        static allocate_func   _allocate;
        static dellocate_func  _dellocate;
        static realloc_func     _reallocate;

        static allocate_func   _unalign_allocate;
        static dellocate_func  _unalign_dellocate;
        static realloc_func     _unalign_reallocate;

        DEBUG_LOG_DECL(_logger);
};


#define CHK_MAX_CHKSIZE (ChkMetadata::chk_mem_sizes\
                [ARRAY_SIZE(ChkMetadata::chk_mem_sizes) - 1])

inline std::ostream & operator<<(std::ostream& stream, const ChkMetadata& chk)
{
        stream << "chkmeta{no=" << chk._no
                        << ", alloc capacity=" << CHK_CAPACITY(chk._alloc_capacity)
                        << ", capacity=" << CHK_CAPACITY(chk._capacity)
                        << ", used=" << CHK_SIZE(chk._used)
                        << ", virgin=" << CHK_SIZE(chk._virgin) << "}";
        return  stream;
}


struct  ChkMemMgr;

struct  ChkIndex
{
        uint16_t             _chk_num;
        uint16_t             _mem_alloc_type:2;
        uint16_t             _compressed:1;
        uint16_t             _padding1:13;
        uint16_t             _chk_addr_size;
        uint16_t             _padding2;
        ChkMetadata**   _chk_addr;

        ChkIndex(CHK_MEM_ALLOC_TYPE alloc_type = CHK_MEM_ALLOC_MAX)
        {
                bzero(this, sizeof(ChkIndex));
                _mem_alloc_type = alloc_type;
        }
        ~ChkIndex()
        {
                if (_chk_addr) {
                        ::free(_chk_addr);
                        _chk_addr = NULL;
                }
                _chk_num = 0;
        }

        void  swap(ChkIndex& rh);

        void  destruct();

        int     init_boot(int boot_size);
        char* boot_addr();

        ChkPtr alloc(int size);
        void  free(ChkPtr chk_ptr, int size = 0);
        ChkMemMgr* mem_mgr();

        void  shrink();
        int  enlarge();
        int  compress(int target_compress_percent = 1000);
        int  uncompress();
        
        bool  is_continuous_chunk();

        static  bool  UNCOMPRESS_SPLIT;

private:
        int  split_continuous_chunk();
        int  chk_enlarge(int just_ext_chk = 0);

        DEBUG_LOG_DECL(_logger);
};


/*
* ChkPtr <-> (addr + chk)
*/
struct  ChkAddrChk
{
        void* addr;
        ChkMetadata* chk;
};

#define  chk_set_addrchk(_addr_chk, _addr, _chk) {_addr_chk.addr = _addr; _addr_chk.chk = _chk;}

#define  chk_ptr2chk(_chk_index, _chk_ptr) \
                (_chk_ptr.is_null() ? NULL : (ChkMetadata*)(_chk_index)->_chk_addr[(_chk_ptr)._chk_no])

#define  chk_ptr2addr(_chk_index, _chk_ptr, TRet) \
                (_chk_ptr.is_null() ? NULL : (TRet*)((char*)chk_ptr2chk(_chk_index, _chk_ptr) + CHK_OFFSET((_chk_ptr)._offset)))

#define  chk_ptr_valid(_chk_index, _chk_ptr) \
                ((_chk_ptr)._chk_no < _chk_index->_chk_num \
                        && CHK_OFFSET((_chk_ptr)._offset) < CHK_SIZE(chk_ptr2chk(_chk_index, _chk_ptr)->_virgin))

#define  chk_ptr2addrchk(_chk_index, _chk_ptr) ({ \
                ChkAddrChk _addr_chk = {chk_ptr2addr(_chk_index, _chk_ptr, void), \
                        chk_ptr2chk(_chk_index, _chk_ptr)}; _addr_chk;\
                })

static inline ChkPtr chk_addrchk2ptr(ChkAddrChk* addr_chk, size_t size = 0)
{
        ChkPtr  chk_ptr = {0};
        chk_ptr._chk_no = addr_chk->chk->_no;

        int nature_offset = (char*)addr_chk->addr - (char*)addr_chk->chk;
        assert(CHK_OFFSET_VALID(nature_offset) && nature_offset > 0 
                        && (uint32_t)nature_offset < CHK_SIZE(addr_chk->chk->_virgin));
        
        chk_ptr._offset = SET_CHK_OFFSET(nature_offset);
        chk_ptr._size = SET_CHK_SIZE(size);
        return  chk_ptr;
}

ChkMetadata* chk_addr2chk(void* addr);
ChkPtr  chk_addr2ptr(void* addr, size_t size = 0);

}
}

#endif

