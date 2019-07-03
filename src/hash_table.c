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

#include "hash_table.h"

// A lock for each hash index
static pthread_mutex_t* mtxs;
// Lock the whoole hash table
static pthread_mutex_t ht_mtx = PTHREAD_MUTEX_INITIALIZER;

hash_table_t* ht_create(long size){
    if(size < 1){
        errno = EINVAL;
        return NULL;
    }

    hash_table_t* hash_table = NULL;

    // Allocates the table
    if((hash_table = (hash_table_t*)malloc(sizeof(hash_table_t))) == NULL)
        return NULL;

    // Allocate the associative array of hash_entry_t pointer
    if((hash_table->hash_table = (hash_entry_t**)malloc(sizeof(hash_entry_t*) * size)) == NULL){
        free(hash_table);
        return NULL;
    }

    // Initializes the pointers to NULL avoiding possibly undefined behavior
    for(int i = 0; i < size; i++)
        hash_table->hash_table[i] = NULL;

    hash_table->size = size;

    // Initializes the indexes locks
    if((mtxs = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * size)) == NULL){
        free(hash_table->hash_table);
        free(hash_table);
        return NULL;
    }
    for(int i = 0; i < size; i++)
        pthread_mutex_init(&mtxs[i], NULL);

    return hash_table;
}

// Java hashing function
long hash(hash_table_t* hash_table, const char* key){
    unsigned long hash = 0;

    for(int i = 0; key[i] != '\0'; i++)
        hash = 31 * hash + key[i];

    return (hash % hash_table->size);
}

int ht_insert(hash_table_t* hash_table, char* key, int value){
    if(hash_table == NULL || key == NULL){
        errno = EINVAL;
        return -1;
    }

    // Get the index where the new entry will be insert
    long index = hash(hash_table, key);

    ec_sc_th(pthread_mutex_lock(&mtxs[index]), PTHREAD_LOCK_ERR);

    // checks if "key" already exists in the hash table, cannot have duplicates
    hash_entry_t* cur = hash_table->hash_table[index];
    while(cur != NULL){
        if(strcmp(cur->key, key) == 0){
            ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);
            return 0;
        }
        cur = cur->next;
    }

    hash_entry_t* new_entry = NULL;
    if((new_entry = (hash_entry_t*)malloc(sizeof(hash_entry_t))) == NULL){
        ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);
        return -1;
    }

    // Initialize the new entry
    if((new_entry->key = (char*)calloc(strlen(key) + 1, sizeof(char))) == NULL){
        ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);
        free(new_entry);
        return -1;
    }
    strncpy(new_entry->key, key, strlen(key));
    new_entry->value = value;
    new_entry->next = NULL;
    
    // Insert in head the new entry at list "index"
    new_entry->next = hash_table->hash_table[index];
    hash_table->hash_table[index] = new_entry;

    ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);

    return 1;
}

char* ht_find(hash_table_t* hash_table, char* key){
    if(hash_table == NULL || key == NULL){
        errno = EINVAL;
        return NULL;
    }

    // Get the index where look for the entry requested
    long index = hash(hash_table, key);

    ec_sc_th(pthread_mutex_lock(&mtxs[index]), PTHREAD_LOCK_ERR);

    hash_entry_t* cur = hash_table->hash_table[index];
    // Look for the key
    while(cur != NULL){
        if(strcmp(cur->key, key) == 0){
            ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);
            return cur->key;
        }
        cur = cur->next;
    }

    ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);

    // Key not found
    return NULL;
}

int ht_remove(hash_table_t* hash_table, char* key){
    if(hash_table == NULL || key == NULL){
        errno = EINVAL;
        return -1;
    }

    // Get the index where look for the entry to remove
    long index = hash(hash_table, key); 

    ec_sc_th(pthread_mutex_lock(&mtxs[index]), PTHREAD_LOCK_ERR);

    hash_entry_t* cur = hash_table->hash_table[index];
    hash_entry_t* prev = NULL;
    // Looks for the key
    while((strcmp(cur->key, key) != 0) && cur->next != NULL){
        prev = cur;
        cur = cur->next;
    }

    // Key found, proceed to remove the entry
    if(strcmp(cur->key, key) == 0){
        if(prev)
            prev->next = cur->next;
        else
            hash_table->hash_table[index] = cur->next;
        free(cur->key);
        free(cur);

        ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);

        return 1;
    }

    ec_sc_th(pthread_mutex_unlock(&mtxs[index]), PTHREAD_UNLOCK_ERR);
    
    // Entry not found
    return 0;
}

int ht_destroy(hash_table_t* hash_table){
    if(hash_table == NULL){
        errno = EINVAL;
        return 0;
    }

    ec_sc_th(pthread_mutex_lock(&ht_mtx), PTHREAD_LOCK_ERR);

    // Free all the entries
    for(int i = 0; i < hash_table->size; i++){
        hash_entry_t* cur = NULL;
        while((cur = hash_table->hash_table[i]) != NULL){
            hash_table->hash_table[i] = hash_table->hash_table[i]->next;
            free(cur->key);
            free(cur); 
        }
    }

    // Free the hash_table
    if(hash_table->hash_table) 
        free(hash_table->hash_table);
    
    if(hash_table) 
        free(hash_table);
    
    if(mtxs) 
        free(mtxs);

    ec_sc_th(pthread_mutex_unlock(&ht_mtx), PTHREAD_UNLOCK_ERR);

    return 1;
}

//Never used
void walk_hash(hash_table_t* hash_table){
    for(int i = 0; i < hash_table->size; i++){
        hash_entry_t* cur = hash_table->hash_table[i];
        while(cur != NULL){
            fprintf(stdout, " %s ", cur->key);
            cur = cur->next;
        }
    }
}
