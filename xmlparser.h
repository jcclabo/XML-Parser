#ifndef _XMLPARSER_
#define _XMLPARSER_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "element.h"
// element.h includes helper.h for faster string cat

/**
 * @brief a callback for repporting information associated with parsing. The type of information is specified in the 1st parameter
 * 
 * @param 1st "Element Name" or "Element Value" or "Attribute Name" or "Attribute Value" or "Comment" or "Error"
 * @param 2nd value
 * @param 3rd path
 */
typedef int callback(char*, char*, char*);

#define MAX_COMMENT_SIZE 12345

El* parse_file(char*, callback*);

El* parse_el(char*, FILE*, char*, callback*);

int parse_close_el(char* c, El*, FILE*, callback*);

int parse_el_content(char*, El*, FILE*, callback*);

int parse_open_el(char*, El*, FILE*, callback*);

int alloc_el_arr(El*, int, callback*);

int alloc_attr_arr(El*, int, callback*);

int parse_attrs(char*, El*, FILE*, callback*);

int unique_attr_check(El*, int, callback*);

int parse_attr_value(char*, char*, FILE*, char*, callback*);

int parse_attr_name(char*, char*, FILE*, char*, callback*);

int parse_el_value(char*, El*, FILE*, callback*);

int parse_el_name(char*, El*, FILE*, callback*);

int report_any_comments(char*, FILE*, char*, callback*);

int report_comment(char*, FILE*, char*, callback*);

char* parse_comment(char*, char* comment, FILE*, char*, callback*);

int no_op_callback(char*, char*, char*);

char parse_entity(char*, FILE*, char*, callback*);

#endif
