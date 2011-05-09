/* KallistiOS ##version##

   malloc.h
   Copyright (C)2003 Dan Potter

*/

/** \file   malloc.h
    \brief  Standard C Malloc functionality

    This implements standard C heap allocation, deallocation, and stats.

    \author Dan Potter
*/

#ifndef __MALLOC_H
#define __MALLOC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* Unlike previous versions, we totally decouple the implementation from
   the declarations. */

/** \brief ANSI C functions */
struct mallinfo {
    /** \brief non-mmapped space allocated from system */
	int arena;
    /** \brief number of free chunks */
	int ordblks;
    /** \brief number of fastbin blocks */
	int smblks;
    /** \brief number of mmapped regions */
	int hblks;
    /** \brief space in mmapped regions */
	int hblkhd;
    /** \brief maximum total allocated space */
	int usmblks;
    /** \brief space available in freed fastbin blocks */
	int fsmblks;
    /** \brief total allocated space */
	int uordblks;
    /** \brief total free space */
	int fordblks;
    /** \brief top-most, releasable (via malloc_trim) space */
	int keepcost;
};

/** \brief allocate memory

    This allocates the specified size of bytes onto the heap. This memory is not
    freed automatically if the returned pointer goes out of scope. Thus you must
    call \ref free to reclaim the used space when finished.

    \param size is the size in bytes to allocate
    \return a pointer to the newly allocated address or NULL on errors.
    \see free
    \note the memory chunk is uninitialized
    \note NULL may also be returned if size is 0
*/
void * malloc(size_t size);

/** \brief allocate memory on the heap and initialize it to 0

    This allocates a chunk of memory of size * nmemb. In otherwords, an array
    with nmemb elements of size or size[nmemb].

    \param nmemb is the amount of elements
    \param size the size of each element
    \return a pointer to the newly allocated address or NULL on errors.
    \see free
    \note the memory chunk is set to zero
    \note NULL may be returned if nmemb or size is 0
*/
void * calloc(size_t nmemb, size_t size);

/** \brief releases memory that was previous allocated

    frees the memory space that had previously been allocated by malloc or
    calloc.

    \param ptr is a pointer to the address of allocated ram
    \note no action is taken if NULL is passed
    \note calling free on the same ptr more than once should not be expected to
        behave in a reproducable maner as it is unpredictable.
*/
void   free(void * ptr);

/** \brief changes the size of previously allocated memory

    The size of ptr is changed to size. If data has already been placed in the
    memory area at that location, it's preserved up to size. If size is larger
    then the previously allocated memory, the new area will be unititialized.

    \param ptr the address pointer that's been previously returned by malloc/calloc
    \param size the new size to give to the memory
    \return a pointer to the new address
    \note if ptr is NULL the call is basically a malloc(size)
    \note if ptr is not NULL, and size is 0 the call is basically a free(ptr)
*/
void * realloc(void * ptr, size_t size);

/** \brief allocate memory aligned memory

    Memory of size is allocated with the address being a multiple of alignment

    \param alignment a multiple of two that the memory address will be aligned to
    \param size the size of the memory to allocate
    \return a pointer to the newly allocated address (aligned to alignment) or
        NULL on errors
*/
void * memalign(size_t alignment, size_t size);

/** \brief allocates memory aligned to the system page size

    Memory is allocated of size and is aligned to the system page size.
    This ends up basically being: memolign(PAGESIZE, size)

    \param size the size of the memory to allocate
    \return a pointer to the newly allocated memory address
    \see arch/arch.h
*/
void * valloc(size_t size);

/** \brief Sets tunable parameters for malloc related options.
*/
struct mallinfo mallinfo();

/* mallopt defines */
#define M_MXFAST 1
#define DEFAULT_MXFAST 64

#define M_TRIM_THRESHOLD -1
#define DEFAULT_TRIM_THRESHOLD (256*1024)

#define M_TOP_PAD -2
#define DEFAULT_TOP_PAD 0

#define M_MMAP_THRESHOLD -3
#define DEFAULT_MMAP_THRESHOLD (256*1024)

#define M_MMAP_MAX -4
#define DEFAULT_MMAP_MAX 65536
int  mallopt(int, int);

/** \brief Debug function
*/
void malloc_stats();

/** \brief KOS specfic calls
*/
int malloc_irq_safe();

/** \brief Only available with KM_DBG
*/
int mem_check_block(void *p);

/** \brief Only available with KM_DBG
 */
int mem_check_all();

__END_DECLS

#endif	/* __MALLOC_H */

