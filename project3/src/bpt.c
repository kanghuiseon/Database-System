/*
 *  bpt.c  
 */
#define Version "1.14"
#include "bpt.h"
Buffer * buffer_pool;
Tables tables[NUM_OF_TABLES];
int buffer_size=0;
int num_of_frame=0;

//table_id를 이용해서 해당 table의 fd값을 가져오는 함수
int find_table_fd(int tableId){
    int fd,i;
    //tableId와 같은 table list내의 table_id를 찾아서 해당 테이블의 fd값을 가져온다.
    for(i=0; i<NUM_OF_TABLES; i++){
        if(tableId == tables[i].table_id){
            fd = tables[i].table_fd;
            break;
        }
    }
    //만약 존재하지 않는다면 -1 반환
    if(i>=10){
        printf("there are not existed such table!\n");
        return -1;
    }
    return fd;
}

void file_read_page(int tableId, pagenum_t pagenum, page_t* dest){
    int fd = find_table_fd(tableId);
    if(fd < 0){
        printf("(read page) cannot find table_fd\n");
        return;
    }
    if(pread(fd, dest, sizeof(page_t), pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"cannot read page %s\n",strerror(errno));
        return;
    }
}
void file_write_page(int tableId, pagenum_t pagenum, const page_t* src){
    int fd = find_table_fd(tableId);
    if(fd < 0){
        printf("(write page) cannot find table_fd\n");
        return;
    }
    if(pwrite(fd, src, PAGE_SIZE, pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"cannot write page %s\n",strerror(errno));
        return;
    }
}
void file_read_header_page(int tableId, pagenum_t pagenum, header_t* dest){
    int fd = find_table_fd(tableId);
    if(fd < 0){
        printf("(read header) cannot find table_fd\n");
        return;
    }
    if(pread(fd, dest, sizeof(header_t), pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"cannot read page %s\n",strerror(errno));
        return;
    }
}
void file_write_header_page(int tableId, pagenum_t pagenum, const header_t* src){
    int fd = find_table_fd(tableId);
    if(fd < 0){
        printf("(write header) cannot find table_fd\n");
        return;
    }
    if(pwrite(fd, src, sizeof(header_t), pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"(write)cannot find header page %s\n",strerror(errno));
        return;
    }
}
int init_db(int num_buf){
    int i;
    
    //num_buf수만큼의 buffer 생성
    buffer_pool = (Buffer*)malloc(sizeof(Buffer)*num_buf);
    if(buffer_pool == NULL)
        return -1;
    buffer_size = num_buf;
    //db를 열자마자 각 table을 아직 사용하지 않는다는 의미로 check=0 을 설정해준다.
    for(i=0; i<NUM_OF_TABLES; i++){
        tables[i].check = 0;
        tables[i].table_id = i+1;
    }
 //   printf("init_db\n");
    return 0;
}
           
int open_table(char* pathname){
    int tableId;
    int fd = open(pathname, O_RDWR | O_EXCL);
    if(fd == -1){
        //만약 존재하지 않는다면 새로운 파일을 생성한다.
        fd = open(pathname, O_RDWR | O_CREAT, S_IRWXU);
        if(fd ==-1){
            fprintf(stderr,"cannot open table %s\n",strerror(errno));
            return -1;
        }
        int i;
        for(int i=0; i<NUM_OF_TABLES; i++){
            //tables에서 비어있는 공간을 찾는다.
            if(tables[i].check == 0){
                //사용한다는 의미로 check를 1로 바꿔주고
                tables[i].check = 1;
                //fd 값과 id 값을 설정해준다.
                tables[i].table_fd = fd;
                tables[i].table_id = i+1;
                tableId = i+1;
                break;
            }
        }
        //만약 table_list에 공간이 없다면 error
        if(i==10){
            printf("There are not existed any space!\n");
            return -1;
        }
        header_t* header = (header_t*)malloc(sizeof(header_t));
        header->free_page_offset = 0;
        header->root_page_offset = 0;
        header->num_of_pages = 1;
        //fd값의 파일에 header 디스크에 먼저 써준다.
        file_write_header_page(tableId, 0, header);
        //header파일을 버퍼에 올린다.
        int header_index = get_page_index_from_buffer_pool(tableId, 0);
        buffer_pool[header_index].header = *header;
    }
    //만약 존재한다면
    else{
        int i;
        for(i=0; i<NUM_OF_TABLES; i++){
            //check가 되어있지않는 곳에 집어넣는다.
            if(tables[i].check==0){
                tables[i].check=1;
                tables[i].table_id = i+1;
                tables[i].table_fd = fd;
                tableId = i+1;
                break;
            }
        }
        //만약 table_list에 공간이 없다면 error
        if(i>=10){
            printf("There are not existed any space!\n");
            return -1;
        }
        //header파일을 읽어와서 버퍼에 올린다.
        int header_index = get_page_index_from_buffer_pool(tableId, 0);
        //root도 버퍼에 올린다.
        int root_index = get_page_index_from_buffer_pool(tableId, buffer_pool[header_index].header.root_page_offset);
    }
 //       printf("table_id: %d\n", tables[tableId-1].table_id);
    return tableId;
}
//buffer_pool을 확인해서 만약 pagenum이 존재하면 page index를 반환한다.
//만약 존재하지 않는다면 load를 한 후 index를 반환한다.
int get_page_index_from_buffer_pool(int tableId, pagenum_t pagenum){
    int i, index;
    //그냥 일반 페이지일 경우,
    if(pagenum != 0){
        page_t* page = (page_t*)malloc(sizeof(page_t));
        //buffer를 확인해서 table_id 와 pagenum이 같은 페이지가 있는 지 찾는다.
        for(i=0; i<buffer_size; i++){
            //만약에 해당 pagenum을 가진 페이지가 존재한다면 break;
            //i가 index
            if(buffer_pool[i].pagenum == pagenum && buffer_pool[i].table_id == tableId){
                break;
            }
        }
        //존재하지 않는다면
        if(i>=buffer_size){
            //디스크에서 해당 페이지를 읽어와서 버퍼에 올린다.
            file_read_page(tableId, pagenum, page);
            index = load_page_to_buffer_pool(tableId, pagenum, page);
            return index;
        }
        //존재한다면 index를 return 해준다.
        return i;
    }
    //만약 header페이지라면
    else{
        header_t* header = (header_t*)malloc(sizeof(header_t));
        //페이지가 위치한 index를 찾는다.
        for(i=0; i<buffer_size; i++){
            if(buffer_pool[i].pagenum == pagenum && buffer_pool[i].table_id == tableId){
                break;
            }
        }
        //만약 버퍼에 header페이지가 존재하지 않는다면
        if(i>=buffer_size){
            //디스크에서 해당 페이지를 읽어와서 버퍼에 올린다.
            file_read_header_page(tableId, 0, header);
            index = load_header_to_buffer_pool(tableId, 0, header);
            return index;
        }
        return i;
    }
    //실패한다면
    return -1;
}

