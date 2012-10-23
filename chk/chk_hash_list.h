#ifndef  _CHK_HASH_LIST_H
#define _CHK_HASH_LIST_H

#include "chk_base.h"
#include "chk_list.h"

namespace  comm { namespace  chk
{

/*
* hash list 特点, 与 chk_str 类似，为节省空间，当只有一条hash list的时候，直接退化成list
*/
struct  ChkHashSList : public LoggerTpl2<ChkHashSList>
{
        union
        {
                ChkSlistNode  list;
                ChkPtr    buckets;
        };

        static const int MAX_BUCKETS = CHK_MAX_CONTINUS_LEN / sizeof(ChkSlistNode);

        void init()
        {
                chk_slist_init(&list);
                list.next._size = 0;
        }
        void uninit(ChkIndex* chk_index)
        {
                if (list.next._size > 0)
                {
                        chk_index->free(buckets);
                }
        }

        int  bucket_size()
        {
                if (list.next._size == 0)
                        return  1;
                return  CHK_SIZE(list.next._size) >> 2;//= /sizeof(ChkSList)
        }

        ChkSlistNode*  get_list(ChkIndex* chk_index, uint32_t key_hash)
        {
                int  bucket_num = bucket_size();
                if (bucket_num == 1)
                {
                        return &list;
                }
                else {
                        return chk_ptr2addr(chk_index, buckets, ChkSlistNode) 
                                        + (key_hash % bucket_num);
                }
        }

        //last param used for callback+data
        //TEqFunc = bool(chk_index, void*, TTarget& key, void*)
        template<typename TEqFunc, typename TTarget>
        std::pair<ChkPtr, ChkSlistNode*> find(ChkIndex* chk_index, int container_offset
                        , TEqFunc& eq_func, const TTarget& key, uint32_t key_hash, void* data=NULL)
        {
                ChkSlistNode* target_list = get_list(chk_index, key_hash);

                ChkPtr chkptr;
                ChkSlistNode *pos, *pre;
                chk_slist_for_each_pre(chkptr, pos, pre, target_list, chk_index)
                {
                        if (eq_func(chk_index, (char*)pos - container_offset, key, data))
                        {
                                return  std::pair<ChkPtr, ChkSlistNode*>(chkptr, pre);
                        }
                }
                
                return  std::pair<ChkPtr, ChkSlistNode*>(CHK_NULL, NULL);
        }

        void  insert(ChkIndex* chk_index, ChkAddrChk* new_node, uint32_t key_hash)
        {
                ChkSlistNode* target_list = get_list(chk_index, key_hash);
                chk_slist_push(new_node, target_list);
        }

        int  erase(ChkIndex* chk_index, ChkAddrChk* node, uint32_t key_hash)
        {
                ChkSlistNode* target_list = get_list(chk_index, key_hash);
                return chk_slist_erase(target_list, chk_index, node);
        }

        //THashFunc = uint32_t(chk_index, void*, void*)
        template<typename THashFunc>
        int  resize_buckets(ChkIndex* chk_index, int new_size
                        , THashFunc& hash_func, int container_offset, void* data=NULL)
        {
                int  bucket_num = bucket_size();
                if (bucket_num == new_size) 
                {
                        DEBUG_LOG("same size=" << new_size);
                        return 0;
                }
                assert(new_size > 0);

                ChkSList  single_list;
                ChkSList* new_buckets = NULL;
                ChkPtr new_ptr, ptr_iter, ptr_tmp;
                ChkAddrChk  addr_chk;
                
                if (new_size > 1)
                {
                        new_ptr = chk_index->alloc(new_size * sizeof(ChkSList));
                        if (new_ptr.is_null())
                                return -1;

                        new_size = CHK_SIZE(new_ptr._size) / sizeof(ChkSList);
                        new_buckets = chk_ptr2addr(chk_index, new_ptr, ChkSList);

                        DEBUG_LOG("new_size=" << new_size);
                        for (int i = 0; i < new_size; ++i)
                        {
                                chk_slist_init(new_buckets + i);
                        }
                }
                else {
                        chk_slist_init(&single_list);
                        new_buckets = &single_list;
                        DEBUG_LOG("new_size=" << 1);
                }

                for (int i = 0; i < bucket_num; ++i)
                {
                        ChkSList* old_list = get_list(chk_index, i), *new_list;
                        ChkSlistNode *pos, *tmp;
                        uint32_t key_hash;
                        
                        chk_slist_for_each_safe(ptr_iter, ptr_tmp, pos, tmp, old_list, chk_index)
                        {
                                key_hash = hash_func(chk_index, (char*)pos - container_offset, data);
                                new_list = new_buckets + key_hash % new_size;

                                addr_chk = chk_ptr2addrchk(chk_index, ptr_iter);
                                chk_slist_push(&addr_chk, new_list);
                        }
                }

                if (bucket_num > 1)
                {
                        chk_index->free(buckets);
                }

                if (new_size > 1)
                {
                        buckets = new_ptr;
                        DEBUG_LOG("hash list new ptr=" << buckets);
                }
                else {
                        list = single_list;
                        list.next._size = 0;
                }

                return  0;
        }
};

}
}

#endif

