Name
====

英文：chunk_offset_mem_struct_lib
中文：基于块和偏移的通用数据架构库

Description
===========
1，	一种适合于各种介质的通用的数据结构，
		比如，自有内存，共享内存，硬盘，管道，socket 等等，
		即数据在内存和硬盘以及其他介质中的表示一样，
		这样就不需要任何编解码而保证数据在各类介质之中通畅流通；
2，	业务数据建立在1上面，且有自己完全独立的环境，
		包括：独有的内存管理，独有的内存页面（和其他业务数据的内存页面完全分开）；
		当然这些业务数据肯定都会稍微有点大，否则就有点牛刀来杀鸡了；
	

其他的优点：
1，	内存管理简单了，不会有什么内存泄漏；回收某个业务数据，只是简单的将
		其所占有的内存页面一一释放即可；就好比操作系统管理进程资源一样，
		即使进程内存泄漏，最终进程一结束，资源肯定全回收；
2，	另稍加一些代码，就可以带来非常强壮和健壮的共享内存设计；

Usage
=====
{
        ChkIndex  chk_index;
        ChkPtr chk_ptr, chkptr_tmp;

        ChkStr  test_change_name, test_change2;
        test_change_name.init();
        test_change2.init();
        
        const char* test_strs[] = {"fdsafasdgadsgsdaffads",
                "'", "。", "133355", "fdsafasd", "fdsafas", "fdsafasd", 
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
}

Status
======

1，资源（内存）管理功能完备
2，基本的数据结构部分完成，包括：string，单向链表，双向链表，hash list；

ToDo
======
1，继续完善底层资源管理，包括：资源整理（主要用于释放空闲内存）
2，加入更多的数据结构，比如：vector，map