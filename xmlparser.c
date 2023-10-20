#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "xmlparser.h"

// used if NULL is passed for the callback* parameter
int no_op_callback(char* infotype, char* value, char* path) {
    return EXIT_SUCCESS;
}

// returns an unescaped entity, or EXIT_FAILURE on error
char parse_entity(char* c, FILE* stream, char* path, callback* callback) {
    // *c == '&'
    *c = fgetc(stream);

    if (*c == 'l') {             // &lt;
        *c = fgetc(stream);
        if (*c == 't') { 
            *c = fgetc(stream);
            if (*c == ';') {
                return '<';     // <
            }
        }
    } else if (*c =='g') {       // &gt;
        *c = fgetc(stream);
        if (*c == 't') { 
            *c = fgetc(stream);
            if (*c == ';') {
                return '>';     // >
            }
        }
    } else if (*c == 'q') {              // &quot;
        *c = fgetc(stream);
        if (*c == 'u') { 
            *c = fgetc(stream);
            if (*c == 'o') {
                *c = fgetc(stream);
                if (*c == 't') { 
                    *c = fgetc(stream);
                    if (*c == ';') {
                        return '\"';    // "
                    }
                }
            }
        }
    } else if (*c == 'a') {              // &apos;
        *c = fgetc(stream);
        if (*c == 'p') { 
            *c = fgetc(stream);
            if (*c == 'o') {
                *c = fgetc(stream);
                if (*c == 's') { 
                    *c = fgetc(stream);
                    if (*c == ';') {
                        return '\'';    // '
                    }
                }
            }   
        } else if (*c == 'm') {      // &amp;
            *c = fgetc(stream);
            if (*c == 'p') { 
                *c = fgetc(stream);
                if (*c == ';') {
                    return '&';     // &
                }
            }
        }
    }
    callback("Error", "Unescaped entity \"&\"", path);
    return EXIT_FAILURE;
}

// returns NULL on error
// *note: *c must equal '!' on entering parse_comment
char* parse_comment(char* c, char* comment, FILE* stream, char* path, callback* callback) {
    comment[0] = '\0';
    char* comment_end = comment; // maintain a pointer to the end of the comment string for faster concatenation

    if (*c != '!')
        return NULL;

    *c = fgetc(stream);
    if (*c != '-') {
        callback("Error", "Incomplete comment starting token \"<!\"", path);
        return NULL;
    }
            
    *c = fgetc(stream);
    if (*c != '-') {
        callback("Error", "Incomplete comment starting token \"<!-\"", path);
        return NULL;
    } 
        
    // found starting characters
    *c = fgetc(stream);
    while (1) {
        while (*c != '-') {
            comment_end = appendchar(comment_end, *c);
            *c = fgetc(stream);
        }
        // *c = '-'
        *c = fgetc(stream);
        if (*c == '-') {
            *c = fgetc(stream);
            if (*c == '>') // found closing characters
                return comment;
            else {
                callback("Error", "Comments cannot contain the substring \"--\"", path);
                return NULL;
            }
        } else {
            comment_end = mystrcat(comment_end, "-");
        }
        comment_end = appendchar(comment_end, *c);
        *c = fgetc(stream);
    }
}

int report_if_comment(char* c, FILE* stream, char* path, callback* callback) {
    char* comment = malloc(sizeof(char) * MAX_COMMENT_SIZE);
    comment = parse_comment(c, comment, stream, path, callback);

    if (comment != NULL)
        callback("Comment", comment, path);
    else 
        return EXIT_FAILURE;

    free(comment);
    return EXIT_SUCCESS;
}

int report_any_comments(char* c, FILE* stream, char* path, callback* callback) {
    // parse and print any comments
    while (*c == '<') {
        *c = fgetc(stream);

        if (*c != '!')
            break;

        int exit = report_if_comment(c, stream, path, callback);
        if (exit == EXIT_FAILURE) 
            return EXIT_FAILURE;

        while (isspace(*c = fgetc(stream)));
    }
    return EXIT_SUCCESS;
}

