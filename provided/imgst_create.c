/**
 * @file imgst_create.c
 * @brief imgStore library: do_create implementation.
 */

#include "imgStore.h"

#include <string.h> // for strncpy

/* **********************************************************************
 * TODO: ADD THE PROTOTYPE OF do_create HERE.
 * **********************************************************************
 */
{
    if (filename == NULL) return ERR_INVALID_ARGUMENT;

    // Sets the DB header name
    strncpy(DBFILE.header.imgst_name, CAT_TXT,  MAX_IMGST_NAME);
    DBFILE.header.imgst_name[MAX_IMGST_NAME] = '\0';

    /* **********************************************************************
     * TODO: WRITE YOUR CODE HERE
     * **********************************************************************
     */

    return ERR_NONE;
}
