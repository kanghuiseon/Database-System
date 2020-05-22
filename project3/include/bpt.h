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
#define NUM_OF_TABLES 10
typedef uint64_t pagenum_t;
// Disk_based
typedef struct record {
    int64_t key;
    char value[NUM_RECORD];
}record_t;

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

//for buffer_manager
typedef struct Buffer{
    union{
        header_t header;
        page_t page;
    };
    int table_id;
    bool ref_bit;
    pagenum_t pagenum;
    bool is_dirty;
    bool is_pinned;
}Buffer;

typedef struct Tables{
    int check;
    int table_id;
    int table_fd;
}Tables;

extern Buffer* buffer_pool;
extern Tables tables[NUM_OF_TABLES];
extern int buffer_size;
extern int num_of_frame;

//for buffer
int init_db(int num_buf);
int open_table(char* pathname);
int close_table(int tableId);
int shutdown_db(void);
//버퍼 내 위치 인덱스 반환 (read)
//만약 버퍼 내에 존재하지 않는다면 load_to_buffer_pool함수를 불러서 버퍼에 올리고 index를 반환한다.
int get_page_index_from_buffer_pool(int tableId, pagenum_t pagenum);
//버퍼에 올리고 위치 인덱스 반환 (write)
int load_header_to_buffer_pool(int tableId, pagenum_t pagenum, header_t* header);
int load_page_to_buffer_pool(int tableId, pagenum_t pagenum, page_t* page);

//FUNCTION PROTOTYPES.
//for api
pagenum_t file_alloc_page(int tableId);
void file_free_page(int tableId, pagenum_t pagenum);
void file_read_page(int tableId, pagenum_t pagenum, page_t* dest);
void file_write_page(int tableId, pagenum_t pagenum, const page_t* src);
void file_read_header_page(int tableId, pagenum_t pagenum, header_t* dest);
void file_write_header_page(int tableId, pagenum_t pagenum, const header_t* src);
int find_table_fd(int tableId);

// Finding
int cut(int length );
int db_find(int tableId, int64_t key, char* ret_val);
int find_leaf_page(int tableId, int64_t key, char* ret_val);
 
// Insertion.
int db_insert(int tableId, int64_t key, char* value);
void insert_into_leaf(int tableId, int leaf_index, int64_t key, char* value);
void insert_into_leaf_after_splitting(int tableId, int leaf_index, int64_t key, char* value);
void insert_into_parent(int tableId, int left_index, int64_t new_key, int right_index);
int get_left_index(int tableId, int left_index);
void insert_into_internal(int tableId, int parent_index , int left_index_p, int64_t new_key, int right_index);
void insert_into_internal_after_splitting(int tableId, int parent_index, int left_index_p, int64_t new_key, int right_index);
void insert_into_new_root(int tableId, int left_index, int64_t new_key, int right_index);
void start_new_tree(int tableId, int64_t key, char* value);

// Deletion.
int db_delete(int tableId, int64_t key);
void delete_entry(int tableId, int64_t key, int delete_index);
void remove_entry_from_page(int tableId, int64_t key, int delete_index);
void redistribute_pages(int tableId, int delete_index, int left_page_index, int left_index, int parent_index, int64_t k_prime, int k_prime_index);
void coalesce_pages(int tableId, int delete_index, int left_page_index, int left_index, int parent_index, int64_t k_prime);
void adjust_root(int tableId, int delete_index);

//test
void find_and_print(int table_id, int64_t key, char * ret_val);

#endif /* __BPT_H__*/