//table을 열 때, header를 버퍼에 미리 올려둔다.
//해당 페이지가 올라간 버퍼의 인덱스를 반환한다.
//버퍼가 꽉찼을 때, 바꿀 페이지의 Is_dirty가 1이라면 디스크에 먼저 쓰고 바꿔준다.
int load_header_to_buffer_pool(int tableId, pagenum_t pagenum, header_t* header){
    //만약 꽉 찼을 경우,
    if(num_of_frame >= buffer_size){
        int i;
        while(1){
            //buffer_size만큼 돌면서 넣을 공간을 찾는다.
            for(i=0; i<buffer_size; i++){
                //만약 pin이 꽂혀있다면 넘어간다.
                if(buffer_pool[i].is_pinned == true)
                    continue;
                //만약 pin은 없지만 ref_bit가 1이라면 0으로 바꿔주고 넘어간다.
                if(buffer_pool[i].ref_bit == true){
                    buffer_pool[i].ref_bit = false;
                    continue;
                }
                
                //버퍼에 있는 페이지의 is_dirty가 true라면 내용을 디스크에 써준다.
                if(buffer_pool[i].is_dirty == true){
                    if(buffer_pool[i].pagenum == 0){
                        file_write_header_page(buffer_pool[i].table_id, buffer_pool[i].pagenum, &buffer_pool[i].header);
                    }
                    else{
                        file_write_page(buffer_pool[i].table_id, buffer_pool[i].pagenum, &buffer_pool[i].page);
                    }
                }
                buffer_pool[i].header = *header;
                buffer_pool[i].table_id = tableId;
                buffer_pool[i].ref_bit = true;
                buffer_pool[i].pagenum = pagenum;
                buffer_pool[i].is_dirty = false;
                buffer_pool[i].is_pinned = false;
                
                return i;
            }//for end
        }//while end
    }
    //buffer에 공간이 남아있을 경우,
    else if(num_of_frame < buffer_size){
        buffer_pool[num_of_frame].header = *header;
        buffer_pool[num_of_frame].table_id = tableId;
        buffer_pool[num_of_frame].ref_bit = true;
        buffer_pool[num_of_frame].pagenum = pagenum;
        buffer_pool[num_of_frame].is_dirty = false;
        buffer_pool[num_of_frame].is_pinned = false;
        num_of_frame++;
        return num_of_frame-1;
    }
    return -1;
}
//버퍼에 page를 올리기 위한 함수
//page가 버퍼 어디에 올라가있는지 Index를 반환한다.
int load_page_to_buffer_pool(int tableId, pagenum_t pagenum, page_t* page){
    //만약 buffer가 꽉 찼을 경우
    //printf("load_page_to_buffer_pool\n");
    if(num_of_frame >= buffer_size){
        int i;
        //buffer_size만큼 돌면서 넣을 공간을 찾는다.
        while(1){
            for(i=0; i<buffer_size; i++){
             //   printf("b\n");
                //만약 pin이 꽂혀있다면 넘어간다.
                if(buffer_pool[i].is_pinned == true){
            //        printf("buffer_pool[i].is_pinned = %d\n",buffer_pool[i].is_pinned);
                    continue;
                }
            //    printf("buffer_pool[i].ref_bit = %d\n",buffer_pool[i].ref_bit);
                //만약 pin은 없지만 ref_bit가 1이라면 0으로 바꿔주고 넘어간다.
                if(buffer_pool[i].ref_bit ==true){
                    buffer_pool[i].ref_bit = false;
                    continue;
                }
                //버퍼에 있는 페이지의 is_dirty가 true라면 내용을 디스크에 써준다.
                if(buffer_pool[i].is_dirty == true){
                    if(buffer_pool[i].pagenum == 0){
                        file_write_header_page(buffer_pool[i].table_id, buffer_pool[i].pagenum, &buffer_pool[i].header);
                    }
                    else{
                        file_write_page(buffer_pool[i].table_id, buffer_pool[i].pagenum, &buffer_pool[i].page);
                    }
                }
                buffer_pool[i].page = *page;
                buffer_pool[i].table_id = tableId;
                buffer_pool[i].ref_bit = true;
                buffer_pool[i].pagenum = pagenum;
                buffer_pool[i].is_dirty = false;
                buffer_pool[i].is_pinned = false;
                num_of_frame++;
                return i;
            }//for end
        }//while end
    }
    //buffer에 공간이 남아있을 경우
    else if(num_of_frame < buffer_size){
        buffer_pool[num_of_frame].page = *page;
        buffer_pool[num_of_frame].table_id = tableId;
        buffer_pool[num_of_frame].ref_bit = true;
        buffer_pool[num_of_frame].pagenum = pagenum;
        buffer_pool[num_of_frame].is_dirty = false;
        buffer_pool[num_of_frame].is_pinned = false;
        num_of_frame++;
        return num_of_frame-1;
    }
    return -1;
}
int close_table(int tableId){
    int i;
    for(i=0; i<buffer_size; i++){
        if(buffer_pool[i].table_id == tableId){
            //만약 변경된적이 있었다면 디스크에 써준다.
            if(buffer_pool[i].is_dirty == true){
                //만약 header라면
                if(buffer_pool[i].pagenum == 0){
                    file_write_header_page(tableId, buffer_pool[i].pagenum, &buffer_pool[i].header);
                }
                //만약 page라면
                else{
                    file_write_page(tableId, buffer_pool[i].pagenum, &buffer_pool[i].page);
                }
            }
            //모두 초기화 해준다.
            buffer_pool[i].table_id = -1;
            buffer_pool[i].ref_bit = false;
            buffer_pool[i].pagenum = 0;
            buffer_pool[i].is_dirty = false;
            buffer_pool[i].is_pinned = false;
            break;
        }
    }
    //tables 배열에서도 빼준다.
    tables[tableId-1].check = 0;
    //fd값을 이용하여 파일을 닫는다.
    close(tables[tableId-1].table_fd);
    return 0;
}

int shutdown_db(void){
    int i;
    //table이 사용되고 있으면 닫아준다.
    for(i=0; i<NUM_OF_TABLES; i++){
        if(tables[i].check == 1)
            close_table(tables[i].table_id);
    }
    free(buffer_pool);
    return 0;
}
//free page의 pagenum을 return한다.
pagenum_t file_alloc_page(int tableId){
    //tableId에 해당하는 table fd를 불러온다.
    int fd = find_table_fd(tableId);
    //버퍼에서 헤더를 불러온다.
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    
    pagenum_t last_offset;
    pagenum_t free_offset = buffer_pool[header_index].header.free_page_offset;
    page_t* free_page = (page_t*)malloc(sizeof(page_t));
    //free page가 없으면
    if(free_offset == 0){
        //파일의 끝 위치를 찾고
        last_offset = lseek(fd, 0, SEEK_END);
        if(last_offset == -1){
            fprintf(stderr,"cannot find offset %s\n",strerror(errno));
            return -1;
        }
        //헤더의 페이지 수를 늘리고 버퍼의 is_dirty를 1로 설정하고
        //버퍼에 새로 올려준다.
        buffer_pool[header_index].header.num_of_pages++;
        buffer_pool[header_index].is_dirty = true;

        //새로만든 free_page는 그냥 디스크에 써준다.
        free_page->right_sibling_or_one_more_page_number = 0;
        free_page->next_free_or_parent_page_number = 0;
        free_page->is_Leaf = 0;
        free_page->num_keys=0;
        file_write_page(tableId, last_offset/PAGE_SIZE, free_page);
        return last_offset/PAGE_SIZE;
    }
    return free_offset;
}
void file_free_page(int tableId, pagenum_t pagenum){
    int fd = find_table_fd(tableId);
    //버퍼 내의 header의 위치를 읽어온다.
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    //버퍼 내의 페이지의 위치를 읽어온다.
    int page_index = get_page_index_from_buffer_pool(tableId, pagenum);
    
    //header
    buffer_pool[header_index].header.free_page_offset = pagenum;
    buffer_pool[header_index].is_dirty = true;
    
    //page
    buffer_pool[page_index].page.next_free_or_parent_page_number = buffer_pool[header_index].header.free_page_offset;
    buffer_pool[page_index].page.is_Leaf = 0;
    buffer_pool[page_index].page.num_keys = 0;
    buffer_pool[page_index].page.right_sibling_or_one_more_page_number = 0;
    buffer_pool[page_index].is_dirty = true;
    buffer_pool[page_index].pagenum = pagenum;
}

int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}

