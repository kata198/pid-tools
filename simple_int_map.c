/*
 * Copyright (c) 2018 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * simple_int_map.c - Interface implementations for a simple integer-based Map
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "pid_tools.h"

#include "simple_int_map.h"

/* ENFORCE_UNIQUE_ENTRIES - If defined, duplicate values are not added (so all values are unique) */
#define ENFORCE_UNIQUE_ENTRIES

SimpleIntMap *simple_int_map_create(unsigned int modSize)
{
    SimpleIntMap *ret;

    ret = malloc( sizeof(SimpleIntMap) );

    ret->modSize = modSize;
    ret->nodeData = calloc( modSize + 1, sizeof(struct SimpleIntMapNode ) );

    ret->nodeHasData = calloc( modSize + 1, ((float)modSize) / ((float)sizeof(char)) );
    ret->numEntries = 0;

    return ret;
}

static int _simple_int_map_search_node_data(struct SimpleIntMapNode *nodeToSearch, int findInt)
{
    struct SimpleIntMapNode *curNode;

    curNode = nodeToSearch;
    do {

        if ( curNode->data == findInt )
            return 1;
        
        curNode = NODE_NEXT(curNode);
    } while ( curNode != NULL );

    return 0;
}

int simple_int_map_contains(SimpleIntMap *intMap, int testInt)
{
    unsigned int modSize;
    unsigned int idxVal;

    modSize = intMap->modSize;

    idxVal = testInt % modSize;

    /* Check if any data is valid here, if not no match. */
    if ( ! intMap->nodeHasData[idxVal] )
        return 0;

    /* Data is present in this mod container, see if the linked list contains our number */

    return _simple_int_map_search_node_data( &(intMap->nodeData[idxVal]), testInt );
}

int simple_int_map_add(SimpleIntMap *intMap, int toAdd)
{
    unsigned int modSize;
    unsigned int idxVal;
    struct SimpleIntMapNode *intMapNode;

    modSize = intMap->modSize;

    idxVal = toAdd % modSize;

    intMapNode = &(intMap->nodeData[idxVal]);

    /* See if any data is yet mapped at this mod container */
    if ( ! intMap->nodeHasData[idxVal] )
    {
        /* Set the "data contained" flag and set the first "data" entry in container */
        intMap->nodeHasData[idxVal] = 1;

        intMapNode->data = toAdd;
        intMapNode->next = NULL; /* TODO: Is this needed with calloc? */

        intMap->numEntries += 1;

        return 1;
    }
    else
    {
        /* Otherwise, we need to add onto the next empty spot in the linked-list */
        struct SimpleIntMapNode *curNode;

        curNode = intMapNode;

        while ( 1 ) 
        {
            #ifdef ENFORCE_UNIQUE_ENTRIES
            if ( curNode->data == toAdd )
            {
                /* We found a duplicate entry, so just exit */
                return 0;
            }
            #endif
            if ( curNode->next == NULL )
            {
                /* We found an empty "next", so take it and mark following as empty */
                curNode->next = malloc( sizeof(struct SimpleIntMapNode) );

                curNode = NODE_NEXT( curNode );

                curNode->data = toAdd;
                curNode->next = NULL;

                intMap->numEntries += 1;

                return 1;
            }
        
            curNode = NODE_NEXT( curNode );

        } /* while ( 1 ) */
    } /* else */

    return 0; /* Should be unreachable */
}

int simple_int_map_rem(SimpleIntMap *intMap, int toRem)
{
    unsigned int modSize;
    unsigned int idxVal;
    struct SimpleIntMapNode *intMapNode;

    modSize = intMap->modSize;

    idxVal = toRem % modSize;

    intMapNode = &(intMap->nodeData[idxVal]);

    /* See if any data is yet mapped at this mod container */
    if ( unlikely( ! intMap->nodeHasData[idxVal] ) )
    {
        return 0;
    }
    else
    {
        /* There are entries in this bin, see if we can find ours and if so remove it. */
        struct SimpleIntMapNode *prevNode;
        struct SimpleIntMapNode *curNode;
        struct SimpleIntMapNode *nextNode;

        prevNode = NULL;
        curNode = intMapNode;

        while ( 1 ) 
        {
            if ( curNode->data == toRem )
            {
                nextNode = NODE_NEXT(curNode);
                /* Found our entry, now remove it. */
                if ( prevNode == NULL )
                {
                    /* We are on the first node. */
                    if ( nextNode == NULL )
                    {
                        /* There is no next node, so remove this bin. */
                        intMap->nodeHasData[idxVal] = 0;
                        /* 
                           Zero-out the "data", even though it should be skipped
                             since nodeHasData[idxVal] == 0
                         */
                        curNode->data = 0;
                    }
                    else
                    {
                        /* There is a next node, so lets move it to the first node */
                        curNode->data = nextNode->data;
                        curNode->next = (void*)nextNode->next;
                        free(nextNode);

                    }

                }
                else
                {
                    /* We are NOT on the first node */

                    /* Make prevNode point to next and free this node */
                    prevNode->next = (void*)nextNode;
                    free(curNode);
                }

                intMap->numEntries -= 1;
                return 1;
            }
            else
            {
                /* Not a match here, move onto next node */
                prevNode = curNode;
                curNode = NODE_NEXT( curNode );
                if ( curNode == NULL )
                {
                    /* We have moved to the end of this bucket's linked list
                        without finding a match. Must not be present.
                    */
                    return 0;
                }
            }

        } /* while ( 1 ) */
    } /* else */

    return 0; /* Should be unreachable */
}

