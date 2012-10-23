#ifndef _CHK_TESTOR_H
#define _CHK_TESTOR_H
/*
 ******************************** UNIT TEST ********************************
 */
#include <gtest/gtest.h>
#include <vector>
#include <map>
#include <string>

#include "chk_string.h"
#include "chk_mem_mgr.h"
#include "chk_list.h"
#include "chk_hash_list.h"
#include "chk_base.h"
#include "chk_memalign.h"

using namespace testing;
using namespace comm::chk;

class ChkTestorCase : public testing::Test
{
public:
        virtual ~ChkTestorCase()
        {
        }
        virtual void SetUp()
        {
                ::srandom(time(NULL));
                
                int  preset_size[3] = {16, 32, 64};
                ChkMemMgr::preset_free_sizes(preset_size);

                if (::random() % 2)
                {
                        std::cout << "use memalign now !" << std::endl;
                        
                        ChkMemalign::instance()->init(8, NULL);
                }
                ChkDefaultMemAlloc::instance()->init(::random() % 2);
        }
};

struct  StrNode
{
        ChkStr  data;
        ChkSlistNode  link;

        struct  hash_func
        {
                uint32_t operator()(ChkIndex* chk_index, void* str_node, void* data) const
                {
                        unsigned long __h = 0;
                        for (const char *__s = ((StrNode*)str_node)->data.c_str(chk_index); *__s; ++__s)
                                __h = 5 * __h + *__s;
                        return size_t(__h);
                }
        };
        struct  hash_eq
        {
                bool  operator()(ChkIndex* chk_index, void* str_node, const std::string& rh, void* data) const
                {
                        ChkStr*  data_ptr = &((StrNode*)str_node)->data;
                        return  data_ptr->length() == rh.length() 
                                        && strcmp(data_ptr->c_str(chk_index), rh.c_str()) == 0;
                }
        };
};

std::string  random_str(int i)
{
        static char buf[1024];
        bzero(buf, sizeof(buf));

        int offset = sprintf(buf, "%d_", i);
        
        int try_cnts = ::random() % 18;
        for (int i = 0; i < try_cnts; ++i)
        {
                offset += sprintf(buf + offset, "%ld", ::random());
        }
        return  buf;
}