// parse element name
int parse_el_name(char* c, El* el, FILE* stream, callback* callback) {
    char* el_name_end = &(el->name[0]);
    // must start with a letter or underscore
    if ((*c != '_') && (*c <= 'A' && *c >= 'Z') && (*c <= 'a' && *c >= 'z')) {
        callback("Error", "Illegal first character in element name", el->path);
        // printf("\n  Character: %c", *c);
        return EXIT_FAILURE;
    }
    // cannot start with the letters xml (or XML, or Xml, etc)
    if (*c == 'X' || *c == 'x') {
        el_name_end = appendchar(el_name_end, *c);
        *c = fgetc(stream);
        if (*c == 'M' || *c == 'm') {
            el_name_end = appendchar(el_name_end, *c);
            *c = fgetc(stream);
            if (*c == 'L' || *c == 'l') {
                callback("Error", "Element names cannot start with the letters xml (or XML, or Xml, etc)", el->path);
                return EXIT_FAILURE;
            }
        }
    }
    while (*c != '>' && *c != '/' && !isspace(*c)) {
        // allowed to contain letters, digits, hyphens, underscores, and periods
        if ( ('A' <= *c && *c <= 'Z') || ('a' <= *c && *c <= 'z') || ('0' <= *c && *c <= '9') || *c == '-' || *c == '_' || *c == '.') {
            el_name_end = appendchar(el_name_end, *c); 
            *c = fgetc(stream);
        } else {
            callback("Error", "Illegal character in element name", el->path);
            // printf("\n  Character: %c", *c);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// parse element value
int parse_el_value(char* c, El* el, FILE* stream, callback* callback) {
    int exit;
    char* el_val_end = el->value;
    while (*c != '/') { // loop until closing tag is found
        // parse and print any comments
        while (*c == '<') {
            *c = fgetc(stream);
            if (*c == '/') // found a closing tag
                break;
            else if (*c == '!') {
                exit = report_if_comment(c, stream, el->path, callback);
                if (exit == EXIT_FAILURE) 
                    return EXIT_FAILURE;

                while (isspace(*c == fgetc(stream)));
            } else {
                // illegal unescaped entity
                callback("Error", "An element's value contained < unescaped", el->path);
                return EXIT_FAILURE;
            }
        }

        if (*c == '/') // found a closing tag
            break; // done parsing the element's value

        if (*c == EOF) {
            callback("Error", "An element is missing an end tag", el->path);
            return EXIT_FAILURE;
        } else if (*c == '>') { // illegal unescaped entity
            callback("Error", "An element's value contained > unescaped", el->path);
            return EXIT_FAILURE;
        } else if (*c == '&') {
            *c = parse_entity(c, stream, el->path, callback);
            if (*c == EXIT_FAILURE) 
                return EXIT_FAILURE;
        }
        el_val_end = appendchar(el_val_end, *c);
        *c = fgetc(stream);
    }
    return EXIT_SUCCESS;
}

// parse attribute name
int parse_attr_name(char* c, char* attr_name, FILE* stream, char* path, callback* callback) {
    char* attr_name_end = attr_name;
    // must start with a letter or underscore
    if ((*c != '_') && (*c <= 'A' || *c >= 'Z') && (*c <= 'a' || *c >= 'z')) {
        callback("Error", "Illegal first character in attribute name", path);
        // printf("\n  Character: %c", *c);
        return EXIT_FAILURE;
    }
    // cannot start with the letters xml (or XML, or Xml, etc)
    if (*c == 'X' || *c == 'x') {
        attr_name_end = appendchar(attr_name_end, *c);
        *c = fgetc(stream);
        if (*c == 'M' || *c == 'm') {
            attr_name_end = appendchar(attr_name_end, *c);
            *c = fgetc(stream);
            if (*c == 'L' || *c == 'l') {
                callback("Error", "Attribute names cannot start with the letters xml (or XML, or Xml, etc)", path);
                return EXIT_FAILURE;
            }
        }
    }
    while (*c != '=' && !isspace(*c)) {
        // allowed to contain letters, digits, hyphens, underscores, and periods
        if ( ('A' <= *c && *c <= 'Z') || ('a' <= *c && *c <= 'z') || ('0' <= *c && *c <= '9') || *c == '-' || *c == '_' || *c == '.') {
            attr_name_end = appendchar(attr_name_end, *c); 
            *c = fgetc(stream);
        } else {
            callback("Error", "Illegal character in attribute name", path);
            // printf("\n  Character: %c", *c);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// parse attribute value
int parse_attr_value(char* c, char* attr_val, FILE* stream, char* path, callback* callback) {
    char* attr_val_end = attr_val;
    if (*c != '=') {
        callback("Error", "An attribute is missing a value", path);
        return EXIT_FAILURE;
    }

    while (isspace(*c = fgetc(stream))); // whitespace is allowed between = and '' or ""
    if (*c != '\'' && *c != '\"') {
        callback("Error", "An attribute's value is not wrapped in quotes", path);
        return EXIT_FAILURE;
    }
          
    // attributes can be wrapped in either single or double quotes
    char exitc = *c;
    *c = fgetc(stream);
    if (*c == exitc) // empty string
        attr_val[0] = '\0';
    while (*c != exitc) {
        // attribute value may not contain <, >, or & unescaped
        if (*c == EOF) {
            callback("Error", "An attribute is missing a quote to wrap it's value", path);
            return EXIT_FAILURE;
        } else if (*c == '<' || *c == '>') {
            callback("Error", "An attribute value contained < or > unescaped", path);
            return EXIT_FAILURE;
        } else if (*c == '&') {
            *c = parse_entity(c, stream, path, callback);
            if (*c == EXIT_FAILURE) 
                return EXIT_FAILURE;
        }
        attr_val_end = appendchar(attr_val_end, *c);
        *c = fgetc(stream);
    }
    return EXIT_SUCCESS;
}

// unique attribute check
int unique_attr_check(El* root, int curr, callback* callback) {
    char* attr_name = root->attr_arr[curr].name;
    int i = 0;
    while (i < curr) {
        // compare names
        const char* prev_attr_name = root->attr_arr[i].name;
        if (strcmp(prev_attr_name, attr_name) == 0) {
            callback("Error", "Element contains duplicate attribute names", root->path);
            // printf("\n  Attribute name: %s", attr_name);
            return EXIT_FAILURE;
        }
        i++;
    }
    return EXIT_SUCCESS;
}

// parse attributes
int parse_attrs(char* c, El* root, FILE* stream, callback* callback) {
    int exit;
    Attr* attr;
    int curr = 0;
    while (isspace(*c)) {

        while (isspace(*c = fgetc(stream)));  

        // if no attribute name
        if (*c == '/' || *c == '>') 
            break;

        // ensure attr_arr can store the attr
        exit = alloc_attr_arr(root, curr, callback);
        if (exit == EXIT_FAILURE) 
            return EXIT_FAILURE;

        attr = &(root->attr_arr[curr]);

        attr->name = (char*)malloc(sizeof(char) * MAX_NAME_SIZE);
        if (attr->name == NULL) {
            callback("Error", "Unable to allocate space for an attribute's name", root->path);
            return EXIT_FAILURE;
        }

        attr->value = (char*)malloc(sizeof(char) * MAX_VAL_SIZE);
        if (attr->value == NULL) {
            callback("Error", "Unable to allocate space for an attribute's value", root->path);
            return EXIT_FAILURE;
        }

        attr->name[0] = '\0';
        attr->value[0] = '\0';

        exit = parse_attr_name(c, attr->name, stream, root->path, callback);
        if (exit == EXIT_FAILURE) 
            return EXIT_FAILURE;
        
        if (attr->name != NULL) {

            exit = unique_attr_check(root, curr, callback); // attr names must be unique
            if (exit == EXIT_FAILURE) 
                return EXIT_FAILURE;

            exit = parse_attr_value(c, attr->value, stream, root->path, callback);
            if (exit == EXIT_FAILURE) 
                return EXIT_FAILURE;

            callback("Attribute Name", attr->name, root->path);
            callback("Attribute Value", attr->value, root->path);
        }

        curr++;
        *c = fgetc(stream);
    } 
    return EXIT_SUCCESS;
}

/**
 * @brief malloc or realloc memory for a null terminated array of attributes so that the current attribute can be added
 * 
 * @param root the current element in the document
 * @param curr the index of the attribute to be assigned to
 * @param path the path to the current element
 */
int alloc_attr_arr(El* root, int curr, callback* callback) {
    const int BASE_AT_AMT = 2; // number of attributes to allocate memory for consecutively
    if (curr == 0) {
        root->attr_arr = (Attr*)malloc(sizeof(Attr) * BASE_AT_AMT);

        if (root->attr_arr == NULL) {
            callback("Error", "Unable to allocate space for an element's attributes", root->path);
            return EXIT_FAILURE;
        }

        // initialize allocated memory pointed to by array elements
        for (int i = 0; i < BASE_AT_AMT; i++) {  
            root->attr_arr[i].name = NULL;
            root->attr_arr[i].value = NULL;
        }
    } else if ((curr % (BASE_AT_AMT - 1)) == 0) { 
        // extends the allocation before reaching the last allocated attribute so it will act as a null terminator when freeing
        Attr* temp = (Attr*)realloc(root->attr_arr, sizeof(Attr) * ((curr + 1) + BASE_AT_AMT)); // curr + 1 because of 0 index

        if (temp == NULL) {
            callback("Error", "Unable to allocate enough space for an element's attributes", root->path);
            return EXIT_FAILURE;
        }

        root->attr_arr = temp;

        // initialize newly allocated memory pointed to by array elements
        for (int i = (curr + 1); i < ((curr + 1) + BASE_AT_AMT); i++) {
            root->attr_arr[i].name = NULL;
            root->attr_arr[i].value = NULL;
        }
    }
    return EXIT_SUCCESS;
}

/**
 * @brief malloc or realloc memory for a null terminated array of elements so that child elements can be added
 * 
 * @param root the parent element
 * @param curr the index of the child element to be assigned to in root->el_arr
 */
int alloc_el_arr(El* root, int curr, callback* callback) {
    const int BASE_EL_AMT = 10; // number of elements to allocate memory for consecutively
    if (curr == 0) {
        root->el_arr = (El*)malloc(sizeof(El) * BASE_EL_AMT);

        if (root->el_arr == NULL) {
            callback("Error", "Unable to allocate space for child elements", root->path);
            return EXIT_FAILURE;
        }

        // initialize allocated memory pointed to by array elements
        for (int i = 0; i < BASE_EL_AMT; i++) {
            root->el_arr[i].name = NULL;
            root->el_arr[i].value = NULL;
            root->el_arr[i].path = NULL;
            root->el_arr[i].el_arr = NULL;
            root->el_arr[i].attr_arr = NULL;
        }
    } else if((curr % (BASE_EL_AMT - 1) == 0)) { // reached the last index in the current array size
        // extend the array so there is always at least one null to end it
        El* temp = (El*)realloc(root->el_arr, sizeof(El) * ((curr + 1) + BASE_EL_AMT)); // curr + 1 because of 0 index

        if (temp == NULL) {
            callback("Error", "Unable to allocate enough space for child elements", root->path);
            return EXIT_FAILURE;
        }

        root->el_arr = temp;

        // initialize newly allocated memory pointed to by array elements
        for (int i = (curr + 1); i < ((curr + 1) + BASE_EL_AMT); i++) {
            root->el_arr[i].name = NULL;
            root->el_arr[i].value = NULL;
            root->el_arr[i].path = NULL;
            root->el_arr[i].el_arr = NULL;
            root->el_arr[i].attr_arr = NULL;
        }
    }
    return EXIT_SUCCESS;
}

// parse opening element tag
int parse_open_el(char* c, El* root, FILE* stream, callback* callback) {
    int exit;
    // parse element name
    parse_el_name(c, root, stream, callback);
    
    if (strcmp(root->path, "") == 0) 
        strcat(root->path, "/"); // don't add root name to path
    else if (strcmp(root->path, "/") == 0) {
        strcat(root->path, root->name);
    } else {
        strcat(root->path, "/");
        strcat(root->path, root->name);
    }
        
    // parse any attribute/value pairs and add them into the attribute array component of the current element
    exit = parse_attrs(c, root, stream, callback);
    if (exit == EXIT_FAILURE) 
        return EXIT_FAILURE;

    if (*c != '/' && *c != '>') {
        callback("Error", "Illegal character in opening element tag", root->path);
        // printf("\n  Expected: > or />, found: %c", *c);
        return EXIT_FAILURE;
    } else if (*c == '/') {
        *c = fgetc(stream);
        if (*c != '>') {
            callback("Error", "Illegal character in empty element", root->path);
            // printf("\n  Expected: >, found: %c", *c);
            return EXIT_FAILURE;
        }
    } // *c == '>'
    // end of opening element tag
    callback("Element Name", root->name, root->path);
    return EXIT_SUCCESS;
}

// parse an element's children or value
int parse_el_content(char* c, El* root, FILE* stream, callback* callback) {
    int exit;
    /* element content could be: 
    comments, element value or children, and/or closing tag */
    while (isspace(*c = fgetc(stream)));

    if (*c != '<') { 
        // element contains a value
        exit = parse_el_value(c, root, stream, callback);
        if (exit == EXIT_FAILURE) 
            return EXIT_FAILURE;

        *c = fgetc(stream); // get next char after '/'
        callback("Element Value", root->value, root->path);
    }
        
    // element may contain children but not a value
    int curr = 0;
    while (*c == '<') {
        exit = report_any_comments(c, stream, root->path, callback);
        if (exit == EXIT_FAILURE) 
            return EXIT_FAILURE;

        /* determine if there is a child or a closing tag */

        if (*c == '/') { // found closing tag
            *c = fgetc(stream); // no whitespace allowed between </ and the element name
            break;
        } 
        // recurse to parse child element
        El* child = parse_el(c, stream, root->path, callback);
        if (child == NULL) 
            return EXIT_FAILURE;

        // ensure there is memory allocated for el_arr to store the current child
        exit = alloc_el_arr(root, curr, callback);
        if (exit == EXIT_FAILURE) 
            return EXIT_FAILURE;

        // add child to el_arr
        root->el_arr[curr].name = child->name;
        root->el_arr[curr].value = child->value;
        root->el_arr[curr].el_arr = child->el_arr;
        root->el_arr[curr].attr_arr = child->attr_arr;
        root->el_arr[curr].path = child->path;
        free(child);
        
        curr++; // parse next child element
    }
    return EXIT_SUCCESS;
}

// parse closing element tag
int parse_close_el(char* c, El* root, FILE* stream, callback* callback) {
    int exit;
    // determine if the closing element name matches the opening element name
    El* el = malloc(sizeof(El));
    el->name = malloc(sizeof(char) * MAX_NAME_SIZE);
    el->name[0] = '\0';
    el->path = malloc(sizeof(char) * MAX_PATH_SIZE);
    el->path[0] = '\0';
    strcpy(el->path, root->path);

    exit = parse_el_name(c, el, stream, callback);
    if (exit == EXIT_FAILURE) 
        return EXIT_FAILURE;

    const char* open_name = root->name;
    const char* close_name = el->name;
    if (strcmp(open_name, close_name) == 0) {
        // make sure the closing tag has a > after the element name
        if (isspace(*c))
            while (isspace(*c = fgetc(stream))); // whitespace is allowed between an element name and the ending character sequence
        if (*c == '>') {
            while (isspace(*c = fgetc(stream)));
        } else {
            callback("Error", "Illegal character in closing element tag", root->path);
            //printf("\n  Expected: > after element name. Instead found: %c", *c);
            return EXIT_FAILURE;
        }
    } else {
        callback("Error", "Improperly nested elements", root->path);
        // printf("\n  Opening element name: %s", open_name);
        // printf("\n  Closing element name: %s", close_name);
        return EXIT_FAILURE;
    }
    free(el->name);
    free(el->path);
    free(el);
    return EXIT_SUCCESS;
}

/**
 * @brief parse a single element, recurse to parse child elements
 * 
 * @param stream the file stream used to access the XML containing file
 * @param path the path to the current element in the document (ex: /first/second)
 * @return a pointer to the element containing all of the XML document data, or NULL on error
 */
El* parse_el(char* c, FILE* stream, char* path, callback* callback) {
    int exit;
    El* root = (El*)malloc(sizeof(El));
    if (root == NULL) {
        callback("Error", "Unable to allocate space for an element", root->path);
        return EXIT_FAILURE;
    }

    root->name = (char*)malloc(sizeof(char) * MAX_NAME_SIZE);
    if (root->name == NULL) {
        callback("Error", "Unable to allocate space for an element's name", root->path);
        return EXIT_FAILURE;
    }

    root->name[0] = '\0';

    root->el_arr = NULL;
    root->attr_arr = NULL;

    root->value = malloc(sizeof(char) * MAX_VAL_SIZE);
    if (root->value == NULL) {
        callback("Error", "Unable to allocate space for an element's value", root->path);
        return EXIT_FAILURE;
    }

    root->value[0] = '\0';
    
    root->path = (char*)malloc(sizeof(char) * MAX_PATH_SIZE);
    if (root->path == NULL) {
        callback("Error", "Unable to allocate space for an element's path", root->path);
        return EXIT_FAILURE;
    }

    root->path[0] = '\0';
    strcpy(root->path, path);

    exit = parse_open_el(c, root, stream, callback);   
    if (exit == EXIT_FAILURE) 
        return NULL;

    exit = parse_el_content(c, root, stream, callback);
    if (exit == EXIT_FAILURE) 
        return NULL;
    
    exit = parse_close_el(c, root, stream, callback);
    if (exit == EXIT_FAILURE) 
        return NULL;
    
    return root;
}

/**
 * @brief parse a file containing XML
 * 
 * @param file_path the path to the file containing XML
 * @return a pointer to the element containing the Document Object Model that can be used to access data from the original file
 *      NULL on error
 */
El* parse_file(char* file_path, callback* callback) {
    int exit;
    El* root;
    FILE* stream = fopen(file_path, "r");
    char* c = malloc(sizeof(char)); // stores the current char from the file stream

    if (callback == NULL)
        callback = no_op_callback;
    
    if (stream == NULL) {
        callback("Error", "File not found", file_path);
        return NULL;        
    }

    *c = fgetc(stream);
    // cannot begin with whitespace
    if (*c != '<') {
        callback("Error", "Illegal first character in file. Expected a <", file_path);  
        return NULL;
    }

    *c = fgetc(stream);
    if (*c == '<') {
        callback("Error", "Duplicate < used in root element tag", file_path);
        return NULL; // very much an edge case, however it could not be easily caught in another method. Thought about report_any_comments, but thats a no go.
    }

    // XML declaration must be first if present
    if (*c == '?') {
        while (*c != '>') // ignore XML declaration tag
            *c = fgetc(stream); 
        while (isspace(*c = fgetc(stream)));
    } else if (*c == '!') {
        exit = report_if_comment(c, stream, "", callback); // report initial comment
        if (exit == EXIT_FAILURE) 
            return NULL;

        // *c == '>'
        while (isspace(*c = fgetc(stream)));
    }
    exit = report_any_comments(c, stream, "", callback); // report any other comments
    if (exit == EXIT_FAILURE) 
        return NULL;

    // *c != '<'

    // parse elements, recursing on children
    root = parse_el(c, stream, "", callback);

    fclose(stream);
    free(c);
    return root;
}