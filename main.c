#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmlparser.h"

// Notes:
//      - parse_file() 
//          will build and return an Element containing the document object model (DOM). 
//          can be found in "xml_parser.c". 
//          takes in two parameters: file path, callback for reporting.
//
//      - free_el() and traversal methods can be found in "element.c"
//
//      - Elements may contain Attributes (Attr). 
//      - The structure of an Element and an Attribute is shown in "element.h".


// callbacks for tests
int print_progress(char* info_type, char* value, char* path);           // test.txt
int print_title_and_price(char* info_type, char* value, char* path);    // books.xml
int print_shipped_date_info(char* info_type, char* value, char* path);  // customers_orders.xml


int main() {

    printf("\n ------------------------- Parsing test.txt -------------------------");

    El* xml_dom = parse_file("test.txt", print_progress);

    printf("\n --------------------------------------------------------------------");

    if (xml_dom != NULL) {
        printf("\n\nOutput the order ids for any order with an amount greater than 100");

        El* order_p = get_first_el(xml_dom, "/order");

        while (order_p != NULL) {
            El* amt_p = get_first_el(order_p, "/amount");
            char* orderid = get_attr_value(order_p, "id");
            char* amt = get_el_value(amt_p);

            if (atoi(amt) > 100)
                printf("\n  order id: %s", orderid);

            order_p = get_next_el_by_name(order_p, "order");
        }
    } else {
        // Handle error
        printf("\n-- An error was caught");
    }
    
    free_el(xml_dom);
    xml_dom = NULL;

    // sample test 1

    printf("\n\n ------------------------- Parsing books.xml ------------------------\n");

    xml_dom = parse_file("books.xml", print_title_and_price);

    free_el(xml_dom);
    xml_dom = NULL;

    // sample test 2
    
    printf("\n ------------------- Parsing customers_orders.xml -------------------\n");

    xml_dom = parse_file("customers_orders.xml", print_shipped_date_info);

    free_el(xml_dom);
    xml_dom = NULL;

    return 0;
}


/* callback examples */ 

int print_progress(char* info_type, char* value, char* path) {
    if (strcmp(path, "") == 0) // comments above the root have a path value of the empty string
        path = "\"\"";

    printf("\n%s: ", info_type);
    printf("%s \t\t\t", value);
    printf("path: %s", path);

    return EXIT_SUCCESS;
}

int print_title_and_price(char* info_type, char* value, char* path) {

    if (strcmp(info_type, "Element Value") == 0) {
        if (strcmp(path, "/book/title") == 0)
            printf("\nTitle: %s", value);

        if (strcmp(path, "/book/price") == 0)
            printf("\nPrice: %s\n", value);
    }

    return EXIT_SUCCESS;
}

int print_shipped_date_info(char* info_type, char* value, char* path) {

    if (strcmp(info_type, "Attribute Name") == 0) {
        if (strcmp(path, "/Orders/Order/ShipInfo") == 0) {
            printf("\n%s \t\t\t| ", value);
            printf("path: %s", path);
        }

    } else if (strcmp(info_type, "Attribute Value") == 0) {
        if (strcmp(path, "/Orders/Order/ShipInfo") == 0) {
            printf("\n%s \t\t| ", value);
            printf("path: %s\n", path);
        }
    }

    return EXIT_SUCCESS;
}
