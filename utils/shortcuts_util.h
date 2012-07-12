#ifndef  __SHORTCUTS_UTIL_20101116
#define __SHORTCUTS_UTIL_20101116

#include <assert.h>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#define  ASSERT(LINE) bool ret = (LINE); assert(ret);
#define  SH_ASSERT_FALSE(LINE) bool ret = (LINE); assert(!ret);

#define  ARRAY_SIZE(a) (sizeof(a) /sizeof(a[0]))

#define  FOR_EACH_A(a, i) \
        for (size_t i = 0; i < ARRAY_SIZE(a); ++i)
#define  FOR_EACH_A_R(a, i) \
        for (int i = (int)ARRAY_SIZE(a)-1; i >= 0; --i)

#define  FOR_EACH_V(v, i) \
        for (size_t i = 0; i < (v).size(); ++i)
#define  FOR_EACH_V_R(v, i) \
        for (int i = (int)(v).size()-1; i >= 0; --i)

#define  SNPRINTF_APPEND(buf_arr, size_val, args...)  size_val += snprintf(buf_arr + size_val, sizeof(buf_arr)-1-size_val, args)

#define SAFE_DELETE(a) do { \
        if (a != NULL) {    \
            delete a;       \
            a = NULL;       \
        }                   \
    } while (false)

#define SAFE_DELETE_ARRAY(a) do { \
        if (a != NULL) {    \
            delete[] a;     \
            a = NULL;       \
        }                   \
    } while (false)


#define  _REPEATE_MACRO(z, cnt, macro)  macro(cnt)
#define  REPEATE_MACRO(n, macro) BOOST_PP_REPEAT(n, _REPEATE_MACRO, macro)


#define my_offsetof(type, member) ((size_t) (&((type *)1)->member)-1)
#ifndef offsetof
#define offsetof(type, member) my_offsetof(type, member)
#endif

#define my_container_of(ptr, type, member) ({          \
                                                 const typeof(((type *)1)->member) * __mptr = (ptr);    \
                                                 (type *)((char *)__mptr - my_offsetof(type, member)); })
#ifndef container_of
#define container_of(ptr, type, member) my_container_of(ptr, type, member)
#endif                                                 


/*
* check -> ret
*/
#define  CHCK_R(LINE) if (LINE) return;
#define  CHCK_RV(LINE, rv) if (LINE) return rv;

/*
*  check -> log -> ret(none)
*/

#define  CHCK_TRACE_R(LINE, e) if (LINE) \
    { \
        SH_LOG_TRACE(, e);\
        return; \
    }
#define  CHCK_DEBUG_R(LINE, e) if (LINE) \
    { \
        SH_LOG_DEBUG(e);\
        return; \
    }
#define  CHCK_INFO_R(LINE, e) if (LINE) \
    { \
        SH_LOG_INFO(e);\
        return; \
    }
#define  CHCK_WARN_R(LINE, e) if (LINE) \
    { \
        SH_LOG_WARN(e);\
        return; \
    }
#define  CHCK_ERROR_R(LINE, e) if (LINE) \
    { \
        SH_LOG_ERROR(e);\
        return; \
    }

/*
*  check -> log -> ret(val)
*/

#define  CHCK_TRACE_RV(LINE, rv, e) if (LINE) \
    { \
        SH_LOG_TRACE(, e);\
        return rv;\
    }
#define  CHCK_DEBUG_RV(LINE, rv, e) if (LINE) \
    { \
        SH_LOG_DEBUG(e);\
        return rv;\
    }
#define  CHCK_INFO_RV(LINE, rv, e) if (LINE) \
    { \
        SH_LOG_INFO(e);\
        return rv;\
    }
#define  CHCK_WARN_RV(LINE, rv, e) if (LINE) \
    { \
        SH_LOG_WARN(e);\
        return rv;\
    }
#define  CHCK_ERROR_RV(LINE, rv, e) if (LINE) \
    { \
        SH_LOG_ERROR(e);\
        return rv;\
    }

/*
* defines ...... so bt !!
*/
#define  ATTR_COPY(target, src, attr_name) (target).attr_name = (src).attr_name

#define  ATTR_COPY_2(target, src, attr1, attr2) \
    ATTR_COPY(target, src, attr1); ATTR_COPY(target, src, attr2)
#define  ATTR_COPY_3(target, src, attr1, attr2, attr3) \
    ATTR_COPY_2(target, src, attr1, attr2); ATTR_COPY(target, src, attr3)
#define  ATTR_COPY_4(target, src, attr1, attr2, attr3, attr4) \
    ATTR_COPY_3(target, src, attr1, attr2, attr3); ATTR_COPY(target, src, attr4)
#define  ATTR_COPY_5(target, src, attr1, attr2, attr3, attr4, attr5) \
    ATTR_COPY_4(target, src, attr1, attr2, attr3, attr4); ATTR_COPY(target, src, attr5)
#define  ATTR_COPY_6(target, src, attr1, attr2, attr3, attr4, attr5, attr6) \
    ATTR_COPY_5(target, src, attr1, attr2, attr3, attr4, attr5); ATTR_COPY(target, src, attr6)
#define  ATTR_COPY_7(target, src, attr1, attr2, attr3, attr4, attr5, attr6, attr7) \
    ATTR_COPY_6(target, src, attr1, attr2, attr3, attr4, attr5, attr6); ATTR_COPY(target, src, attr7)

#define THRIFT_SQL_GET_SET(msg_member, field_name) \
    { \
        bool ret = _recordset->get_field_value(field_name, msg_member); \
        if(!ret) {LOG_WARN(logger,"get field value failed field:"<<field_name<<" member:"<<#msg_member);} \
    }\

#define SQL_GET_FIELD(msg, field_name) \
    { \
        bool ret = _recordset->get_field_value(# field_name, msg->field_name); \
        if (!ret) { LOG_WARN(logger, "get field value failed field:" <<# field_name); } \
    } \

#define SQL_GET_FIELD_2(msg, f1, f2) \
    SQL_GET_FIELD(msg, f1) \
    SQL_GET_FIELD(msg, f2)
#define SQL_GET_FIELD_3(msg, f1, f2, f3) \
    SQL_GET_FIELD_2(msg, f1, f2) \
    SQL_GET_FIELD(msg, f3)
#define SQL_GET_FIELD_4(msg, f1, f2, f3, f4) \
    SQL_GET_FIELD_3(msg, f1, f2, f3) \
    SQL_GET_FIELD(msg, f4)
#define SQL_GET_FIELD_5(msg, f1, f2, f3, f4, f5) \
    SQL_GET_FIELD_4(msg, f1, f2, f3, f4) \
    SQL_GET_FIELD(msg, f5)
#define SQL_GET_FIELD_6(msg, f1, f2, f3, f4, f5, f6) \
    SQL_GET_FIELD_5(msg, f1, f2, f3, f4, f5) \
    SQL_GET_FIELD(msg, f6)
#define SQL_GET_FIELD_7(msg, f1, f2, f3, f4, f5, f6, f7) \
    SQL_GET_FIELD_6(msg, f1, f2, f3, f4, f5, f6) \
    SQL_GET_FIELD(msg, f7)

#endif

