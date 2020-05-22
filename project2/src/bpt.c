     /*
 *  bpt.c  
 */
#define Version "1.14"
#include "bpt.h"
int fd;
header_t* header;
page_t* root;
void file_read_page(pagenum_t pagenum, page_t* dest){
    if(pread(fd, dest, sizeof(page_t), pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"cannot read page %s\n",strerror(errno));
        return;
    }
}
void file_write_page(pagenum_t pagenum, const page_t* src){
    if(pwrite(fd, src, PAGE_SIZE, pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"cannot write page %s\n",strerror(errno));
        return;
    }
}
void file_read_header_page(pagenum_t pagenum, header_t* dest){
    if(pread(fd, dest, sizeof(header_t), pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"cannot read page %s\n",strerror(errno));
        return;
    }
}
void file_write_header_page(pagenum_t pagenum, const header_t* src){
    if(pwrite(fd, src, sizeof(header_t), pagenum*PAGE_SIZE)==-1){
        fprintf(stderr,"(write)cannot find header page %s\n",strerror(errno));
        return;
    }
}
int open_table(char* pathname){
    header = (header_t*)malloc(sizeof(header_t));
    fd = open(pathname, O_RDWR);
    if(fd == -1){
        //만약 존재하지 않는다면 새로운 파일을 생성한다.
        fd = open(pathname, O_RDWR | O_CREAT, S_IRWXU);
        if(fd ==-1){
            fprintf(stderr,"cannot open table %s\n",strerror(errno));
            return -1;
        }
        
        header->free_page_offset = 0;
        header->root_page_offset = 0;
        header->num_of_pages = 1;
        file_write_header_page(0,header);
    }
    else{
        root = (page_t*)malloc(sizeof(page_t));
        file_read_header_page(0,header);
        pagenum_t root_offset = header->root_page_offset;
        file_read_page(root_offset, root);
    }
    return fd;
}

pagenum_t file_alloc_page(){
    pagenum_t last_offset;
    pagenum_t free_offset = header->free_page_offset;
    page_t* free_page = (page_t*)malloc(sizeof(page_t));
    //if not exists free page
    if(free_offset == 0){
        last_offset = lseek(fd, 0, SEEK_END);
        if(last_offset == -1){
            fprintf(stderr,"cannot find offset %s\n",strerror(errno));
            return -1;
        }
        header->num_of_pages++;
        file_write_header_page(0,header);
        free_page->right_sibling_or_one_more_page_number = 0;
        free_page->next_free_or_parent_page_number = 0;
        free_page->is_Leaf = 0;
        free_page->num_keys=0;
        file_write_page(last_offset/PAGE_SIZE, free_page);
        return last_offset/PAGE_SIZE;
    }
    return free_offset;
}
void file_free_page(pagenum_t pagenum){
    page_t* page = (page_t*)malloc(sizeof(page_t));
    file_read_page(pagenum, page);
    page->next_free_or_parent_page_number=header->free_page_offset;
    header->free_page_offset = pagenum;
    page->is_Leaf=0;
    page->num_keys=0;
    page->right_sibling_or_one_more_page_number=0;
    file_write_page(pagenum, page);
    file_write_header_page(0,header);
    file_read_header_page(0,header);
}

int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}

int db_insert(int64_t key, char* value){
    pagenum_t root_offset = header->root_page_offset;
    pagenum_t leaf_offset;
    page_t* leaf = (page_t*)malloc(sizeof(page_t));
    if(header->root_page_offset == 0){
        start_new_tree(key, value);
        return 0;
    }
    //이미 키가 존재하면
    if(db_find(key, value) != NULL){
        printf("key is already existed!\n");
        return -1;
    }
    //존재하지 않는다면
    leaf_offset = find_leaf_page(key, value);
    file_read_page(leaf_offset, leaf);
    if(leaf->num_keys < MAX_LEAF){
        insert_into_leaf(leaf_offset, key, value);
        return 0;
    }
    insert_into_leaf_after_splitting(leaf_offset, key, value);
    return 0;
}

