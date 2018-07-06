/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file hal_altera_pci_mem.c
 *
 * Description:
 *   A simple implementation of HAL's malloc() and free() to be used in combination
 *   with the Altera FPGA board. Free'd blocks get combined with adjacent free
 *   blocks into a single larger block immediately.
 *
 *****************************************************************************/


#include <ebase/trace.h>

#include "hal_api.h"

#if defined ( HAL_ALTERA )

CREATE_TRACER(HALMEM_DEBUG, "HALMEM: ", INFO, 0);
//CREATE_TRACER(HALMEM_INFO , "HALMEM: ", INFO, 1);
//CREATE_TRACER(HALMEM_ERROR, "HALMEM: ", ERROR, 1);


/******************************************************************************
 * local macro definitions
 *****************************************************************************/

/******************************************************************************
 * local type definitions
 *****************************************************************************/
// the item type for lists of memory blocks
typedef struct BlockList_s
{
    struct BlockList_s  *pNext;     //!< Pointer to next block entry in list of blocks.
    uint32_t            Addr;       //!< The base addr of this block.
    uint32_t            Size;       //!< The size of this block.
} BlockList_t;

// the ext mem allocator context type
typedef struct ExtMemContext_s
{
    uint32_t    BaseAddr;           //!< Base address of memory.
    uint32_t    Size;               //!< Size of memory.
    uint32_t    Alignment;          //!< Alignment of allocated blocks.
    BlockList_t FreeBlocksList;     //!< List of blocks of free memory.
    BlockList_t UsedBlocksList;     //!< List of blocks of used memory.
} ExtMemContext_t;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
static ExtMemContext_t ExtMemContext = {0, 0, 0, {0,0,0}, {0,0,0}}; //!< our ExtMemContext for now
static ExtMemContext_t *pExtMemContext = NULL;    //!< use this pointer to access ExtMemContext; this allows for easy upgrade later on to support multiple allocators.


/******************************************************************************
 * local function prototypes
 *****************************************************************************/
/******************************************************************************
 * @brief           put a block on the list of free blocks; keep ascending
 *                  order of list with regard to block size
 *****************************************************************************/
INLINE void ExtMemFreedBlock( BlockList_t *pFreed );


/******************************************************************************
 * @brief           put a block on the list of used blocks; keep ascending
 *                  order of list with regard to block address
 *****************************************************************************/
INLINE void ExtMemUsedBlock( BlockList_t *pUsed );


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * ExtMemCreate()
 * @brief           Create & initialze external memory allocator.
 *****************************************************************************/
RESULT ExtMemCreate( uint32_t BaseAddr, uint32_t Size, uint32_t Alignment )
{
    // check params
    if ( !Size || (Size<=Alignment) )
    {
        return RET_INVALID_PARM;
    }

    // only one instance allowed
    if (pExtMemContext != NULL)
    {
        return RET_WRONG_STATE;
    }

    // 'create' allocator context
    pExtMemContext = &ExtMemContext;
    memset( pExtMemContext, 0, sizeof(ExtMemContext_t) );

    // setup the allocator context; align BaseAddr up & Size down
    pExtMemContext->BaseAddr  = (BaseAddr + Alignment-1) & ~(Alignment-1);
    pExtMemContext->Size      = Size - (pExtMemContext->BaseAddr - BaseAddr);
    pExtMemContext->Alignment = Alignment;

    // create initial free block item
    BlockList_t *pItem = (BlockList_t *) malloc( sizeof(BlockList_t) );
    if (!pItem)
    {
        pExtMemContext = NULL;
        return RET_OUTOFMEM;
    }

    // initialize first item to cover the whole memory area
    pItem->pNext = NULL;
    pItem->Addr  = pExtMemContext->BaseAddr;
    pItem->Size  = pExtMemContext->Size;

    // initialize the lists in the allocator context
    pExtMemContext->FreeBlocksList.pNext = pItem;
    pExtMemContext->UsedBlocksList.pNext = NULL;

    return RET_SUCCESS;
}


/******************************************************************************
 * ExtMemDestroy()
 * @brief           Destroy external memory allocator.
 *****************************************************************************/
