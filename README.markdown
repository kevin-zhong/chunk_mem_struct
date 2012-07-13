Name
====

Ӣ�ģ�chunk_offset_mem_struct_lib
���ģ����ڿ��ƫ�Ƶ�ͨ�����ݼܹ���

Description
===========
1��	һ���ʺ��ڸ��ֽ��ʵ�ͨ�õ����ݽṹ��
		���磬�����ڴ棬�����ڴ棬Ӳ�̣��ܵ���socket �ȵȣ�
		���������ڴ��Ӳ���Լ����������еı�ʾһ����
		�����Ͳ���Ҫ�κα�������֤�����ڸ������֮��ͨ����ͨ��
2��	ҵ�����ݽ�����1���棬�����Լ���ȫ�����Ļ�����
		���������е��ڴ�������е��ڴ�ҳ�棨������ҵ�����ݵ��ڴ�ҳ����ȫ�ֿ�����
		��Ȼ��Щҵ�����ݿ϶�������΢�е�󣬷�����е�ţ����ɱ���ˣ�
	

�������ŵ㣺
1��	�ڴ������ˣ�������ʲô�ڴ�й©������ĳ��ҵ�����ݣ�ֻ�Ǽ򵥵Ľ�
		����ռ�е��ڴ�ҳ��һһ�ͷż��ɣ��ͺñȲ���ϵͳ���������Դһ����
		��ʹ�����ڴ�й©�����ս���һ��������Դ�϶�ȫ���գ�
2��	���Լ�һЩ���룬�Ϳ��Դ����ǳ�ǿ׳�ͽ�׳�Ĺ����ڴ���ƣ�

Usage
=====
{
        ChkIndex  chk_index;
        ChkPtr chk_ptr, chkptr_tmp;

        ChkStr  test_change_name, test_change2;
        test_change_name.init();
        test_change2.init();
        
        const char* test_strs[] = {"fdsafasdgadsgsdaffads",
                "'", "��", "133355", "fdsafasd", "fdsafas", "fdsafasd", 
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

1����Դ���ڴ棩�������걸
2�����������ݽṹ������ɣ�������string����������˫������hash list��

ToDo
======
1���������Ƶײ���Դ������������Դ������Ҫ�����ͷſ����ڴ棩
2�������������ݽṹ�����磺vector��map