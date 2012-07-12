#include "chk_base.h"
#include "chk_mem_mgr.h"
#include "chk_hash_list.h"
#include <algorithm>
#include <snappy.h>
#include <snappy-sinksource.h>

namespace  comm { namespace  chk
{

const  ChkPtr CHK_NULL = {CHK_MAX_CHK_NUM, 0, 0};

size_t  ChkMetadata::chk_mem_sizes[] = {1024, 2048, 4096, 8192, 12288, 16384};

const int ChkHashSList::MAX_BUCKETS;

static void* allocate_wrap(size_t size)
{
        void *p;
        int err = posix_memalign(&p, CHK_CAPACITY(1), size);
        if (err)
                p = NULL;
        if (p)
                bzero(p, size);
        return p;
}
static void* realloc_wrap(void* ptr, size_t org_size, size_t size)
{
        void *p;
        int err = posix_memalign(&p, CHK_CAPACITY(1), size);
        if (err)
                return NULL;
        assert((((uintptr_t)p) & (CHK_CAPACITY(1)-1)) == 0);
        if (p)
                bzero(p, size);
        if (ptr)
        {
                memcpy(p, ptr, std::min(size, (size_t)((ChkMetadata*)ptr)->capacity()));
                ChkMetadata::_dellocate(ptr, org_size);
        }
        return  p;
}

static void  dellocate_wrap(void* ptr, size_t size)
{
        ::free(ptr);
}

static void* unalign_allocate_wrap(size_t size)
{
        void* p = ::malloc(size);
        if (p)
                bzero(p, size);
        return p;
}
static void* unalign_realloc_wrap(void* ptr, size_t org_size, size_t size)
{
        return  ::realloc(ptr, size);
}


ChkMetadata::allocate_func   ChkMetadata::_allocate = allocate_wrap;
ChkMetadata::dellocate_func  ChkMetadata::_dellocate = dellocate_wrap;
ChkMetadata::realloc_func     ChkMetadata::_reallocate = realloc_wrap;

ChkMetadata::allocate_func   ChkMetadata::_unalign_allocate = unalign_allocate_wrap;
ChkMetadata::dellocate_func  ChkMetadata::_unalign_dellocate = dellocate_wrap;
ChkMetadata::realloc_func     ChkMetadata::_unalign_reallocate = unalign_realloc_wrap;


DEBUG_LOG_IMPL(ChkMetadata, _logger);

bool  ChkMetadata::can_enlarge()
{
        return  this->_alloc_capacity && 
                CHK_CAPACITY(_capacity) < CHK_MAX_CHKSIZE;
}

ChkMetadata*  ChkMetadata::enlarge(ChkMetadata* old, CHK_MEM_ALLOC_TYPE alloc_type)
{
        ChkMetadata* new_chk = NULL;
        int  capacity_level = 0;
        
        if (old && !old->can_enlarge())
                return  NULL;

        if (alloc_type == CHK_MEM_ALLOC_MAX)
        {
                capacity_level = chk_mem_sizes[ARRAY_SIZE(chk_mem_sizes) - 1];
        }
        else {
                int index = std::upper_bound(chk_mem_sizes, 
                                chk_mem_sizes + ARRAY_SIZE(chk_mem_sizes), 
                                CHK_CAPACITY(old->_capacity)) - chk_mem_sizes;

                capacity_level = chk_mem_sizes[index];
                assert(capacity_level > (int)CHK_CAPACITY(old->_capacity));
        }
        
        if (old)
                DEBUG_LOG("enlarge old=" << *old);
        
        new_chk = (ChkMetadata*)_reallocate(old, old ? old->capacity() : 0, capacity_level);
        if (new_chk == NULL)
                return  NULL;

        if (old == NULL)
        {
                bzero(new_chk, sizeof(ChkMetadata));
                
                new_chk->_magic = CHK_MAGIC;
                new_chk->_padding3 = CHK_PADDING;
                new_chk->_used = SET_CHK_SIZE(chk_align_def(sizeof(ChkMetadata)));
                new_chk->_virgin = new_chk->_used;
        }

        new_chk->_capacity = SET_CHK_CAPACITY(capacity_level);
        new_chk->_alloc_capacity = new_chk->_capacity;
        
        DEBUG_LOG("enlarge new=" << *new_chk);
        return  new_chk;
}

/*
* if success, will ret new chk
*/
ChkMetadata*  ChkMetadata::shrink(ChkMetadata* old)
{
        int  reset_chk = SET_CHK_CAPACITY(old->capacity() - (int)CHK_SIZE(old->_virgin));
        assert(reset_chk >= 0);
        if (reset_chk == 0)
        {
                DEBUG_LOG("old no need to shrink, info=" << *old);
                return  NULL;
        }

        ChkMetadata* new_chk = NULL;
        int  new_capacity = old->_capacity - reset_chk;
        
        if (reset_chk < new_capacity)
        {
                DEBUG_LOG("shrink by _reallocate, old chk=" << *old);
                
                new_chk = (ChkMetadata*)_reallocate(old, old->capacity(), 
                                                                CHK_CAPACITY(new_capacity));
                if (new_chk == NULL)
                        return  NULL;
        }
        else {
                DEBUG_LOG("shrink by new+copy, old chk=" << *old);
                
                new_chk = (ChkMetadata*)_allocate(CHK_CAPACITY(new_capacity));
                if (new_chk == NULL)
                        return  NULL;
                memcpy(new_chk, old, CHK_SIZE(old->_virgin));
                _dellocate(old, old->capacity());
        }

        new_chk->_capacity = new_capacity;
        new_chk->_alloc_capacity = new_chk->_capacity;
        DEBUG_LOG("shrink new chk=" << *new_chk);
        return  new_chk;
}


ChkPtr ChkMetadata::alloc(size_t size)
{
        ChkPtr chk_ptr = CHK_NULL;
        
        size = chk_align_def(size);

        if (size > CHK_MAX_CONTINUS_LEN)
        {
                DEBUG_LOG("too big target, size=" << size);
                return chk_ptr;
        }
        
        if (CHK_SIZE(_virgin) + size > CHK_CAPACITY(_capacity))
                return  chk_ptr;

        chk_ptr._chk_no = _no;
        chk_ptr._offset = CHK_SIZE_2_OFFSET(_virgin);
        chk_ptr._size = SET_CHK_SIZE(size);
        
        _virgin += chk_ptr._size;

        DEBUG_LOG("alloc success, ret=" << chk_ptr << ", _virgin=" << CHK_SIZE(_virgin));
        return  chk_ptr;
}

//recv ret 0, else ret -1
int  ChkMetadata::free(ChkPtr chk_ptr)
{
        assert(chk_ptr._chk_no == _no && chk_ptr._size);
        
        if (CHK_OFFSET_2_SIZE(chk_ptr._offset) + chk_ptr._size == _virgin)
        {
                _virgin -= chk_ptr._size;
                DEBUG_LOG("free to chk success, ptr=" << chk_ptr << ", _virgin=" << _virgin);
                return 0;
        }
        return -1;
}

void  ChkMetadata::destruct(ChkMetadata* old)
{
        if (old->_alloc_capacity)
        {
                DEBUG_LOG("destruct chk, info=" << *old);
                
                _dellocate(old, CHK_CAPACITY(old->_alloc_capacity));
        }
        else {
                DEBUG_LOG("not allocted chk, cant destructed alone, info=" << *old);
        }
}


inline ChkMemMgr* ChkIndex::mem_mgr()
{
        return  (ChkMemMgr*)((char*)_chk_addr[0] + chk_align_def(sizeof(ChkMetadata)));
}


DEBUG_LOG_IMPL(ChkIndex, _logger);

