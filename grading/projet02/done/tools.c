/* ** NOTE: undocumented in Doxygen
 * @file tools.c
 * @brief implementation of several tool functions for imgStore
 *
 * @author Mia Primorac
 */

#include "imgStore.h"

#include <stdint.h> // for uint8_t
#include <stdio.h> // for sprintf
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <stdlib.h> // for calloc

/********************************************************************//**
 * Human-readable SHA
 */
static void
sha_to_string (const unsigned char* SHA,
               char* sha_string)
{
    if (SHA == NULL) {
        return;
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i*2], "%02x", SHA[i]);
    }

    sha_string[2*SHA256_DIGEST_LENGTH] = '\0';
}

/********************************************************************//**
 * imgStore header display.
 */
void
print_header(const struct imgst_header* header)
{
    printf("*****************************************\n");
    printf("**********IMGSTORE HEADER START**********\n");
    printf("TYPE: %31s\n", header->imgst_name);
    printf("VERSION: %" PRIu32 "\n", header->imgst_version);
    printf("IMAGE COUNT: %" PRIu32 "\t\tMAX IMAGES: %" PRIu32 "\n", header->num_files, header->max_files);
    printf("THUMBNAIL: %" PRIu16 " x %" PRIu16 "\tSMALL: %" PRIu16 " x %" PRIu16 "\n", header->res_resized[2*RES_THUMB], header->res_resized[2*RES_THUMB+1], header->res_resized[2*RES_SMALL], header->res_resized[2*RES_SMALL+1]);
    printf("***********IMGSTORE HEADER END***********\n");
    printf("*****************************************\n");
}

/********************************************************************//**
 * Metadata display.
 */
void
print_metadata (const struct img_metadata* metadata)
{
    char sha_printable[2*SHA256_DIGEST_LENGTH+1];
    sha_to_string(metadata->SHA, sha_printable);

    printf("IMAGE ID: %s\n", metadata->img_id);
    printf("SHA: %s\n", sha_printable);
    printf("VALID: %" PRIu16 "\n", metadata->is_valid);
    printf("UNUSED: %" PRIu16 "\n", metadata->unused_16);
    printf("OFFSET ORIG. : %" PRIu64 "\t\tSIZE ORIG. :%" PRIu32 "\n", metadata->offset[RES_ORIG], metadata->size[RES_ORIG]);
    printf("OFFSET THUMB. : %" PRIu64 "\t\tSIZE THUMB. :%" PRIu32 "\n", metadata->offset[RES_THUMB], metadata->size[RES_THUMB]);
    printf("OFFSET SMALL : %" PRIu64 "\t\tSIZE SMALL :%" PRIu32 "\n", metadata->offset[RES_SMALL], metadata->size[RES_SMALL]);
    printf("ORIGINAL: %" PRIu32 " x %" PRIu32 "\n", metadata->res_orig[0], metadata->res_orig[1]);
    printf("*****************************************\n");
}

/**
 * Open a file and load it into a struct
 * @param imgst_filename
 * @param open_mode
 * @param imgst_file
 * @return the error associated to the error code in error.h
 */
int
do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file)
{
    if (imgst_filename==NULL || open_mode == NULL || imgst_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    FILE* file = fopen(imgst_filename, open_mode);
    if (file==NULL) {
        return ERR_IO;
    }

    size_t nb_read = 0;
    nb_read += fread(&imgst_file->header, sizeof(struct imgst_header), 1, file);
    if(imgst_file->header.max_files > MAX_MAX_FILES) { //do not read a valid file (not even from software)
        do_close(imgst_file);
        return ERR_IO;
    }

    imgst_file->metadata = calloc(imgst_file->header.max_files, sizeof(struct img_metadata));
    if(imgst_file->metadata == NULL) {
        do_close(imgst_file);
        return ERR_OUT_OF_MEMORY;
    }

    nb_read += fread(imgst_file->metadata, sizeof(struct img_metadata), imgst_file->header.max_files, file);
    imgst_file->file = file;
    //num_files+1 is the number of metadatas + the header
    if (imgst_file->header.max_files+1!=nb_read) {
        do_close(imgst_file);
        return ERR_IO;
    }

    return 0;
}

/**
 * Close an opened file and free the memory
 * @param imgst_file
 */
void
do_close (struct imgst_file* imgst_file)
{
    if (imgst_file != NULL && imgst_file->file!=NULL) {
        if (imgst_file->metadata != NULL) {
            free(imgst_file->metadata);
            imgst_file->metadata = NULL;
        }
        fclose(imgst_file->file);
        imgst_file->file = NULL;
    }
}

int resolution_atoi(char* str)
{
    if (!strcmp(str, "thumb") || !strcmp(str, "thumbnail")) {
        return RES_THUMB;
    } else if(!strcmp(str, "small")) {
        return RES_SMALL;
    } else if (!strcmp(str, "orig") || !strcmp(str, "original")) {
        return RES_ORIG;
    } else {
        return -1;
    }
}