void insert_into_leaf(pagenum_t leaf_offset, int64_t key, char* value){
    page_t* leaf = (page_t*)malloc(sizeof(page_t));
    file_read_page(leaf_offset, leaf);
    int i, insertion_point=0;
    while(insertion_point < leaf->num_keys && leaf->l_records[insertion_point].key < key)
        insertion_point++;
    for(i=leaf->num_keys; i>insertion_point; i--)
        leaf->l_records[i] = leaf->l_records[i-1];
    leaf->l_records[insertion_point].key = key;
    strcpy(leaf->l_records[insertion_point].value, value);
    leaf->num_keys++;
    file_write_page(leaf_offset, leaf);
}
void insert_into_leaf_after_splitting(pagenum_t leaf_offset, int64_t key, char* value){
 //   printf("insert_into_leaf_after_splitting\n");
    int i, j, split;
    page_t* new_leaf = (page_t*)malloc(sizeof(page_t));
    pagenum_t new_leaf_offset;
    new_leaf_offset = file_alloc_page();
    file_read_page(new_leaf_offset,new_leaf);
    new_leaf->is_Leaf=1;
    page_t* leaf = (page_t*)malloc(sizeof(page_t));
    file_read_page(leaf_offset, leaf);
    
    int insertion_index=0;
    //key값을 넣을 index를 찾는다.
    while(insertion_index < MAX_LEAF && leaf->l_records[insertion_index].key < key)
        insertion_index++;
 //   printf("insertion_index : %d\n", insertion_index); //insertion_index == 31
    record_t* tmp_record = (record_t*)malloc(sizeof(record_t)*(MAX_LEAF+1));
    //j가 insertion_index와 같다면 j++을 해주어서 key값을 넣을 곳을 비워둔다.
    //ex 만약 key가 7이라면
    //tmp : 1 4 5   9 10
    for(i=0, j=0; i<leaf->num_keys; i++, j++){
        if(j==insertion_index)
            j++;
        tmp_record[j].key = leaf->l_records[i].key;
        strcpy(tmp_record[j].value, leaf->l_records[i].value);
    }
    //insertion_index자리에 record를 넣어주고 leaf에 다시 넣어야 하므로 num_keys값을 0으로 초기화 해준다.
    //split변수 : split할 부분
    tmp_record[insertion_index].key = key;
    strcpy(tmp_record[insertion_index].value, value);
     leaf->num_keys=0;
    split = cut(MAX_LEAF);
    //split 이전까지의 값들을 복사한다.
    for(i=0; i<split; i++){
        leaf->l_records[i].key = tmp_record[i].key;
        strcpy(leaf->l_records[i].value, tmp_record[i].value);
        leaf->num_keys++;
    }
  //split 이후의 값들을 복사한다.
    for(i=split, j=0; i<MAX_LEAF+1; i++, j++){
        new_leaf->l_records[j].key = tmp_record[i].key;
        strcpy(new_leaf->l_records[j].value, tmp_record[i].value);
        new_leaf->num_keys++;
    }
    //원래의 leaf가 가리키던 오른쪽 형제를 새로운 leaf가 가리키게 하고 leaf의 오른쪽을 새로운 leaf로 지정한다.
    new_leaf->right_sibling_or_one_more_page_number = leaf->right_sibling_or_one_more_page_number;
    leaf->right_sibling_or_one_more_page_number = new_leaf_offset;
    //원래 page에 있던 값들은 0으로 초기화 시켜준다.
    for(i=leaf->num_keys; i<MAX_LEAF; i++)
        leaf->l_records[i].key = 0;
    for(i=new_leaf->num_keys; i<MAX_LEAF; i++)
        new_leaf->l_records[i].key = 0;
    
    new_leaf->next_free_or_parent_page_number = leaf->next_free_or_parent_page_number;
    int64_t new_key;
    //new_key : parent에 들어갈 새로운 키
    new_key = new_leaf->l_records[0].key;
    
    file_write_page(leaf_offset, leaf);
    file_write_page(new_leaf_offset, new_leaf);
    insert_into_parent(leaf_offset, new_key, new_leaf_offset);
    return;
}

int get_left_index(pagenum_t left_offset){
    int left_index;
    page_t* child = (page_t*)malloc(sizeof(page_t));
    file_read_page(left_offset, child);
    pagenum_t parent_offset = child->next_free_or_parent_page_number;
    free(child);
    page_t* parent = (page_t*)malloc(sizeof(page_t));
    file_read_page(parent_offset, parent);
    //만약에 left_offset이 one_more_page이면 -1을 return
    if(left_offset == parent->right_sibling_or_one_more_page_number)
        return -1;
    //부모가 가리키는 page_offset과 left_offset이 같을때까지 index++을 해준다.
    for(left_index=0; left_index < parent->num_keys; left_index++){
        if(parent->i_records[left_index].page_offset == left_offset)
            break;
    }
    //만약에 없으면 parent를 free해주고 -1을 return해준다.
    if(left_index == parent->num_keys){
        free(parent);
        return -1;
    }
    free(parent);
    return left_index;
}

