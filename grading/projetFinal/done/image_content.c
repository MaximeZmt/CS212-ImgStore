/**
 * @file imgst_content.c
 * @brief imgStore library: Contains image modification function implementation.
 */

#include "image_content.h"
#include "imgStore.h"
#include <stdio.h>
#include <vips/vips.h>
#include <stdlib.h>

/**
 * Computes the ratio to shrink the image
 *
 * @param image
 * @param max_width
 * @param max_height
 * @return ratio
 */
double shrink_value(const VipsImage *image, int max_width, int max_height)
{
    const double h_shrink = (double) max_width / (double) image->Xsize;
    const double v_shrink = (double) max_height / (double) image->Ysize;
    return h_shrink > v_shrink ? v_shrink : h_shrink;
}

/**
 * Take an image from input_buffer, resize it, and can be used on output_buffer
 *
 * @param output_buffer new image
 * @param input_buffer input image
 * @param im_file imgst_file structure
 * @param im_size_orig original size
 * @param res new resolution to be computed
 * @param im_size_new pointer on the new size of the new image
 * @return error code according error.h
 */
int inp_to_out_buffer(void ** output_buffer, void ** input_buffer, const struct imgst_file *im_file, uint64_t im_size_orig, int res, size_t* im_size_new)
{
    VipsImage **im_resized_array = NULL;
    VipsImage *array;
    array = vips_image_new();
    int err = 0;

    // Create an empty array for the future resized image
    im_resized_array = (VipsImage **) vips_object_local_array(VIPS_OBJECT(array), 1);

    VipsImage *im_input;
    // Convert the buffer to a Vips Image and resize it
    err = vips_jpegload_buffer(*input_buffer, im_size_orig, &im_input, NULL); // VIPS ERROR: 0 if ok, -1 if error
    if(err != 0) {
        return ERR_IMGLIB;
    }

    const double ratio = shrink_value(im_input, im_file->header.res_resized[2 * res],
                                      im_file->header.res_resized[2 * res + 1]);
    vips_resize(im_input, im_resized_array, ratio, NULL);

    // Write the resized vips image to an output buffer
    err = vips_jpegsave_buffer(im_resized_array[0], output_buffer, im_size_new, NULL);
    g_object_unref(im_input);
    g_object_unref(array);
    if(err != 0) {
        return ERR_IMGLIB;
    }

    return ERR_NONE;
}

int lazily_resize(int res, const struct imgst_file *im_file, size_t index)
{
    if (res == RES_ORIG) {
        return ERR_NONE;
    } else if (im_file == NULL || res < 0 || NB_RES <= res || index >= im_file->header.max_files) {
        return ERR_INVALID_ARGUMENT;
    }
    if (im_file->metadata[index].size[res] != 0) {
        return ERR_NONE;
    } else {

        // Load the metadata where we are going to work
        struct img_metadata meta = im_file->metadata[index];
        uint64_t im_size_orig = meta.size[RES_ORIG];

        // Create an input buffer to load the image
        void *input_buffer = malloc(im_size_orig);
        void *output_buffer = NULL;

        // Loading the image from binary
        fseek(im_file->file, (int64_t)meta.offset[RES_ORIG], SEEK_SET);
        size_t nb_read = fread(input_buffer, im_size_orig, 1, im_file->file);
        if(nb_read != 1) {
            free(input_buffer);
            return  ERR_IO;
        }

        size_t im_size_new = 0;

        int err = inp_to_out_buffer(&output_buffer, &input_buffer, im_file, im_size_orig, res, &im_size_new);
        if(err != ERR_NONE) {
            free(input_buffer);
            return err;
        }

        fseek(im_file->file, 0, SEEK_END);

        //reading offset from ftell, which is first signed, if <0 there is an error, otherwise cast to unsigned
        int64_t signed_new_offset = ftell(im_file->file);
        if(signed_new_offset < 0) {
            free(input_buffer);
            free(output_buffer);
            return ERR_IO;
        }
        uint64_t new_offset = (uint64_t)signed_new_offset;

        size_t nb_written = 0;
        nb_written = fwrite(output_buffer, im_size_new, 1, im_file->file);

        free(input_buffer);
        free(output_buffer);
        output_buffer = NULL;
        input_buffer = NULL;
        if (nb_written != 1) {
            return ERR_IO;
        }

        meta.offset[res] = new_offset;
        meta.size[res] = (uint32_t)im_size_new;

        im_file->metadata[index] = meta;

        fseek(im_file->file, (uint32_t)(sizeof(struct imgst_header) + sizeof(struct img_metadata) * (index)), SEEK_SET);
        fwrite(&im_file->metadata[index], sizeof(struct img_metadata), 1, im_file->file);

        return ERR_NONE;
    }
}

int get_resolution(uint32_t *height, uint32_t *width, const char *image_buffer, size_t image_size)
{
    // if vips err: err_imglib otherwise err_none
    VipsImage *im_input;

    // Convert the buffer to a Vips Image and resize it
    int isOk = vips_jpegload_buffer((void *) image_buffer, image_size, &im_input, NULL); //CAST on purpose
    if (isOk != 0) {
        return ERR_IMGLIB;
    }

    g_object_unref(im_input);

    // Here we cast because there is no reason that the height and width are negative
    *width = (uint32_t) vips_image_get_width(im_input);
    *height = (uint32_t) vips_image_get_height(im_input);
    return ERR_NONE;
}
