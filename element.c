#include "element.h"

/**
 * @brief get the first element with the path provided
 * 
 * @param root the element to start the traversal from
 * @param path the path from the root param to the element to get
 * @return a pointer to the first El at the path provided, or NULL if none are found
 */
El* get_first_el(El* root, char* path) {
    El* prev = root;
    El* el;
    char el_name[MAX_NAME_SIZE];
    el_name[0] = '\0';
    char* el_name_end = el_name; // pointer for holding the end of el_name

    int i = 1; // ignore first '/'
    if (path[i] == '\0')
        return root;

    // traverse the path
    while (path[i] != '\0') {
        el_name_end = appendchar(el_name_end, path[i]);
        if (path[i] == '/') {
            int j = 0;
            el = &(prev->el_arr[j]);
            while (el->name != NULL && (strcmp(el->name, el_name) != 0)) {
                j++;
                el = &(prev->el_arr[j]);
            }
            if (el->name == NULL) {
                return NULL; // An element specified in the path was not found
            } 
            // found next element in the path
            prev = el;
            el_name[0] = '\0';
            el_name_end = el_name;
        }
        i++;
    } 

    if (el_name[0] == '\0') // path contained a trailing '/'
        return el;

    // find the last element in the path
    i = 0;
    el = &(prev->el_arr[i]);
    while (el->name != NULL && (strcmp(el->name, el_name) != 0)) {
        i++;
        el = &(prev->el_arr[i]);
    }

    if (el->name == NULL) {
        return NULL; // The last element specified in the path was not found
    }

    return el;
}

/**
 * @brief get the next sibling element
 * 
 * @param el the element to start traversal from
 * @return a pointer to the next element that is a sibling of el, or NULL if none are found
 */
El* get_next_el(El* el) {
    if (el[1].name != NULL)
        return &(el[1]); // could be reduced to ++el
    else 
        return NULL; // The next element is a null terminator
}

/**
 * @brief get the next sibling element by name
 * 
 * @param el the element to start traversal from
 * @param el_name the name of the next sibling element to return
 * @return a pointer to the next sibling with the name specified in el_name, or NULL if none are found
 */
El* get_next_el_by_name(El* el, char* el_name) {
    El* next = get_next_el(el);
    if (next == NULL)
        return NULL;   

    while (strcmp(next->name, el_name) != 0) {
        next = get_next_el(el);
        if (next == NULL)
            return NULL; // An element with the specified name was not found
    }

    return next;
}

char* get_el_value(El* el) {
    return el->value;
}

Attr* get_next_attr(Attr* attr) {
    if (attr[1].name != NULL)
        return &(attr[1]); // could be reduced to ++attr
    else 
        return NULL; // The next element is a null terminator
}

/**
 * @brief get an attribute value by name
 * 
 * @param el the element to start traversal from
 * @param attr_name the name of the attribute to return a value for
 * @return the value of the attribute with the name specified in attr_name, or NULL if none are found
 */
char* get_attr_value(El* el, char* attr_name) {
    Attr* attr = &(el->attr_arr[0]);
    while (strcmp(attr->name, attr_name) != 0) {
        attr = get_next_attr(attr);
        if (attr == NULL)
            return NULL; // An attribute with the specified name was not found
    }
    return attr->value;
} 

// recursively free all memory allocated for children, grandchildren, etc.
void free_child(El* root) {
    // recurse to the furthest element array and free each element before returning
    El* child = &(root->el_arr[0]);
    if (child == NULL)
        return;
    while (child != NULL) { // uses name as the null terminator since all elements must have a name
        free_child(child);

        free(child->name);
        free(child->value);
        free(child->path); 
        free(child->el_arr);

        // free attributes iteratively
        Attr* attr = &(child->attr_arr[0]);
        if (attr != NULL) {
            while (attr != NULL) {
                free(attr->name);
                free(attr->value);
                attr = get_next_attr(attr);
            }
        }

        child = get_next_el(child);
    }
}

// recursively free an element
void free_el(El* root) {
    if (root == NULL) // cannot free memory of a null pointer
        return;
        
    // free all subtrees 
    free_child(root);

    // free the true root last
    free(root->name);
    free(root->value);
    free(root->path);
    free(root->el_arr);
    free(root->attr_arr);
    free(root);
}