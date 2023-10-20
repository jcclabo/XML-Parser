#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmlparser.h"

// TLDR:
//      - parse_file() will build and return a DOM. The parse_file() method can be found in "xml_parser.c"
//          The parse_file() method takes in two parameters: a file path, and a callback for reporting
//
//      - Methods to traverse the DOM can be found in "element.c"
//          "element.c" also contains a free_el() method for freeing the DOM
//
//      - The DOM is constructed of Elements (El), and Elements may contain Attributes (Attr)
//          The structure of an Element and an Attribute is shown in "element.h"


// callbacks for tests
int print_progress(char* info_type, char* value, char* path);           // test.txt
int print_title_and_price(char* info_type, char* value, char* path);    // books.xml
int print_shipped_date_info(char* info_type, char* value, char* path);  // customers_orders.xml


int main() {

    // The below examples illustrate how "xmlparser" can be used to build a Document Object Model.

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


    // I have included two additional XML documents I found online

    printf("\n\n ------------------------- Parsing books.xml ------------------------\n");

    xml_dom = parse_file("books.xml", print_title_and_price);

    free_el(xml_dom);
    xml_dom = NULL;

    // Another test
    
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