int db_find(int tableId, int64_t key, char* ret_val){
    //find_leaf_page함수를 이용해 key가 존재하는 leaf page를 찾고 페이지 번호를 반환한다.
    //find_leaf_page에서 header 와 root~leaf page에 pin을 꽂아준다.
    int page_index = find_leaf_page(tableId, key, ret_val);
    int i;
    //page가 buffer에 존재하지 않으면
    if(page_index == -1){
        buffer_pool[page_index].is_pinned = false;
        free(ret_val);
        return 0;
    }
    //key가 존재하는지 찾는다.
    for(i=0; i < buffer_pool[page_index].page.num_keys; i++){
        if(buffer_pool[page_index].page.l_records[i].key == key)
            break;
    }
    //만약에 없으면 0
    if(i == buffer_pool[page_index].page.num_keys){
        buffer_pool[page_index].is_pinned = false;
        free(ret_val);
        return 0;
    }
    //있으면 1을 return
    else{
        strcpy(ret_val, buffer_pool[page_index].page.l_records[i].value);
        buffer_pool[page_index].is_pinned = false;
        return 1;
    }
}

int find_leaf_page(int tableId, int64_t key, char* ret_val){
    int i;
    //header를 버퍼에 올린다.
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    //p_offset은 해당 table의 root의 위치 (pagenum)
    pagenum_t p_offset = buffer_pool[header_index].header.root_page_offset;
    //root를 버퍼에 올린다.
    int root_index = get_page_index_from_buffer_pool(tableId, p_offset);
    buffer_pool[root_index].is_pinned = true;
    int page_index = root_index;
    
    //root가 존재하지 않는다면
    if(buffer_pool[header_index].header.root_page_offset == 0){
        printf("Empty tree.\n");
        return -1;
    }
    //p가 leaf node일 때까지 돈다.
    while(!buffer_pool[page_index].page.is_Leaf){
        i=0;
        while(i < buffer_pool[page_index].page.num_keys){
            if(key >= buffer_pool[page_index].page.i_records[i].key)
                i++;
            else break;
        }
        //만약에 i==0이면 가장 왼쪽에 있는 key이다.
        if(i==0)
            p_offset = buffer_pool[page_index].page.right_sibling_or_one_more_page_number;
        else
            //i번째의 key값이 넣으려는 key값보다 크므로 i-1번째의 page에다가 넣는다.
            p_offset = buffer_pool[page_index].page.i_records[i-1].page_offset;
        //해당 p_offset을 p로 다시 읽어들인다. is_Leaf==1일때까지 위의 행동을 반복한다.
        buffer_pool[page_index].is_pinned = false;
        page_index = get_page_index_from_buffer_pool(tableId, p_offset);
        buffer_pool[page_index].is_pinned = true;
    }
    //buffer에서 page의 위치 반환
    return page_index;
}
int db_insert(int tableId, int64_t key, char* value){
    //tableId에 해당하는 header의 인덱스를 불러온다.
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    pagenum_t root_offset = buffer_pool[header_index].header.root_page_offset;
    //key가 존재하는 페이지의 index
    int leaf_index;
    //root가 존재하지 않는다면
    if(root_offset == 0){
        start_new_tree(tableId, key, value);
        return 0;
    }
    char* tmp_value = (char*)malloc(sizeof(char)*NUM_RECORD);
    //이미 키가 존재하면
    if(db_find(tableId, key, tmp_value) == 1){
        printf("key is already existed!\n");
        free(tmp_value);
        return -1;
    }
    //존재하지 않는다면
    leaf_index = find_leaf_page(tableId, key, value);
   // printf("leaf_index : %d\n", buffer_pool[leaf_index].is_pinned); //true;
    //넣을 공간이 있으면
    if(buffer_pool[leaf_index].page.num_keys < MAX_LEAF){
        buffer_pool[leaf_index].is_pinned = true;
        insert_into_leaf(tableId, leaf_index, key, value);
   //     printf("leaf_index : %d\n", buffer_pool[leaf_index].is_pinned); //false
        return 0;
    }
    //없으면 split후 insert
    buffer_pool[leaf_index].is_pinned = true;
    insert_into_leaf_after_splitting(tableId, leaf_index, key, value);
   // printf("leaf_index : %d\n", buffer_pool[leaf_index].is_pinned); // false
    return 0;
}

void insert_into_leaf(int tableId, int leaf_index, int64_t key, char* value){
    int i, insertion_point=0;
    
    while(insertion_point < buffer_pool[leaf_index].page.num_keys && buffer_pool[leaf_index].page.l_records[insertion_point].key < key)
        insertion_point++;
    
    for(i=buffer_pool[leaf_index].page.num_keys; i>insertion_point; i--)
        buffer_pool[leaf_index].page.l_records[i] = buffer_pool[leaf_index].page.l_records[i-1];
    
    buffer_pool[leaf_index].page.l_records[insertion_point].key = key;
    strcpy(buffer_pool[leaf_index].page.l_records[insertion_point].value, value);
    buffer_pool[leaf_index].page.num_keys++;
    buffer_pool[leaf_index].is_dirty = true;
    buffer_pool[leaf_index].is_pinned = false;
}
void insert_into_leaf_after_splitting(int tableId, int leaf_index, int64_t key, char* value){
  //  printf("insert_into_leaf_after_splitting\n");
    int i, j, split;
    //새로운 leaf를 만들어 준다.
    pagenum_t new_leaf_offset = file_alloc_page(tableId);
  //  printf("before_new_leaf_index\n");
    int new_leaf_index = get_page_index_from_buffer_pool(tableId, new_leaf_offset);
  //  printf("after_new_leaf_index : %d\n", new_leaf_index);
    buffer_pool[new_leaf_index].is_pinned = true;
    buffer_pool[new_leaf_index].page.is_Leaf = 1;
    
    int insertion_index = 0;
    //key값을 넣을 index를 찾는다.
    while(insertion_index < MAX_LEAF && buffer_pool[leaf_index].page.l_records[insertion_index].key < key)
        insertion_index++;
  //  printf("insertion_index : %d\n", insertion_index); //insertion_index == 31
    record_t* tmp_record = (record_t*)malloc(sizeof(record_t)*(MAX_LEAF+1));
    //j가 insertion_index와 같다면 j++을 해주어서 key값을 넣을 곳을 비워둔다.
    //ex 만약 key가 7이라면
    //tmp : 1 4 5   9 10
    for(i=0, j=0; i<buffer_pool[leaf_index].page.num_keys; i++, j++){
        if(j==insertion_index)
            j++;
        tmp_record[j].key = buffer_pool[leaf_index].page.l_records[i].key;
        strcpy(tmp_record[j].value, buffer_pool[leaf_index].page.l_records[i].value);
    }
    //insertion_index자리에 record를 넣어주고 leaf에 다시 넣어야 하므로 num_keys값을 0으로 초기화 해준다.
    //split변수 : split할 부분
    tmp_record[insertion_index].key = key;
    strcpy(tmp_record[insertion_index].value, value);
     buffer_pool[leaf_index].page.num_keys=0;
    split = cut(MAX_LEAF);
    //split 이전까지의 값들을 복사한다.
    for(i=0; i<split; i++){
        buffer_pool[leaf_index].page.l_records[i].key = tmp_record[i].key;
        strcpy(buffer_pool[leaf_index].page.l_records[i].value, tmp_record[i].value);
        buffer_pool[leaf_index].page.num_keys++;
    }
  //split 이후의 값들을 복사한다.
    for(i=split, j=0; i<MAX_LEAF+1; i++, j++){
        buffer_pool[new_leaf_index].page.l_records[j].key = tmp_record[i].key;
        strcpy(buffer_pool[new_leaf_index].page.l_records[j].value, tmp_record[i].value);
         buffer_pool[new_leaf_index].page.num_keys++;
    }
    //원래의 leaf가 가리키던 오른쪽 형제를 새로운 leaf가 가리키게 하고 leaf의 오른쪽을 새로운 leaf로 지정한다.
     buffer_pool[new_leaf_index].page.right_sibling_or_one_more_page_number =  buffer_pool[leaf_index].page.right_sibling_or_one_more_page_number;
     buffer_pool[leaf_index].page.right_sibling_or_one_more_page_number = new_leaf_offset;
    //원래 page에 있던 값들은 0으로 초기화 시켜준다.
    for(i= buffer_pool[leaf_index].page.num_keys; i<MAX_LEAF; i++)
         buffer_pool[leaf_index].page.l_records[i].key = 0;
    for(i= buffer_pool[new_leaf_index].page.num_keys; i<MAX_LEAF; i++)
         buffer_pool[new_leaf_index].page.l_records[i].key = 0;
    
     buffer_pool[new_leaf_index].page.next_free_or_parent_page_number =  buffer_pool[leaf_index].page.next_free_or_parent_page_number;
    int64_t new_key;
    //new_key : parent에 들어갈 새로운 키
    new_key =  buffer_pool[new_leaf_index].page.l_records[0].key;
    buffer_pool[leaf_index].is_dirty = true;
    buffer_pool[new_leaf_index].is_dirty = true;
 //   printf("leaf -> parent\n");
    insert_into_parent(tableId, leaf_index, new_key, new_leaf_index);
    return;
}
void insert_into_parent(int tableId, int left_index, int64_t new_key, int right_index){
  //  printf("insert_into_parent\n");
 //   printf("left index pagenum : %llu\n", buffer_pool[left_index].pagenum);
    pagenum_t parent_offset = buffer_pool[left_index].page.next_free_or_parent_page_number;
  //  printf("before_parent_index\n");
    int parent_index = get_page_index_from_buffer_pool(tableId, parent_offset);
  //  printf("after_parent_index : %d\n", parent_index);
    buffer_pool[parent_index].is_dirty = true;
    buffer_pool[parent_index].is_pinned = true;
    //parent가 없다면 새로운 root를 만들어서 key를 넣어준다.
  //  printf("parnet offset : %llu\n", parent_offset);
    if(parent_offset==0){
  //      printf("left_index : %d\n", buffer_pool[left_index].is_pinned);
   //     printf("right_index : %d\n", buffer_pool[right_index].is_pinned);
        insert_into_new_root(tableId, left_index, new_key, right_index);
   //     printf("left_index : %d\n", buffer_pool[left_index].is_pinned);
    //    printf("right_index : %d\n", buffer_pool[right_index].is_pinned);
        return;
    }
    int left_index_p;
    left_index_p = get_left_index(tableId, left_index);
   // printf("left_index_p : %d\n", left_index_p);
   // printf("left_index_pagenum : %llu\n",buffer_pool[left_index].pagenum);
    buffer_pool[left_index].is_pinned = false;
    
    if(buffer_pool[parent_index].page.num_keys >= MAX_INTERNAL){
        insert_into_internal_after_splitting(tableId, parent_index, left_index_p, new_key, right_index);
        return;
    }
    insert_into_internal(tableId, parent_index, left_index_p, new_key, right_index);
    return;
}

