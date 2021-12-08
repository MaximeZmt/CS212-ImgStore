/**
 * @file imgst_list.c
 * @brief imgStore library: do_list implementation.
 */

#include "imgStore.h"
#include <json-c/json.h>

/**
 * Lists all metadata of an imgst_file in either stdout or json mode
 *
 * @param file imgst_file
 * @param mode stdout or json
 * @param json_array
 * @return an error code according to error.h
 */
int process_all_metadata(const struct imgst_file* file, enum do_list_mode mode, json_object* json_array)
{
    uint32_t counter = 0;
    uint32_t iter = 0;
    // Go across all the metadata
    while(counter<file->header.num_files && iter<file->header.max_files) {
        // Check if this is a valid one (otherwise ignore it)
        if(file->metadata[iter].is_valid == 1) {
            if (mode == STDOUT) {
                print_metadata(&file->metadata[iter]);
            } else if (mode == JSON) {
                json_object* json_im_id = json_object_new_string(file->metadata[iter].img_id);
                if (json_im_id == NULL) {
                    return -1;
                }
                int err = json_object_array_add(json_array, json_im_id); //returns 0 on success and -1 on error
                if (err != 0) {
                    return err;
                }
            }
            counter += 1;
        }
        iter += 1;
    }
    return 0;
}

char*
do_list(const struct imgst_file* file, enum do_list_mode mode)
{
    if(file == NULL) {
        fprintf(stderr, "Cannot call do list, NullPointerException!\n");
        return NULL;
    }

    if (mode == STDOUT) {
        print_header(&file->header);
        if (file->header.num_files==0) {
            printf("<< empty imgStore >>\n");
        } else {
            int err = process_all_metadata(file, STDOUT, NULL);
            if (err != 0) {
                printf("Internal Json error");
            }
        }
        return NULL;

    } else if (mode == JSON) {
        json_object* main_json = json_object_new_object();
        if(main_json == NULL) {
            printf("Internal Json error");
            return NULL;
        }
        json_object* json_array = json_object_new_array();
        if (json_array == NULL) {
            printf("Internal Json error");
            json_object_put(main_json);
            return NULL;
        }
        int err;
        err = process_all_metadata(file, JSON, json_array);
        if (err != 0) {
            printf("Internal Json error");
            json_object_put(main_json);
            json_object_put(json_array);
            return NULL;
        }
        err = json_object_object_add(main_json, "Images", json_array);
        if (err != 0) {
            printf("Internal Json error");
            json_object_put(main_json);
            return NULL;
        }


        const char* str = json_object_to_json_string(main_json);
        char* new_str = malloc(strlen(str)+1);
        strcpy(new_str, str);
        json_object_put(main_json);
        return new_str;
    } else {
        char my_error[] = "unimplemented do_list output mode";
        char* ret = malloc(strlen(my_error)+1);
        strcpy(ret, my_error);
        return ret;
    }
}



