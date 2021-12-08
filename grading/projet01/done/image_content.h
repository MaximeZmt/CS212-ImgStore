#ifndef DONE_IMAGE_CONTENT_H
#define DONE_IMAGE_CONTENT_H

#include <stdio.h>
#include "imgStore.h"
#include <vips/vips.h>
#include <stdlib.h>

/**
 * Resizes the given image to the given resolution and store it in the given file
 *
 * @param res The given resolution
 * @param im_file The given imgst_file
 * @param index The index of the image in the file
 * @return The error associated to the error code in error.h
 */
int lazily_resize(int res, struct imgst_file* im_file, size_t index);

/**
 * Computes the shrinking factor (keeping aspect ratio). Same function as provided in week02
 *
 * @param image The image to be resized.
 * @param max_thumbnail_width The maximum width allowed for thumbnail creation.
 * @param max_thumbnail_height The maximum height allowed for thumbnail creation.
 * @return The shrinking factor
 */
double shrink_value(const VipsImage *image, int max_thumbnail_width, int max_thumbnail_height);

#endif
