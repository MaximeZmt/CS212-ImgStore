#include "imgStore.h"

/**
 * @file imgst_delete.c
 * @brief imgStore library: do_delete implementation.
 */

/**
 * Deletes an image contained in an imgst_file
 * @param img_id
 * @param im_file
 * @return the error associated to the error code in error.h
 */
int do_delete(const char* img_id, struct imgst_file* im_file)
{
    if ( img_id==NULL || im_file==NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    int found = 0;
    uint32_t counter = 0;

    // Go across all pictures in metadata and find if there is any with the id img_id
    while(counter<im_file->header.max_files && found == 0) {
        if (strcmp(img_id, im_file->metadata[counter].img_id)==0 && (im_file->metadata[counter].is_valid == 1) ) {
            im_file->metadata[counter].is_valid = 0;
            found = 1;
        }
        counter += 1;
    }

    if (found != 1) {
        // If not found
        return ERR_FILE_NOT_FOUND;
    } else {
        // If found
        im_file->header.imgst_version += 1;
        im_file->header.num_files -= 1;
        int64_t offset_fseek = sizeof(struct imgst_header)+sizeof(struct img_metadata)*(counter-1);
        fseek(im_file->file, offset_fseek, SEEK_SET);
        fwrite(&im_file->metadata[counter-1], sizeof (struct img_metadata), 1, im_file->file);
        fseek(im_file->file, 0, SEEK_SET);
        fwrite(&im_file->header, sizeof(struct imgst_header), 1, im_file->file);
        return ERR_NONE;
    }

}
