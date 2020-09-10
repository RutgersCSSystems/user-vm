//Please recognize we use the math library and for the program to work you must add
//the -lm flag in the benchmarks to compile
#include "my_vm.h"
#include "bit_methods.c"
#include "hashtable.c"
#ifdef SEPERATE_MATH
#include "math_vm.c"
#endif

void *entire_space;

int num_of_outer_bits;
int num_of_pde;
outer_table_data outer_table;
int num_of_inner_bits;
int num_of_pte;
int page_offset;

unsigned long long number_of_phys_pages;
bool memory_initialized = false;

char *phys_bitmap;

pthread_mutex_t mem_lock = PTHREAD_MUTEX_INITIALIZER;

struct tlb *tlb_entries;

int hits, misses;

inner_table_data *allocate_inner_tabe()
{
    /* Allocates data necessary for an innter page table
    *
    * Returns:
    * inner_table_data: inner_table, represents a pointer to a newly generated
    * inner_table
    *
    */
    inner_table_data *inner_table = malloc(sizeof(inner_table_data));
    assert(inner_table);

    //allocate an array within the inner table to hold all of the data
    inner_table->physical_mapping = calloc(num_of_pte, sizeof(pte_t));
    assert(inner_table->physical_mapping);

    //as well as a bitmap to tell us whether or not an entry has been used
    inner_table->bitmap = calloc(num_of_pte / 8 + 1, 1);
    assert(inner_table->bitmap);

    return inner_table;
}

unsigned long get_pde(void *va)
{
    /*
    * Given a virtual address, extracts its 
    * page directory bits and returns it in the form of a number
    * 
    * Parameters:
    * void * va: The virtual address to extract from
    * 
    * Returns
    * unsigned long: The extracted number
    */
    unsigned long start = page_offset + num_of_inner_bits;
    unsigned long end = start + num_of_outer_bits;
    return get_bits_between(va, start, end);
}

unsigned long get_pte(void *va)
{
    /*
    * Given a virtual address, extracts its inner 
    * page table entry bits and returns it in the form of a number
    * 
    * Parameters:
    * void * va: The virtual address to extract from
    * 
    * Returns
    * unsigned long: The extracted number
    */
    unsigned long start = page_offset;
    unsigned long end = start + num_of_inner_bits;
    return get_bits_between(va, start, end);
}

unsigned long get_offset(void *va)
{
    /*
    * Given a virtual address, extracts its page offset 
    * bits and returns it in the form of a number
    * 
    * Parameters:
    * void * va: The virtual address to extract from
    * 
    * Returns
    * unsigned long: The extracted number
    */
    unsigned long start = 0;
    unsigned long end = page_offset;
    return get_bits_between(va, start, end);
}

unsigned long long get_vert(void *va)
{
    /*
    * Given a virtual address, extracts its 
    * virtual address bits and returns it in the form of a number
    * 
    * Parameters:
    * void * va: The virtual address to extract from
    * 
    * Returns
    * unsigned long: The extracted number
    */
    unsigned long start = page_offset;
    unsigned long end = start + num_of_inner_bits + num_of_outer_bits;
    return get_bits_between(va, start, end);
}

