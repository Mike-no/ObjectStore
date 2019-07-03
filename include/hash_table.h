/**
 * ########################################################
 * 
 * @author: Michael De Angelis
 * @mat: 560049
 * @project: Sistemi Operativi e Laboratorio [SOL]
 * @A.A: 2018 / 2019
 * 
 * ########################################################
 */

#ifndef HASH_TABLE_H_

#define HASH_TABLE_H_

#include "error_control.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <pthread.h>

// Entry of an hash table
struct hash_entry_s{
    char* key;
    int value;
    struct hash_entry_s* next;
};
typedef struct hash_entry_s hash_entry_t;

// Hash table of size "size"
struct hash_table_s{
    long size;
    struct hash_entry_s** hash_table;
};
typedef struct hash_table_s hash_table_t;

/**
 * Create a new hash table of size "size"
 * @return:
 *      hash_table_t* : hash table successfully created
 *      NULL : the creation fails, error(s) occurred, errno setted
 */
hash_table_t* ht_create(long size);

// Hash function
long hash(hash_table_t* hash_table, const char* key);

/**
 * Insert a new entry with key "key" and value "value" in the hash table
 * @return:
 *      1 : entry successfully inserted
 *      0 : key already exists in the hash table
 *      -1 : error(s) occurred, errno setted  
 */ 
int ht_insert(hash_table_t* hash_table, char* key, int value);

/**
 * Look for the entry with key "key" in the hash table
 * @return:
 *      char* : entry found
 *      NULL : entry not found or invalid argument(s)
 */
char* ht_find(hash_table_t* hash_table, char* key);

/**
 * Remove the entry with key "key" from the hash table
 * @return:
 *      1 : entry removed
 *      0 : entry not found
 *      -1 : error(s) occurred, errno setted
 */
int ht_remove(hash_table_t* hash_table, char* key);

/**
 * Remove all the entry from the hash table and then free the hash table
 * @return:
 *      1 : hash table destroyed
 *      0 : error occurred, errno setted
 */
int ht_destroy(hash_table_t* hash_table);

// Print all the hash table content 
// ### Never used
void walk_hash(hash_table_t* hash_table);

#endif // HASH_TABLE_H_
