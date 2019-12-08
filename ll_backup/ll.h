// Circular doubly-linked lists
// All arguments are pointers to structs with next & prev members
// Example:
//     struct foo {
//         int bar;
//         struct foo * next;
//         struct foo * prev;
//     };
// The next & prev members are *never* NULL
// Lists with only 1 element have x == x->next == x->prev

#define LL_INIT(node) ({ \
    __auto_type _n = (node); \
    _n->prev = _n; \
    _n->next = _n; })

#define LL_PUSH(list, node) ({ \
    __auto_type _l = (list); \
    __auto_type _n = (node); \
    assert(_n->prev == _n && _n->next == _n); \
    _n->prev = _l->prev; \
    _n->prev->next = _n; \
    _n->next = _l; \
    _n->next->prev = _n; })

#define LL_PREPEND(list, node) ({ \
    __auto_type l = (list); \
    __auto_type n = (node); \
    assert(n->prev == n && n->next == n); \
    n->next = l->next; \
    n->next->prev = n; \
    n->prev = l; \
    n->prev->next = n; })

#define LL_POP(list) ({ \
    __auto_type l = (list); \
    __auto_type n = (list)->next; \
    l->next = l->next->next; \
    l->next->prev = l; \
    LL_INIT(n); \
    (n == l) ? NULL : n; })

#define LL_EXTRACT(node) ({ \
    __auto_type n = (node); \
    n->prev->next = n->next; \
    n->next->prev = n->prev; \
    LL_INIT(n); \
    })

#define LL_PEEK(list) ({ \
    __auto_type l = (list); \
    l->next == l ? NULL : l->next; })

#define LL_UNION(left, right) ({ \
    __auto_type l = (left); \
    __auto_type r = (right); \
    if (l == r) { } \
    else if (l->next == l) { LL_PUSH(r, l); } \
    else if (r->next == r) { LL_PUSH(l, r); } \
    else { \
        __auto_type l_prev = l->prev; \
        __auto_type l_next = l->next; \
        __auto_type r_prev = r->prev; \
        __auto_type r_next = r->next; \
        if (l_prev != r && l_next != r && r_prev != l && r_next != l && r_next != l_prev && r_prev != l_next) { \
            l->next = r; \
            r->prev = l; \
            r->next = l_next; \
            l_next->prev = r; \
            l_prev->next = r_next; \
            r_next->prev = l_prev; \
            r_prev->next = l; \
            l->prev = r_prev; \
        } \
    } \
    })

