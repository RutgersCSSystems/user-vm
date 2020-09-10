//Please recognize we use the math library and for the program to work you must add
//the -lm flag in the benchmarks to compile, if you do not wish to link the math library  in
//simple uncomment "SEPERATE_MATH" or define it somehow
#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

//Uncomment to use our own math library
//#define SEPERATE_MATH

//Uncomment to see output on the bits and number of pages for the 2 level page table
//#define BASIC_INFO

//Uncomment to see extra output for the TLB miss rate calculation, changes result of TLB miss rate
//#define TLB_CALC_INFO

//Uncomment to use the TLB without minor prefetching
//#define NO_PREFETCH

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need
#ifndef SEPERATE_MATH
#include <math.h>
#else
#define log2 logM
#define ceil ceilM
#define pow powM
#endif
#include "pthread.h"
#include "string.h"
#include "assert.h"

#define PGSIZE 4096

// Maximum size of virtual memory -> by default 2^32
#define MAX_MEMSIZE 4ULL * 1024 * 1024 * 1024

// Size of "physcial memory"
#define MEMSIZE 1024 * 1024 * 1024

// Represents a page table entry
typedef unsigned long pte_t;

// Represents a page directory entry
typedef unsigned long pde_t;

//a structure to contain every pde_t entry
typedef struct outer_page_table
{
    void **physical_mapping;
    char *bitmap;
} outer_table_data;

//a structure to contain every pte_t entry
typedef struct inner_page_table
{
    void **physical_mapping;
    char *bitmap;
} inner_table_data;

#define TLB_ENTRIES 512
//Structure to represents TLB
struct tlb
{
    /*Assume your TLB is a direct mapped TLB with number of entries as TLB_ENTRIES
    * Think about the size of each TLB entry that performs virtual to physical
    * address translation.
    */
    unsigned long long *keys;
    void **translations;
};

// We do not use this but rather tread the TLB as a pointer to a hashtable,
// it would be fairly simple to reformat the code to use this (we use a tlb_entries * instead)
struct tlb tlb_store;

void set_physical_mem();
pte_t *translate(pde_t *pgdir, void *va);
int page_map(pde_t *pgdir, void *va, void *pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *a_malloc(unsigned int num_bytes);
void a_free(void *va, int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();

#endif
