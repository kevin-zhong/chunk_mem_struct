#ifndef  _CHK_LIST_H
#define _CHK_LIST_H

#include "chk_base.h"

namespace  comm { namespace  chk
{

/*
* single list
*/
struct  ChkSlistNode
{
        ChkPtr  next;
};

typedef ChkSlistNode ChkSList;

#define chk_slist_init(slist) (slist)->next.set_null()

#define chk_slist_node_reset(_slist_node) (_slist_node)->next.set_null()

#define chk_slist_empty(slist) (slist)->next.is_null()

#define chk_slist_push(addrchk, slist) do { \
        ((ChkSlistNode*)(addrchk)->addr)->next = (slist)->next; \
        (slist)->next = chk_addrchk2ptr(addrchk); \
} while (0)


#define chk_slist_erase_node(chk_index, pre_node, target_node) \
        (pre_node)->next = (target_node)->next; \
        chk_slist_node_reset(target_node);

static inline ChkAddrChk chk_slist_pop(ChkSList* slist, ChkIndex* chk_index)
{
        ChkAddrChk addr_chk = {NULL};
        
        if (chk_slist_empty(slist))
                return addr_chk;
        
        ChkSlistNode* poped = chk_ptr2addr(chk_index, slist->next, ChkSlistNode);
        addr_chk.addr = poped;
        addr_chk.chk = chk_ptr2chk(chk_index, slist->next);
        
        chk_slist_erase_node(chk_index, slist, poped);
        
        return  addr_chk;
}

#define chk_slist_for_each(chkptr, pos, slist, chk_index) \
        for (chkptr = (slist)->next, pos = chk_ptr2addr(chk_index, chkptr, ChkSlistNode); \
                pos != NULL; chkptr = pos->next, pos = chk_ptr2addr(chk_index, chkptr, ChkSlistNode))

#define chk_slist_for_each_pre(chkptr, pos, pre, slist, chk_index) \
        for (pre = slist, chkptr = (slist)->next, pos = chk_ptr2addr(chk_index, chkptr, ChkSlistNode); \
                pos != NULL; pre = pos, chkptr = pos->next, pos = chk_ptr2addr(chk_index, chkptr, ChkSlistNode))        

/*
* so btt...
*/
#define chk_slist_for_each_safe(chkptr, tmp_ptr, pos, tmp, slist, chk_index) \
        for (chkptr = (slist)->next, pos = chk_ptr2addr(chk_index, chkptr, ChkSlistNode), \
                tmp_ptr = pos ? pos->next : CHK_NULL, tmp = chk_ptr2addr(chk_index, tmp_ptr, ChkSlistNode); \
                pos != NULL; chkptr = tmp_ptr, tmp_ptr = tmp ? tmp->next : CHK_NULL, \
                pos = tmp, tmp = chk_ptr2addr(chk_index, tmp_ptr, ChkSlistNode))

static inline int chk_slist_erase(ChkSList* slist, ChkIndex* chk_index
                , ChkAddrChk* addr_chk)
{
        ChkPtr chkptr;
        ChkSlistNode* slist_node;
        ChkSlistNode* pre_node;
        
        chk_slist_for_each_pre(chkptr, slist_node, pre_node, slist, chk_index)
        {
                if (slist_node == addr_chk->addr)
                {
                        chk_slist_erase_node(chk_index, pre_node, slist_node);
                        return  0;
                }
                pre_node = slist_node;
        }
        return -1;
}

/*
* double list
*/
struct  ChklistNode
{
        ChkPtr  next;
        ChkPtr  prev;
};

typedef ChklistNode ChkList;

#define  CHK_LIST_ADDR(addr_chk) ((ChklistNode*)(addr_chk)->addr)

#define chk_list_init(addr_chk) do { \
                CHK_LIST_ADDR(addr_chk)->next = chk_addrchk2ptr(addr_chk); \
                CHK_LIST_ADDR(addr_chk)->prev = CHK_LIST_ADDR(addr_chk)->next; \
} while (0)

#define chk_list_empty(addr_chk) (CHK_LIST_ADDR(addr_chk)->next == chk_addrchk2ptr(addr_chk))

#define chk_list_node_reset(_list_node) do { \
        (_list_node)->prev.set_null(); \
        (_list_node)->next.set_null(); \
} while (0)

#define chk_list_node_unlinked(_list_node) ((_list_node)->prev.is_null() || (_list_node)->next.is_null())