int *simple_int_map_values(SimpleIntMap *intMap, size_t *retLen)
{
    int *ret;
    unsigned int i;
    unsigned int retIdx;
    struct SimpleIntMapNode *curNode, *nextNode;

    ret = malloc( intMap->numEntries * sizeof(int) );
    retIdx = 0;

    for ( i = 0; i < intMap->modSize; i++ )
    {
        if ( ! intMap->nodeHasData[i] )
            continue;

        curNode = &(intMap->nodeData[i]);

        /* Add in top-level data to return result */
        ret[ retIdx++ ] = curNode->data;

        /* Iterate until no more data to add */
        nextNode = NODE_NEXT(curNode);
        while ( nextNode != NULL )
        {
            ret[ retIdx++ ] = nextNode->data;

            nextNode = NODE_NEXT(nextNode);
        }

    }

    *retLen = intMap->numEntries;

    return ret;
}

void simple_int_map_destroy(SimpleIntMap *intMap)
{
    unsigned int i;

    void **toFree;
    unsigned int toFreeIdx;

    struct SimpleIntMapNode *curNode, *nextNode;

    if ( likely( intMap->numEntries > 0 ) )
    {
        toFree = malloc( sizeof(void *) * intMap->numEntries );
        toFreeIdx = 0;

        for ( i = 0; i < intMap->modSize; i++ )
        {
            if ( ! intMap->nodeHasData[i] )
                continue;

            curNode = &(intMap->nodeData[i]);

            /* Add in top-level data to list of items to free */
            /*toFree[ toFreeIdx++ ] = curNode;*/

            /* Iterate until no more nodes found at this level */
            nextNode = NODE_NEXT(curNode);
            while ( nextNode != NULL )
            {
                toFree[ toFreeIdx++ ] = nextNode;

                nextNode = NODE_NEXT(nextNode);
            }
        } /* for i < intMap->modSize */

        for( i=0; i < toFreeIdx; i++ )
        {
            free( toFree[i] );
        }

        free(toFree);
    }

    free(intMap->nodeData);
    free(intMap->nodeHasData);
    free(intMap);

}

/*typedef struct {
    
    int curBucket;
    struct SimpleIntMapNode *curNode;

} SimpleIntMapIterator;
*/

SimpleIntMapIterator *simple_int_map_get_iter(SimpleIntMap *intMap)
{
    SimpleIntMapIterator *iter = calloc( sizeof(SimpleIntMapIterator), 1 );

    iter->intMap = intMap;

    return iter;
}

void simple_int_map_iter_destroy(SimpleIntMapIterator *mapIter)
{
    free( mapIter );
}

void simple_int_map_iter_reset(SimpleIntMapIterator *mapIter)
{
    mapIter->curBucket = 0;
    mapIter->curNode = NULL;
}

static int _simple_int_map_move_iter_forward(SimpleIntMapIterator *mapIter)
{
    SimpleIntMap *intMap;
    struct SimpleIntMapNode *nextNode;
    struct SimpleIntMapNode *curNode;
    int bucketNum;
    int stopIteration = 0;

    intMap = mapIter->intMap;

    curNode = mapIter->curNode;

    if ( curNode != NULL )
    {
    
        nextNode = NODE_NEXT(curNode);
        if ( nextNode != NULL )
        {
            mapIter->curNode = nextNode;
            return stopIteration;
        }
        else
        {
            /* Clear curNode marker, bump up bucket, and do the walk */
            mapIter->curNode = NULL;
            mapIter->curBucket += 1;
        }
    }

    if ( unlikely( mapIter->curBucket >= intMap->modSize ) )
    {
        /* We are already at the end. */
        stopIteration = 1;
        return stopIteration;
    }

    for( bucketNum = mapIter->curBucket; bucketNum < intMap->modSize; bucketNum++ )
    {
        if ( intMap->nodeHasData[ bucketNum ] )
        {
            mapIter->curBucket = bucketNum;
            mapIter->curNode = &(intMap->nodeData[bucketNum]);
            return stopIteration;
        }
    }
    /* We hit the end without finding a value */
    mapIter->curBucket = bucketNum;
    mapIter->curNode = NULL;

    stopIteration = 1;
    
    return stopIteration;
}

int simple_int_map_iter_next(SimpleIntMapIterator *mapIter, int *completedIterationPtr)
{
    int ret;
    struct SimpleIntMapNode *curNode;
    int stopIteration = 0;

    curNode = mapIter->curNode;

    if ( curNode == NULL )
    {
        /* If we aren't on a node yet, move onto one */
        stopIteration = _simple_int_map_move_iter_forward(mapIter);
        if ( stopIteration )
            *completedIterationPtr = stopIteration;
    }

    curNode = mapIter->curNode;
    if ( curNode == NULL )
    {
        /* No value, stop iteration. */
        *completedIterationPtr = 2;
        return 0;
    }

    /* If we moved onto a node, grab and return data */
    ret = curNode->data;

    stopIteration = _simple_int_map_move_iter_forward(mapIter);
    if ( stopIteration )
        *completedIterationPtr = stopIteration;

    return ret;
}
