#include <stdio.h>
#include "order_queue.h"
#include "courier.h"
#include <unistd.h>
#include <string.h>



int main(int argc, char *argv[]) {

    /*
    initialize_order_list(30);

    item_1.duration = 23;
    item_1.id = 2222;
    item_1.prt = STANDARD;
    strcpy(item_1.name, "example");

    item_2.duration = 33;
    item_2.id = 2322;
    item_2.prt = ECONOMY;
    strcpy(item_2.name, "örnek");

    item_3.duration = 3;
    item_3.id = 22220;
    item_3.prt = EXPRESS;
    strcpy(item_3.name, "misal");

    write_order_list(&item_1);
    write_order_list(&item_2);
    write_order_list(&item_3);

    read_order_list(&item_1);
    print_priority(item_1.prt);

    read_order_list(&item_2);
    print_priority(item_2.prt);

    read_order_list(&item_3);
    print_priority(item_3.prt);

    



    get_arguments(argc, argv, &cmd);
    
    read_file(cmd.input_file);

    for(int i = 0; queue->size; i++) {
        read_order_list(&item_1);

        printf("id = %d, name = %s, duration = %d ", item_1.id, item_1.name, item_1.duration);
        print_priority(item_1.prt);
        printf("\n");
    }

    release_list(); */

    courier_main(argc, argv);

    return 0;

}