void insert_into_internal(pagenum_t parent_offset, int left_index, int64_t new_key, pagenum_t right_offset){
 //   printf("insert_into_internal\n");
    page_t* parent = (page_t*)malloc(sizeof(page_t));
    file_read_page(parent_offset, parent);
    int i;
    //left page 옆에다가 만들어야하므로 left_index+1자리를 비워두고 오른쪽으로 한칸씩 옮겨준다.
    for(i=parent->num_keys; i>left_index+1; i--){
        parent->i_records[i].page_offset = parent->i_records[i-1].page_offset;
        parent->i_records[i].key = parent->i_records[i-1].key;
    }
    //left page 옆에다가 만들어야하므로 left_index+1에다가 집어넣는다.
    parent->i_records[left_index+1].page_offset = right_offset;
    parent->i_records[left_index+1].key = new_key;
    parent->num_keys++;
    file_write_page(parent_offset, parent);
    //만약에 internal이 root page라면 전역으로 선언된 root를 업데이트 해주고 다시 써준다.
    if(parent_offset == header->root_page_offset){
        file_read_page(parent_offset, root);
        file_write_page(parent_offset, root);
    }
    return;
}
void insert_into_internal_after_splitting(pagenum_t parent_offset, int left_index, int64_t new_key, pagenum_t right_offset){
   // printf("insert_into_internal_after_splitting\n");
    int i, j, split;
    page_t* parent = (page_t*)malloc(sizeof(page_t));
    file_read_page(parent_offset, parent);
    
    page_t* new_parent = (page_t*)malloc(sizeof(page_t));
    pagenum_t new_parent_offset = file_alloc_page();
    file_read_page(new_parent_offset, new_parent);
    internal_record* tmp_record = (internal_record*)malloc(sizeof(internal_record)*(MAX_INTERNAL+1));
    //record가 들어갈 자리를 비워주고 tmp_record에 넣어준다.
    for(i=0, j=0; i<parent->num_keys; i++, j++){
        //left_index+1이 들어갈 자리
        if(j==left_index+1)
            j++;
        tmp_record[j].key = parent->i_records[i].key;
        tmp_record[j].page_offset = parent->i_records[i].page_offset;
    }
    //오른쪽 key가 들어갈 자리에 넣어준다.
    tmp_record[left_index+1].key = new_key;
    tmp_record[left_index+1].page_offset = right_offset;
    
    split = cut(MAX_INTERNAL);
    parent->num_keys=0;
    //split 전까지의 key값을 parent에다가 넣어줌
    //for문이 끝나면 i==split
    for(i=0; i<split; i++){
        parent->i_records[i].key = tmp_record[i].key;
        parent->i_records[i].page_offset = tmp_record[i].page_offset;
        parent->num_keys++;
    }
    //부모한테 넘겨줄 값
    int64_t k_prime;
    //split위치의 다음 페이지의 key값이 부모에게 갈 값이다.
    k_prime = tmp_record[i+1].key;
    //split위치가 제일 왼쪽 페이지니까 one_more_page로 해준다.
    new_parent->right_sibling_or_one_more_page_number = tmp_record[i].page_offset;
    //split다음값부터 new_parent에 넣어준다.
    for(++i, j=0; i<MAX_INTERNAL+1; i++, j++){
        new_parent->i_records[j].key = tmp_record[i].key;
        new_parent->i_records[j].page_offset = tmp_record[i].page_offset;
        new_parent->num_keys++;
    }
    free(tmp_record);
    
    new_parent->next_free_or_parent_page_number = parent->next_free_or_parent_page_number;
    //new_parent의 one_more_page의 부모를 new_parent로 지정을 하지않았으므로 다음 코드를 추가시킨다.
    page_t* child_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(new_parent->right_sibling_or_one_more_page_number, child_page);
    child_page->next_free_or_parent_page_number = new_parent_offset;
    file_write_page(new_parent->right_sibling_or_one_more_page_number, child_page);
    //one_more_page이외의 다른 자식 페이지들의 부모도 new_parent로 지정해준다.
    for(i=0; i<new_parent->num_keys; i++){
        int64_t child_page_offset = new_parent->i_records[i].page_offset;
        file_read_page(child_page_offset, child_page);
        child_page->next_free_or_parent_page_number = new_parent_offset;
        file_write_page(child_page_offset,child_page);
    }
    file_write_page(new_parent_offset, new_parent);
    file_write_page(parent_offset, parent);
    insert_into_parent(parent_offset, k_prime, new_parent_offset);
    return;
}
void insert_into_parent(pagenum_t left_offset, int64_t new_key, pagenum_t right_offset){
  //  printf("insert_into_parent\n");
    page_t* left_page = (page_t*)malloc(sizeof(page_t));
    page_t* right_page = (page_t*)malloc(sizeof(page_t));
    page_t* parent_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(left_offset, left_page);
    file_read_page(right_offset, right_page);
    pagenum_t parent_offset = left_page->next_free_or_parent_page_number;
    file_read_page(parent_offset, parent_page);
    
    if(parent_offset==0){
        insert_into_new_root(left_offset, new_key, right_offset);
        return;
    }
    int left_index;
    left_index = get_left_index(left_offset);
    if(parent_page->num_keys >= MAX_INTERNAL){
        insert_into_internal_after_splitting(parent_offset, left_index, new_key, right_offset);
        return;
    }
    insert_into_internal(parent_offset, left_index, new_key, right_offset);
    return;
}
void insert_into_new_root(pagenum_t left_offset, int64_t new_key, pagenum_t right_offset){
    //새로운 root에 key값을 넣어주고 left, right page의 부모를 root로 설정해준다.
    page_t* new_root = (page_t*)malloc(sizeof(page_t));
    pagenum_t new_root_offset = file_alloc_page();
    file_read_page(new_root_offset, new_root);
    new_root->is_Leaf=0;
    new_root->i_records[0].key = new_key;
    new_root->i_records[0].page_offset = right_offset;
    new_root->right_sibling_or_one_more_page_number = left_offset;
    new_root->num_keys++;
    new_root->next_free_or_parent_page_number=0;
    
    page_t* left_page = (page_t*)malloc(sizeof(page_t));
    page_t* right_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(left_offset, left_page);
    file_read_page(right_offset, right_page);
    
    left_page->next_free_or_parent_page_number = new_root_offset;
    right_page->next_free_or_parent_page_number = new_root_offset;
    file_write_page(left_offset, left_page);
    file_write_page(right_offset, right_page);
    file_write_page(new_root_offset, new_root);
    header->root_page_offset = new_root_offset;
    file_write_header_page(0,header);
    file_read_page(header->root_page_offset, root);
    file_read_header_page(0,header);
}
char* db_find(int64_t key, char* ret_val){
    char* value = (char*)malloc(sizeof(char)*NUM_RECORD);
    //find_leaf_page함수를 이용해 key가 존재하는 leaf page를 찾고 페이지 번호를 반환한다.
    pagenum_t p_offset = find_leaf_page(key, ret_val);
    page_t* p = (page_t*)malloc(sizeof(page_t));
    file_read_page(p_offset,p);
    int i;
    if(p_offset == -1)
        return NULL;
    //key가 존재하는지 찾는다.
    for(i=0; i < p->num_keys; i++){
        if(p->l_records[i].key == key)
            break;
    }
    //만약에 없으면 null
    if(i == p->num_keys){
        free(p);
        return NULL;
    }
    //있으면 해당 value를 return
    else{
        strcpy(value, p->l_records[i].value);
        free(p);
        return value;
    }
}
pagenum_t find_leaf_page(int64_t key, char* ret_val){
    int i;
    //root페이지를 읽어들여서 leaf page를 찾는다.
    pagenum_t p_offset = header->root_page_offset;
    page_t* p = (page_t*)malloc(sizeof(page_t));
    file_read_page(p_offset, p);
    //root가 존재하지 않는다면
    if(p == NULL){
        printf("Empty tree.\n");
        return -1;
    }
    //p가 leaf node일 때까지 돈다.
    while(!p->is_Leaf){
        i=0;
        while(i < p->num_keys){
            if(key >= p->i_records[i].key)
                i++;
            else break;
        }
        //만약에 i==0이면 가장 왼쪽에 있는 key이다.
        if(i==0)
            p_offset = p->right_sibling_or_one_more_page_number;
        else
            //i번째의 key값이 넣으려는 key값보다 크므로 i-1번째의 page에다가 넣는다.
            p_offset = p->i_records[i-1].page_offset;
        //해당 p_offset을 p로 다시 읽어들인다. is_Leaf==1일때까지 위의 행동을 반복한다.
        file_read_page(p_offset,p);
    }
    free(p);
    return p_offset;
}
//현재 file에 header만 있으면 root를 만들어준다.
void start_new_tree(int64_t key, char* value){
   // printf("start_new_tree\n");
    root = (page_t*)malloc(sizeof(page_t));
    pagenum_t root_offset = file_alloc_page();
    header->root_page_offset = root_offset;
   // printf("root offset : %llu\n", root_offset);
    file_read_page(root_offset, root);
    root->is_Leaf=1;
    root->l_records[0].key = key;
    strcpy(root->l_records[0].value, value);
    root->num_keys = 1;
    file_write_page(root_offset, root);
    file_write_header_page(0,header);
}
int db_delete(int64_t key){
  //  printf("db_delete\n");
    //root가 없다면
    if(root->num_keys==0)
        return -1;
    char tmp[120];
    char* tmp_value = db_find(key, tmp);
    //해당 key가 존재하지않는다.
    if(tmp == NULL)
        return -1;
    //해당 key값이 위치한 leaf page를 찾아서 지워준다.
    pagenum_t delete_offset = find_leaf_page(key, tmp_value);
  //  printf("delete_offset : %llu\n",delete_offset);
  //  printf("root_offset : %llu\n",header->root_page_offset);
    delete_entry(key, delete_offset);
  //  printf("delete key\n");
    return 0;
}
void remove_entry_from_page(int64_t key, pagenum_t delete_offset){
  //  printf("remove_entry_from_page\n");
    int i;
    page_t* delete_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(delete_offset, delete_page);
    //internal page라면
    if(!delete_page->is_Leaf){
        i=0;
        //key값이 포함된 record의 index찾기
        //i번째가 지워야할 key가 들어있는 record
        while(delete_page->i_records[i].key != key)
            i++;
        //지워야할 key값 뒤의 값들을 앞으로 당기면서 덮어씌운다.
        for(++i; i<delete_page->num_keys; i++){
            delete_page->i_records[i-1].key = delete_page->i_records[i].key;
            delete_page->i_records[i-1].page_offset = delete_page->i_records[i].page_offset;
        }
        //앞으로 당겨진 i번째 값은 0으로 초기화한다.
        delete_page->i_records[i-1].key = 0;
        delete_page->i_records[i-1].page_offset = 0;
        delete_page->num_keys--;
        file_write_page(delete_offset, delete_page);
        if(delete_offset == header->root_page_offset)
            file_read_page(delete_offset, root);
    }
    //leaf page라면
    else{
    //    printf("leaf\n");
        i=0;
        while(delete_page->l_records[i].key != key)
            i++;
        for(++i; i < delete_page->num_keys; i++){
            delete_page->l_records[i-1].key = delete_page->l_records[i].key;
            strcpy(delete_page->l_records[i-1].value , delete_page->l_records[i].value);
        }
        //i-1번째 값은 0으로 초기화해준다.
        delete_page->l_records[i-1].key = 0;
  //      strcpy(delete_page->l_records[i].value, NULL);
        delete_page->num_keys--;
        file_write_page(delete_offset, delete_page);
        if(delete_offset == header->root_page_offset)
            file_read_page(delete_offset, root);
    }
    return;
}