int get_left_index(int tableId, int left_index){
    int left_index_p;
    int parent_index = get_page_index_from_buffer_pool(tableId, buffer_pool[left_index].page.next_free_or_parent_page_number);
    buffer_pool[parent_index].is_pinned = true;
    //만약에 left_offset이 one_more_page이면 -1을 return
    if(buffer_pool[left_index].pagenum == buffer_pool[parent_index].page.right_sibling_or_one_more_page_number){
        //buffer_pool[parent_index].is_pinned = false;
        return -1;
    }
    //부모가 가리키는 page_offset과 left_offset이 같을때까지 index++을 해준다.
    for(left_index_p=0; left_index_p < buffer_pool[parent_index].page.num_keys; left_index_p++){
        if(buffer_pool[parent_index].page.i_records[left_index_p].page_offset == buffer_pool[left_index].pagenum)
            break;
    }
    //만약에 없으면  -1을 return해준다.
    if(left_index_p == buffer_pool[parent_index].page.num_keys){
        //buffer_pool[parent_index].is_pinned = false;
        return -1;
    }
    //buffer_pool[parent_index].is_pinned = false;
    return left_index_p;
}

void insert_into_internal(int tableId, int parent_index, int left_index_p, int64_t new_key, int right_index){
    //printf("insert_into_internal\n");
    int i;
    //left page 옆에다가 만들어야하므로 left_index+1자리를 비워두고 오른쪽으로 한칸씩 옮겨준다.
    for(i=buffer_pool[parent_index].page.num_keys; i>left_index_p+1; i--){
        buffer_pool[parent_index].page.i_records[i].page_offset = buffer_pool[parent_index].page.i_records[i-1].page_offset;
        buffer_pool[parent_index].page.i_records[i].key = buffer_pool[parent_index].page.i_records[i-1].key;
    }
    //left page 옆에다가 만들어야하므로 left_index+1에다가 집어넣는다.
    buffer_pool[parent_index].page.i_records[left_index_p+1].page_offset = buffer_pool[right_index].pagenum;
    buffer_pool[parent_index].page.i_records[left_index_p+1].key = new_key;
    buffer_pool[parent_index].page.num_keys++;
    
    buffer_pool[parent_index].is_dirty = true;
    buffer_pool[parent_index].is_pinned = false;
    buffer_pool[right_index].is_pinned = false;
    return;
}
void insert_into_internal_after_splitting(int tableId, int parent_index, int left_index_p, int64_t new_key, int right_index){
 //   printf("insert_into_internal_after_splitting\n");
    int i, j, split;
    pagenum_t new_parent_offset = file_alloc_page(tableId);
    int new_parent_index = get_page_index_from_buffer_pool(tableId, new_parent_offset);
    buffer_pool[new_parent_index].is_pinned = true;
    internal_record* tmp_record = (internal_record*)malloc(sizeof(internal_record)*(MAX_INTERNAL+1));
    //record가 들어갈 자리를 비워주고 tmp_record에 넣어준다.
    for(i=0, j=0; i<buffer_pool[parent_index].page.num_keys; i++, j++){
        //left_index+1이 들어갈 자리
        if(j==left_index_p+1)
            j++;
        tmp_record[j].key = buffer_pool[parent_index].page.i_records[i].key;
        tmp_record[j].page_offset = buffer_pool[parent_index].page.i_records[i].page_offset;
    }
    //오른쪽 key가 들어갈 자리에 넣어준다.
    tmp_record[left_index_p+1].key = new_key;
    tmp_record[left_index_p+1].page_offset = buffer_pool[right_index].pagenum;
    
    split = cut(MAX_INTERNAL);
    buffer_pool[parent_index].page.num_keys=0;
    //split 전까지의 key값을 parent에다가 넣어줌
    //for문이 끝나면 i==split
    for(i=0; i<split; i++){
        buffer_pool[parent_index].page.i_records[i].key = tmp_record[i].key;
        buffer_pool[parent_index].page.i_records[i].page_offset = tmp_record[i].page_offset;
        buffer_pool[parent_index].page.num_keys++;
    }
    //부모한테 넘겨줄 값
    int64_t k_prime;
    //split위치의 다음 페이지의 key값이 부모에게 갈 값이다.
    k_prime = tmp_record[i+1].key;
    //split위치가 제일 왼쪽 페이지니까 one_more_page로 해준다.
    buffer_pool[new_parent_index].page.right_sibling_or_one_more_page_number = tmp_record[i].page_offset;
    //split다음값부터 new_parent에 넣어준다.
    for(++i, j=0; i<MAX_INTERNAL+1; i++, j++){
        buffer_pool[new_parent_index].page.i_records[j].key = tmp_record[i].key;
        buffer_pool[new_parent_index].page.i_records[j].page_offset = tmp_record[i].page_offset;
        buffer_pool[new_parent_index].page.num_keys++;
    }
    free(tmp_record);
    buffer_pool[new_parent_index].page.next_free_or_parent_page_number = buffer_pool[parent_index].page.next_free_or_parent_page_number;
    //new_parent의 one_more_page의 부모를 new_parent로 지정을 하지않았으므로 다음 코드를 추가시킨다.
    int child_index = get_page_index_from_buffer_pool(tableId, buffer_pool[new_parent_index].page.right_sibling_or_one_more_page_number);
    buffer_pool[child_index].page.next_free_or_parent_page_number = buffer_pool[new_parent_index].pagenum;
    //one_more_page이외의 다른 자식 페이지들의 부모도 new_parent로 지정해준다.
    for(i=0; i<buffer_pool[new_parent_index].page.num_keys; i++){
        child_index = get_page_index_from_buffer_pool(tableId, buffer_pool[new_parent_index].page.i_records[i].page_offset);
        buffer_pool[child_index].is_pinned = true;
        buffer_pool[child_index].page.next_free_or_parent_page_number = buffer_pool[new_parent_index].pagenum;
        buffer_pool[child_index].is_dirty = true;
        buffer_pool[child_index].is_pinned = false;
    }
    buffer_pool[parent_index].is_dirty = true;
    buffer_pool[new_parent_index].is_dirty = true;
    buffer_pool[right_index].is_dirty = true;
    buffer_pool[right_index].is_pinned = false;
  //  printf("inter -> parent\n");
    insert_into_parent(tableId, parent_index, k_prime, new_parent_index);
    return;
}

