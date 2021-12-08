#pragma once

/**
 * @file image_content.h
 * @brief Header file for function that modify the image.
 *
 * Here are defined two function one that help to resize image (create thumbnail and small version)
 * The other one return back the resolution of an image given in arguments
 *
 */

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
int lazily_resize(int res, const struct imgst_file* im_file, size_t index);

/**
 * Given an image buffer, set the value of width and height given by pointer of the image
 *
 * @param height pointer where to store the height of image_buffer
 * @param width pointer where to store the width of image_buffer
 * @param image_buffer buffer that contain the image
 * @param image_size size of the buffer
 * @return error_code
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size);


#endif
