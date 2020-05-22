 #ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_LEAF 31
#define MAX_INTERNAL 248
#define PAGE_RESERVED 104
#define HEADER_RESERVED 4072
#define PAGE_SIZE 4096
#define NUM_RECORD 120
typedef uint64_t pagenum_t;
// Disk_based

typedef struct record {
    int64_t key;
    char value[120];
} record_t;

typedef struct internal_record{
    int64_t key;
    pagenum_t page_offset;
}internal_record;

typedef struct page_t{
    //for page_header
    pagenum_t next_free_or_parent_page_number;
    pagenum_t right_sibling_or_one_more_page_number;
    int32_t is_Leaf;
    int32_t num_keys;
    char reserved[PAGE_RESERVED];
    union{
        //for leaf_page
        record_t l_records[MAX_LEAF];
        //for internal_page
        internal_record i_records[MAX_INTERNAL];
    };
}__attribute__((packed)) page_t;

typedef struct Header_page {
    pagenum_t free_page_offset;
    pagenum_t root_page_offset;
    int64_t num_of_pages;
    char reserved[HEADER_RESERVED];
}header_t;

extern int fd;
extern header_t* header;
extern page_t* root;
//FUNCTION PROTOTYPES.
//for api 
pagenum_t file_alloc_page();
void file_free_page(pagenum_t pagenum);
void file_read_page(pagenum_t pagenum, page_t* dest);
void file_write_page(pagenum_t pagenum, const page_t* src);
void file_read_header_page(pagenum_t pagenum, header_t* dest);
void file_write_header_page(pagenum_t pagenum, const header_t* src);
// Finding
int cut( int length );//
char* db_find(int64_t key, char* ret_val);//
pagenum_t find_leaf_page(int64_t key, char* ret_val); //
 
// Insertion.
int open_table(char *pathname);//
int db_insert(int64_t key, char* value); //
void insert_into_leaf(pagenum_t leaf_offset, int64_t key, char* value); //
void insert_into_leaf_after_splitting(pagenum_t leaf_offset, int64_t key, char* value); //
void insert_into_parent(pagenum_t left, int64_t new_key, pagenum_t right);//
int get_left_index(pagenum_t left_offset); //
void insert_into_internal(pagenum_t parent_offset, int left_index, int64_t new_key, pagenum_t right_offset);//
void insert_into_internal_after_splitting(pagenum_t parent_offset, int left_index, int64_t new_key, pagenum_t right_offset);//
void insert_into_new_root(pagenum_t left_offset, int64_t new_key, pagenum_t right_offset); //
void start_new_tree(int64_t key, char* value);//

// Deletion.
int db_delete(int64_t key); //
void delete_entry(int64_t key, pagenum_t delete_offset); //
void remove_entry_from_page(int64_t key, pagenum_t delete_offset); //
void redistribute_pages(pagenum_t delete_offset, int left_page_index, pagenum_t left_page_offset, pagenum_t parent_page_offset, int64_t k_prime, int k_prime_index); //
void coalesce_pages(pagenum_t delete_offset, int left_page_index, pagenum_t left_page_offset, pagenum_t parent_page_offset, int64_t k_prime);
void adjust_root(pagenum_t delete_offset);

//test
void find_and_print(int64_t key, char * ret_val);

#endif /* __BPT_H__*/

