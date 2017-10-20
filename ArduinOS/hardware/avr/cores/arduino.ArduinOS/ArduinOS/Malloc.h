
#ifndef MALLOC_H
#define	MALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __DOXYGEN__

#ifndef __ATTR_MALLOC__
# define __ATTR_MALLOC__ __attribute__((__malloc__))
#endif


#endif

    /**
    The malloc() function allocates \c size bytes of memory.
    If malloc() fails, a NULL pointer is returned.

    Note that malloc() does \e not initialize the returned memory to
    zero bytes.

    See the chapter about \ref malloc "malloc() usage" for implementation
    details.
    */
    extern void *Malloc(size_t __size) __ATTR_MALLOC__;

    /**
    The free() function causes the allocated memory referenced by \c
    ptr to be made available for future allocations.  If \c ptr is
    NULL, no action occurs.
    */
    extern void Free(void *__ptr);

    /**
    \c malloc() \ref malloc_tunables "tunable".
    */
    extern size_t __malloc_margin;

    /**
    \c malloc() \ref malloc_tunables "tunable".
    */
    extern char *__malloc_heap_start;

    /**
    \c malloc() \ref malloc_tunables "tunable".
    */
    extern char *__malloc_heap_end;

    /**
    Allocate \c nele elements of \c size each.  Identical to calling
    \c malloc() using <tt>nele * size</tt> as argument, except the
    allocated memory will be cleared to zero.
    */
    extern void *Calloc(size_t __nele, size_t __size) __ATTR_MALLOC__;

    /**
    The realloc() function tries to change the size of the region
    allocated at \c ptr to the new \c size value.  It returns a
    pointer to the new region.  The returned pointer might be the
    same as the old pointer, or a pointer to a completely different
    region.

    The contents of the returned region up to either the old or the new
    size value (whatever is less) will be identical to the contents of
    the old region, even in case a new region had to be allocated.

    It is acceptable to pass \c ptr as NULL, in which case realloc()
    will behave identical to malloc().

    If the new memory cannot be allocated, realloc() returns NULL, and
    the region at \c ptr will not be changed.
    */
    extern void *Realloc(void *__ptr, size_t __size) __ATTR_MALLOC__;


#ifdef __cplusplus
}
#endif

#endif