static inline void chk_list_insert(ChkAddrChk *new_node
                , ChkAddrChk *prev, ChkAddrChk *next)
{
        CHK_LIST_ADDR(next)->prev = chk_addrchk2ptr(new_node);
        CHK_LIST_ADDR(new_node)->next = chk_addrchk2ptr(next);
        CHK_LIST_ADDR(new_node)->prev = chk_addrchk2ptr(prev);
        CHK_LIST_ADDR(prev)->next = CHK_LIST_ADDR(next)->prev;
}


static inline void chk_list_add(ChkAddrChk *new_node
                , ChkAddrChk *head, ChkIndex *chk_index)
{
        ChkPtr next_ptr = CHK_LIST_ADDR(head)->next;
        ChkAddrChk next = chk_ptr2addrchk(chk_index, next_ptr);
        
        chk_list_insert(new_node, head, &next);
}


static inline void chk_list_add_tail(ChkAddrChk *new_node
                , ChkAddrChk *head, ChkIndex *chk_index)
{
        ChkPtr prev_ptr = CHK_LIST_ADDR(head)->prev;
        ChkAddrChk prev = chk_ptr2addrchk(chk_index, prev_ptr);
        
        chk_list_insert(new_node, &prev, head);
}

static inline void __chk_list_del(ChkAddrChk *prev, ChkAddrChk *next)
{
        CHK_LIST_ADDR(next)->prev = chk_addrchk2ptr(prev);
        CHK_LIST_ADDR(prev)->next = chk_addrchk2ptr(next);
}


static inline void chk_list_del(ChkAddrChk *entry, ChkIndex *chk_index)
{
        if (chk_list_node_unlinked(CHK_LIST_ADDR(entry)))
                return;
        
        ChkAddrChk prev = chk_ptr2addrchk(chk_index, CHK_LIST_ADDR(entry)->prev);
        ChkAddrChk next = chk_ptr2addrchk(chk_index, CHK_LIST_ADDR(entry)->next);
        
        __chk_list_del(&prev, &next);

        chk_list_node_reset(CHK_LIST_ADDR(entry));
}

static inline void chk_list_splice(ChkAddrChk *from, ChkAddrChk *to
                , ChkIndex *chk_index)
{
        if (chk_list_empty(from))
                return;
        
        ChkAddrChk first = chk_ptr2addrchk(chk_index, CHK_LIST_ADDR(from)->next);
        ChkAddrChk last = chk_ptr2addrchk(chk_index, CHK_LIST_ADDR(from)->prev);
        ChkAddrChk at = chk_ptr2addrchk(chk_index, CHK_LIST_ADDR(to)->prev);

        CHK_LIST_ADDR(&first)->prev = chk_addrchk2ptr(&at);
        CHK_LIST_ADDR(&at)->next = chk_addrchk2ptr(&first);

        CHK_LIST_ADDR(&last)->next = chk_addrchk2ptr(to);
        CHK_LIST_ADDR(to)->prev = chk_addrchk2ptr(&last);

        chk_list_init(from);
}


#define __chk_list_for_each(chkptr, pos, list, chk_index, _dirct) \
        for (chkptr = ((ChkList*)(list))->_dirct, pos = chk_ptr2addr(chk_index, chkptr, ChklistNode); \
                pos != (ChkList*)(list); chkptr = pos->_dirct, pos = chk_ptr2addr(chk_index, chkptr, ChklistNode))

#define chk_list_for_each(chkptr, pos, list, chk_index) \
        __chk_list_for_each(chkptr, pos, list, chk_index, next)

#define chk_list_for_each_r(chkptr, pos, list, chk_index) \
        __chk_list_for_each(chkptr, pos, list, chk_index, prev)

#define __chk_list_for_each_safe(chkptr, tmp_ptr, pos, tmp, list, chk_index, _dirct) \
        for (chkptr = ((ChkList*)(list))->_dirct, pos = chk_ptr2addr(chk_index, chkptr, ChklistNode), \
                tmp_ptr = pos->_dirct, tmp = chk_ptr2addr(chk_index, tmp_ptr, ChklistNode); \
                pos != (ChkList*)(list); chkptr = tmp_ptr, pos = tmp, \
                tmp_ptr = tmp->_dirct, tmp = chk_ptr2addr(chk_index, tmp_ptr, ChklistNode))
             
#define chk_list_for_each_safe(chkptr, tmp_ptr, pos, tmp, list, chk_index) \
        __chk_list_for_each_safe(chkptr, tmp_ptr, pos, tmp, list, chk_index, next)
        
#define chk_list_for_each_safe_r(chkptr, tmp_ptr, pos, tmp, list, chk_index) \
        __chk_list_for_each_safe(chkptr, tmp_ptr, pos, tmp, list, chk_index, prev)

