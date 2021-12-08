/**
 * @file imgStoreMgr.c
 * @brief imgStore Manager: command line interpretor for imgStore core commands.
 *
 * Image Database Management Tool
 *
 * @author Mia Primorac
 */

#include "util.h" // for _unused
#include "imgStore.h"
#include "image_content.h"
#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>

/********************************************************************//**
 * Opens imgStore file and calls do_list command.
 ********************************************************************** */
int
do_list_cmd (const char* filename _unused)
{

    struct imgst_file myfile;
    memset(&myfile, 0, sizeof(myfile));
    int err_open = do_open(filename, "rb", &myfile);

    if(err_open == ERR_NONE) {
        do_list(&myfile);
        do_close(&myfile);
    }

    return err_open;
}

/********************************************************************//**
 * Prepares and calls do_create command.
********************************************************************** */
int
do_create_cmd (const char* filename _unused)
{
    // This will later come from the parsing of command line arguments
    const uint32_t max_files   =  10;
    const uint16_t thumb_res_x =  64;
    const uint16_t thumb_res_y =  64;
    const uint16_t small_res_x = 256;
    const uint16_t small_res_y = 256;

    puts("Create");

    struct imgst_file im_file;
    memset(&im_file, 0, sizeof(im_file));

    im_file.header.max_files = max_files;
    im_file.header.res_resized[0] = thumb_res_x;
    im_file.header.res_resized[1] = thumb_res_y;
    im_file.header.res_resized[2] = small_res_x;
    im_file.header.res_resized[3] = small_res_y;

    int is_error = do_create(filename, &im_file);

    if (is_error==ERR_NONE) {
        print_header(&im_file.header);
        do_close(&im_file);
    }

    return is_error;
}

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int
help (void)
{
    printf("imgStoreMgr [COMMAND] [ARGUMENTS]\n");
    printf("  help: displays this help.\n");
    printf("  list <imgstore_filename>: list imgStore content.\n");
    printf("  create <imgstore_filename>: create a new imgStore.\n");
    printf("  delete <imgstore_filename> <imgID>: delete image imgID from imgStore.\n");
    return 0;
}

/********************************************************************//**
 * Deletes an image from the imgStore.
 */
int
do_delete_cmd (const char* filename _unused, const char* imgID _unused)
{
    int error = ERR_NONE;

    if (imgID==NULL || strlen(imgID)>MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    } else {
        struct imgst_file myfile;
        error = do_open(filename, "r+b", &myfile);
        if (error != ERR_NONE) {
            return error;
        }
        error = do_delete(imgID, &myfile);
        do_close(&myfile);
    }

    return error;
}

/********************************************************************//**
 * MAIN
 */
int main (int argc, char* argv[])
{
    int ret = ERR_NONE;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        /* **********************************************************************
         * TODO WEEK 07: THIS PART SHALL BE EXTENDED, THEN REVISED (WEEK 08).
         * **********************************************************************
         */

        if (VIPS_INIT(argv[0])) {
            vips_error_exit("unable to start VIPS");
        }

        argc--;
        argv++; // skips command call name
        if (!strcmp("list", argv[0])) {
            if (argc < 2) {
                ret = ERR_NOT_ENOUGH_ARGUMENTS;
            } else {
                ret = do_list_cmd(argv[1]);
            }
        } else if (!strcmp("create", argv[0])) {
            if (argc < 2) {
                ret = ERR_NOT_ENOUGH_ARGUMENTS;
            } else {
                ret = do_create_cmd(argv[1]);
            }
        } else if (!strcmp("delete", argv[0])) {
            if (argc < 3) {
                ret = ERR_NOT_ENOUGH_ARGUMENTS;
            } else {
                ret = do_delete_cmd(argv[1], argv[2]);
            }
        } else if (!strcmp("help", argv[0])) {
            ret = help();
        } else {
            ret = ERR_INVALID_COMMAND;
        }
        vips_shutdown();
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MESSAGES[ret]);
        help();
    }

    return ret;
}
