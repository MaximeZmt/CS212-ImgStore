/**
 * @file imgStore_server.c
 * @brief imgStore server: Contains the webserver application.
 */


#include <signal.h>
#include <stdlib.h>
#include "mongoose.h"
#include "imgStore.h"
#include <vips/vips.h>
#include "util.h"
#include "string.h"

//these values comes from mongoose mg_http_upload function
#define MAX_IMG_NAME_STRLEN 200
#define MAX_IMG_OFFSET_STRLEN 40

#define PREFIX_STRLEN_TEMP 6

#define HTTP_OK_CODE 200
#define HTTP_REDIRECT_CODE 302
#define HTTP_ERROR_CODE 500

#define POLL_TIME 1000

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo)
{
    s_signo = signo;
}

// ======================================================================
static const char *s_listening_address = "http://localhost:8000";
static const char *s_web_directory = ".";

// ======================================================================
/**
 * @brief Handles server events (eg HTTP requests).
 * For more check https://cesanta.com/docs/#event-handler-function
 */

/**
 * Method that reply an error given a error code from error.h
 *
 * @param nc a libmongoose connection
 * @param error an error code from error.h
 */
static void mg_error_msg(struct mg_connection *nc, int error)
{
    mg_http_reply(nc, HTTP_ERROR_CODE, "", "Error: %s", ERR_MESSAGES[error]);
}

/**
 * Event handler for do_list
 *
 * @param nc a libmongoose connection
 * @param file an imgst_file that we are going to use
 */
static void handle_list_call(struct mg_connection *nc, const struct imgst_file* file)
{
    char* do_list_reply = do_list(file, JSON);
    mg_http_reply(nc, HTTP_OK_CODE, "Content-Type: application/json\r\n", "%s", do_list_reply);
    if(do_list_reply != NULL) {
        free(do_list_reply);
    }
}

/**
 * Event handler for do_read
 *
 * @param nc a libmongoose connection
 * @param hm the http_message that contains http information
 * @param file an imgst_file that we are going to use
 */
static void handle_read_call(struct mg_connection *nc, struct mg_http_message *hm, const struct imgst_file* file)
{
    char* img_id = malloc(MAX_IMG_ID+1);
    char* res = malloc(MAX_RES_TXT_LEN+1);

    const struct mg_str * mg_struct = (const struct mg_str *) &(hm->query.ptr); // cast to transfer to the good type of pointer

    int e1 = mg_http_get_var(mg_struct, "img_id", img_id, MAX_IMG_ID+1);
    int e2 = mg_http_get_var(mg_struct, "res", res, MAX_RES_TXT_LEN+1);
    if(e1 <= 0 || e2 <= 0) {
        free(img_id);
        free(res);
        mg_error_msg(nc, ERR_INVALID_ARGUMENT);
        return;
    }

    int res_code = resolution_atoi(res);
    if (res_code == -1) {
        free(img_id);
        free(res);
        mg_error_msg(nc, ERR_RESOLUTIONS);
        return;
    }

    char* image_buffer = NULL;
    uint32_t image_size = 0;

    int error = do_read(img_id, res_code, &image_buffer, &image_size, file);
    if(error != ERR_NONE) {
        free(img_id);
        free(res);
        mg_error_msg(nc, error);
        return;
    }

    mg_printf(nc, "HTTP/1.1 %d OK\r\nContent-Type: image/jpeg\r\nContent-Length: %" PRIu32 "\r\n\r\n", HTTP_OK_CODE, image_size);

    mg_send(nc, (void*) image_buffer, image_size);

    free(img_id);
    free(res);
    free(image_buffer);
}

/**
 * Event handler for do_delete
 *
 * @param nc a libmongoose connection
 * @param hm the http_message that contains http information
 * @param file an imgst_file that we are going to use
 */
static void handle_delete_call(struct mg_connection *nc, struct mg_http_message *hm, struct imgst_file* file)
{
    char* img_id = malloc(MAX_IMG_ID+1);
    if(img_id == NULL) {
        mg_error_msg(nc, ERR_OUT_OF_MEMORY);
        return;
    }

    const struct mg_str * mg_struct = (const struct mg_str *) &(hm->query.ptr); // cast to transfer to the good type of pointer

    int e1 = mg_http_get_var(mg_struct, "img_id", img_id, MAX_IMG_ID+1);
    if(e1 <= 0) {
        free(img_id);
        mg_error_msg(nc, ERR_FILE_NOT_FOUND);
        return;
    }

    int err = do_delete(img_id, file);
    free(img_id);

    if(err == ERR_NONE) {
        mg_http_reply(nc, HTTP_REDIRECT_CODE, "Location: /index.html\r\n", "");
    } else {
        mg_error_msg(nc, err);
    }

}

