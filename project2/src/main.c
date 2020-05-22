#include "bpt.h"

// MAIN

int main( int argc, char ** argv ) {
    uint64_t key;
    char value[120];
    char instruction;
    char * ret_val;
    int i=0;
    open_table("test.db");
    while(scanf("%c",&instruction) != EOF){
        switch(instruction){
            case 'i':
                for(i=1; i<4001; i++){
                printf("*************%d, %s\n", i, "ins");
                db_insert(i, "ins");
                find_and_print(i, "ins");
                }
                break;
            case 'j':
                scanf("%lld %s", &key, value);
                db_insert(key, value);
                find_and_print(i,"ins");
                break;
            case 'f':
                scanf("%lld",&key);
                ret_val = db_find(key, ret_val);
                if(ret_val != NULL){
                    printf("Key: %lld, Value: %s\n", key,ret_val);
                }
                else
                    printf("Not Exists\n");
                
                break;
            case 'd':
                scanf("%ld", &key);
                db_delete(key);
                find_and_print(key,"ins");
                break;
            case 'p':
                scanf("%lld", &key);
                find_and_print(key, "ins");
                break;
            case 'q':
                while(getchar() != (int)'\n');
                return EXIT_SUCCESS;
                break;
        }
        while (getchar() != (int)'\n');
    }
    printf("\n");
    return 0;
}