TEST_F(ChkTestorCase, TestStrList)
{
        ChkIndex  chk_index;
        ChkPtr chk_ptr, chkptr_tmp;

        ChkStr  test_change_name, test_change2;
        test_change_name.init();
        test_change2.init();
        
        const char* test_strs[] = {"fdsafasdgadsgsdaffads",
                "'", "¡£", "133355", "fdsafasd", "fdsafas", "fdsafasd", 
                "aa", "bbbbb", "c", "dddddd", "123456789", "1234567", "123456"};

        for (int try_cnt = 0; try_cnt < 4; ++try_cnt)
        {
                FOR_EACH_A(test_strs, i)
                {
                        std::cout << "set name=" << test_strs[i] << std::endl;
                        
                        test_change_name.set_data(test_strs[i], strlen(test_strs[i]), &chk_index);
                        ASSERT_EQ(std::string(test_strs[i]), 
                                        std::string(test_change_name.c_str(&chk_index)));

                        int itmp = ARRAY_SIZE(test_strs) - 1 - i;
                        std::cout << "set name2=" << test_strs[itmp] << std::endl;
                        test_change2.set_data(test_strs[itmp], strlen(test_strs[itmp]), &chk_index);
                        ASSERT_EQ(std::string(test_strs[itmp]), 
                                        std::string(test_change2.c_str(&chk_index)));
                }
        }
        
        ChkPtr chk_ptr_list = chk_index.alloc(sizeof(ChkSList));
        ASSERT_TRUE(!chk_ptr_list.is_null());

        ChkSList*  str_list = chk_ptr2addr(&chk_index, chk_ptr_list, ChkSList);
        //must init list
        chk_slist_init(str_list);

        ChkAddrChk addrchk;

        std::string str_cmp[12800];
        for (size_t i = 0; i < ARRAY_SIZE(str_cmp); ++i)
        {
                str_cmp[i] = random_str(i);
                
                chk_ptr = chk_index.alloc(sizeof(StrNode));
                ASSERT_TRUE(!chk_ptr.is_null());
                std::cout<<"alloc node ptr=" << chk_ptr << std::endl;

                StrNode* str_node = chk_ptr2addr(&chk_index, chk_ptr, StrNode);
                str_node->data.init();
                str_node->data.set_data(str_cmp[i].c_str(), str_cmp[i].length(), &chk_index);

                chk_set_addrchk(addrchk, (char*)&str_node->link, 
                                chk_ptr2chk(&chk_index, chk_ptr));
                
                chk_slist_push(&addrchk, str_list);
        }

        int cnt = 0;
        ChkSlistNode *pos, *tmp;
        chk_slist_for_each(chk_ptr, pos, str_list, &chk_index)
        {
                StrNode* str_node = container_of(pos, StrNode, link);
                std::string& str_tmp = str_cmp[ARRAY_SIZE(str_cmp) - 1 -cnt];
                
                ASSERT_EQ(std::string(str_node->data.c_str(&chk_index)), str_tmp);

                str_tmp = random_str(cnt);
                str_node->data.set_data(str_tmp.c_str(), str_tmp.length(), &chk_index);
                ++cnt;
        }

        ASSERT_EQ(cnt, (int)ARRAY_SIZE(str_cmp));
        cnt = 0;

        chk_slist_for_each_safe(chk_ptr, chkptr_tmp, pos, tmp, str_list, &chk_index)
        {
                StrNode* str_node = container_of(pos, StrNode, link);
                std::string& str_tmp = str_cmp[ARRAY_SIZE(str_cmp) - 1 -cnt];
                
                ASSERT_EQ(std::string(str_node->data.c_str(&chk_index), str_node->data.length()), 
                                str_tmp);
                
                chk_slist_erase_node(&chk_index, str_list, pos);
                
                str_node->data.uninit(&chk_index);

                chk_ptr.sub(offsetof(StrNode, link));
                std::cout<<"free node ptr=" << chk_ptr << std::endl;
                
                chk_index.free(chk_ptr, sizeof(StrNode));
                ++cnt;
        }

        ASSERT_TRUE(chk_slist_empty(str_list));

        std::cout << "last mem_mgr=" << chk_index.mem_mgr()<< std::endl;

        chk_index.destruct();
}


TEST_F(ChkTestorCase, TestHashList)
{
        ASSERT_EQ((int)sizeof(ChkHashSList), 4);
        
        ChkIndex  chk_index;
        ChkPtr chk_ptr;

        ChkPtr chk_ptr_hash = chk_index.alloc(sizeof(ChkSList));
        ASSERT_TRUE(!chk_ptr_hash.is_null());

        StrNode::hash_func hash_func;
        StrNode::hash_eq hash_eq;
        
        ChkHashSList*  hash_list = chk_ptr2addr(&chk_index, chk_ptr_hash, ChkHashSList);
        hash_list->init();
        
        hash_list->resize_buckets(&chk_index, 512, hash_func, offsetof(StrNode, link));
        ASSERT_EQ(hash_list->bucket_size(), 512);

        ChkAddrChk addrchk;

        std::string str_cmp[2048];
        uint32_t hash_vals[ARRAY_SIZE(str_cmp)];
        
        for (size_t i = 0; i < ARRAY_SIZE(str_cmp); ++i)
        {
                str_cmp[i] = random_str(i);
                
                chk_ptr = chk_index.alloc(sizeof(StrNode));
                ASSERT_TRUE(!chk_ptr.is_null());

                StrNode* str_node = chk_ptr2addr(&chk_index, chk_ptr, StrNode);
                str_node->data.init();
                str_node->data.set_data(str_cmp[i].c_str(), str_cmp[i].length(), &chk_index);

                hash_vals[i] = hash_func(&chk_index, str_node, NULL);
                //std::cout << "test str=" << str_cmp[i] 
                                //<< ", hash_val=" << hash_vals[i] << std::endl;

                chk_set_addrchk(addrchk, (char*)&str_node->link, 
                                chk_ptr2chk(&chk_index, chk_ptr));

                hash_list->insert(&chk_index, &addrchk, hash_vals[i]);
        }

        std::cout << "find in hash list now " << std::endl;

        std::pair<ChkPtr, ChkSlistNode*> pair_ret;

        for (int cnt = 0; cnt < 5; ++cnt)
        {
                for (size_t i = 0; i < ARRAY_SIZE(str_cmp); ++i)
                {
                        pair_ret = hash_list->find(&chk_index, offsetof(StrNode, link), 
                                        hash_eq, str_cmp[i], hash_vals[i]);
                        ASSERT_TRUE(!pair_ret.first.is_null());

                        StrNode* str_node = container_of(chk_ptr2addr(&chk_index, pair_ret.first, ChkSlistNode)
                                                , StrNode, link);
                        ASSERT_EQ(std::string(str_node->data.c_str(&chk_index)), str_cmp[i]);
                }

                int buckets_num = std::min((int)random() % 1024 + 256, ChkHashSList::MAX_BUCKETS);
                
                int ret = hash_list->resize_buckets(&chk_index, buckets_num, 
                                hash_func, offsetof(StrNode, link));
                ASSERT_EQ(ret, 0);
                
                std::cout << "resize to buckets, target=" << buckets_num 
                                << ", real=" << hash_list->bucket_size() << std::endl;
        }

        chk_index.destruct();
}