void insert_into_new_root(int tableId, int left_index, int64_t new_key, int right_index){
  //  printf("insert_into_new_root\n");
    //새로운 root에 key값을 넣어주고 left, right page의 부모를 root로 설정해준다.
    pagenum_t new_root_offset = file_alloc_page(tableId);
 //   printf("new root offset : %llu\n", new_root_offset);
    int new_root_index = get_page_index_from_buffer_pool(tableId, new_root_offset);
    buffer_pool[new_root_index].is_pinned = true;
    buffer_pool[new_root_index].page.is_Leaf=0;
    buffer_pool[new_root_index].page.i_records[0].key = new_key;
    buffer_pool[new_root_index].page.i_records[0].page_offset = buffer_pool[right_index].pagenum;
    buffer_pool[new_root_index].page.right_sibling_or_one_more_page_number = buffer_pool[left_index].pagenum;
    buffer_pool[new_root_index].page.num_keys++;
    buffer_pool[new_root_index].page.next_free_or_parent_page_number=0;
    buffer_pool[new_root_index].is_dirty = true;

    buffer_pool[left_index].page.next_free_or_parent_page_number = buffer_pool[new_root_index].pagenum;
    buffer_pool[right_index].page.next_free_or_parent_page_number = buffer_pool[new_root_index].pagenum;
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    buffer_pool[header_index].header.root_page_offset = new_root_offset;
    buffer_pool[header_index].is_dirty = true;
    buffer_pool[left_index].is_dirty = true;
    buffer_pool[right_index].is_dirty = true;
    buffer_pool[left_index].is_pinned = false;
    buffer_pool[right_index].is_pinned = false;
    buffer_pool[new_root_index].is_pinned = false;
}

//현재 file에 header만 있으면 root를 만들어준다.
void start_new_tree(int tableId, int64_t key, char* value){
  //  printf("start_new_tree\n");
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    page_t* root = (page_t*)malloc(sizeof(page_t));
    pagenum_t root_offset = file_alloc_page(tableId);
    buffer_pool[header_index].header.root_page_offset = root_offset;
    buffer_pool[header_index].is_dirty = true;
   // printf("root offset : %llu\n", root_offset);
    file_read_page(tableId, root_offset, root);
    root->is_Leaf = 1;
    root->l_records[0].key = key;
    strcpy(root->l_records[0].value, value);
    root->num_keys = 1;
    //디스크에 root를 쓰고 버퍼에 올린다.
    file_write_page(tableId, root_offset, root);
    int root_index = get_page_index_from_buffer_pool(tableId, root_offset);
}

int db_delete(int tableId, int64_t key){
  //  printf("db_delete\n");
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    pagenum_t root_offset = buffer_pool[header_index].header.root_page_offset;
    int root_index = get_page_index_from_buffer_pool(tableId, root_offset);
    //root가 없다면
    buffer_pool[root_index].is_pinned = true;
    if(buffer_pool[root_index].page.num_keys==0){
        buffer_pool[root_index].is_pinned = false;
        return -1;
    }
    buffer_pool[root_index].is_pinned = false;
    char* tmp_value = (char*)malloc(sizeof(char)*NUM_RECORD);
    int ex = db_find(tableId, key, tmp_value);
    //해당 key가 존재하지않는다.
    if(ex == 0)
        return -1;
    //해당 key값이 위치한 leaf page를 찾아서 지워준다.
    int delete_index = find_leaf_page(tableId, key, tmp_value);
    delete_entry(tableId, key, delete_index);
  //  printf("delete key\n");
    return 0;
}
void remove_entry_from_page(int tableId, int64_t key, int delete_index){
  //  printf("remove_entry_from_page\n");
    int i;
    buffer_pool[delete_index].is_pinned = true;
    //internal page라면
    if(!buffer_pool[delete_index].page.is_Leaf){
        i=0;
        //key값이 포함된 record의 index찾기
        //i번째가 지워야할 key가 들어있는 record
        while(buffer_pool[delete_index].page.i_records[i].key != key)
            i++;
        //지워야할 key값 뒤의 값들을 앞으로 당기면서 덮어씌운다.
        for(++i; i<buffer_pool[delete_index].page.num_keys; i++){
            buffer_pool[delete_index].page.i_records[i-1].key = buffer_pool[delete_index].page.i_records[i].key;
            buffer_pool[delete_index].page.i_records[i-1].page_offset = buffer_pool[delete_index].page.i_records[i].page_offset;
        }
        buffer_pool[delete_index].page.num_keys--;
        buffer_pool[delete_index].is_dirty = true;
        buffer_pool[delete_index].is_pinned = false;
    }
    //leaf page라면
    else{
     //   printf("leaf\n");
        i=0;
        while(buffer_pool[delete_index].page.l_records[i].key != key)
            i++;
        for(++i; i < buffer_pool[delete_index].page.num_keys; i++){
            buffer_pool[delete_index].page.l_records[i-1].key = buffer_pool[delete_index].page.l_records[i].key;
            strcpy(buffer_pool[delete_index].page.l_records[i-1].value , buffer_pool[delete_index].page.l_records[i].value);
        }
       
        buffer_pool[delete_index].page.num_keys--;
        buffer_pool[delete_index].is_dirty = true;
        buffer_pool[delete_index].is_pinned = false;
    }
    return;
}

