/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
/**
 * @brief Some macros
 * 
 */
/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */
#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Somnus",
    /* First member's full name */
    "Ze-Yin Chen",
    /* First member's email address */
    "chenzeyin9867@buaa.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char* heap_listp;
static void* coalesce(void *bp);
static void *extend_heap(size_t words);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += 2 * WSIZE; 
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    extend_heap(CHUNKSIZE/WSIZE);
        // return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if(size == 0) return NULL;
    int newsize = ALIGN(size + SIZE_T_SIZE);
    /* Traverse from the plist */
    char * bp = heap_listp;
    int found = 0;
    while(GET_SIZE(HDRP(bp)) > 0){
        if(GET_ALLOC(HDRP(bp)) == 1 || GET_SIZE(HDRP(bp)) < newsize){
            bp = NEXT_BLKP(bp);
        }else{
            found = 1;
            break;
        }
    }

    if(found){
        /* split this block into two blocks */
        size_t origin_size = GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(newsize, 1)); // flag as allocated
        PUT(HDRP(bp), PACK(newsize, 1));
        
        size_t left = origin_size - newsize;
        if(left > 0){
            char *next = NEXT_BLKP(bp);
            PUT(HDRP(next), PACK(left, 0));
            PUT(FTRP(next), PACK(left, 0));
        }
        return bp;
    }else{
        // call sbrk
        void *p = extend_heap(newsize);
        return p;
    }  

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0)); /* set header free*/
    PUT(FTRP(ptr), PACK(GET_SIZE(FTRP(ptr)), 0)); /* set footer free*/
    coalesce(ptr); /* coalesce adjecnt blocks */
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


/* Extend the heap */
static void *extend_heap(size_t words){
    /**
     * @brief Allocate even number of Words to make algnment
     */
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((bp = mem_sbrk(size))== (void*)-1){
        perror("mem_sbrk");
    } 
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void* coalesce(void *bp){
    /* according the adjacent relationship of bp, to coalesce free list */
    char* prev = PREV_BLKP(bp);
    char* next = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(HDRP(prev));
    // size_t prev_alloc = GET_ALLOC(HDRP(prev));
    size_t next_alloc = GET_ALLOC(HDRP(next));
    size_t cur_size = GET_SIZE(HDRP(bp));
    /* situation 1: prev and next are all allocate */    
    if(prev_alloc == 1 && next_alloc == 1){
        return bp;
    }

    /* Situation 2: prev alloc, next free */
    if(prev_alloc == 1 && next_alloc == 0){
        /* merge bp and next */
        
        cur_size += GET_SIZE(HDRP(next));

        /* update the new header and footer */
        PUT(HDRP(bp), PACK(cur_size, 0));
        PUT(FTRP(bp), PACK(cur_size, 0));
        return bp;
    }

    /* Situation 3: prev free, next alloc */
    if(GET_ALLOC(prev) == 0 && GET_ALLOC(next) == 1){
        size_t prev_sz = GET_SIZE(HDRP(prev));
        cur_size += prev_sz;
        /* update new header and footer */
        PUT(HDRP(prev), PACK(cur_size, 0));
        PUT(FTRP(prev), PACK(cur_size, 0));
        return prev;
    }

    /* Situation 4: prev free, next free */
    if(prev_alloc == 0 && next_alloc == 0){
        size_t new_sz = GET_SIZE(HDRP(prev)) + GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(next));
        /* update header and footer */
        PUT(HDRP(prev), PACK(new_sz, 0));
        PUT(FTRP(prev), PACK(new_sz, 0));
        return prev;
    }
}











