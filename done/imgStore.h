/**
 * @file imgStore.h
 * @brief Main header file for imgStore core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The image imgStore starts with exactly one header structure
 * followed by exactly imgst_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * imgStore file and addressed by offsets in the metadata structure.
 *
 * @author Mia Primorac
 */

#ifndef IMGSTOREPRJ_IMGSTORE_H
#define IMGSTOREPRJ_IMGSTORE_H

#include "error.h" /* not needed in this very file,
                    * but we provide it here, as it is required by
                    * all the functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH

#define CAT_TXT "EPFL ImgStore binary"

/* constraints */
#define MAX_IMGST_NAME  31  // max. size of a ImgStore name
#define MAX_IMG_ID     127  // max. size of an image id
#define MAX_MAX_FILES 100000  // version from week 7

#define MAX_RES_TXT_LEN 9 //for thumbnail

/* For is_valid in imgst_metadata */
#define EMPTY 0
#define NON_EMPTY 1

// imgStore library internal codes for different image resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3

#define RES_SUFFIX_LEN 6
#define DOT_JPG_SUFFIX_LEN 4
#define RES_SUFFIX_ORIG "orig"
#define RES_SUFFIX_THUMB "thumb"
#define RES_SUFFIX_SMALL "small"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prints imgStore header informations.
 *
 * @param header The header to be displayed.
 */
struct imgst_header {
    char imgst_name[MAX_IMGST_NAME+1];
    uint32_t imgst_version;
    uint32_t num_files;
    uint32_t max_files;
    uint16_t res_resized[2*(NB_RES-1)];
    uint32_t unused_32;
    uint64_t unused_64;
};

struct img_metadata {
    char img_id[MAX_IMG_ID+1]; // plus 1 char
    unsigned char SHA[SHA256_DIGEST_LENGTH];
    uint32_t res_orig[2];
    uint32_t size[NB_RES];
    uint64_t offset[NB_RES];
    uint16_t is_valid;
    uint16_t unused_16;
};

struct imgst_file {
    FILE* file;
    struct imgst_header header;
    struct img_metadata* metadata; //[MAX_MAX_FILES];
};

/**
 * Prints header informtions
 *
 * @param header header to be printed
 */
void print_header(const struct imgst_header* header);

/**
 * @brief Prints image metadata informations.
 *
 * @param metadata The metadata of one image.
 */
void print_metadata (const struct img_metadata*  metadata);

/**
 * @brief Open imgStore file, read the header and all the metadata.
 *
 * @param imgst_filename Path to the imgStore file
 * @param open_mode Mode for fopen(), eg.: "rb", "rb+", etc.
 * @param imgst_file Structure for header, metadata and file pointer.
 */
int do_open (const char* imgst_filename, const char* open_mode, struct imgst_file* imgst_file);

/**
 * @brief Do some clean-up for imgStore file handling.
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */
void do_close (struct imgst_file* imgst_file);

/**
 * @brief List of possible output modes for do_list
 *
 * @param imgst_file Structure for header, metadata and file pointer to be freed/closed.
 */

enum do_list_mode {
    STDOUT,
    JSON
};

/**
 * @brief Displays (on stdout) imgStore metadata.
 *
 * @param imgst_file In memory structure with header and metadata.
 */

char* do_list(const struct imgst_file* file, enum do_list_mode mode);

/**
 * @brief Creates the imgStore called imgst_filename. Writes the header and the
 *        preallocated empty metadata array to imgStore file.
 *
 * @param imgst_filename Path to the imgStore file
 * @param imgst_file In memory structure with header and metadata.
 */

int do_create(const char* filename, struct imgst_file* DBFILE);

/**
 * @brief Deletes an image from a imgStore imgStore.
 *
 * Effectively, it only invalidates the is_valid field and updates the
 * metadata.  The raw data content is not erased, it stays where it
 * was (and  new content is always appended to the end; no garbage
 * collection).
 *
 * @param img_id The ID of the image to be deleted.
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */

int do_delete(const char* img_id, struct imgst_file* im_file);

/**
 * @brief Transforms resolution string to its int value.
 *
 * @param resolution The resolution string. Shall be "original",
 *        "orig", "thumbnail", "thumb" or "small".
 * @return The corresponding value or -1 if error.
 */
int resolution_atoi(const char* str);

/**
 * @brief Reads the content of an image from a imgStore.
 *
 * @param img_id The ID of the image to be read.
 * @param resolution The desired resolution for the image read.
 * @param image_buffer Location of the location of the image content
 * @param image_size Location of the image size variable
 * @param imgst_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_read(const char* img_id, int res_code, char** image_buffer, uint32_t* image_size, const struct imgst_file* im_file);

/**
 * @brief Insert image in the imgStore file
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @param imgst_file the struct where we are going to add the image
 * @return Some error code. 0 if no error.
 */
int do_insert(const char* img_buffer, size_t im_size, const char* img_id, struct imgst_file* im_file);

/**
 * @brief Removes the deleted images by moving the existing ones
 *
 * @param imgst_path The path to the imgStore file
 * @param imgst_tmp_bkp_path The path to the a (to be created) temporary imgStore backup file
 * @return Some error code. 0 if no error.
 */
int do_gbcollect(const char* orig_filename, const char* tmp_filename);

/**
 * @brief Generate a composite name of a readed file given an argument a resolution
 *
 * @param fname Where to write the generated char*
 * @param argument_name argument main name
 * @param res_code resolution code
 * @return An Error code according error.h
 */
int name_gen(char* fname, const char* argument_name, int res_code);

/**
 * Reads an image from disk and store it in img_buffer
 *
 * @param fname file name
 * @param mode reading mode for fopen
 * @param img_buffer buffer for the image
 * @param size_of_buffer size of buffer
 * @return An Error code according error.h
 */
int read_disk_image(char* fname, const char* mode, char** img_buffer, uint64_t* size_of_buffer);

/**
 * Writes an image on disk from a buffer
 *
 * @param fname file name
 * @param mode reading mode for fopen
 * @param image_size size of image
 * @param image_buffer buffer for the image
 * @return An Error code according error.h
 */
int write_disk_image(char* fname, const char* mode, uint32_t image_size, char** image_buffer);

#ifdef __cplusplus
}
#endif
#endif
