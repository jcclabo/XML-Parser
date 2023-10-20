#ifndef _ELEMENT_
#define _ELEMENT_

#include "helper.h"

#define MAX_NAME_SIZE 256
#define MAX_VAL_SIZE 2048
#define MAX_PATH_SIZE 1024

typedef struct Attribute {
    char* name; 
    char* value;
} Attr;

typedef struct Element {
    char* name;
    struct Element* el_arr;
    struct Attribute* attr_arr;
    char* value;
    char* path;
} El;

El* get_first_el(El*, char*);

El* get_next_el(El*); 

El* get_next_el_by_name(El*, char*); 

char* get_el_value(El*);

Attr* get_next_attr(Attr*);

char* get_attr_value(El*, char*);

void free_child(El*);

void free_el(El*);

#endif