void *combine_virt(pde_t outer, pte_t inner)
{
    /*
    * Given the outer and inner bits of a virtual address
    * combines them into one address with 0 set for the 
    * page offset
    * 
    * Parameters:
    * pde_t outer: Number that represents the outer bits
    * pte_t inner: Number that represents the inner bits
    * 
    * Returns
    * void * va: The virtual address
    */

    //Move the outer bits to the correct location by moving it past all of the
    //inner bits and the page offset
    unsigned long outer_offset = page_offset + num_of_inner_bits;

    //Move the inner bits to the correct location by moving it past the page offset
    unsigned long inner_offset = page_offset;

    //Combine both portions using a bitwise OR to generate the final virtual address
    void *va = (void *)((outer << outer_offset) | (inner << inner_offset));

    return va;
}

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem()
{

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them
    unsigned long long page_size = PGSIZE;
    unsigned long long mem_size = MEMSIZE;
    unsigned long long max_mem_size = MAX_MEMSIZE;

    entire_space = malloc(mem_size);
    assert(entire_space);

    page_offset = log2(page_size);

    num_of_pte = page_size / sizeof(pte_t);

    num_of_inner_bits = log2(num_of_pte);
    number_of_phys_pages = mem_size / page_size;
    phys_bitmap = calloc((number_of_phys_pages) / 8 + 1, 1);
    assert(phys_bitmap);

    num_of_outer_bits = log2(max_mem_size) - (num_of_inner_bits + page_offset);
    num_of_pde = pow(2, num_of_outer_bits);

    //allocate an array within the outer table to hold all of the page table directories
    outer_table.physical_mapping = calloc(num_of_pde, sizeof(pde_t *));
    assert(outer_table.physical_mapping);

    outer_table.bitmap = calloc(num_of_pde / 8 + 1, 1);
    assert(outer_table.bitmap);

    //allocate the first inner table for usage
    outer_table.physical_mapping[0] = allocate_inner_tabe();
    set_bit(&outer_table.bitmap, 0, 1);

    tlb_entries = table_create();
#ifdef BASIC_INFO
    printf("The page offset is: %d\n", page_offset);
    printf("The number of pde is: %d\n", num_of_pde);
    printf("The number of outer bits is: %d\n", num_of_outer_bits);
    printf("The number of pte is: %d\n", num_of_pte);
    printf("The number of inner bits is: %d\n", num_of_inner_bits);
#endif
}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int add_TLB(void *va, void *pa)
{

    // If the VA is NULL, return an error
    if (va == NULL)
    {
        return -1;
    }

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */
    unsigned long long key = get_vert(va);
    table_insert(tlb_entries, key, pa);

    return 0;
}

/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va)
{

    /* Part 2: TLB lookup code here */

    unsigned long long key = get_vert(va);
    void *ret = table_get(tlb_entries, key);

    // The table was calloc'd, meaning if the value returned by ret is 0, it
    // shouldn't contain an actual translation
    if (ret == NULL)
    {
        return NULL;
    }
    return ret;
}

void perform_mat_exp(int SIZE, void *original, void *copy_of_original, void *result, int power)
{
    /*
    * A general function to perform matrix exponentiation on a matrix
    * A copy of the original matrix is needed 
    * (intended for mainly self use for TLB testing)
    * 
    * Parameters:
    * int SIZE: The size of the matrix
    * void * original: The original matrix
    * void * copy_of_origin: Copy of the original matrix
    * void * result: Storage location of the exponentiation result
    * int power: Power to raise the matrix to
    */
    int i = 0, j = 0, k = 0;
    int y;
    int address_a = 0, address_b = 0;
    int address_c = 0;
    // Calls mat_mult power - 1 times, raising the matrix to the given power
    for (i = 0; i < power - 1; ++i)
    {
        mat_mult(original, copy_of_original, SIZE, result);

        // Put the values from result into copy_of_original
        for (k = 0; k < SIZE; k++)
        {
            for (j = 0; j < SIZE; j++)
            {
                address_a = (unsigned int)result + ((k * SIZE * sizeof(int))) + (j * sizeof(int));
                address_b = (unsigned int)copy_of_original + ((k * SIZE * sizeof(int))) + (j * sizeof(int));
                get_value((void *)address_a, &y, sizeof(int));
                put_value((void *)address_b, &y, sizeof(int));
            }
        }
    }
}
/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void print_TLB_missrate()
{

    double miss_rate = 0;

    /*Part 2 Code here to calculate and print the TLB miss rate*/
    hits = 0;
    misses = 0;
    int ARRAY_SIZE = 400;
    int x = 1, SIZE = 3;
    int y, z;
    int i = 0, j = 0, k = 0;
    int address_a = 0, address_b = 0;
    int address_c = 0;
    int num_of_times = 5;
    int times_performed = 0;
    void *original = a_malloc(ARRAY_SIZE);
    void *copy_of_original = a_malloc(ARRAY_SIZE);
    void *result = a_malloc(ARRAY_SIZE);

// Creates two 3x3 matricies filled with 1s
#ifdef TLB_CALC_INFO
    printf("Storing integers to generate a SIZExSIZE matrix\n");
#endif
    for (times_performed = 0; times_performed < num_of_times; times_performed++)
    {
        for (i = 0; i < SIZE; i++)
        {
            for (j = 0; j < SIZE; j++)
            {
                address_a = (unsigned int)original + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
                address_b = (unsigned int)copy_of_original + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
                put_value((void *)address_a, &x, sizeof(int));
                put_value((void *)address_b, &x, sizeof(int));
            }
        }

        perform_mat_exp(SIZE, original, copy_of_original, result, 10);

#ifdef TLB_CALC_INFO
        printf("Output from matrix exponentiation number: %d", times_performed + 1);
        for (i = 0; i < SIZE; i++)
        {
            for (j = 0; j < SIZE; j++)
            {
                address_c = (unsigned int)result + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
                get_value((void *)address_c, &y, sizeof(int));
                printf("%d ", y);
            }
            printf("\n");
        }
        puts("");
#endif
    }
#ifdef TLB_CALC_INFO
    printf("Done with Matrix\n");
#endif
    a_free(original, ARRAY_SIZE);
    a_free(copy_of_original, ARRAY_SIZE);
    a_free(result, ARRAY_SIZE);
    miss_rate = (double)misses / (double)(hits + misses);
    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
    misses = 0;
}

