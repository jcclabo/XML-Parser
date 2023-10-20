#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helper.h"

/**
 * @brief concatenate strings without traversing the entire dest string
 * 
 * @return a pointer to the end of the string
 */
char* mystrcat(char* dest, char* src)
{
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return --dest;
}

/**
 * @brief add a char to the end of a string without traversing the entire dest string, 
 *    a new null terminator is also added after c
 * 
 * @return a pointer to the end of the string
 */
char* appendchar(char* dest, char c) {
    char str[2] = {c, '\0'};
    dest = mystrcat(dest, str);
    return dest;
}