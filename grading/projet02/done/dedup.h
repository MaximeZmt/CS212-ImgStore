#pragma once

/**
 * @file dedup.h
 * @brief deduplicate image
 */

#include "imgStore.h"
#include <stdio.h>
#include <openssl/sha.h>

/**
 * Removes duplicata of name or content of the image at the given index in the given file
 *
 * @param im_file file of image duplicata
 * @param index index of image in the file
 * @return same error code as in error.c
 */
int do_name_and_content_dedup(struct imgst_file* im_file, uint32_t index);

/**
 * Compares 2 sha codes
 *
 * @param SHA1
 * @param SHA2
 * @return same error code as memcmp
 */
int shacmp(unsigned char SHA1[SHA256_DIGEST_LENGTH], unsigned char SHA2[SHA256_DIGEST_LENGTH]);
