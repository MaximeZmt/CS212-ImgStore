/**
 * @file dedup.c
 * @brief imgStore library: do_name_and_content_dedup implementation and sha comparator.
 */

#include "dedup.h"
#include "imgStore.h"
#include <stdio.h>
#include <openssl/sha.h>

/**
 * Compares 2 sha codes
 *
 * @param SHA1
 * @param SHA2
 * @return same error code as memcmp
 */
int shacmp(unsigned char SHA1[SHA256_DIGEST_LENGTH], unsigned char SHA2[SHA256_DIGEST_LENGTH])
{
    return memcmp(SHA1, SHA2, SHA256_DIGEST_LENGTH);
}


int do_name_and_content_dedup(struct imgst_file *im_file, uint32_t index)
{

    if (im_file == NULL || index >= im_file->header.max_files) {
        return ERR_INVALID_ARGUMENT;
    }

    // for all valid images in the imgst_file, if i != index and if names are identical return ERR_DUPLICATE_ID
    size_t i = 0;
    int has_content_dup = 0;
    while (i < im_file->header.max_files) {
        if (im_file->metadata[i].is_valid == 1) {
            if (i != index) {

                // check name duplication
                if (!strcmp(im_file->metadata[i].img_id, im_file->metadata[index].img_id)) {
                    return ERR_DUPLICATE_ID;
                }

                // check sha duplication
                if (!shacmp(im_file->metadata[i].SHA, im_file->metadata[index].SHA) && has_content_dup == 0) {
                    has_content_dup = 1;

                    im_file->metadata[index].offset[RES_SMALL] = im_file->metadata[i].offset[RES_SMALL];
                    im_file->metadata[index].offset[RES_THUMB] = im_file->metadata[i].offset[RES_THUMB];
                    im_file->metadata[index].offset[RES_ORIG] = im_file->metadata[i].offset[RES_ORIG];

                    im_file->metadata[index].size[RES_THUMB] = im_file->metadata[i].size[RES_THUMB];
                    im_file->metadata[index].size[RES_SMALL] = im_file->metadata[i].size[RES_SMALL];

                }
            }
        }
        i += 1;
    }

    // in case of no content duplication
    if (has_content_dup == 0) {
        im_file->metadata[index].offset[RES_ORIG] = 0;
    }

    return ERR_NONE;
}