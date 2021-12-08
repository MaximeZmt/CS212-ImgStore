#include "imgStore.h"
#include "image_content.h"

/**
 * Checks in file for metadata with given id
 *
 * @param img_id metadata id
 * @param im_file file to be searched
 * @param meta output pointer for the found metadata
 * @param index output index of metadata in file
 * @return same error code as in error.c
 */
int find_metadata_with_id(char *img_id, struct imgst_file *im_file, struct img_metadata *meta, size_t *index)
{
    for (size_t i = 0; i < im_file->header.num_files; i++) {
        if (!strcmp(im_file->metadata[i].img_id, img_id)) {
            *meta = im_file->metadata[i];
            *index = i;
            return ERR_NONE;
        }
    }
    return ERR_FILE_NOT_FOUND;
}

int do_read(char *img_id, int res_code, char **image_buffer, uint32_t *image_size, struct imgst_file *im_file)
{

    // finding correct metadata
    struct img_metadata meta;
    size_t index;
    int err = find_metadata_with_id(img_id, im_file, &meta, &index);
    if (err != ERR_NONE) {
        return err;
    }

    //check if image already exists
    if (meta.offset[res_code] == 0 || meta.size[res_code] == 0) {
        err = lazily_resize(res_code, im_file, index);
        if (err != ERR_NONE) {
            return err;
        }
        meta = im_file->metadata[index]; //if lazily_resize, meta has been changed, so take new one
    }


    *image_size = meta.size[res_code];

    *image_buffer = calloc(1, (size_t) * image_size);
    if (*image_buffer == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    err = fseek(im_file->file, (int64_t)meta.offset[res_code], SEEK_SET);
    if (err == -1) {
        return ERR_IO;
    }

    size_t nb_read = fread(*image_buffer, *image_size, 1, im_file->file);
    if (nb_read != 1) {
        return ERR_IO;
    }

    return ERR_NONE;
}
