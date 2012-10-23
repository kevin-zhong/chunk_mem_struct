#ifndef  _CHK_STRING_H
#define _CHK_STRING_H

#include "chk_base.h"
#include <strings.h>

namespace  comm { namespace  chk
{

/*
* 特点，当存储字符串长度小于8时，直接利用本地存储
*/
struct ChkStr 
{
        union
        {
                struct {
                        ChkPtr     data_ptr;
                        uint32_t   str_len:13;//real len, max<8k
                        uint32_t   padding:11;
                        uint32_t   flag:8;
                } chk_data;
                
                char  data[8];//support <=7 bytes string;
        };

        void init()
        {
                bzero(this, sizeof(ChkStr));
        }
        void uninit(ChkIndex* chk_index)
        {
                if (chk_data.flag)
                {
                        chk_index->free(chk_data.data_ptr);
                }
        }

        void clear(ChkIndex* chk_index)
        {
                if (chk_data.flag)
                {
                        chk_index->free(chk_data.data_ptr);
                }
                bzero(this, sizeof(ChkStr));
        }
        bool empty() const
        {
                return length() == 0;
        }

        size_t  length() const
        {
                if (chk_data.flag)
                        return  chk_data.str_len;
                else
                        return  strlen(data);
        }

        const char* c_str(ChkIndex* chk_index) const
        {
                if (chk_data.flag)
                        return  chk_ptr2addr(chk_index, chk_data.data_ptr, char);
                else
                        return  data;
        }

        char* c_str(ChkIndex* chk_index)
        {
                if (chk_data.flag)
                        return  chk_ptr2addr(chk_index, chk_data.data_ptr, char);
                else
                        return  data;
        }

        bool  equall(const ChkStr* rh, ChkIndex* chk_index, int ignore_case=0) const
        {
                size_t len = length();
                if (len != rh->length())
                        return false;
                
                if (ignore_case == 0)
                        return strncmp(c_str(chk_index), rh->c_str(chk_index), len) == 0;
                else
                        return strncasecmp(c_str(chk_index), rh->c_str(chk_index), len) == 0;
        }

        bool  equall_str(ChkIndex* chk_index, const char* buf, size_t blen, int ignore_case=0) const
        {
                size_t len = length();
                if (len != blen)
                        return false;
                
                if (ignore_case == 0)
                        return strncmp(c_str(chk_index), buf, len) == 0;
                else
                        return strncasecmp(c_str(chk_index), buf, len) == 0;                
        }

        static size_t hash_str(const char* data_tmp, size_t len_tmp, int ignore_case=0)
        {
                size_t __h = 0;
                for (size_t i = 0; i < len_tmp; ++i, ++data_tmp)
                {
                        __h = 5 * __h + (ignore_case ? toupper(*data_tmp) : *data_tmp);
                }
                return __h;
        }

        size_t  hash(ChkIndex* chk_index, int ignore_case=0) const
        {
                return hash_str(c_str(chk_index), length(), ignore_case);
        }

        int  set_data(const char* buf, size_t buf_len, ChkIndex* chk_index)
        {
                if (buf_len <= 7)
                {
                        if (chk_data.flag)
                        {
                                chk_index->free(chk_data.data_ptr);
                                chk_data.flag = 0;
                        }
                        memmove(data, buf, buf_len);
                        data[buf_len] = 0;
                        return  0;
                }
                
                if (chk_data.flag && chk_data.str_len >= buf_len)
                {
                        char* str_addr = c_str(chk_index);
                        memmove(str_addr, buf, buf_len);
                        
                        chk_data.str_len = buf_len;
                        str_addr[buf_len] = 0;
                        return  0;
                }

                ChkPtr new_ptr = chk_index->alloc(buf_len+1);//note, alloc more 1 byte
                if (new_ptr.is_null())
                        return -1;

                if (chk_data.flag)
                {
                        chk_index->free(chk_data.data_ptr);
                }
                else
                        chk_data.flag = 1;

                chk_data.data_ptr = new_ptr;
                chk_data.str_len = buf_len;

                char* str_addr = c_str(chk_index);
                memmove(str_addr, buf, buf_len);
                str_addr[buf_len] = 0;
                return  0;
        }
};

}
}
#endif