void delete_entry(int64_t key, pagenum_t delete_offset){
    printf("delete_entry\n");
 //   //page에서 key가 포함되어있는 record를 삭제한다.
    remove_entry_from_page(key, delete_offset);
    //만약 root만 있을 때, 지워야한다면 root를 조정해준다.
    if(delete_offset == header->root_page_offset){
        adjust_root(delete_offset);
        return;
    }
    //delete_offset = 지워야할 key가 있는 leaf page or internal page의 pagenum
    page_t* delete_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(delete_offset, delete_page);
   // printf("delete_page num keys : %llu",delete_page->num_keys);
    //max_page : 이웃 page와 key값을 합쳤을 때, max_page를 초과하면 merge할 수 없다.
    //cut_page : 최소 개수
    int max_page, cut_page;
    if(delete_page -> is_Leaf){
        //31개까지 들어갈 수 있다.
        max_page = MAX_LEAF;
        cut_page = 1;
    }
    else{
        //248까지 들어갈 수 있다.
        max_page = MAX_INTERNAL;
        cut_page = 1;
    }
    //만약에 최소 수보다 많이 있으면 굳이 할 필요가 없으므로 return 한다.
    if(delete_page->num_keys > cut_page)
        return;
    int i;
    int left_page_index;
    pagenum_t left_page_offset;
    //k_prime_index : k_prime이 존재하는 page의 index
    //k_prime : parent에 넘겨줄 key값
    int k_prime_index;
    int64_t k_prime;
    //지우려고 하는 page의 parent_page
    pagenum_t parent_page_offset = delete_page->next_free_or_parent_page_number;
    page_t* parent_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(parent_page_offset, parent_page);
    //만약 지우려는 page의 위치가 가장 왼쪽이라면 오른쪽 page에서 가져온다.
   // printf("parent_page-> one_more_page : %llu\n",parent_page->right_sibling_or_one_more_page_number);
    if(delete_offset == parent_page->right_sibling_or_one_more_page_number){
        left_page_index = -2;
        left_page_offset = parent_page->i_records[0].page_offset;
        k_prime_index = 0;
        k_prime = parent_page->i_records[0].key;
    }
    //위치가 두번째라면 left page를 one_more_page로 설정한다.
    else if(delete_offset == parent_page->i_records[0].page_offset){
        left_page_index = -1;
        left_page_offset = parent_page->right_sibling_or_one_more_page_number;
        k_prime_index = 0;
        k_prime = parent_page->i_records[0].key;
    }
    //그 외의 경우일 때,
    else{
        //delete_offset과 같은 page_offset의 위치를 찾는다.
        for(i=0; i<=parent_page->num_keys; i++){
            if(delete_offset == parent_page->i_records[i].page_offset)
                break;
        }
        //delete_offset == i
        left_page_index = i-1;
        left_page_offset = parent_page->i_records[left_page_index].page_offset;
        k_prime_index = left_page_index+1;
        k_prime = parent_page->i_records[k_prime_index].key;
    }
   // printf("left_page_offset: %llu\n", left_page_offset);
    page_t* left_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(left_page_offset, left_page);
    //합이 max_page를 넘기면 왼쪽 page에서 key를 가져온다.
  //  printf("max : %llu\n", left_page->num_keys + delete_page->num_keys);
    if(left_page->num_keys + delete_page->num_keys > max_page){
        redistribute_pages(delete_offset, left_page_index, left_page_offset, parent_page_offset, k_prime, k_prime_index);
        return;
    }
    //그렇지 않으면 merge한다.
    else{
        coalesce_pages(delete_offset, left_page_index, left_page_offset, parent_page_offset, k_prime);
        return;
    }
}
//key를 sibling page에서 가져오는 함수
void redistribute_pages(pagenum_t delete_offset, int left_page_index, pagenum_t left_page_offset, pagenum_t parent_page_offset, int64_t k_prime, int k_prime_index){
  //  printf("redistribute_pages\n");
    page_t* delete_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(delete_offset, delete_page);
    page_t* left_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(left_page_offset, left_page);
    page_t* parent_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(parent_page_offset, parent_page);
    //만약 one_more_page가 delete_page라면
    if(left_page_index == -2){
        //leaf page 라면
        //여기서는 left_page는 오른쪽 page를 의미한다.
        if(delete_page->is_Leaf){
            delete_page->l_records[delete_page->num_keys].key = left_page->l_records[0].key;
            strcpy(delete_page->l_records[delete_page->num_keys].value, left_page->l_records[0].value);
            delete_page->num_keys++;
            int i;
            //오른쪽의 0번째 값을 왼쪽에 줬으니 오른쪽의 나머지 값들을 앞으로 당긴다.
            for(i=0; i<left_page->num_keys-1; i++){
                left_page->l_records[i].key = left_page->l_records[i+1].key;
                strcpy(left_page->l_records[i].value, left_page->l_records[i+1].value);
            }
            //앞으로 옮기고나서 i번째의 값은 0으로 초기화해준다.
            left_page->l_records[i].key = 0;
     //       strcpy(left_page->l_records[i].value, NULL);
            left_page->num_keys--;
            parent_page->i_records[k_prime_index].key = left_page->l_records[0].key;
            parent_page->i_records[k_prime_index].page_offset = left_page_offset;
        }
        //internal page라면
        else{
            //k_prime : 오른쪽 페이지의 첫번째 값
            delete_page->i_records[delete_page->num_keys].key = k_prime;
            //page_offset은 오른쪽 페이지의 one_more_page의 offset으로 설정한다.
            delete_page->i_records[delete_page->num_keys].page_offset = left_page->right_sibling_or_one_more_page_number;
            page_t* child_page = (page_t*)malloc(sizeof(page_t));
           //child_page는 오른쪽 페이지에서 가져온 one_more_page를 의미한다.
            // child_page를 왼쪽에 옮겼으므로 child_page의 parent를 바꿔준다.
            file_read_page(delete_page->i_records[delete_page->num_keys].page_offset, child_page);
            child_page->next_free_or_parent_page_number = delete_offset;
            file_write_page(delete_page->i_records[delete_page->num_keys].page_offset, child_page);
            delete_page->num_keys++;
            //오른쪽 page의 one_more_page가 없어졌으므로 0번째를 one_more_page로 만든다.
            left_page->right_sibling_or_one_more_page_number = left_page->i_records[0].page_offset;
            //앞으로 당겨준다.
            int i;
            for(i=0; i<left_page->num_keys-1; i++){
                left_page->i_records[i].key = left_page->i_records[i+1].key;
                left_page->i_records[i].page_offset = left_page->i_records[i+1].page_offset;
            }
            //당겨주고 남은 값은 0으로 초기화해준다.
            left_page->i_records[i].key=0;
            left_page->i_records[i].page_offset=0;
            left_page->num_keys--;
            //parent_page의 0번째 key를 덮어씌운다.
            parent_page->i_records[k_prime_index].key = left_page->i_records[0].key;
        }
    }
    //one_more_page가 아닌 다른 페이지일 때
    else{
        //leaf page일 때,
        if(delete_page->is_Leaf){
            int i;
            //페이지의 값들을 한칸씩 뒤로 미룬다.
            for(i=delete_page->num_keys; i>0; i--){
                delete_page->l_records[i].key = delete_page->l_records[i-1].key;
                strcpy(delete_page->l_records[i].value, delete_page->l_records[i-1].value);
            }
            //왼쪽 페이지의 마지막 값을 오른쪽 페이지의 맨앞에다가 놓는다.
            delete_page->l_records[0].key = left_page->l_records[left_page->num_keys-1].key;
            strcpy(delete_page->l_records[0].value, delete_page->l_records[left_page->num_keys-1].value);
            left_page->l_records[left_page->num_keys-1].key=0;
            parent_page->i_records[k_prime_index].key = delete_page->l_records[0].key;
            left_page->num_keys--;
            delete_page->num_keys++;
        }
        //internal page일 때,
        else{
            int i;
            //페이지의 값들을 뒤로 미룬다.
            for(i=delete_page->num_keys; i>0; i--){
                delete_page->i_records[i].key = delete_page->i_records[i-1].key;
                delete_page->i_records[i].page_offset = delete_page->i_records[i-1].page_offset;
            }
            delete_page->i_records[0].key = k_prime;
            delete_page->i_records[0].page_offset = delete_page->right_sibling_or_one_more_page_number;
            delete_page->right_sibling_or_one_more_page_number = left_page->i_records[left_page->num_keys-1].page_offset;
            left_page->num_keys--;
            delete_page->num_keys++;
            //옮겨준 one_more_page의 부모를 delete_page로 바꾼다.
            page_t* child_page = (page_t*)malloc(sizeof(page_t));
            file_read_page(delete_page->right_sibling_or_one_more_page_number, child_page);
            child_page->next_free_or_parent_page_number = delete_offset;
            file_write_page(delete_page->right_sibling_or_one_more_page_number,child_page);
            parent_page->i_records[k_prime_index].key = delete_page->i_records[0].key;
        }
    }
    file_write_page(delete_offset, delete_page);
    file_write_page(parent_page_offset, parent_page);
    file_write_page(left_page_offset, left_page);
    return;
}
//merge 함수
//페이지에 레코드가 1개가 남아있을때까지 merge를 하지 않는다.
void coalesce_pages(pagenum_t delete_offset, int left_page_index, pagenum_t left_page_offset, pagenum_t parent_page_offset, int64_t k_prime){
    //printf("coalesce_pages\n");
    int i, j, left_insertion_index, left_end;
    //만약 one_more_page라면
    //delete_offset 과 left_page_offset을 교체한다.
    if(left_page_index == -2){
        pagenum_t tmp_offset;
        tmp_offset = delete_offset;
        delete_offset = left_page_offset;
        left_page_offset = tmp_offset;
    }
    page_t* delete_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(delete_offset, delete_page);
    page_t* left_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(left_page_offset, left_page);
    page_t* parent_page = (page_t*)malloc(sizeof(page_t));
    file_read_page(parent_page_offset, parent_page);
    
    left_insertion_index = left_page->num_keys;
    left_end = delete_page->num_keys;
  //  printf("left_insertion_index : %d\n",left_page->num_keys);
  //  printf("left_end : %d\n", left_end);
    //leaf page라면
    //뒤로 key값을 밀고
    if(delete_page->is_Leaf){
        for(i=left_insertion_index, j=0; j<left_end; i++, j++){
            left_page->l_records[i].key = delete_page->l_records[j].key;
            strcpy(left_page->l_records[i].value, delete_page->l_records[j].value);
            left_page->num_keys++;
            delete_page->num_keys--;
        }
        left_page->right_sibling_or_one_more_page_number = delete_page->right_sibling_or_one_more_page_number;
    //    printf("left_page : \n");
    /*    for(int t=0; t<left_page->num_keys; t++)
            printf("%llu ",left_page->l_records[t].key);
        printf("\n");
  */  //    printf("delete_page : \n");
    /*    for(int t=0; t<delete_page->num_keys; t++)
            printf("%llu ",delete_page->l_records[t].key);
        printf("\n");
  */  }
    //internal page일 때
    else{
        left_page->i_records[left_insertion_index].key = k_prime;
        left_page->i_records[left_insertion_index].page_offset = delete_page->right_sibling_or_one_more_page_number;
        left_page->num_keys++;
        
        for(i=left_insertion_index+1, j=0; j<left_end; i++, j++){
            left_page->i_records[i].key = delete_page->i_records[j].key;
            left_page->i_records[i].page_offset = delete_page->i_records[j].page_offset;
            left_page->num_keys++;
            delete_page->num_keys--;
        }
        for(i=0; i<left_page->num_keys+1; i++){
            page_t* tmp = (page_t*)malloc(sizeof(page_t));
            file_read_page(left_page->i_records[i].page_offset, tmp);
            tmp->next_free_or_parent_page_number = left_page;
            free(tmp);
        }
    }
    file_write_page(left_page_offset, left_page);
    delete_entry(k_prime, parent_page_offset);
    file_free_page(delete_offset);
    return;
}
//root의 key값들이 모두 없어졌을 때, 새로운 root를 생성하는 함수.
void adjust_root(pagenum_t delete_offset){
  //  printf("adjust_root\n");
    if(root->num_keys==0){
        if(!root->is_Leaf){
            pagenum_t new_root_offset = root->right_sibling_or_one_more_page_number;
            page_t* new_root = (page_t*)malloc(sizeof(page_t));
            file_read_page(new_root_offset, new_root);
            new_root->next_free_or_parent_page_number=0;
            file_free_page(delete_offset);
            header->root_page_offset = new_root_offset;
            file_write_header_page(0, header);
            file_write_page(new_root_offset, new_root);
            file_read_header_page(0,header);
            file_read_page(header->root_page_offset, root);
        }
        else{
            file_free_page(delete_offset);
            header->root_page_offset = 0;
            file_write_header_page(0,header);
            file_read_header_page(0, header);
        }
    }
    return;
}


