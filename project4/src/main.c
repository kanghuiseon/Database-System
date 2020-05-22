
//  main.c
//  project4
//
//  Created by 김도은 on 07/11/2019.
//  Copyright © 2019 김도은. All rights reserved.
//
#include "bpt.h"
#include <time.h>
#include <stdlib.h>

int main(void) {
    int i;
    int table_id;
    int size;
    int64_t input;
    int tmpTable_id1;
    int tmpTable_id;
    char instruction;
    char buf[120];
    char path[120];
    clock_t start, end;

//   print_file();
    init_db(400);
    tmpTable_id1 = open_table("test1.db");
    printf("table ID is : %d, FD is : %d\n", tmpTable_id1, find_table_fd(tmpTable_id1));
//    printf("%s table ID is : %d, table FD is: %d\n", path, open_table("test.db"), db_find_fd(open_table("test.db")));
//    printf("again fd : %d\n", db_find_fd(1));
//    printf("> ");
    while(scanf("%c", &instruction) != EOF){
        switch(instruction){
            case 'c':
                scanf("%d", &table_id);
                close_table(table_id);
//               print_buffer();
                printf("main - close table %d\n", table_id);
                break;
            case 'd':
                scanf("%d %lld", &table_id, &input);
                db_delete(table_id, input);
           //     find_and_print(table_id, input, "ins");
                break;
            case 'b':
                start = clock();
                for(i=1; i<4001; i++){
                    printf("del : i : %d\n", i);
                    db_delete(1, i);
          //          find_and_print(1, i, "ins");
                }
                end = clock();
                printf("delay : %.3f\n", (float)(end-start)/CLOCKS_PER_SEC);
      //          find_and_print(1, i, "ins");
                break;
            case 'i':
//                scanf("%d %lld %s", &table_id, &input, buf);
//                db_insert(table_id, input, buf);
                start = clock();
                for(i=1; i<=200; i++){
                    //printf("ins : i : %d\n", i);
                    db_insert(1, i, "ins1");
                    //find_and_print(1, i, "ins");
                    //i++;
                }
                for(i=1; i<=200; i++){
                    db_insert(2, i, "ins2");
                }
                end = clock();
                printf("delay : %.3f\n", (float)(end-start)/CLOCKS_PER_SEC);
           //     find_and_print(1, i, "ins1");
//                printf("after insert %ld\n", input);
//                print_file(1);
                break;
            case 'a':
//                scanf("%d %lld %s", &table_id, &input, buf);
//                db_insert(table_id, input, buf);
                start = clock();
                for(i=1; i<=200; i++){
                    //printf("ins : i : %d\n", i);
                    db_insert(2, i, "ins2");
                    //find_and_print(1, i, "ins");
                    i++;
                }
                end = clock();
                printf("delay : %.3f\n", (float)(end-start)/CLOCKS_PER_SEC);
           //     find_and_print(2, i, "ins2");
//                printf("after insert %ld\n", input);
//                print_file(1);
                break;
            case 'e':
                scanf("%d %lld %s", &table_id, &input, buf);
                db_insert(table_id, input, buf);
          //      find_and_print(table_id, input, buf);
                break;
            case 'j':
                start = clock();
                join_table(1, 2, "result_join");
                end = clock();
                printf("delay : %.3f\n", (float)(end-start)/CLOCKS_PER_SEC);
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
            case 'n':
                scanf("%d", &size);
                init_db(size);
//                printf("Buffer setted!\n");
                break;
            case 'o':
                tmpTable_id = open_table("test2.db");
                printf("table ID is : %d, FD is : %d\n", tmpTable_id, find_table_fd(tmpTable_id));
                break;
            case 'p':
                scanf("%d %lld", &table_id, &input);
             //   find_and_print(table_id, input, "ins");
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