 bool  ChkIndex::UNCOMPRESS_SPLIT = false;

int  ChkIndex::chk_enlarge(int just_ext_chk)
{
        ChkMetadata* chk_metadata = NULL;
        int capacity = 0;
        
        if (_chk_num)
        {
                chk_metadata = _chk_addr[_chk_num - 1];
                
                if (chk_metadata->can_enlarge())
                {
                        capacity = chk_metadata->capacity();
                        
                        chk_metadata = ChkMetadata::enlarge(chk_metadata, 
                                        CHK_MEM_ALLOC_TYPE(_mem_alloc_type));
                        if (chk_metadata == NULL)
                                return  -1;

                        _chk_addr[_chk_num - 1] = chk_metadata;

                        capacity = chk_metadata->capacity() - capacity;
                        
                        mem_mgr()->_total_capacity += SET_CHK_CAPACITY(capacity);

                        DEBUG_LOG("enlarge, chk_num=" << _chk_num - 1 
                                        << ", chk_mem_mgr=" << *mem_mgr()
                                        << ", added_capacity=" << capacity
                                        << ", chkmeta=" << *chk_metadata);
                        return  0;
                }
        }

        if (just_ext_chk)
        {
                DEBUG_LOG("enlarge chk do nothing");
                return  0;
        }

        if (_chk_num >= CHK_MAX_CHK_NUM)
        {
                DEBUG_LOG("too much chk_num=" << _chk_num);
                return -1;
        }

        if (_chk_num == _chk_addr_size)
        {
                _chk_addr_size = (_chk_addr_size ? (_chk_addr_size << 1) : 1);
                void* new_addr = realloc(_chk_addr, _chk_addr_size * sizeof(void*));
                assert(new_addr);
                _chk_addr = (ChkMetadata**)new_addr;

                DEBUG_LOG("_chk_addr enlarge, _chk_addr_size=" << _chk_addr_size);
        }
        
        chk_metadata = ChkMetadata::enlarge(NULL, 
                        CHK_MEM_ALLOC_TYPE(_mem_alloc_type));
        if (chk_metadata == NULL)
                return  -1;

        chk_metadata->_no = _chk_num;
        _chk_addr[_chk_num++] = chk_metadata;

        //fist chk must set mem mgr
        if (_chk_num == 1)
        {
                ChkPtr chk_ptr = chk_metadata->alloc(sizeof(ChkMemMgr));
                assert(!chk_ptr.is_null());

                chk_metadata->_used += chk_ptr._size;
                
                ChkMemMgr* chk_mem_mgr = chk_ptr2addr(this, chk_ptr, ChkMemMgr);
                ::new(chk_mem_mgr)ChkMemMgr();
        }

        mem_mgr()->_total_capacity += SET_CHK_CAPACITY(chk_metadata->capacity());
        mem_mgr()->_total_used += chk_metadata->_used;

        DEBUG_LOG("chk_num="<< _chk_num 
                        << ", chk_mem_mgr=" << *mem_mgr()
                        << ", chkmeta=" << *chk_metadata);
        return  0;
}


void  ChkIndex::shrink()
{
        if (_chk_num == 0)
                return;

        int capacity = _chk_addr[_chk_num - 1]->capacity();
        
        ChkMetadata* chk_metadata = ChkMetadata::shrink(_chk_addr[_chk_num - 1]);
        if (chk_metadata != NULL)
        {
                _chk_addr[_chk_num - 1] = chk_metadata;
                int sub_capacity = capacity - chk_metadata->capacity();
                
                mem_mgr()->_total_capacity -= SET_CHK_CAPACITY(sub_capacity);

                DEBUG_LOG("chk_num="<< _chk_num << ", sub_capacity=" << sub_capacity
                                << ", chk_mem_mgr=" << *mem_mgr()
                                << ", chkmeta=" << *chk_metadata);
        }
}

int  ChkIndex::enlarge()
{
        CHCK_WARN_RV(uncompress() != 0, -1, "uncompress failed");
        
        CHCK_WARN_RV(split_continuous_chunk() != 0, -2, "split failed");
        
        CHCK_WARN_RV(chk_enlarge(1) != 0, -3, "enlarge failed");

        return  0;
}


ChkPtr ChkIndex::alloc(int size)
{
        assert(size > 0);

        size = chk_align_def(size);
        if (size > CHK_MAX_CONTINUS_LEN)
        {
                DEBUG_LOG("too big target, size=" << size);
                return CHK_NULL;
        }

        CHCK_WARN_RV(uncompress() != 0, CHK_NULL, "uncompress failed");
        
        if (_chk_num == 0)
        {
                if (chk_enlarge() < 0)
                {
                        DEBUG_LOG("chk_enlarge failed !");
                        return CHK_NULL;
                }
        }

        assert(_chk_num);
        if (_mem_alloc_type == CHK_MEM_ALLOC_MAX
                && _chk_addr[_chk_num-1]->can_enlarge())
        {
                DEBUG_LOG("you should call enlarge first!");
                return  CHK_NULL;
        }
        
        ChkMetadata* chk_metadata = NULL;

        ChkPtr chk_ptr = mem_mgr()->alloc(size, this);
        if (!chk_ptr.is_null())
        {
                chk_metadata = chk_ptr2chk(this, chk_ptr);

                mem_mgr()->_total_free -= chk_ptr._size;
        }
        else {
                int old_chk_num = _chk_num;
try_again:
                chk_metadata = _chk_addr[_chk_num-1];
                chk_ptr = chk_metadata->alloc(size);
                if (chk_ptr.is_null())
                {
                        if (chk_enlarge() < 0)
                                return chk_ptr;

                        if (old_chk_num != _chk_num)
                        {
                                int reset_size = chk_metadata->capacity() 
                                                - CHK_SIZE(chk_metadata->_virgin);
                                
                                DEBUG_LOG("chk_enlarge add chk, put rest mem to pool, "
                                                << "size=" << reset_size
                                                << ", chkmeta=" << *chk_metadata);
                                
                                if (reset_size > 0)
                                {
                                        ChkPtr collect_ptr = chk_metadata->alloc(reset_size);
                                        mem_mgr()->free(collect_ptr, this);
                                }
                        }
                        goto try_again;
                }
        }

        chk_metadata->_used += chk_ptr._size;
        mem_mgr()->_total_used += chk_ptr._size;

        DEBUG_LOG("alloc size=" << size << ", ptr=" << chk_ptr
                        << ", chkmeta=" << *chk_metadata 
                        << ", chk_mem_mgr=" << *mem_mgr());
        return  chk_ptr;
}


void  ChkIndex::free(ChkPtr chk_ptr, int size)
{
        CHCK_WARN_R(uncompress() != 0, "uncompress failed");
        
        if (size > 0)
                chk_ptr._size = SET_CHK_SIZE(size);

        char* free_mem = chk_ptr2addr(this, chk_ptr, char);
        bzero(free_mem, CHK_SIZE(chk_ptr._size));
        
        ChkMetadata* chk_metadata = chk_ptr2chk(this, chk_ptr);
        
        if (chk_metadata->free(chk_ptr) < 0)
        {
                mem_mgr()->_total_free += chk_ptr._size;
                
                mem_mgr()->free(chk_ptr, this);
        }

        chk_metadata->_used -= chk_ptr._size;
        mem_mgr()->_total_used -= chk_ptr._size;

        DEBUG_LOG("free ptr=" << chk_ptr 
                        << ", chkmeta=" << *chk_metadata 
                        << ", chk_mem_mgr=" << *mem_mgr());        
}


int ChkIndex::init_boot(int boot_size)
{
        ChkPtr  chk_ptr = alloc(boot_size);
        if (chk_ptr.is_null())
                return -1;

        if (chk_ptr._chk_no != 0 || CHK_OFFSET(chk_ptr._offset) != 
                (chk_align_def(sizeof(ChkMetadata)) + chk_align_def(sizeof(ChkMemMgr))))
        {
                free(chk_ptr);
                return  -2;
        }
        return  0;
}

char* ChkIndex::boot_addr()
{
        CHCK_WARN_RV(uncompress() != 0, NULL, "uncompress failed");
        
        if (_chk_num == 0)
                return  NULL;

        return  (char*)_chk_addr[0] 
                        + chk_align_def(sizeof(ChkMetadata)) 
                        + chk_align_def(sizeof(ChkMemMgr));
}

void  ChkIndex::destruct()
{
        if (_compressed)
        {
                char* compressed_bytes = (char*)_chk_addr[0];
                int  compressed_len = *((int*)compressed_bytes);
                DEBUG_LOG("destruct compressed mem, len=" << compressed_len);
                
                ChkMetadata::_unalign_dellocate(compressed_bytes, compressed_len);
        }
        else {
                DEBUG_LOG("before destruct, mem_mgr=" << *mem_mgr());
                
                for (int i = _chk_num - 1; i >= 0; --i)
                {
                        ChkMetadata::destruct(_chk_addr[i]);
                        _chk_addr[i] = NULL;
                }
        }
        _chk_num = 0;
        _compressed = 0;
}

bool  ChkIndex::is_continuous_chunk()
{
        ChkMemMgr* pmem_mgr = mem_mgr();
        
        return _chk_num > 1 && pmem_mgr->_total_capacity 
                        == _chk_addr[0]->_alloc_capacity;
}

int  ChkIndex::split_continuous_chunk()
{
        if (!is_continuous_chunk())
                return 0;

        ChkMetadata* chk_org = _chk_addr[0];
        
        for (uint16_t i = 0; i < _chk_num; ++i)
        {
                int capacity = _chk_addr[i]->capacity();
                
                ChkMetadata* new_chk = (ChkMetadata*)ChkMetadata::_allocate(capacity);
                assert(new_chk);

                if (i < _chk_num -1)
                        memcpy(new_chk, _chk_addr[i], capacity);
                else {
                        //must copy effective, cause real mem...
                        memcpy(new_chk, _chk_addr[i], CHK_SIZE(_chk_addr[i]->_virgin));
                }
                new_chk->_alloc_capacity = new_chk->_capacity;

                DEBUG_LOG("split to chunk[" << i << "]=" << *new_chk
                                << ", org=" << _chk_addr[i]);
                
                _chk_addr[i] = new_chk;
        }

        UNCOMPRESS_SPLIT ? ::free(chk_org) : ChkMetadata::destruct(chk_org);
        return 0;
}


/*
* compress & uncompress
*/
class  ChkCompressSource : public snappy::Source
                , public LoggerTpl2<ChkCompressSource>
{
public:
        ChkCompressSource(ChkIndex* chk_index) 
                : _chk_index(chk_index)
                , _chk_no(0)
        {
                _is_continuous = chk_index->is_continuous_chunk();

                ChkMetadata* last_chk = chk_index->_chk_addr[chk_index->_chk_num - 1];
                int  total_capacity = CHK_CAPACITY(chk_index->mem_mgr()->_total_capacity);
                
                _bytes_left = total_capacity - last_chk->capacity() + CHK_SIZE(last_chk->_virgin);
                
                DEBUG_LOG("continuous=" << _is_continuous
                                << ", chk_num=" << chk_index->_chk_num
                                << ", org bytes=" << total_capacity 
                                << ", compress bytes=" << _bytes_left);

                if (!_is_continuous)
                        _chk_offset = 0;
                else
                        _ptr = (char*)chk_index->_chk_addr[0];
        }
        
        virtual ~ChkCompressSource()
        {
        }

        virtual size_t Available() const
        {
                return  _bytes_left;
        }
        
        virtual const char* Peek(size_t* len)
        {
                if (_is_continuous)
                {
                       *len = _bytes_left;
                       return  _ptr;
                }

                ChkMetadata* chk_meta = _chk_index->_chk_addr[_chk_no];

                *len = chk_meta->capacity() - _chk_offset;
                assert(*len);
                return  (char*)chk_meta + _chk_offset;
        }
        
        virtual void Skip(size_t n)
        {
                if (n == 0)
                        return;
                if (_is_continuous)
                {
                        _bytes_left -= n;
                        _ptr += n;
                        return;
                }
                
                ChkMetadata* chk_meta = _chk_index->_chk_addr[_chk_no];

                _chk_offset += n;
                int  rest = chk_meta->capacity() - _chk_offset;
                assert(rest >= 0);
                if (rest == 0) {
                        _chk_offset = 0;
                        ++_chk_no;
                }
        }
        int  uncompress_len() {
                return  _bytes_left;
        }
        
private:
        ChkIndex* _chk_index;
        short  _chk_no;
        short  _is_continuous;

        int  _bytes_left;
        union {
                int       _chk_offset;
                char*   _ptr;
        };
};


static char* chk_compress(ChkIndex* chk_index) 
{
        ChkCompressSource  reader(chk_index);

        size_t prealloc_len = sizeof(int) + snappy::MaxCompressedLength(reader.uncompress_len());
        char* compressed = (char*)ChkMetadata::_unalign_allocate(prealloc_len);
        CHCK_RV(compressed == NULL, NULL);
        
        snappy::UncheckedByteArraySink writer(compressed + sizeof(int));
        size_t compressed_length = snappy::Compress(&reader, &writer) + sizeof(int);
        
        *((int*)compressed) = compressed_length;

        char* shrink_bytes = (char*)ChkMetadata::_unalign_reallocate(compressed, 
                                                prealloc_len, compressed_length);
        
        if (shrink_bytes == NULL)
        {
                ChkMetadata::_unalign_dellocate(compressed, prealloc_len);
                return  NULL;
        }

        return  shrink_bytes;
}


int  ChkIndex::compress(int target_compress_percent)
{
        if (_compressed)
                return 0;

        ChkMemMgr* pmem_mgr = mem_mgr();

        char* compressed_bytes = chk_compress(this);
        CHCK_RV(compressed_bytes == NULL, -1);
        
        int  compressed_len = *((int*)compressed_bytes);
        int  org_len = CHK_CAPACITY(pmem_mgr->_total_capacity);

        DEBUG_LOG("after compress bytes len=" << compressed_len
                                << ", org capacity=" << org_len);
        
        if (((uint64_t)compressed_len) * 100 > ((uint64_t)target_compress_percent) * org_len)
        {
                DEBUG_LOG("compress rate not satify target, give up, target compress percent="
                                << target_compress_percent);
                
                ChkMetadata::_unalign_dellocate(compressed_bytes, compressed_len);
                return  0;
        }

        destruct();

        _chk_addr[0] = (ChkMetadata*)compressed_bytes;
        _compressed = 1;
        return  0;
}


int  ChkIndex::uncompress()
{
        if (!_compressed)
                return  0;

        char* compressed_bytes = (char*)_chk_addr[0];
        char* data_bytes = compressed_bytes + sizeof(int);
        int  compressed_len = *((int*)compressed_bytes);
        int  data_len = compressed_len - sizeof(int);

        size_t ulength;
        if (!snappy::GetUncompressedLength(data_bytes, data_len, &ulength)) 
        {
                DEBUG_LOG("GetUncompressedLength failed, compressed_len=" 
                                << compressed_len);
                return -1;
        }
        size_t  align_capacity = chk_align_capaicty(ulength);
        bool  chkmeta_dirctly = (!UNCOMPRESS_SPLIT) 
                                || align_capacity <= CHK_MAX_CHKSIZE;
        
        char* uncompressed = (char*)(chkmeta_dirctly ? 
                               ChkMetadata::_allocate(align_capacity) : ::malloc(ulength));
        CHCK_RV(uncompressed == NULL, -2);

        if (!snappy::RawUncompress(data_bytes, data_len, uncompressed))
        {
                DEBUG_LOG("RawUncompress failed, compressed_len=" 
                                << compressed_len << ", uncompress_len=" << ulength);

                chkmeta_dirctly ? ChkMetadata::_dellocate(uncompressed, align_capacity)
                                :  ::free(uncompressed);
                return -1;
        }

        size_t  capacity_caculate = 0;
        char* uncompressed_end = uncompressed + align_capacity;
        ChkMetadata* chk_meta = (ChkMetadata*)uncompressed;

        _chk_num = 0;
        while ((char*)chk_meta < uncompressed_end)
        {
                chk_meta->_alloc_capacity = 0;
                if (uncompressed_end - (char*)chk_meta < chk_meta->capacity())
                {
                        chk_meta->_capacity = SET_CHK_CAPACITY(
                                                                uncompressed_end - (char*)chk_meta);
                }
                capacity_caculate += chk_meta->_capacity;

                _chk_addr[_chk_num] = chk_meta;
                ++_chk_num;

                chk_meta = (ChkMetadata*)((char*)chk_meta 
                                                + CHK_CAPACITY(chk_meta->_capacity));
        }
        
        _chk_addr[0]->_alloc_capacity = SET_CHK_CAPACITY(align_capacity);
        assert(capacity_caculate == _chk_addr[0]->_alloc_capacity);
        
        DEBUG_LOG("mem capacity from org=" << mem_mgr()->_total_capacity
                                << ", to=" << _chk_addr[0]->_alloc_capacity);
        mem_mgr()->_total_capacity = _chk_addr[0]->_alloc_capacity;

        ChkMetadata::_unalign_dellocate(compressed_bytes, compressed_len);

        _compressed = 0;

        if (UNCOMPRESS_SPLIT)
                split_continuous_chunk();
        
        return  0;
}


ChkMetadata* chk_addr2chk(void* addr)
{
        char*  addr_align = (char*)chk_align((uintptr_t)addr, CHK_CAPACITY(1));
        ChkMetadata* chk_meta;
        
        for (uint32_t i = 1; i < 17; ++i)
        {
                chk_meta = (ChkMetadata*)(addr_align - CHK_CAPACITY(i));
                
                if (chk_meta->_magic == CHK_MAGIC 
                        && chk_meta->_padding3 == CHK_PADDING
                        && chk_meta->_capacity <= 16
                        && chk_meta->_capacity >= i)
                {
                        return  chk_meta;
                }
        }
        return  NULL;
}


ChkPtr  chk_addr2ptr(void* addr, size_t size)
{
        ChkMetadata* chk_meta = chk_addr2chk(addr);
        if (chk_meta == NULL)
                return  CHK_NULL;

        ChkAddrChk addrchk = {addr, chk_meta};
        return  chk_addrchk2ptr(&addrchk, size);
}

}
}
