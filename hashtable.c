#include "my_vm.h"

struct tlb * table_create(){
    // An array of TLBs with size of TLB_ENTRIES with everything zeroed out
    struct tlb *ht = calloc(TLB_ENTRIES, sizeof(struct tlb*));
    assert(ht);
    ht->keys = calloc(TLB_ENTRIES, sizeof(unsigned long long));
    assert(ht->keys);
    ht->translations = calloc(TLB_ENTRIES, sizeof(void *));
    assert(ht->translations);
    return ht;
}

// Inserts into the hash table, we are given the head of the hashtable, the key, and the translation
void table_insert(struct tlb * ht_head, unsigned long long key, void * translation){
    if(ht_head == NULL){
        return;
    }

    // Insert the key and translation into the table at the proper index of key % TLB_ENTRIES
    (ht_head->keys)[key % TLB_ENTRIES] = key;
    (ht_head->translations)[key % TLB_ENTRIES] = translation;
}

// Returns a translation given the hashtable and the key
void * table_get(struct tlb * ht_head, unsigned long long key){
    if(ht_head == NULL){
        return;
    }
    unsigned long long stored_key = (ht_head->keys)[key % TLB_ENTRIES];
    if(stored_key == key){
        return (ht_head->translations)[key % TLB_ENTRIES];
    } else {
        return NULL;
    }
}

// Removes a translation given the hashtable and the key
int table_remove(struct tlb * ht_head, unsigned long long key){
    if(ht_head == NULL){
        return;
    }
    unsigned long long stored_key = (ht_head->keys)[key % TLB_ENTRIES];
    
    if(stored_key == key){
        (ht_head->translations)[key % TLB_ENTRIES] = NULL;
        (ht_head->keys)[key % TLB_ENTRIES] = 0;
        return 0;
    } else {
        return -1;
    }
}