void print_TLB_missrate_2(){
    //A secondary TLB miss rate function for the purposes of going beyond
    //The TLB

    double miss_rate = 0;
    hits = 0;
    misses = 0;

    int NUM_OF_RUNS = 10000;
    int **arr = a_malloc(NUM_OF_RUNS * sizeof(int *));
    int *inner_ptr = 0;
    int val;
    int i;
    for (i = 0; i < NUM_OF_RUNS; ++i){
        inner_ptr = a_malloc(sizeof(int));

        if (inner_ptr == NULL){
            puts("Out of memory.");
            return;
        }
        put_value(inner_ptr, &i, sizeof(int));
        put_value((void *)(arr + i), &inner_ptr, sizeof(int *));
    }

    for (i = 0; i < NUM_OF_RUNS; i++){
        get_value(arr + i, &inner_ptr, sizeof(int *));
        get_value(inner_ptr, &val, sizeof(int));
        if (val != i){
            puts("An error has occured");
            printf("The value was: %d and yet i was: %d, \n The inner pointer was : %x\n\n", val, i, inner_ptr);

            return;
        }
        a_free(inner_ptr, sizeof(int));
    }
    a_free(arr, NUM_OF_RUNS * sizeof(int *));

    miss_rate = (double)misses / (double)(hits + misses);
    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}


/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va)
{
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */
    if (va == NULL)
    {
        return NULL;
    }
    void *tlb_phys_res = check_TLB(va);
    if (tlb_phys_res != NULL)
    {
        ++hits;
        return tlb_phys_res + get_offset(va);
    }
    else
    {
        ++misses;
    }
    if (pgdir == NULL)
    {
        return NULL;
    }
    pde_t page_dir_index = get_pde(va);
    if (!get_bit(outer_table.bitmap, page_dir_index))
    {
        return NULL;
    }
    inner_table_data *inner_table = (inner_table_data *)outer_table.physical_mapping[page_dir_index];
    pte_t page_table_index = get_pte(va);

    if (!get_bit(inner_table->bitmap, page_table_index))
    {
        return NULL;
    }
    else
    {
        void *phys_addr = inner_table->physical_mapping[page_table_index];
        add_TLB(va, phys_addr);
        return phys_addr + get_offset(va);
    }
}

/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int page_map(pde_t *pgdir, void *va, void *pa)
{
    if (pgdir == NULL)
    {
        return -1;
    }
    pde_t page_dir_index = get_pde(va);
    if (!get_bit(outer_table.bitmap, page_dir_index))
    {
        //This location has not yet been mapped to
        //initialize the inner page table for this index
        outer_table.physical_mapping[page_dir_index] = allocate_inner_tabe();
        set_bit(&(outer_table.bitmap), page_dir_index, 1);
        // puts("This has occured as well");
    }
    inner_table_data *inner_table = (inner_table_data *)outer_table.physical_mapping[page_dir_index];
    pte_t page_entry_index = get_pte(va);
    if (!get_bit(inner_table->bitmap, page_entry_index))
    {
        inner_table->physical_mapping[page_entry_index] = pa;
        set_bit(&(inner_table->bitmap), page_entry_index, 1);
        return 0;
    }
    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */
    return -1;
}

