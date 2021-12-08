#include "imgStore.h"
#include <string.h> // for strncpy
#include <stdlib.h> // for calloc

/**
 * @file imgst_create.c
 * @brief imgStore library: do_create implementation.
 */

/**
 * Initialise an empty database
 *
 * @param filename File name
 * @param DBFILE Database file
 * @return The error associated to the error code in error.h
 */
int do_create(const char* filename, struct imgst_file* DBFILE)
{
    if (filename == NULL || DBFILE == NULL) return ERR_INVALID_ARGUMENT;

    // Sets the DB header name
    strncpy(DBFILE->header.imgst_name, CAT_TXT,  MAX_IMGST_NAME);
    DBFILE->header.imgst_name[MAX_IMGST_NAME] = '\0';

    DBFILE->header.num_files = 0;
    DBFILE->header.imgst_version = 0;

    DBFILE->metadata = calloc(DBFILE->header.max_files, sizeof(struct img_metadata));

    if(DBFILE->metadata == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    FILE *file;
    file = fopen(filename, "wb");
    DBFILE->file = file;
    if (file==NULL) {
        return ERR_IO;
    }

    size_t nb_written = 0;
    nb_written += fwrite(&DBFILE->header, sizeof(struct imgst_header),1, file);
    nb_written += fwrite(DBFILE->metadata, sizeof(struct img_metadata), DBFILE->header.max_files, file);

    printf("%zu item(s) written\n", nb_written);

    if(nb_written != DBFILE->header.max_files+1 ) {
        return ERR_IO;
    }

    return ERR_NONE;
}
