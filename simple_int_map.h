/*
 * Copyright (c) 2018 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * simple_int_map.h - Interface definitions for a simple integer-based Map
 *
 */

#ifndef _SIMPLE_INT_MAP_H
#define _SIMPLE_INT_MAP_H

#include <sys/types.h>

#include "pid_tools.h"

/*******************
 * DATA TYPES
 ******************/

/**
 *   struct SimpleIntMapNode - A node of a single-linked list for use in SimpleIntMap
 *
 *          You should not need to reference this directly.
 */
struct SimpleIntMapNode {
    
    int data;
    void *next;

} ALIGN_16; 


/**
 *   SimpleIntMap - The public structure for using a simple int map
 *
 *      Create with - simple_int_map_create
 *
 *      Free/Destroy with - simple_int_map_destroy
 *
 *      Other operations -- see functions below
 */
typedef struct {

    int modSize;
    struct SimpleIntMapNode *nodeData;
    char *nodeHasData; /* TODO: instead of 1 or 0 taking up entire byte, split by bits? */

    size_t numEntries;

} SimpleIntMap ALIGN_32;


/*******************
 * MACROS
 ******************/

/* NODE_NEXT - Get the "next" node from a current node */
#define NODE_NEXT(curNode) ((struct SimpleIntMapNode*)(curNode)->next)

#define MAP_NUM_ENTRIES(mapObj) ((mapObj)->numEntries)

/*******************
 * PUBLIC FUNCTIONS
 ******************/

/**
 *    simple_int_map_create - Allocate a SimpleIntMap for use
 *
 *          @param modSize <int> - The number of buckets and modulus used in this map.
 *                      Powers of 10 work well.
 *
 *                      Smaller values take up less space but may be less efficient
 *
 *          @return - Pointer to an allocated SimpleIntMap ready to use
 *
 *              This must be freed using simple_int_map_destroy
 */
SimpleIntMap *simple_int_map_create(int modSize);

/**
 *    simple_int_map_destroy - Deallocate a SimpleIntMap including all referenced memory
 *
 *          @param intMap <SimpleIntMap *> - Pointer to the map to free
 */
void simple_int_map_destroy(SimpleIntMap *intMap);

/**
 *    simple_int_map_contains - Check if the given intmap contains a value
 *
 *          @param intMap <SimpleIntMap *> - Pointer to the map to search
 *
 *          @param testInt <int> - Int to search for
 *
 *          @return - 1 if found
 *                    0 if not found
 */
int simple_int_map_contains(SimpleIntMap *intMap, int testInt);

/**
 *    simple_int_map_add - Add an entry to the map.
 *
 *      @param intMap <SimpleIntMap *> - Pointer to the map into which to add
 *
 *      @param toAdd <int> - Integer to add
 *
 *      @return <int> - 1 if added
 *                      0 if already present
 *
 *      NOTE: If #testInt is already present in #intMap it will not be added twice (unique values only)
 */
int simple_int_map_add(SimpleIntMap *intMap, int toAdd);

/**
 *    simple_int_map_values - Return a list of all the values in #intMap
 *
 *      @param intMap <SimpleIntMap *> - Pointer to the map of interest
 *
 *      @param retLen <int *> - The size of the returned list will be stored here
 *
 *      @return <size_t *> - A list of all the values in #intMap. Size of allocation is stored in #retLen
 *
 *          You are responsible for freeing this list
 */
int *simple_int_map_values(SimpleIntMap *intMap, size_t *retLen);



#endif