/*Function that gets the next available page
*/
void *get_next_avail(int num_pages)
{
    //Use virtual address bitmap to find the next free page
    unsigned long long i = 0;
    unsigned long long j = 0;
    unsigned long long avail_count = 0;
    unsigned long long outer_start = 0;
    unsigned long long inner_start = 0;
    void *va;

    for (i = 0; i < num_of_pde; i++)
    {
        //go through every outer table and find its inner table
        if (get_bit(outer_table.bitmap, i))
        {
            inner_table_data *inner_page_table = (inner_table_data *)outer_table.physical_mapping[i];
            void *inner_bitmap = inner_page_table->bitmap;
            //if it is currently in use go to the inner table and go through that
            for (j = 0; j < num_of_pte; j++)
            {
                if (i == 0 & j == 0)
                {
                    //skip this address, it will map to null so it
                    //can never be used
                    continue;
                }
                if (!get_bit(inner_page_table->bitmap, j))
                {
                    if (avail_count == 0)
                    {
                        inner_start = j;
                        outer_start = i;
                    }
                    //this page is not in use, count it as one that is possible
                    avail_count++;
                }
                else
                {
                    avail_count = 0;
                }

                if (avail_count >= num_pages)
                {
                    //we are done
                    return combine_virt(outer_start, inner_start);
                }
            }
        }
        else
        {
            if (avail_count == 0)
            {
                inner_start = 0;
                outer_start = i;
            }
            //we can use all of the inner pages here
            avail_count += num_of_pte;

            if (avail_count >= num_pages)
            {
                //we are done
                return combine_virt(outer_start, inner_start);
            }
        }
    }
    return NULL;
}

void *get_next_phys()
{
    //find a free physical page

    //this is not necessarily efficent, it is better
    //to just find every single phys necessary
    unsigned long long i;
    for (i = 0; i < number_of_phys_pages; i++)
    {
        //find the first free page we can

        if (!get_bit(phys_bitmap, i))
        {
            set_bit(&phys_bitmap, i, 1);
            unsigned long long page_size = PGSIZE;
            //this page is free, return this
            return entire_space + page_size * i;
        }
    }

    return NULL;
}