struct  IntNode
{
        int  val;
        ChklistNode  link;

        bool  operator< (const IntNode& rh) const
        {
                return  val < rh.val;
        }
};

struct int_node_cmp
{
        int operator()(const IntNode& lf, const IntNode& rh) const
        {
                return  lf.val - rh.val;
        }
};

TEST_F(ChkTestorCase, TestDoubleList)
{
        ChkIndex  chk_index;

        ChkPtr chk_ptr_list = chk_index.alloc(sizeof(ChkList));
        ASSERT_TRUE(!chk_ptr_list.is_null());

        ChkAddrChk addr_chk_list = chk_ptr2addrchk(&chk_index, chk_ptr_list);
        chk_list_init(&addr_chk_list);
        ASSERT_TRUE(chk_list_empty(&addr_chk_list));

        ChkAddrChk addr_chk_node;
        ChkPtr  node_ptrs[128];
        int  vals[ARRAY_SIZE(node_ptrs)];

        //insert
        FOR_EACH_A(node_ptrs, i)
        {
                node_ptrs[i] = chk_index.alloc(sizeof(IntNode));
                ASSERT_TRUE(!node_ptrs[i].is_null());

                vals[i] = (random() & ((1<<28) - 1)) + 1;
                printf("vals[%ld]=%d\n", i , vals[i]);
                chk_ptr2addr(&chk_index, node_ptrs[i], IntNode)->val = vals[i];

                node_ptrs[i].add(offsetof(IntNode, link));

                addr_chk_node = chk_ptr2addrchk(&chk_index, node_ptrs[i]);
                chk_list_add_tail(&addr_chk_node, &addr_chk_list, &chk_index);
        }

        int cnt = 0;
        ChkPtr chkptr, tmp_ptr;
        ChklistNode *pos, *tmp;

        //check
        chk_list_for_each(chkptr, pos, addr_chk_list.addr, &chk_index)
        {
                IntNode* int_node = container_of(pos, IntNode, link);
                ASSERT_EQ(int_node->val, vals[cnt]);
                cnt++;
        }
        cnt = ARRAY_SIZE(vals) - 1;
        chk_list_for_each_r(chkptr, pos, addr_chk_list.addr, &chk_index)
        {
                IntNode* int_node = container_of(pos, IntNode, link);
                ASSERT_EQ(int_node->val, vals[cnt]);
                cnt--;
        }

        //iterator
        typedef  ChkListIterator<IntNode> list_iterator;
        
        ChkList* chk_list = (ChkList*)(addr_chk_list.addr);
        list_iterator iter_end(chk_list);
        list_iterator iter(offsetof(IntNode, link), 
                        CHK_LIST_BEGIN_PTR(chk_list), 
                        &chk_index, chk_list), iter_tmp;

        cnt = 0;
        for (; iter != iter_end; ++iter)
        {
                IntNode* int_node = &(*iter);
                ASSERT_EQ(int_node->val, vals[cnt]);
                cnt++;
        }
        
        iter = list_iterator(offsetof(IntNode, link), 
                        CHK_LIST_RBEGIN_PTR(chk_list), 
                        &chk_index, chk_list);
        cnt = ARRAY_SIZE(vals) - 1;
        for (; iter != iter_end; --iter)
        {
                IntNode* int_node = &(*iter);
                ASSERT_EQ(int_node->val, vals[cnt]);
                cnt--;
        }

        //erase
        for (int i = 0; i < (int)ARRAY_SIZE(vals)/2; ++i)
        {
                int random_index = random() % ARRAY_SIZE(vals);
                for (; vals[random_index] == 0; random_index = (random_index+1) % ARRAY_SIZE(vals))
                        ;

                addr_chk_node = chk_ptr2addrchk(&chk_index, node_ptrs[random_index]);
                ASSERT_TRUE(!chk_list_node_unlinked(CHK_LIST_ADDR(&addr_chk_node)));
                
                chk_list_del(&addr_chk_node, &chk_index);

                vals[random_index] = 0;
        }

        chk_list_for_each_safe(chkptr, tmp_ptr, pos, tmp, addr_chk_list.addr, &chk_index)
        {
                addr_chk_node = chk_ptr2addrchk(&chk_index, chkptr);
                
                chk_list_del(&addr_chk_node, &chk_index);
        }

        ASSERT_TRUE(chk_list_empty(&addr_chk_list));

        // merge
        chkptr = chk_index.alloc(sizeof(ChkList));
        ChkAddrChk other_chk_list = chk_ptr2addrchk(&chk_index, chkptr);
        chk_list_init(&other_chk_list);

        std::set<int>  set_tmp;
        for (size_t i = 0; i < ARRAY_SIZE(vals); ++i)
        {
                if (!set_tmp.insert(vals[i]).second)
                {
                        vals[i] += 1;
                        --i;
                }
        }
        
        size_t  mid_index = ARRAY_SIZE(vals) / 2;
        std::sort(vals, vals + mid_index);
        std::sort(vals + mid_index, vals + ARRAY_SIZE(vals));

        for (size_t i = 0; i < ARRAY_SIZE(vals); ++i)
                std::cout << "vals[" << i << "]=" << vals[i]<<std::endl;
        
        for (size_t i = 0; i < ARRAY_SIZE(vals); ++i)
        {
                chkptr = chk_index.alloc(sizeof(IntNode));
                ASSERT_TRUE(!chkptr.is_null());

                chk_ptr2addr(&chk_index, chkptr, IntNode)->val = vals[i];
                
                chkptr.add(offsetof(IntNode, link));
                addr_chk_node = chk_ptr2addrchk(&chk_index, chkptr);
                
                chk_list_add_tail(&addr_chk_node, 
                                i < mid_index ? &addr_chk_list : &other_chk_list, 
                                &chk_index);
        }

        int_node_cmp  func_cmp;
        chk_list_merge<IntNode>(chk_list, (ChkList*)CHK_LIST_ADDR(&other_chk_list), 
                        func_cmp, &chk_index, (int)offsetof(IntNode, link));

        iter_tmp = list_iterator(offsetof(IntNode, link), 
                        CHK_LIST_BEGIN_PTR(chk_list), 
                        &chk_index, chk_list);
        
        for (iter = iter_tmp; iter != iter_end; ++iter)
        {
                IntNode* int_node = &(*iter);
                std::cout<<"sorted val=" << int_node->val << std::endl;
        }

        std::sort(vals, vals + ARRAY_SIZE(vals));
        
        cnt = 0;
        for (iter = iter_tmp; iter != iter_end; ++iter)
        {
                IntNode* int_node = &(*iter);
                ASSERT_EQ(int_node->val, vals[cnt]);
                cnt++;
        }

        //test chk_listnode2chk TODO...
        for (iter = iter_tmp; iter != iter_end; )
        {
                iter_tmp = iter++;
                
                addr_chk_node = chk_ptr2addrchk(&chk_index, iter_tmp.link_ptr);
                ASSERT_EQ(addr_chk_node.chk, chk_listnode2chk(iter_tmp.pnode, &chk_index));

                chk_list_del(&addr_chk_node, &chk_index);
                ASSERT_EQ(addr_chk_node.chk, chk_listnode2chk(iter_tmp.pnode, &chk_index));
        }

        ASSERT_TRUE(chk_list_empty(&addr_chk_list));

        chk_index.destruct();

        std::cout << "list test success end !!" << std::endl;
}


