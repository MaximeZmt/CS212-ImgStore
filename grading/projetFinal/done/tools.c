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

    return ERR_NONE;
}

/**
 * Close an opened file and free the memory
 * @param imgst_file
 */
void
do_close (struct imgst_file* imgst_file)
{
    if (imgst_file != NULL) {
        if(imgst_file->file != NULL) {
            fclose(imgst_file->file);
        }
        if (imgst_file->metadata != NULL) {
            free(imgst_file->metadata);
            imgst_file->metadata = NULL;
        }
        imgst_file->file = NULL;
    }

}

int resolution_atoi(const char* str)
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


int name_gen(char* fname, const char* argument_name, int res_code)
{
    memset(fname, 0, MAX_IMG_ID+RES_SUFFIX_LEN+DOT_JPG_SUFFIX_LEN+1);
    fname[MAX_IMG_ID+RES_SUFFIX_LEN+DOT_JPG_SUFFIX_LEN] = '\0';
    strcat(fname, argument_name);
    strcat(fname, "_");
    if(res_code == 0) {
        strcat(fname, RES_SUFFIX_THUMB);
    } else if (res_code == 1) {
        strcat(fname, RES_SUFFIX_SMALL);
    } else {
        strcat(fname, RES_SUFFIX_ORIG);
    }
    strcat(fname, ".jpg");
    return ERR_NONE;
}

int read_disk_image(char* fname, const char* mode, char** img_buffer, uint64_t* size_of_buffer)
{
    FILE* file = fopen(fname, mode);
    if (file == NULL) {
        return ERR_IO;
    }

    fseek(file, 0, SEEK_END);

    int64_t signed_size_of_buffer = ftell(file);
    if (signed_size_of_buffer < 0) {
        fclose(file);
        return ERR_IO;
    }

    *size_of_buffer = (uint64_t)signed_size_of_buffer;
    fseek(file, 0, SEEK_SET);

    *img_buffer = malloc(*size_of_buffer);
    if (img_buffer == NULL) {
        fclose(file);
        return ERR_IO;
    }

    size_t nb_read = fread(*img_buffer, *size_of_buffer, 1, file);
    fclose(file);
    if (nb_read != 1) {
        return ERR_IO;
    }
    return ERR_NONE;
}

int write_disk_image(char* fname, const char* mode, uint32_t image_size, char** image_buffer)
{
    FILE* sortie = fopen(fname, mode);
    if(sortie == NULL) {
        return ERR_IO;
    }
    size_t nb_ok = fwrite(*image_buffer, image_size, 1, sortie);
    fclose(sortie);
    if (nb_ok != 1) {
        return ERR_IO;
    }
    return ERR_NONE;
}

