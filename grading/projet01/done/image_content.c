#include "image_content.h"
#include "imgStore.h"
#include <stdio.h>
#include <vips/vips.h>
#include <stdlib.h>

double shrink_value(const VipsImage *image, int max_thumbnail_width, int max_thumbnail_height)
{
    const double h_shrink = (double) max_thumbnail_width  / (double) image->Xsize ;
    const double v_shrink = (double) max_thumbnail_height / (double) image->Ysize ;
    return h_shrink > v_shrink ? v_shrink : h_shrink ;
}

int lazily_resize(int res, struct imgst_file* im_file, size_t index)
{
    if (res==RES_ORIG) {
        return ERR_NONE;
    } else if (im_file==NULL || (res<0 || NB_RES<res)) {
        return ERR_INVALID_ARGUMENT;
    }
    if (im_file->metadata[index].size[res]!=0) {
        return ERR_NONE;
    } else {

        // Load the metadata where we are going to work
        struct img_metadata meta = im_file->metadata[index];
        uint64_t im_size_orig = meta.size[RES_ORIG];

        // Create an input buffer to load the image
        void* input_buffer = calloc(1, im_size_orig);

        // Loading the image from binary
        fseek(im_file->file, meta.offset[RES_ORIG], SEEK_SET);
        fread(input_buffer, im_size_orig, 1, im_file->file);

        VipsImage** im_resized_array = NULL;
        VipsImage* array ;
        array = vips_image_new();

        // Create an empty array for the future resized image
        VipsImage* im_input;
        im_resized_array = (VipsImage **) vips_object_local_array(VIPS_OBJECT(array), 1);

        // Convert the buffer to a Vips Image and resize it
        vips_jpegload_buffer(input_buffer, im_size_orig, &im_input, NULL);
        const double ratio = shrink_value(im_input, im_file->header.res_resized[2*res], im_file->header.res_resized[2*res+1]);
        vips_resize(im_input, im_resized_array, ratio, NULL);

        void* output_buffer = NULL;
        size_t im_size_new = 0;

        // Write the resized vips image to an output buffer
        vips_jpegsave_buffer(im_resized_array[0], &output_buffer, &im_size_new, NULL);
        fseek(im_file->file, 0, SEEK_END);

        uint64_t new_offset = ftell(im_file->file);
        size_t nb_written = 0;
        nb_written = fwrite(output_buffer, im_size_new, 1, im_file->file);

        free(input_buffer);
        input_buffer = NULL;
        if (nb_written!=1) {
            return ERR_IO;
        }

        meta.offset[res] = new_offset;
        meta.size[res] = im_size_new;

        im_file->metadata[index] = meta;

        fseek(im_file->file, sizeof(struct imgst_header)+sizeof(struct img_metadata)*(index), SEEK_SET);
        fwrite(&im_file->metadata[index], sizeof (struct img_metadata), 1, im_file->file);

        return ERR_NONE;
    }
}