TEST_F(ChkTestorCase, TestShrink)
{
        ChkIndex  chk_index;
        ChkPtr chk_ptr_list = chk_index.alloc(sizeof(ChkList));
        
        ChkAddrChk addr_chk_list = chk_ptr2addrchk(&chk_index, chk_ptr_list), addr_chk;
        chk_list_init(&addr_chk_list);

        ChkPtr  node_ptrs[10240];
        int  vals[ARRAY_SIZE(node_ptrs)];

        int shrink_time[ARRAY_SIZE(node_ptrs)] = {0};
        int shrink_cnts = random() % 64 + 8;
        for (int i = 0; i < shrink_cnts; ++i)
        {
                int shrink_index = random() % ARRAY_SIZE(shrink_time);
                
                if (shrink_time[shrink_index] == 0)
                {
                        shrink_time[shrink_index] = 1;
                        std::cout<<"shrink_index=" << shrink_index << std::endl;
                }
        }
        
        //insert
        for (size_t i = 0; i < ARRAY_SIZE(node_ptrs); ++i)
        {
                if (shrink_time[i])
                {
                        switch (i % 3)
                        {
                                case 0:
                                chk_index.shrink();
                                if (i > 0)
                                {
                                        addr_chk = chk_ptr2addrchk(&chk_index, node_ptrs[i-1]);
                                        //test chk_addr2chk
                                        ChkMetadata* chk_get = chk_addr2chk(addr_chk.addr);
                                        ASSERT_EQ(chk_get, addr_chk.chk);
                                }
                                ASSERT_EQ(chk_index.enlarge(), 0);
                                addr_chk_list = chk_ptr2addrchk(&chk_index, chk_ptr_list);
                                break;

                                case 1:
                                ASSERT_EQ(chk_index.compress(), 0);
                                ASSERT_EQ(chk_index.enlarge(), 0);
                                addr_chk_list = chk_ptr2addrchk(&chk_index, chk_ptr_list);
                                break;

                                case 2:
                                std::cout << "compress->uncompress->compress->enlarge..." << std::endl;
                                ASSERT_EQ(chk_index.compress(), 0);
                                chk_index.boot_addr();
                                ASSERT_EQ(chk_index.compress(), 0);
                                ASSERT_EQ(chk_index.enlarge(), 0);
                                addr_chk_list = chk_ptr2addrchk(&chk_index, chk_ptr_list);
                                break;
                        }
                }
                
                node_ptrs[i] = chk_index.alloc(sizeof(IntNode));

                vals[i] = (random() & ((1<<28) - 1)) + 1;
                chk_ptr2addr(&chk_index, node_ptrs[i], IntNode)->val = vals[i];

                node_ptrs[i].add(offsetof(IntNode, link));

                addr_chk = chk_ptr2addrchk(&chk_index, node_ptrs[i]);
                chk_list_add(&addr_chk, &addr_chk_list, &chk_index);

                //test chk_addr2chk
                ChkMetadata* chk_get = chk_addr2chk(addr_chk.addr);
                ASSERT_EQ(chk_get, addr_chk.chk);
        }

        int cnt = 0;
        ChkPtr chkptr;
        ChklistNode *pos;

        //check
        chk_list_for_each_r(chkptr, pos, addr_chk_list.addr, &chk_index)
        {
                IntNode* int_node = container_of(pos, IntNode, link);
                ASSERT_EQ(int_node->val, vals[cnt]);
                cnt++;
        }

        ASSERT_EQ(chk_index.compress(), 0);

        if (random() % 2)
        {
                std::cout << "uncompress again !!" << std::endl;
                
                //after compress and revert, check
                chk_index.boot_addr();

                addr_chk_list = chk_ptr2addrchk(&chk_index, chk_ptr_list);
                cnt = 0;
                chk_list_for_each_r(chkptr, pos, addr_chk_list.addr, &chk_index)
                {
                        IntNode* int_node = container_of(pos, IntNode, link);
                        ASSERT_EQ(int_node->val, vals[cnt]);
                        cnt++;
                }
        }
        else {
                std::cout << "dont uncompress again !!" << std::endl;
        }

        chk_index.destruct();
        
        std::cout << "end TestShrink now!" << std::endl;
}


TEST_F(ChkTestorCase, TestSysAlloc)
{
        size_t  alloc_size = 0;
        std::string  test_strs[100];
        return;
        
        for (int i = 0; i < 1; ++i)
        {
                for (int k = 0; k < 100; ++k)
                {
                        test_strs[k] = random_str(k);
                }
        }
}

#endif