RESULT ExtMemDestroy( void )
{
    RESULT result = RET_SUCCESS;
    BlockList_t *pItem, *pFree;

    // there should only be exactly one single block in the FreeBlocksList
    // and no block at all in the UsedBlocksList; check it!
    if ( !pExtMemContext->FreeBlocksList.pNext
      || pExtMemContext->FreeBlocksList.pNext->pNext
      || pExtMemContext->UsedBlocksList.pNext        )
    {
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // nevertheless iterate over both lists and free the list items
    pItem = pExtMemContext->FreeBlocksList.pNext;
    while ( pItem )
    {
        pFree = pItem;
        pItem = pItem->pNext;
        free( pFree );
    }
    pItem = pExtMemContext->UsedBlocksList.pNext;
    while ( pItem )
    {
        pFree = pItem;
        pItem = pItem->pNext;
        free( pFree );
    }

    // 'destroy' context
    memset( pExtMemContext, 0, sizeof(*pExtMemContext) );

    // not initialized again
    pExtMemContext = NULL;

    return result;
}


/******************************************************************************
 * ExtMemAlloc()
 * @brief           allocate a block of external memory
 *****************************************************************************/
uint32_t ExtMemAlloc( uint32_t ReqSize )
{
    uint32_t BlockAddr = 0;

    TRACE( HALMEM_DEBUG, "%s: block to alloc: Size=0x%08x\n", __FUNCTION__, ReqSize );

    // check params
    if ( !ReqSize || (ReqSize > pExtMemContext->Size) )
    {
        return 0;
    }

    // align the requested size
    ReqSize += pExtMemContext->Alignment - 1;
    ReqSize &= ~(pExtMemContext->Alignment - 1);

    //TODO: need to lock this block
    {
        BlockList_t *pItem;
        BlockList_t *pFound;

        // iterate through the list until a block is found that has an equal or larger size than the requested size
        pItem = &pExtMemContext->FreeBlocksList;
        while ( pItem->pNext && (pItem->pNext->Size < ReqSize) )
        {
            pItem = pItem->pNext;
        }
        pFound = pItem->pNext;

        // did we find a suitable block?
        if (pFound)
        {
            // remove found block from list of free blocks
            pItem->pNext = pFound->pNext;

            // split off the remainder of the block?
            if( (pFound->Size - ReqSize) >= pExtMemContext->Alignment )
            {
                // allocate a new block list item
                pItem = (BlockList_t *) malloc( sizeof(BlockList_t) );

                // silently don't split in case block item allocation failed
                if (pItem)
                {
                    // adjust size & addr of found & split off block
                    pItem->Addr  = pFound->Addr + ReqSize;
                    pItem->Size  = pFound->Size - ReqSize;
                    pFound->Size = ReqSize;

                    // add split off block to list of free blocks
                    TRACE( HALMEM_DEBUG, "%s: new free block: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pItem->Addr, pItem->Size, pItem );
                    ExtMemFreedBlock( pItem );
                }
            }

            // add found block to list of used blocks
            TRACE( HALMEM_DEBUG, "%s: new used block: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pFound->Addr, pFound->Size, pFound );
            ExtMemUsedBlock( pFound );

            // prepare returning the address of that block
            BlockAddr = pFound->Addr;
        }
    }

    TRACE( HALMEM_DEBUG, "%s: block allocated: Addr=0x%08x\n", __FUNCTION__, BlockAddr );

    return BlockAddr;
}


/******************************************************************************
 * ExtMemFree()
 * @brief           free block of external memory
 *****************************************************************************/
void ExtMemFree( uint32_t BlockAddr )
{
    TRACE( HALMEM_DEBUG, "%s: block to free: Addr=0x%08x\n", __FUNCTION__, BlockAddr );

    if (BlockAddr)
    {
        //TODO: need to lock this block
        {
            BlockList_t *pItem, *pFreed;

            // iterate through the list until the block with that addr is found
            pItem = &pExtMemContext->UsedBlocksList;
            while ( pItem->pNext && (pItem->pNext->Addr != BlockAddr) )
            {
                pItem = pItem->pNext;
            }

            // remove that block from the list of used blocks
            pFreed = pItem->pNext;
            pItem->pNext = pFreed->pNext;
            TRACE( HALMEM_DEBUG, "%s: block to free: Addr=0x%08x, Size=0x%08x => Block=%p (BlockAdrr=0x%08x)\n", __FUNCTION__, pFreed->Addr, pFreed->Size, pFreed, BlockAddr );

            // check if we can recombine it; the used blocks list is sorted with regard to the addresses,
            // so if the preceeding or succeeding items/upper memory limit aren't direct neighbours in memory, the
            // free blocks list must hold suitable blocks for combining and it's worth doing the costly traversal
            if ( ( (pItem->Addr  + pItem->Size) !=                    pFreed->Addr                                                             )
              || ( (pFreed->Addr + pItem->Size) != ((pFreed->pNext) ? pFreed->pNext->Addr : (pExtMemContext->BaseAddr + pExtMemContext->Size)) ) )
            {
                BlockList_t *pLocItem;
                BlockList_t *pPredPrev = NULL;
                BlockList_t *pSuccPrev = NULL;

                // iterate through the list to try to find blocks suitable for combining with the free'd one
                pLocItem = &pExtMemContext->FreeBlocksList;
                while ( pLocItem->pNext )
                {
                    if ((pLocItem->pNext->Addr + pLocItem->pNext->Size) == pFreed->Addr)
                    {
                        pPredPrev = pLocItem;
                        TRACE( HALMEM_DEBUG, "%s: preceeding block: Addr=0x%08x, Size=0x%08x => Block=%p (PrevBlock=%p)\n", __FUNCTION__, pPredPrev->pNext->Addr, pPredPrev->pNext->Size, pPredPrev->pNext, pPredPrev );
                    }
                    if ((pFreed->Addr + pFreed->Size) == pLocItem->pNext->Addr)
                    {
                        pSuccPrev = pLocItem;
                        TRACE( HALMEM_DEBUG, "%s: succeeding block: Addr=0x%08x, Size=0x%08x => Block=%p (PrevBlock=%p)\n", __FUNCTION__, pSuccPrev->pNext->Addr, pSuccPrev->pNext->Size, pSuccPrev->pNext, pSuccPrev );
                    }
                    pLocItem = pLocItem->pNext;
                }

                // now combine those into the free'd buffer
                // note: first the succeeding, then the preceeding block to avoid rendering the succeeding
                //       block item invalid if the succeeding item is next in list to preceeding item
                if (pSuccPrev)
                {
                    // get the block right behind us in memory
                    pLocItem = pSuccPrev->pNext;

                    // only need to adjust our size up
                    pFreed->Size += pLocItem->Size;
                    TRACE( HALMEM_DEBUG, "%s: new block to free: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pFreed->Addr, pFreed->Size, pFreed );

                    // remove that block from the list of free blocks and free its block item
                    pSuccPrev->pNext = pLocItem->pNext;
                    TRACE( HALMEM_DEBUG, "%s: freeing succ block: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pLocItem->Addr, pLocItem->Size, pLocItem );

                    free( pLocItem );
                    TRACE( HALMEM_DEBUG, "%s: succ block item freed\n", __FUNCTION__ );
                }
                if (pPredPrev)
                {
                    // get the block right before us in memory
                    pLocItem = pPredPrev->pNext;

                    // adjust our addr down & size up
                    pFreed->Addr  = pLocItem->Addr;
                    pFreed->Size += pLocItem->Size;
                    TRACE( HALMEM_DEBUG, "%s: new block to free: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pFreed->Addr, pFreed->Size, pFreed );

                    // remove that block from the list of free blocks and free its block item
                    pPredPrev->pNext = pLocItem->pNext;
                    TRACE( HALMEM_DEBUG, "%s: freeing prev block: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pLocItem->Addr, pLocItem->Size, pLocItem );

                    free( pLocItem );
                    TRACE( HALMEM_DEBUG, "%s: prev block item freed\n", __FUNCTION__ );
                }

                TRACE( HALMEM_DEBUG, "%s: done combining\n", __FUNCTION__ );
            }
            else
            {
                TRACE( HALMEM_DEBUG, "%s: no adjacent block free\n", __FUNCTION__ );
            }

            // add (possibly combined) block to the list of free blocks
            TRACE( HALMEM_DEBUG, "%s: new free block: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pFreed->Addr, pFreed->Size, pFreed );
            ExtMemFreedBlock( pFreed );
        }
    }

    TRACE( HALMEM_DEBUG, "%s: block free'd\n", __FUNCTION__ );
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * ExtMemFreedBlock()
 * @brief           put a block on the list of free blocks; keep ascending
 *                  order of list with regard to block size
 *****************************************************************************/
INLINE void ExtMemFreedBlock( BlockList_t *pFreed )
{
    BlockList_t *pItem;

    // iterate through the list until a block is found that has a size equal to or larger than the one to be inserted
    pItem = &pExtMemContext->FreeBlocksList;
    while ( pItem->pNext && (pItem->pNext->Size < pFreed->Size) )
    {
        pItem = pItem->pNext;
    }

    // insert block after current item
    pFreed->pNext = pItem->pNext;
    pItem->pNext  = pFreed;

    // iterate through the list and generate debug output
    pItem = &pExtMemContext->FreeBlocksList;
    while ( pItem->pNext )
    {
        pItem = pItem->pNext;
        TRACE( HALMEM_DEBUG, "%s: free block: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pItem->Addr, pItem->Size, pItem );
    }
}


/******************************************************************************
 * ExtMemUsedBlock()
 *****************************************************************************/
INLINE void ExtMemUsedBlock( BlockList_t *pUsed )
{
    BlockList_t *pItem;

    // iterate through the list until a block is found that has an addr equal to or larger than the one to be inserted
    pItem = &pExtMemContext->UsedBlocksList;
    while ( pItem->pNext && (pItem->pNext->Addr < pUsed->Addr) )
    {
        pItem = pItem->pNext;
    }

    // insert block after current item
    pUsed->pNext = pItem->pNext;
    pItem->pNext = pUsed;

    // iterate through the list and generate debug output
    pItem = &pExtMemContext->UsedBlocksList;
    while ( pItem->pNext )
    {
        pItem = pItem->pNext;
        TRACE( HALMEM_DEBUG, "%s: used block: Addr=0x%08x, Size=0x%08x => Block=%p\n", __FUNCTION__, pItem->Addr, pItem->Size, pItem );
    }
}


#endif // defined ( HAL_ALTERA )
