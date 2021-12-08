#include "imgStore.h"

/**
 * List in the shell the header and all the metadata of an imgst_file
 * @param file
 */
void
do_list(struct imgst_file* file)
{
    if(file == NULL) {
        fprintf(stderr, "Cannot call do list, NullPointerException!\n");
        return;
    }
    print_header(&file->header);
    if (file->header.num_files==0) {
        printf("<< empty imgStore >>\n");
    } else {
        uint32_t counter = 0;
        uint32_t iter = 0;
        // Go across all the metadata
        while(counter<file->header.num_files && iter<file->header.max_files) {
            // Check if this is a valid one (otherwise ignore it)
            if(file->metadata[iter].is_valid == 1) {
                print_metadata(&file->metadata[iter]);
                counter += 1;
            }
            iter += 1;
        }
    }
}

