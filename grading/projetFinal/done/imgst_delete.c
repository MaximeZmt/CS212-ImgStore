/**
* @file imgst_delete.c
* @brief imgStore library: do_delete implementation.
*/

#include "imgStore.h"

int do_delete(const char* img_id, struct imgst_file* im_file)
{
    if ( img_id==NULL || im_file==NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    int found = 0;
    uint32_t counter = 0;

    if(im_file->header.num_files == 0) {
        return ERR_FILE_NOT_FOUND;
    }

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
        uint64_t offset_fseek = sizeof(struct imgst_header)+sizeof(struct img_metadata)*(counter-1);
        fseek(im_file->file, (int64_t)offset_fseek, SEEK_SET);
        size_t nb_written = fwrite(&im_file->metadata[counter-1], sizeof (struct img_metadata), 1, im_file->file);
        if(nb_written != 1) {
            im_file->header.imgst_version -= 1;
            im_file->header.num_files += 1;
            return ERR_IO;
        }

        fseek(im_file->file, 0, SEEK_SET);
        nb_written = fwrite(&im_file->header, sizeof(struct imgst_header), 1, im_file->file);
        if(nb_written != 1) {
            im_file->header.imgst_version -= 1;
            im_file->header.num_files += 1;
            return ERR_IO;
        }

        return ERR_NONE;
    }

}