void delete_entry(int tableId, int64_t key, int delete_index){
  //  printf("delete_entry\n");
    
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    
    //page에서 key가 포함되어있는 record를 삭제한다.
    remove_entry_from_page(tableId, key, delete_index);
    buffer_pool[delete_index].is_pinned = true;
    //만약 root만 있을 때, 지워야한다면 root를 조정해준다.
    if(buffer_pool[delete_index].pagenum == buffer_pool[header_index].header.root_page_offset){
        buffer_pool[delete_index].is_pinned = false;
        adjust_root(tableId, delete_index);
        return;
    }
    
    //max_page : 이웃 page와 key값을 합쳤을 때, max_page를 초과하면 merge할 수 없다.
    //cut_page : 최소 개수
    int max_page, cut_page;
    if( buffer_pool[delete_index].page.is_Leaf){
        //31개까지 들어갈 수 있다.
        max_page = MAX_LEAF;
        cut_page = MIN_ORDER;
    }
    else{
        //248까지 들어갈 수 있다.
        max_page = MAX_INTERNAL;
        cut_page = MIN_ORDER;
    }
    //만약에 최소 수보다 많이 있으면 굳이 할 필요가 없으므로 return 한다.
    if(buffer_pool[delete_index].page.num_keys > cut_page)
        return;
    int i;
    int left_page_index;
    int left_index;
    //k_prime_index : k_prime이 존재하는 page의 index
    //k_prime : parent에 넘겨줄 key값
    int k_prime_index;
    int64_t k_prime;
    //지우려고 하는 page의 parent_page
    int parent_index = get_page_index_from_buffer_pool(tableId,  buffer_pool[delete_index].page.next_free_or_parent_page_number);
    buffer_pool[parent_index].is_pinned = true;
    //만약 지우려는 page의 위치가 가장 왼쪽이라면 오른쪽 page에서 가져온다.
   // printf("parent_page-> one_more_page : %llu\n", buffer_pool[parent_index].page.right_sibling_or_one_more_page_number);
    if( buffer_pool[delete_index].pagenum ==  buffer_pool[parent_index].page.right_sibling_or_one_more_page_number){
        left_page_index = -2;
        left_index = get_page_index_from_buffer_pool(tableId,buffer_pool[parent_index].page.i_records[0].page_offset);
        buffer_pool[left_index].is_pinned = true;
        k_prime_index = 0;
        k_prime = buffer_pool[parent_index].page.i_records[0].key;
    }
    //위치가 두번째라면 left page를 one_more_page로 설정한다.
    else if(buffer_pool[delete_index].pagenum == buffer_pool[parent_index].page.i_records[0].page_offset){
        left_page_index = -1;
        left_index = get_page_index_from_buffer_pool(tableId,buffer_pool[parent_index].page.right_sibling_or_one_more_page_number);
        buffer_pool[left_index].is_pinned = true;
        k_prime_index = 0;
        k_prime = buffer_pool[parent_index].page.i_records[0].key;
    }
    //그 외의 경우일 때,
    else{
        //delete_offset과 같은 page_offset의 위치를 찾는다.
        for(i=0; i<=buffer_pool[parent_index].page.num_keys; i++){
            if(buffer_pool[delete_index].pagenum == buffer_pool[parent_index].page.i_records[i].page_offset)
                break;
        }
        //delete_offset == i
        left_page_index = i-1;
        left_index = get_page_index_from_buffer_pool(tableId, buffer_pool[parent_index].page.i_records[left_page_index].page_offset);
        buffer_pool[left_index].is_pinned = true;
        k_prime_index = left_page_index+1;
        k_prime = buffer_pool[parent_index].page.i_records[k_prime_index].key;
    }

    //합이 max_page를 넘기면 왼쪽 page에서 key를 가져온다.
    if(buffer_pool[left_index].page.num_keys + buffer_pool[delete_index].page.num_keys > max_page){
        redistribute_pages(tableId, delete_index, left_page_index, left_index, parent_index, k_prime, k_prime_index);
        return;
    }
    //그렇지 않으면 merge한다.
    else{
        coalesce_pages(tableId, delete_index, left_page_index, left_index, parent_index, k_prime);
        return;
    }
}
//key를 sibling page에서 가져오는 함수
void redistribute_pages(int tableId, int delete_index, int left_page_index, int left_index, int parent_index ,int64_t k_prime, int k_prime_index){
  //  printf("redistribute_pages\n");
    //만약 one_more_page가 delete_page라면
    if(left_page_index == -2){
        //leaf page 라면
        //여기서는 left_page는 오른쪽 page를 의미한다.
        if(buffer_pool[delete_index].page.is_Leaf){
            buffer_pool[delete_index].page.l_records[buffer_pool[delete_index].page.num_keys].key = buffer_pool[left_index].page.l_records[0].key;
            strcpy(buffer_pool[delete_index].page.l_records[buffer_pool[delete_index].page.num_keys].value, buffer_pool[left_index].page.l_records[0].value);
            buffer_pool[delete_index].page.num_keys++;
            buffer_pool[delete_index].is_dirty = true;
            int i;
            //오른쪽의 0번째 값을 왼쪽에 줬으니 오른쪽의 나머지 값들을 앞으로 당긴다.
            for(i=0; i<buffer_pool[left_index].page.num_keys-1; i++){
                buffer_pool[left_index].page.l_records[i].key = buffer_pool[left_index].page.l_records[i+1].key;
                strcpy(buffer_pool[left_index].page.l_records[i].value, buffer_pool[left_index].page.l_records[i+1].value);
            }
            //앞으로 옮기고나서 i번째의 값은 0으로 초기화해준다.
            buffer_pool[left_index].page.l_records[i].key = 0;
     //       strcpy(left_page->l_records[i].value, NULL);
            buffer_pool[left_index].page.num_keys--;
            buffer_pool[left_index].is_dirty = true;
            buffer_pool[parent_index].page.i_records[k_prime_index].key = buffer_pool[left_index].page.l_records[0].key;
            buffer_pool[parent_index].page.i_records[k_prime_index].page_offset = buffer_pool[left_index].pagenum;
            buffer_pool[parent_index].is_dirty = true;
        }
        //internal page라면
        else{
            //k_prime : 오른쪽 페이지의 첫번째 값
            buffer_pool[delete_index].page.i_records[buffer_pool[delete_index].page.num_keys].key = k_prime;
            //page_offset은 오른쪽 페이지의 one_more_page의 offset으로 설정한다.
            buffer_pool[delete_index].page.i_records[buffer_pool[delete_index].page.num_keys].page_offset = buffer_pool[left_index].page.right_sibling_or_one_more_page_number;
            buffer_pool[delete_index].is_dirty = true;
           //child_page는 오른쪽 페이지에서 가져온 one_more_page를 의미한다.
            // child_page를 왼쪽에 옮겼으므로 child_page의 parent를 바꿔준다.
            int child_index = get_page_index_from_buffer_pool(tableId, buffer_pool[delete_index].page.i_records[buffer_pool[delete_index].page.num_keys].page_offset);
            buffer_pool[child_index].page.next_free_or_parent_page_number = buffer_pool[delete_index].pagenum;
            buffer_pool[delete_index].page.num_keys++;
            buffer_pool[child_index].is_dirty = true;
            //오른쪽 page의 one_more_page가 없어졌으므로 0번째를 one_more_page로 만든다.
            buffer_pool[left_index].page.right_sibling_or_one_more_page_number = buffer_pool[left_index].page.i_records[0].page_offset;
            //앞으로 당겨준다.
            int i;
            for(i=0; i<buffer_pool[left_index].page.num_keys-1; i++){
                buffer_pool[left_index].page.i_records[i].key = buffer_pool[left_index].page.i_records[i+1].key;
                buffer_pool[left_index].page.i_records[i].page_offset = buffer_pool[left_index].page.i_records[i+1].page_offset;
            }
            //당겨주고 남은 값은 0으로 초기화해준다.
            buffer_pool[left_index].page.i_records[i].key=0;
             buffer_pool[left_index].page.i_records[i].page_offset=0;
             buffer_pool[left_index].page.num_keys--;
            buffer_pool[left_index].is_dirty = true;
            //parent_page의 0번째 key를 덮어씌운다.
             buffer_pool[parent_index].page.i_records[k_prime_index].key =  buffer_pool[left_index].page.i_records[0].key;
            buffer_pool[parent_index].is_dirty = true;
        }
    }
    //one_more_page가 아닌 다른 페이지일 때
    else{
        //leaf page일 때,
        if(buffer_pool[delete_index].page.is_Leaf){
            int i;
            //페이지의 값들을 한칸씩 뒤로 미룬다.
            for(i=buffer_pool[delete_index].page.num_keys; i>0; i--){
                buffer_pool[delete_index].page.l_records[i].key = buffer_pool[delete_index].page.l_records[i-1].key;
                strcpy(buffer_pool[delete_index].page.l_records[i].value, buffer_pool[delete_index].page.l_records[i-1].value);
            }
            //왼쪽 페이지의 마지막 값을 오른쪽 페이지의 맨앞에다가 놓는다.
            buffer_pool[delete_index].page.l_records[0].key = buffer_pool[left_index].page.l_records[buffer_pool[left_index].page.num_keys-1].key;
            strcpy(buffer_pool[delete_index].page.l_records[0].value, buffer_pool[delete_index].page.l_records[buffer_pool[left_index].page.num_keys-1].value);
            buffer_pool[delete_index].is_dirty = true;
            buffer_pool[left_index].page.l_records[buffer_pool[left_index].page.num_keys-1].key=0;
            buffer_pool[parent_index].page.i_records[k_prime_index].key = buffer_pool[delete_index].page.l_records[0].key;
            buffer_pool[left_index].page.num_keys--;
            buffer_pool[delete_index].page.num_keys++;
            buffer_pool[left_index].is_dirty = true;
            buffer_pool[parent_index].is_dirty = true;
        }
        //internal page일 때,
        else{
            int i;
            //페이지의 값들을 뒤로 미룬다.
            for(i=buffer_pool[delete_index].page.num_keys; i>0; i--){
                buffer_pool[delete_index].page.i_records[i].key = buffer_pool[delete_index].page.i_records[i-1].key;
                buffer_pool[delete_index].page.i_records[i].page_offset = buffer_pool[delete_index].page.i_records[i-1].page_offset;
            }
            buffer_pool[delete_index].page.i_records[0].key = k_prime;
            buffer_pool[delete_index].page.i_records[0].page_offset = buffer_pool[delete_index].page.right_sibling_or_one_more_page_number;
            buffer_pool[delete_index].page.right_sibling_or_one_more_page_number = buffer_pool[left_index].page.i_records[buffer_pool[left_index].page.num_keys-1].page_offset;
            buffer_pool[left_index].page.num_keys--;
            buffer_pool[delete_index].page.num_keys++;
            //옮겨준 one_more_page의 부모를 delete_page로 바꾼다.
            int child_index = get_page_index_from_buffer_pool(tableId, buffer_pool[delete_index].page.right_sibling_or_one_more_page_number);
            buffer_pool[child_index].page.next_free_or_parent_page_number = buffer_pool[delete_index].pagenum;
            
            buffer_pool[parent_index].page.i_records[k_prime_index].key = buffer_pool[delete_index].page.i_records[0].key;
            buffer_pool[delete_index].is_dirty = true;
            buffer_pool[left_index].is_dirty = true;
            buffer_pool[parent_index].is_dirty = true;
        }
    }
    buffer_pool[delete_index].is_pinned = false;
    buffer_pool[left_index].is_pinned = false;
    buffer_pool[parent_index].is_pinned = false;
    return;
}
//merge 함수
//페이지에 레코드가 1개가 남아있을때까지 merge를 하지 않는다.
void coalesce_pages(int tableId, int delete_index, int left_page_index, int left_index, int parent_index, int64_t k_prime){
  //  printf("coalesce_pages\n");
    int i, j, left_insertion_index, left_end;
    //만약 one_more_page라면
    //delete_offset 과 left_page_offset을 교체한다.
    if(left_page_index == -2){
        int tmp_index;
        tmp_index = delete_index;
        delete_index = left_index;
        left_index = tmp_index;
    }
    
    left_insertion_index = buffer_pool[left_index].page.num_keys;
    left_end = buffer_pool[delete_index].page.num_keys;
    //leaf page라면
    //뒤로 key값을 밀고
    if(buffer_pool[delete_index].page.is_Leaf){
        for(i=left_insertion_index, j=0; j<left_end; i++, j++){
            buffer_pool[left_index].page.l_records[i].key = buffer_pool[delete_index].page.l_records[j].key;
            strcpy(buffer_pool[left_index].page.l_records[i].value, buffer_pool[delete_index].page.l_records[j].value);
            buffer_pool[left_index].page.num_keys++;
            buffer_pool[delete_index].page.num_keys--;
        }
        buffer_pool[left_index].page.right_sibling_or_one_more_page_number = buffer_pool[delete_index].page.right_sibling_or_one_more_page_number;
   //     printf("left_page : \n");
  /*      for(int t=0; t<buffer_pool[left_index].page.num_keys; t++)
            printf("%llu ",buffer_pool[left_index].page.l_records[t].key);
        printf("\n");
        printf("delete_page : \n");
        for(int t=0; t<buffer_pool[delete_index].page.num_keys; t++)
            printf("%llu ",buffer_pool[delete_index].page.l_records[t].key);
        printf("\n");
    */}
    //internal page일 때
    else{
        buffer_pool[left_index].page.i_records[left_insertion_index].key = k_prime;
        buffer_pool[left_index].page.i_records[left_insertion_index].page_offset = buffer_pool[delete_index].page.right_sibling_or_one_more_page_number;
        buffer_pool[left_index].page.num_keys++;
        
        for(i=left_insertion_index+1, j=0; j<left_end; i++, j++){
            buffer_pool[left_index].page.i_records[i].key = buffer_pool[delete_index].page.i_records[j].key;
            buffer_pool[left_index].page.i_records[i].page_offset = buffer_pool[delete_index].page.i_records[j].page_offset;
            buffer_pool[left_index].page.num_keys++;
            buffer_pool[delete_index].page.num_keys--;
        }
    /*    for(i=0; i<buffer_pool[left_index].page.num_keys+1; i++){
            int tmp_index = get_page_index_from_buffer_pool(tableId, buffer_pool[left_index].page.i_records[i].page_offset);
            buffer_pool[tmp_index].page.next_free_or_parent_page_number = buffer_pool[left_index].pagenum;
        }
   */ }
    buffer_pool[left_index].is_dirty = true;
    buffer_pool[left_index].is_pinned = false;
    buffer_pool[delete_index].is_pinned = false;
    buffer_pool[parent_index].is_pinned = false;
    delete_entry(tableId, k_prime, parent_index);
    file_free_page(tableId, delete_index);
    return;
}
//root의 key값들이 모두 없어졌을 때, 새로운 root를 생성하는 함수.
void adjust_root(int tableId, int delete_index){
 //   printf("adjust_root\n");
    int header_index = get_page_index_from_buffer_pool(tableId, 0);
    int root_index = get_page_index_from_buffer_pool(tableId, buffer_pool[header_index].header.root_page_offset);
    buffer_pool[root_index].is_pinned = true;
    if(buffer_pool[root_index].page.num_keys==0){
        if(!buffer_pool[root_index].page.is_Leaf){
            int new_root_index = get_page_index_from_buffer_pool(tableId, buffer_pool[root_index].page.right_sibling_or_one_more_page_number);
            buffer_pool[new_root_index].page.next_free_or_parent_page_number=0;
            file_free_page(tableId, delete_index);
            buffer_pool[header_index].header.root_page_offset = buffer_pool[new_root_index].pagenum;
            buffer_pool[new_root_index].is_dirty = true;
            buffer_pool[header_index].is_dirty = true;
            buffer_pool[root_index].is_dirty = true;
        }
        else{
            file_free_page(tableId, delete_index);
            buffer_pool[header_index].header.root_page_offset = 0;
            buffer_pool[header_index].is_dirty = true;
        }
    }
    buffer_pool[root_index].is_pinned = false;
    return;
}


