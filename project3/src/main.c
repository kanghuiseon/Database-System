#include "bpt.h"

int main(void) {
    int i;
    int table_id;
    int size;
    int64_t input;
    char instruction;
    char buf[120];
    char path[120];

//   print_file();
    init_db(40);
    if(open_table("test.db")==-1){
        printf("cannot open table!\n");
        return 0;
    };
//    printf("%s table ID is : %d, table FD is: %d\n", path, open_table("test.db"), db_find_fd(open_table("test.db")));
//    printf("again fd : %d\n", db_find_fd(1));
//    printf("> ");
    while(scanf("%c", &instruction) != EOF){
        switch(instruction){
            case 'c':
                scanf("%d", &table_id);
                close_table(table_id);
//                print_buffer();
                printf("main - close table %d\n", table_id);
                break;
            case 'd':
                scanf("%d %lld", &table_id, &input);
                db_delete(table_id, input);
             //   find_and_print(table_id, input, "ins");
                break;
            case 'i':
//                scanf("%d %lld %s", &table_id, &input, buf);
//                db_insert(table_id, input, buf);
                for(i=1; i<10000; i++){
                    printf("i : %d\n", i);
                    db_insert(1, i, "ins");
                //    find_and_print(1, i, "ins");
                }
                find_and_print(1, i, "ins");
//                printf("after insert %ld\n", input);
//                print_file(1);
                break;
            case 'j':
                scanf("%d %lld %s", &table_id, &input, buf);
                db_insert(table_id, input, buf);
                find_and_print(table_id, input, buf);
                break;
            case 'f':
                scanf("%d %lld", &table_id, &input);
                char * ftest = (char*)malloc(120*sizeof(char));
                if(db_find(table_id, input, ftest) != 0){
                    printf("Key: %lld, Value: %s\n", input, ftest);
                    fflush(stdout);
                    free(ftest);
                }
                else{
                    printf("Not Exists\n");
                    fflush(stdout);
                }
                break;
            case 'l':
                scanf("%d", &table_id);
                //print_file(table_id);
                break;
            case 'n':
                scanf("%d", &size);
                init_db(size);
//                printf("Buffer setted!\n");
                break;
            case 'o':
                scanf("%s", path);
                table_id = open_table(path);
                if(table_id == -1){
                    printf("cannot open table!\n");
                    return 0;
                }
                printf("%s table ID is : %d, FD is : %d\n", path, table_id, find_table_fd(table_id));
                break;
            case 'p':
                scanf("%d %lld", &table_id, &input);
                find_and_print(table_id, input, "ins");
                break;
            case 'q':
                shutdown_db();
                while(getchar() != (int64_t)'\n');
                return 0;
                break;
        }
        while(getchar() != (int)'\n');
//                printf("> ");
    }
    printf("\n");

    return 0;
}
