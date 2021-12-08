/**
 * @file imgst_insert.c
 * @brief imgStore library: do_insert implementation.
 */

#include "imgStore.h"
#include "image_content.h"
#include "dedup.h"
#include <stdio.h>
#include <string.h> // for strlen()
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH and SHA256()

int do_insert(const char *img_buffer, size_t im_size, const char *img_id, struct imgst_file *im_file)
{
    if (im_file == NULL || img_buffer == NULL || img_id == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    if (im_file->header.num_files >= im_file->header.max_files) {
        return ERR_FULL_IMGSTORE;
    }

    size_t nb_written = 0;
    uint32_t index = 0;
    int has_found = 0;
    uint32_t counter = 0;
    while (counter < im_file->header.max_files) {
        if (im_file->metadata[index].is_valid == 0) {
            has_found = 1;
        } else {
            if (!strcmp(im_file->metadata[index].img_id, img_id)) {
                return ERR_DUPLICATE_ID;
            }
            if (has_found == 0) {
                index += 1;
            }
        }
        counter += 1;
    }

    memset(&im_file->metadata[index], 0, sizeof(struct img_metadata));

    //Here we cast because we know that a char is > 0 so we can convert it to unsigned
    SHA256((unsigned char*) img_buffer, im_size, im_file->metadata[index].SHA);

    strncpy(im_file->metadata[index].img_id, img_id, MAX_IMG_ID);
    im_file->metadata[index].size[RES_ORIG] = (uint32_t)im_size;

    int err_dedup = do_name_and_content_dedup(im_file, index);
    if (err_dedup != ERR_NONE) {
        return err_dedup;
    }
    if (im_file->metadata[index].offset[RES_ORIG] == 0) {
        fseek(im_file->file, 0, SEEK_END);

        //safe cast from signed to unsigned
        int64_t signed_new_offset = ftell((im_file->file));
        if (signed_new_offset < 0) {
            return ERR_IO;
        }
        im_file->metadata[index].offset[RES_ORIG] = (uint64_t)signed_new_offset;

        nb_written = fwrite(img_buffer, im_size, 1, im_file->file);
        if (nb_written != 1) {
            return ERR_IO;
        }
    }

    uint32_t height = 0;
    uint32_t width = 0;
    int err_reso = get_resolution(&height, &width, img_buffer, im_size);
    if (err_reso != ERR_NONE) {
        return err_reso;
    }

    im_file->metadata[index].res_orig[0] = width;
    im_file->metadata[index].res_orig[1] = height;

    im_file->metadata[index].is_valid = 1;
    im_file->header.num_files = im_file->header.num_files + 1;
    im_file->header.imgst_version = im_file->header.imgst_version + 1;

    fseek(im_file->file, 0, SEEK_SET);
    nb_written = fwrite(&im_file->header, sizeof(struct imgst_header), 1, im_file->file);
    if (nb_written != 1) {
        return ERR_IO;
    }
    /*Here we cast from unsigned to signed, should not cause error because index>0 and img_header and img_metadata are a known size
    that is small enough to not create problem by signing it */
    fseek(im_file->file, (int32_t)(sizeof(struct imgst_header) + sizeof(struct img_metadata) * (index)), SEEK_SET);
    nb_written = fwrite(&im_file->metadata[index], sizeof(struct img_metadata), 1, im_file->file);
    if (nb_written != 1) {
        return ERR_IO;
    }

    return ERR_NONE;
}