// print
void find_and_print(int table_id, int64_t key, char * ret_val){
    int i;
    int header_index = get_page_index_from_buffer_pool(table_id, 0);
    int root_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[header_index].header.root_page_offset);
    pagenum_t root_pagenum = buffer_pool[header_index].header.root_page_offset;
    printf("root pagenum : %llu\n", buffer_pool[header_index].header.root_page_offset);
    buffer_pool[root_idx].is_pinned = 1;
    if(root_pagenum == 0){
        printf("There is not Root\n");
        buffer_pool[root_idx].is_pinned = 0;
        return;
    }
    
    //printf("root isleaf? %d, root numkeys : %d\n", root->isLeaf, root->num_keys);
    if(buffer_pool[root_idx].page.is_Leaf){
        printf("@@@@@@@@@@@@@ Root is Leaf @@@@@@@@@@@@@\n");
        for(i=0; i<buffer_pool[root_idx].page.num_keys; i++){
            printf("%5lld ",buffer_pool[root_idx].page.l_records[i].key);
        }
    }
    else{
        printf("@@@@@@@@@@@@@ Root is inter @@@@@@@@@@@@@\n");
        for(i=0; i<buffer_pool[root_idx].page.num_keys; i++){
            printf("%5lld ",buffer_pool[root_idx].page.i_records[i].key);
        }
        printf("\n\n");
        
        int tmpLeft_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[root_idx].page.right_sibling_or_one_more_page_number);
        buffer_pool[tmpLeft_idx].is_pinned = 1;
        
        if(buffer_pool[tmpLeft_idx].page.is_Leaf){
            printf("@@@@@@@@@@@@@ Root's sp_o child @@@@@@@@@@@@@\n");
            printf("left : \n");
            for(i=0; i<buffer_pool[tmpLeft_idx].page.num_keys; i++){
                printf("%5lld ",buffer_pool[tmpLeft_idx].page.l_records[i].key);
            }
            
            tmpLeft_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[root_idx].page.i_records[0].page_offset);
            
            printf("\n@@@@@@@@@@@@@ Root's [0] child @@@@@@@@@@@@@\n");
            printf("\n%lld - right : \n", buffer_pool[root_idx].page.i_records[0].page_offset);
            for(i=0; i<buffer_pool[tmpLeft_idx].page.num_keys; i++){
                printf("%5lld ",buffer_pool[tmpLeft_idx].page.l_records[i].key);
            }
            buffer_pool[tmpLeft_idx].is_pinned = 0;
            tmpLeft_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[root_idx].page.i_records[buffer_pool[root_idx].page.num_keys-2].page_offset);
            buffer_pool[tmpLeft_idx].is_pinned = 1;
            printf("\n@@@@@@@@@@@@@ Root's last-2 child @@@@@@@@@@@@@\n");
            printf("\n%lld - left : \n", buffer_pool[root_idx].page.i_records[buffer_pool[root_idx].page.num_keys-2].page_offset);
            for(i=0; i<buffer_pool[tmpLeft_idx].page.num_keys; i++){
                printf("%5lld ",buffer_pool[tmpLeft_idx].page.l_records[i].key);
            }
            buffer_pool[tmpLeft_idx].is_pinned = 0;
            tmpLeft_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[root_idx].page.i_records[buffer_pool[root_idx].page.num_keys-1].page_offset);
            buffer_pool[tmpLeft_idx].is_pinned = 1;
            printf("\n@@@@@@@@@@@@@ Root's last-1 child @@@@@@@@@@@@@\n");
            printf("\n%lld - right : \n",buffer_pool[root_idx].page.i_records[buffer_pool[tmpLeft_idx].page.num_keys-1].page_offset);
            for(i=0; i<buffer_pool[tmpLeft_idx].page.num_keys; i++){
                printf("%5lld ",buffer_pool[tmpLeft_idx].page.l_records[i].key);
            }
            buffer_pool[tmpLeft_idx].is_pinned = 0;
        }
        else{
            printf("\n@@@@@@@@@@@@@ Root's 3 height @@@@@@@@@@@@@\n");
            
            tmpLeft_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[root_idx].page.right_sibling_or_one_more_page_number);
            buffer_pool[tmpLeft_idx].is_pinned = 1;
            printf("@@@@@@@@@@@@@ Root's sp_o child @@@@@@@@@@@@@\n");
            printf("left : \n");
            for(i=0; i<buffer_pool[tmpLeft_idx].page.num_keys; i++){
                printf("%5lld ",buffer_pool[tmpLeft_idx].page.i_records[i].key);
            }
            buffer_pool[tmpLeft_idx].is_pinned = 0;
            tmpLeft_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[root_idx].page.i_records[0].page_offset);
            buffer_pool[tmpLeft_idx].is_pinned = 1;
            printf("\n@@@@@@@@@@@@@ Root's [0] child @@@@@@@@@@@@@\n");
            printf("right : \n");
            for(i=0; i<buffer_pool[tmpLeft_idx].page.num_keys; i++){
                printf("%5lld ",buffer_pool[tmpLeft_idx].page.i_records[i].key);
            }
            int tmptmp_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[tmpLeft_idx].page.right_sibling_or_one_more_page_number);
            buffer_pool[tmptmp_idx].is_pinned = 1;
            if(buffer_pool[tmptmp_idx].page.is_Leaf){
                printf("\n\n\n@@@@@@@@@@@@@ tempLeft's sp_o child @@@@@@@@@@@@@\n");
                printf("left : \n");
                for(i=0; i<buffer_pool[tmptmp_idx].page.num_keys; i++){
                    printf("%5lld ",buffer_pool[tmptmp_idx].page.l_records[i].key);
                }
                
                printf("\n@@@@@@@@@@@@@ tempLeft's [0] child @@@@@@@@@@@@@\n");
                buffer_pool[tmptmp_idx].is_pinned = 0;
                tmptmp_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[tmpLeft_idx].page.i_records[0].page_offset);
                buffer_pool[tmptmp_idx].is_pinned = 1;
                
                printf("right : \n");
                for(i=0; i<buffer_pool[tmptmp_idx].page.num_keys; i++){
                    printf("%5lld ",buffer_pool[tmptmp_idx].page.l_records[i].key);
                }
                buffer_pool[tmptmp_idx].is_pinned = 0;
                tmptmp_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[tmpLeft_idx].page.i_records[buffer_pool[tmpLeft_idx].page.num_keys-2].page_offset);
                buffer_pool[tmptmp_idx].is_pinned = 1;
                printf("\n@@@@@@@@@@@@@ tempLeft's [last's Left] child @@@@@@@@@@@@@\n");
                printf("%lld - left : \n", buffer_pool[tmpLeft_idx].page.i_records[buffer_pool[tmpLeft_idx].page.num_keys-2].page_offset);
                for(i=0; i<buffer_pool[tmptmp_idx].page.num_keys; i++){
                    printf("%5lld ",buffer_pool[tmptmp_idx].page.l_records[i].key);
                }
                buffer_pool[tmptmp_idx].is_pinned = 0;
                tmptmp_idx = get_page_index_from_buffer_pool(table_id, buffer_pool[tmpLeft_idx].page.i_records[buffer_pool[tmpLeft_idx].page.num_keys-1].page_offset);
                buffer_pool[tmptmp_idx].is_pinned = 1;
                printf("\n@@@@@@@@@@@@@ tempLeft's [last's Right] child @@@@@@@@@@@@@\n");
                printf("%lld - right : \n",buffer_pool[tmpLeft_idx].page.i_records[buffer_pool[tmpLeft_idx].page.num_keys-1].page_offset);
                for(i=0; i<buffer_pool[tmptmp_idx].page.num_keys; i++){
                    printf("%5lld ",buffer_pool[tmptmp_idx].page.l_records[i].key);
                }
                buffer_pool[tmptmp_idx].is_pinned = 0;
                buffer_pool[tmpLeft_idx].is_pinned = 0;
            }
            
        }
    }
    buffer_pool[root_idx].is_pinned = 0;
    printf("\n");
    printf("======================\n");
}
