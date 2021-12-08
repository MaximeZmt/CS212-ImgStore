/**
 * @file imgStoreMgr.c
 * @brief imgStore Manager: command line interpretor for imgStore core commands.
 *
 * Image Database Management Tool
 *
 * @author Mia Primorac
 */

#include "util.h" // for atoint32
#include "imgStore.h"
#include "image_content.h"
#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>

#define CMD_NBR 7
#define MAX_FILE_ARG_REQ 1
#define RES_ARG_REQ 2

#define MAX_SMALL_X 512
#define MAX_SMALL_Y 512
#define MAX_THUMB_X 128
#define MAX_THUMB_Y 128

typedef int (*command)(int argc, char* argv[]);

struct command_mapping {
    const char* value;
    command cmd;
};

/********************************************************************//**
 * Opens imgStore file and calls do_list command.
 ********************************************************************** */
int
do_list_cmd (int argc, char* argv[])
{
    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    struct imgst_file myfile;
    memset(&myfile, 0, sizeof(myfile));
    int err_open = do_open(argv[1], "rb", &myfile);

    if(err_open == ERR_NONE) {
        do_list(&myfile, STDOUT);
        do_close(&myfile);
    }

    return err_open;
}

/********************************************************************//**
 * Prepares and calls do_create command.
********************************************************************** */
int
do_create_cmd (int argc, char* argv[])
{

    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    // This will later come from the parsing of command line arguments
    uint32_t max_files   =  10;
    uint16_t thumb_res_x =  64;
    uint16_t thumb_res_y =  64;
    uint16_t small_res_x = 256;
    uint16_t small_res_y = 256;

    for (int index = 2; index<argc; index++) {
        if(!strcmp(argv[index], "-max_files")) {
            if(argc <= index + MAX_FILE_ARG_REQ) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            uint32_t new_max_file = atouint32(argv[index + 1]);
            if(new_max_file == 0 || new_max_file > MAX_MAX_FILES) {
                return ERR_MAX_FILES;
            }
            max_files = new_max_file;
            index += 1;
        } else if (!strcmp(argv[index], "-thumb_res")) {
            if(argc <= index + RES_ARG_REQ) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            uint16_t new_thumb_res_x = atouint16(argv[index + 1]);
            uint16_t new_thumb_res_y = atouint16(argv[index + 2]);
            if(new_thumb_res_x == 0 || new_thumb_res_y == 0 || new_thumb_res_x>MAX_THUMB_X || new_thumb_res_y > MAX_THUMB_Y) {
                return ERR_RESOLUTIONS;
            }
            thumb_res_x = new_thumb_res_x;
            thumb_res_y = new_thumb_res_y;
            index += 2;
        } else if(!strcmp(argv[index], "-small_res")) {
            if(argc <= index + RES_ARG_REQ) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            uint16_t new_small_res_x = atouint16(argv[index + 1]);
            uint16_t new_small_res_y = atouint16(argv[index + 2]);
            if(new_small_res_x == 0 || new_small_res_y == 0 || new_small_res_x>MAX_SMALL_X || new_small_res_y > MAX_SMALL_Y) {
                return ERR_RESOLUTIONS;
            }
            small_res_x = new_small_res_x;
            small_res_y = new_small_res_y;
            index += 2;
        } else {
            return ERR_INVALID_ARGUMENT;
        }

    }

    puts("Create");

    struct imgst_file im_file;
    memset(&im_file, 0, sizeof(im_file));

    im_file.header.max_files = max_files;
    im_file.header.res_resized[0] = thumb_res_x;
    im_file.header.res_resized[1] = thumb_res_y;
    im_file.header.res_resized[2] = small_res_x;
    im_file.header.res_resized[3] = small_res_y;

    int is_error = do_create(argv[1], &im_file);

    if (is_error==ERR_NONE) {
        print_header(&im_file.header);
        do_close(&im_file);
    } else {
        if(im_file.metadata != NULL) {
            free(im_file.metadata);
            im_file.metadata = NULL;
        }
    }

    return is_error;
}

/********************************************************************//**
 * Displays some explanations.
 ********************************************************************** */
int
help (int argc, char* argv[])
{
    printf("imgStoreMgr [COMMAND] [ARGUMENTS]\n");
    printf("  help: displays this help.\n");
    printf("  list <imgstore_filename>: list imgStore content.\n");
    printf("  create <imgstore_filename> [options]: create a new imgStore.\n");
    printf("      options are:\n");
    printf("          -max_files <MAX_FILES>: maximum number of files.\n");
    printf("                                  default value is 10\n");
    printf("                                  maximum value is %d\n", MAX_MAX_FILES);
    printf("          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n");
    printf("                                  default value is 64x64\n");
    printf("                                  maximum value is %dx%d\n", MAX_THUMB_X, MAX_THUMB_Y);
    printf("          -small_res <X_RES> <Y_RES>: resolution for small images.\n");
    printf("                                  default value is 256x256\n");
    printf("                                  maximum value is %dx%d\n", MAX_SMALL_X, MAX_SMALL_Y);
    printf("  read   <imgstore_filename> <imgID> [original|orig|thumbnail|thumb|small]:\n");
    printf("      read an image from the imgStore and save it to a file.\n");
    printf("      default resolution is \"original\".\n");
    printf("  insert <imgstore_filename> <imgID> <filename>: insert a new image in the imgStore.\n");
    printf("  delete <imgstore_filename> <imgID>: delete image imgID from imgStore.\n");
    printf("  gc <imgstore_filename> <tmp imgstore_filename>: performs garbage collecting on imgStore. Requires a temporary filename for copying the imgStore.\n");
    return ERR_NONE;
}

