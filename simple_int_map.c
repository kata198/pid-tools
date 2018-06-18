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

SimpleIntMap *simple_int_map_create(int modSize)
{
    SimpleIntMap *ret;

    ret = malloc( sizeof(SimpleIntMap) );

    ret->modSize = modSize;
    ret->nodeData = calloc( modSize + 1, sizeof(struct SimpleIntMapNode ) );

    ret->nodeHasData = calloc( modSize + 1, ((float)modSize) / ((float)sizeof(char)) );
    ret->numEntries = 0;

    return ret;
}

int _simple_int_map_search_node_data(struct SimpleIntMapNode *nodeToSearch, int findInt)
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
    int modSize;
    int idxVal;

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
    int modSize;
    int idxVal;
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

int *simple_int_map_values(SimpleIntMap *intMap, int *retLen)
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