/*
* very usefull define
*/
#define chk_listnode2chk(listnode, chkindex) ((listnode)->prev.is_null() ? \
                chk_addr2chk(listnode) : chk_ptr2chk(chkindex, \
                        chk_ptr2addr(chkindex, (listnode)->prev, ChklistNode)->next))


template<typename TContainer>
struct  ChkListIterator
{
        typedef  ChkListIterator<TContainer> self;
        
        ChkListIterator(int linkoffset = 0, ChkPtr linkptr = CHK_NULL
                , ChkIndex *chkindex = NULL, ChkList *chklist = NULL)
                : link_offset(linkoffset)
                , chk_index(chkindex)
                , chk_list(chklist)
        {
                link_ptr = linkptr;
                if (link_ptr.is_null()) {
                        pnode = NULL;
                }
                else {
                        pnode = chk_ptr2addr(chk_index, link_ptr, ChklistNode);
                }
        }
        ChkListIterator(ChklistNode *node)
                : link_offset(0)
                , chk_index(NULL)
                , chk_list(NULL)
                , pnode(node)
        {
                link_ptr = CHK_NULL;
        }

        bool operator==(const self& x) const { return pnode == x.pnode; }
        bool operator!=(const self& x) const { return pnode != x.pnode; }
        TContainer* operator->() const { return (TContainer*)((char*)pnode - link_offset);}
        TContainer& operator*() const { return *(operator->()); }

        self& operator++() 
        {
                link_ptr = pnode->next;
                pnode = chk_ptr2addr(chk_index, link_ptr, ChklistNode);
                return *this;
        }
        self operator++(int) { 
                self tmp = *this;
                ++*this;
                return tmp;
        }
        self& operator--() 
        {
                link_ptr = pnode->prev;
                pnode = chk_ptr2addr(chk_index, link_ptr, ChklistNode);
                return *this;
        }
        self operator--(int) { 
                self tmp = *this;
                --*this;
                return tmp;
        }
        
        ChkPtr  link_ptr;
        int        link_offset;
        ChkIndex *chk_index;
        ChkList   *chk_list;
        ChklistNode *pnode;
};


#define  CHK_LIST_BEGIN_PTR(_list) ((_list)->next)
#define  CHK_LIST_RBEGIN_PTR(_list) ((_list)->prev)

/*
* TCmpFunc= int (const TContainer& lf, const TContainer& rh) const
*/
template<typename TContainer, typename TCmpFunc>
static inline void  chk_list_merge(ChkList* lf, ChkList* rh
                , TCmpFunc& cmp_func, ChkIndex *chkindex, int linkoffset)
{
        typedef  ChkListIterator<TContainer> iterator;
        
        iterator iter_lf(linkoffset, CHK_LIST_BEGIN_PTR(lf), chkindex, lf);
        iterator end_if(lf);
        iterator iter_rh(linkoffset, CHK_LIST_BEGIN_PTR(rh), chkindex, rh);
        iterator end_rh(rh);
        iterator tmp_lf, tmp_rh;

        ChkAddrChk pre_node, next_node, new_node;
        ChkAddrChk merged_list = {rh, chk_listnode2chk(rh, chkindex)};

        while (true)
        {
                if (iter_lf == end_if || iter_rh == end_rh)
                        break;

                int cmp_ret = cmp_func(*iter_lf, *iter_rh);
                
                if (cmp_ret < 0)
                {
                        ++iter_lf;
                }
                else if (cmp_ret >= 0)
                {
                        tmp_rh = iter_rh++;
                        chk_set_addrchk(new_node, tmp_rh.pnode, chk_ptr2chk(chkindex, tmp_rh.link_ptr));
                        chk_list_del(&new_node, chkindex);

                        if (cmp_ret == 0)
                        {
                                ++iter_lf;
                                continue;
                        }
                        
                        tmp_lf = iter_lf;
                        --tmp_lf;

                        chk_set_addrchk(pre_node, tmp_lf.pnode, chk_ptr2chk(chkindex, tmp_lf.link_ptr));
                        chk_set_addrchk(next_node, iter_lf.pnode, chk_ptr2chk(chkindex, iter_lf.link_ptr));
                        
                        chk_list_insert(&new_node, &pre_node, &next_node);

                        iter_lf = tmp_rh;
                }
        }

        if (!chk_list_empty(&merged_list))
        {
                ChkAddrChk target_list = {lf, chk_listnode2chk(lf, chkindex)};
                chk_list_splice(&merged_list, &target_list, chkindex);
        }
}

}
}

#endif