/********************************************************************//**
 * Deletes an image from the imgStore.
********************************************************************** */
int
do_delete_cmd (int argc, char* argv[])
{
    int error = ERR_NONE;

    if (argc < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    if (argv[2]==NULL || strlen(argv[2])>MAX_IMG_ID || strlen(argv[2])==0 ) {
        return ERR_INVALID_IMGID;
    } else {
        struct imgst_file myfile;
        error = do_open(argv[1], "r+b", &myfile);
        if (error != ERR_NONE) {
            return error;
        }
        error = do_delete(argv[2], &myfile);
        do_close(&myfile);
    }

    return error;
}


/********************************************************************//**
 * Prepares, open new picture and load it into buffer and calls do_insert command.
********************************************************************** */
int do_insert_cmd(int argc, char* argv[])
{
    if (argc < 4) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    if(strlen(argv[2])>MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    }

    struct imgst_file myfile;
    memset(&myfile, 0, sizeof(myfile));

    int err_open = do_open(argv[1], "r+b", &myfile);
    if (err_open != ERR_NONE) {
        return err_open;
    }

    char* img_buffer = NULL;
    uint64_t size_of_buffer;

    int err_reading = read_disk_image(argv[3], "rb", &img_buffer, &size_of_buffer);
    if (err_reading != ERR_NONE) {
        do_close(&myfile);
        free(img_buffer);
        return err_reading;
    }


    err_open = do_insert(img_buffer, size_of_buffer, argv[2], &myfile);
    do_close(&myfile);
    free(img_buffer);

    return err_open;

}

/********************************************************************//**
 * Prepares, create buffer, calls do_read and store the jpg image command.
********************************************************************** */
int do_read_cmd(int argc, char* argv[])
{
    if (argc < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    if (argv[1]==NULL || strlen(argv[1])>MAX_IMGST_NAME) {
        return ERR_INVALID_FILENAME;
    }
    if (argv[2]==NULL || strlen(argv[2])>MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    }

    int res_code = RES_ORIG;
    if (argc > 3) {
        int code = resolution_atoi(argv[3]);
        if (code == -1) {
            return ERR_RESOLUTIONS;
        } else {
            res_code = code;
        }
    }

    struct imgst_file myfile;
    char* image_buffer = NULL;


    int error = do_open(argv[1], "r+b", &myfile);
    if (error != ERR_NONE) {
        return error;
    }

    uint32_t image_size = 0;

    error = do_read(argv[2], res_code, &image_buffer, &image_size, &myfile);
    if(error != ERR_NONE) {
        do_close(&myfile);
        return error;
    }


    char fname[MAX_IMG_ID+RES_SUFFIX_LEN+DOT_JPG_SUFFIX_LEN+1];
    name_gen(fname, argv[2], res_code);


    int error_writing = write_disk_image(fname, "wb", image_size, &image_buffer);
    if(error_writing != ERR_NONE) {
        do_close(&myfile);
        free(image_buffer);
        return error_writing;
    }


    do_close(&myfile);
    free(image_buffer);

    return error;
}

int do_gc_cmd(int argc, char* argv[])
{
    if (argc < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    return do_gbcollect(argv[1], argv[2]);
}

/********************************************************************//**
 * MAIN
 */
int main (int argc, char* argv[])
{
    int ret = ERR_NONE;

    struct command_mapping commands[CMD_NBR] = {
        {"list", do_list_cmd},
        {"create", do_create_cmd},
        {"help", help},
        {"delete", do_delete_cmd},
        {"insert", do_insert_cmd},
        {"read", do_read_cmd},
        {"gc", do_gc_cmd}
    };

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        if (VIPS_INIT(argv[0])) {
            vips_error_exit("unable to start VIPS");
            return ERR_IMGLIB;
        }

        argc--;
        argv++; // skips command call name
        ret = ERR_INVALID_COMMAND;
        for (size_t index = 0; index < CMD_NBR; index++) {
            if(!strcmp(commands[index].value, argv[0])) {
                ret = commands[index].cmd(argc, argv);
            }
        }

        vips_shutdown();
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MESSAGES[ret]);
        help(argc, argv);
    }

    return ret;
}