/**
 * Event handler for do_insert
 *
 * @param nc a libmongoose connection
 * @param hm the http_message that contains http information
 * @param file an imgst_file that we are going to use
 */
static void handle_insert_call(struct mg_connection *nc, struct mg_http_message *hm, struct imgst_file* file)
{
    if (hm->body.len != 0) {
        mg_http_upload(nc, hm, "/tmp");
    } else {
        char offset[MAX_IMG_OFFSET_STRLEN] = "";
        char img_id[MAX_IMG_NAME_STRLEN] = "";
        mg_http_get_var(&hm->query, "offset", offset, sizeof(offset));
        mg_http_get_var(&hm->query, "name", img_id, sizeof(img_id));

        uint32_t size_of_buffer = atouint32(offset);

        char* fImName = malloc(PREFIX_STRLEN_TEMP+strlen(img_id)+1);
        strncpy(fImName,"/tmp/",PREFIX_STRLEN_TEMP);
        strcat(fImName, img_id);

        FILE* fileIm = fopen(fImName, "rb");
        if (fileIm==NULL) {
            free(fImName);
            mg_error_msg(nc, ERR_FILE_NOT_FOUND);
            return;
        }

        //want to have the pointer on the last .??? that is used as an extension for a file.
        char* ptr = strrchr(img_id, '.');
        if(ptr != NULL) {
            *ptr = '\0';
        }

        if(strlen(img_id)>=MAX_IMG_ID) {
            free(fImName);
            mg_error_msg(nc, ERR_INVALID_IMGID);
            return;
        }

        char* img_buffer = malloc(size_of_buffer+1);

        fread(img_buffer, size_of_buffer, 1, fileIm);
        fclose(fileIm);

        int err = do_insert(img_buffer, size_of_buffer, img_id, file);

        if(img_buffer != NULL) {
            free(img_buffer);
            img_buffer = NULL;
        }

        if(fImName != NULL) {
            free(fImName);
            fImName = NULL;
        }


        if(err == ERR_NONE) {
            mg_http_reply(nc, HTTP_REDIRECT_CODE, "Location: /index.html\r\n", "");
        } else {
            mg_error_msg(nc, err);
        }
    }

}

/**
 * Main event handler that split the task give the url to different subhandler
 *
 * @param nc nc a libmongoose connection
 * @param ev an event number, defined in mongoose.h (from libmongoose docs: https://cesanta.com/docs/)
 * @param ev_data pointer to the event-specific data (from libmongoose docs: https://cesanta.com/docs/)
 * @param fn_data a user-defined pointer for the connection (from libmongoose docs: https://cesanta.com/docs/)
 */
static void event_handler(struct mg_connection *nc,
                          int ev,
                          void *ev_data,
                          void *fn_data
                         )
{
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    switch (ev) {
    case MG_EV_HTTP_MSG:
        if (mg_http_match_uri(hm, "/imgStore/list")) {
            handle_list_call(nc, fn_data);
        } else if (mg_http_match_uri(hm, "/imgStore/read")) {
            handle_read_call(nc, hm, fn_data);
        } else if (mg_http_match_uri(hm, "/imgStore/delete")) {
            handle_delete_call(nc, hm, fn_data);
        } else if (mg_http_match_uri(hm, "/imgStore/insert") && !mg_vcasecmp(&(hm->method),"POST")) {
            handle_insert_call(nc, hm, fn_data);
        } else {
            struct mg_http_serve_opts opts = {.root_dir = s_web_directory};
            mg_http_serve_dir(nc, ev_data, &opts);
        }
    }
}

// ======================================================================
/**********************************************************************
 * MAIN
 *********************************************************************/
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "%s", ERR_MESSAGES[ERR_NOT_ENOUGH_ARGUMENTS]);
        return 1;
    }

    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
        fprintf(stderr, "%s", ERR_MESSAGES[ERR_IMGLIB]);
        return 1;
    }


    //image_filename = argv[1];
    struct imgst_file myfile;
    memset(&myfile, 0, sizeof(myfile));
    int err_open = do_open(argv[1], "r+b", &myfile);
    if(err_open != ERR_NONE) {
        fprintf(stderr, "%s", ERR_MESSAGES[ERR_IO]);
        return 1;
    }

    //create signal
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Create server */
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    if (mg_http_listen(&mgr, s_listening_address, event_handler, &myfile) == NULL) {
        fprintf(stderr, "Error starting server on address %s\n", s_listening_address);
        return 1;
    }

    printf("Starting imgStore server on %s\n", s_listening_address);
    print_header(&(myfile.header));

    /* Poll */
    while (s_signo == 0) mg_mgr_poll(&mgr, POLL_TIME);
    /* Cleanup */
    vips_shutdown();
    mg_mgr_free(&mgr);
    do_close(&myfile);

    return ERR_NONE;
}