void find_and_print(int64_t key, char * ret_val){
    char *v = db_find(key, ret_val);
    if(v == NULL)
        printf("Record not found\n", key, v);
    else
        printf("Record found -- key %d, value %s\n", key, v);
    
    printf("======================\n");
    int i;
    file_read_page(header->root_page_offset, root);
    //printf("root isleaf? %d, root numkeys : %d\n", root->isLeaf, root->num_keys);
    if(root->is_Leaf){
        printf("@@@@@@@@@@@@@ Root is Leaf @@@@@@@@@@@@@\n");
        for(i=0; i<root->num_keys; i++){
            printf("%d ",root->l_records[i].key);
        }
    }
    else{
        printf("@@@@@@@@@@@@@ Root is inter @@@@@@@@@@@@@\n");
        for(i=0; i<root->num_keys; i++){
            printf("%d ",root->i_records[i].key);
        }
        printf("\n\n");
        page_t * tmpLeft = (page_t*)calloc(1, sizeof(page_t));
        file_read_page(root->right_sibling_or_one_more_page_number, tmpLeft);
        
        if(tmpLeft->is_Leaf){
            printf("@@@@@@@@@@@@@ Root's sp_o child @@@@@@@@@@@@@\n");
            printf("left : \n");
            for(i=0; i<tmpLeft->num_keys; i++){
                printf("%d ",tmpLeft->l_records[i].key);
            }
            file_read_page(root->i_records[0].page_offset, tmpLeft);
            printf("\n@@@@@@@@@@@@@ Root's [0] child @@@@@@@@@@@@@\n");
            printf("\nright : \n");
            for(i=0; i<tmpLeft->num_keys; i++){
                printf("%d ",tmpLeft->l_records[i].key);
            }
            
            file_read_page(root->i_records[root->num_keys-2].page_offset, tmpLeft);
            printf("\n@@@@@@@@@@@@@ Root's last-2 child @@@@@@@@@@@@@\n");
            printf("\n%d - left : \n", root->i_records[root->num_keys-2].page_offset);
            for(i=0; i<tmpLeft->num_keys; i++){
                printf("%d ",tmpLeft->l_records[i].key);
            }
            file_read_page(root->i_records[root->num_keys-1].page_offset, tmpLeft);
            printf("\n@@@@@@@@@@@@@ Root's last-1 child @@@@@@@@@@@@@\n");
            printf("\n%d - right : \n",root->i_records[tmpLeft->num_keys-1].page_offset);
            for(i=0; i<tmpLeft->num_keys; i++){
                printf("%d ",tmpLeft->l_records[i].key);
            }
        }
        else{
            printf("\n@@@@@@@@@@@@@ Root's 3 height @@@@@@@@@@@@@\n");
            file_read_page(root->right_sibling_or_one_more_page_number, tmpLeft);
            printf("@@@@@@@@@@@@@ Root's sp_o child @@@@@@@@@@@@@\n");
            printf("left : \n");
            for(i=0; i<tmpLeft->num_keys; i++){
                printf("%d ",tmpLeft->i_records[i].key);
            }
            file_read_page(root->i_records[0].page_offset, tmpLeft);
            printf("\n@@@@@@@@@@@@@ Root's [0] child @@@@@@@@@@@@@\n");
            printf("right : \n");
            for(i=0; i<tmpLeft->num_keys; i++){
                printf("%d ",tmpLeft->i_records[i].key);
            }
            
            page_t * tmptmp = (page_t*)calloc(1, sizeof(page_t));
            file_read_page(tmpLeft->right_sibling_or_one_more_page_number, tmptmp);
            
            if(tmptmp->is_Leaf){
                printf("\n\n\n@@@@@@@@@@@@@ tempLeft's sp_o child @@@@@@@@@@@@@\n");
                printf("left : \n");
                for(i=0; i<tmptmp->num_keys; i++){
                    printf("%d ",tmptmp->l_records[i].key);
                }
                
                printf("\n@@@@@@@@@@@@@ tempLeft's [0] child @@@@@@@@@@@@@\n");
                file_read_page(tmpLeft->i_records[0].page_offset, tmptmp);
                printf("right : \n");
                for(i=0; i<tmptmp->num_keys; i++){
                    printf("%d ",tmptmp->l_records[i].key);
                }
                
                file_read_page(tmpLeft->i_records[tmpLeft->num_keys-2].page_offset, tmptmp);
                printf("\n@@@@@@@@@@@@@ tempLeft's [last's Left] child @@@@@@@@@@@@@\n");
                printf("%d - left : \n", tmpLeft->i_records[tmpLeft->num_keys-2].page_offset);
                for(i=0; i<tmptmp->num_keys; i++){
                    printf("%d ",tmptmp->l_records[i].key);
                }
                file_read_page(tmpLeft->i_records[tmpLeft->num_keys-1].page_offset, tmptmp);
                printf("\n@@@@@@@@@@@@@ tempLeft's [last's Right] child @@@@@@@@@@@@@\n");
                printf("%d - right : \n",tmpLeft->i_records[tmpLeft->num_keys-1].page_offset);
                for(i=0; i<tmptmp->num_keys; i++){
                    printf("%d ",tmptmp->l_records[i].key);
                }
            }
            
        }
    }
    printf("\n");
    printf("======================\n");
}