/* Function responsible for allocating pages
and used by the benchmark
*/
void *a_malloc(unsigned int num_bytes)
{
    pthread_mutex_lock(&mem_lock);
    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */
    /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */
    if (!memory_initialized)
    {
        set_physical_mem();
        memory_initialized = true;
    }
    unsigned long long page_size = PGSIZE;
    int num_of_pages_needed = ceil((double)num_bytes / page_size);
    void *initial_va = get_next_avail(num_of_pages_needed);
    if (initial_va == NULL)
    {
        pthread_mutex_unlock(&mem_lock);
        return NULL;
    }
    else
    {
        //we got a valid initial virtual address from get_next_avail
        //now for every single page we must allocate, create a page map
        //and finially return the initial va
        int i;
        void *next_virt = initial_va;
        for (i = 0; i < num_of_pages_needed; i++)
        {
            void *next_phys = get_next_phys();
            if (next_phys == NULL)
            {
                return NULL;
            }
            if (page_map((pde_t *)outer_table.physical_mapping, next_virt, next_phys) == -1)
            {
                return NULL;
            }
//Adding to the TLB in a sort of prefetch fashion
#ifndef NO_PREFETCH
            add_TLB(next_virt, next_phys);
            //The misses incrementer has been commented out in response to a piazza post
            //It was originally incremented in order to emulate the first add the TLB as a
            //"Cold miss"
            misses++;
#endif
            next_virt = next_virt + page_size; //move over one virtual page to map that as well
        }
        pthread_mutex_unlock(&mem_lock);
        return initial_va;
    }

    pthread_mutex_unlock(&mem_lock);
    return NULL;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void a_free(void *va, int size)
{
    pthread_mutex_lock(&mem_lock);

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */
    //first test if va+size is a valid translation
    if (translate((pde_t *)outer_table.physical_mapping, va + size) != NULL)
    {
        int to_free_next = 0;
        //free every item necessary
        while (size > to_free_next)
        {
            //first translate the item
            void *phys_addr = translate(((pde_t *)outer_table.physical_mapping), va + to_free_next);
            if (phys_addr == NULL)
            {
                break;
            }
            //remove from hashtable
            unsigned long long key = get_vert(va);
            table_remove(tlb_entries, key);

            //free the phys_addr
            //to free the phys, we need to find the index to free
            unsigned long long page_size = PGSIZE;
            int index_to_free = (phys_addr - entire_space) / page_size;

            set_bit(&phys_bitmap, index_to_free, 0);

            //set the va to be free
            pde_t page_dir_index = get_pde(va);
            inner_table_data *inner_table = (inner_table_data *)outer_table.physical_mapping[page_dir_index];
            pte_t page_table_index = get_pte(va);
            set_bit(&(inner_table->bitmap), page_table_index, 0);
            to_free_next += page_size;
        }
    }

    pthread_mutex_unlock(&mem_lock);
}

unsigned long get_space_left(void *va)
{
    /*
    * Gets the amount of space left in a page
    * given a virtual address
    * 
    * Parameters:
    * void * va: The virtual address
    * 
    * Returns:
    * unsigned long space_left: The amount of space left
    */

    unsigned long page_size = PGSIZE;
    //get the current space we are at
    unsigned long offset = get_offset(va);

    unsigned long space_left = page_size - offset;
    return space_left;
}

int copy_data(void *dst, void *src, int size, unsigned long space_left)
{
    /*
    * Copies data from one location to another a maximum of
    * a page at a time
    * 
    * Parameters: 
    * void * dst: location to copy to
    * void * src: location to copy from
    * int size: amount to copy
    * unsigned long space_left: amount of space left in a page
    * 
    * Returns:
    * int size: amount left to copy
    */

    unsigned long amount_written = 0;
    //if the size to write left is less than the amount left in a page
    //just write the size in
    if (size <= space_left)
    {
        memcpy(dst, src, size);
        amount_written = size;
    }
    else
    {
        //otherwise just copy the maximum we can
        memcpy(dst, src, space_left);
        amount_written = space_left;
    }

    //reduce the amount we have to write by the amount we wrote
    size = size - amount_written;
    return size;
}

/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void put_value(void *va, void *val, int size)
{
    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */

    if (va == NULL || val == NULL)
    {
        return;
    }

    pthread_mutex_lock(&mem_lock);

    while (size > 0)
    {
        //perform a translation first to find the physical address
        void *phys_addr = (void *)translate((pde_t *)outer_table.physical_mapping, va);
        //Actually getting a value from here would cause a segmentation fault
        //but we decided to be "nice" and just not copy the values over
        if (phys_addr == NULL)
        {
            return;
        }
        unsigned long space_left = get_space_left(va);

        size = copy_data(phys_addr, val, size, space_left);

        if (size != 0)
        {
            //we did not finish copying over all the data
            //move over the virtual address a page to finish copying
            va = va + space_left;
        }
    }
    pthread_mutex_unlock(&mem_lock);
}

/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size)
{
    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */

    if (va == NULL || val == NULL)
    {
        return;
    }

    pthread_mutex_lock(&mem_lock);

    //perform the reverse of put_value
    while (size > 0)
    {
        //perform a translation first
        void *phys_addr = (void *)translate((pde_t *)outer_table.physical_mapping, va);

        //Actually getting a value from here would cause a segmentation fault
        //but we decided to be "nice" and just not copy the values over
        if (phys_addr == NULL)
        {
            return;
        }
        //now that we have the physical location write in the value in
        unsigned long space_left = get_space_left(va);

        size = copy_data(val, phys_addr, size, space_left);

        if (size != 0)
        {
            //we did not finish copying over all the data
            //move over the virtual address a page to finish copying
            va = va + space_left;
        }
    }
    pthread_mutex_unlock(&mem_lock);
}

/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer)
{

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */

    int i, j, k;
    int mat_sum = 0;
    int address_a = 0, address_b = 0, address_c = 0;
    int mat1_val = 0, mat2_val = 0;

    for (i = 0; i < size; ++i)
    {
        for (j = 0; j < size; ++j)
        {
            mat_sum = 0;
            for (k = 0; k < size; ++k)
            {
                // Get the address of the indicies we're going to multiply
                address_a = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                address_b = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));

                // Get the value's at those addresses
                get_value((void *)address_a, &mat1_val, sizeof(int));
                get_value((void *)address_b, &mat2_val, sizeof(int));

                // Multiply and add the values
                mat_sum += (mat1_val * mat2_val);
            }

            // Get the address of the correct index in our answer matrix and assign the value to the
            // calculated mat_sum
            address_c = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            put_value((void *)address_c, &mat_sum, sizeof(int));
        }
    }
}
