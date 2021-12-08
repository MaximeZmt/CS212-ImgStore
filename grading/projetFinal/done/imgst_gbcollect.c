/**
 * @file imgst_gbcollect.c
 * @brief imgStore library: garbadge collector implementation.
 */

#include "imgStore.h"
#include "image_content.h"

int do_gbcollect(const char* orig_filename, const char* tmp_filename)
{

    if(orig_filename == NULL || tmp_filename == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    int err = ERR_NONE;

    struct imgst_file orig_file;
    memset(&orig_file, 0, sizeof(orig_file));

    err = do_open(orig_filename, "r+b", &orig_file);
    if (err != ERR_NONE) {
        return err;
    }

    struct imgst_file tmp_file;
    memset(&tmp_file, 0, sizeof(tmp_file));

    tmp_file.header.max_files = orig_file.header.max_files;
    for(int i = 0; i <= NB_RES; ++i) {
        tmp_file.header.res_resized[i] = orig_file.header.res_resized[i];
    }

    err = do_create(tmp_filename, &tmp_file);
    if (err != ERR_NONE) {
        return err;
    }

    char* img_buf;
    uint32_t img_size;
    struct img_metadata img;
    size_t index_new = 0;

    for (uint32_t i = 0; i < orig_file.header.max_files; i++) {
        img = orig_file.metadata[i];
        if (img.is_valid == 1) {

            err = do_read(img.img_id, RES_ORIG, &img_buf, &img_size, &orig_file);
            if (err != ERR_NONE) {
                return err;
            }

            err = do_insert(img_buf, img_size, img.img_id, &tmp_file);
            if (err != ERR_NONE) {
                return err;
            }
            for (int res = 0; res < NB_RES-1; res++) {
                if (img.offset[res] != 0) {
                    err = lazily_resize(res, &tmp_file, index_new);
                    if (err != ERR_NONE) {
                        return err;
                    }
                }
            }
            index_new += 1;
        }
    }
    err = remove(orig_filename);
    if (err != ERR_NONE) {
        return ERR_IO;
    }

    err = rename(tmp_filename, orig_filename);
    if (err != ERR_NONE) {
        return ERR_IO;
    }

    do_close(&orig_file);
    do_close(&tmp_file);
    return ERR_NONE;
}
