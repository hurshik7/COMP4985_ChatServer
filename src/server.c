#include "my_ndbm.h"
#include "server.h"
#include "util.h"
#include "ncurses_ui.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_CLIENTS (255)

extern connected_user active_users[MAX_CLIENTS];

int handle_request(int fd, const char* clnt_addr, connected_user* cache)
{
    int result;
    chat_header_t header;
    memset(&header, 0, sizeof(header));
    result = read_header(fd, &header);

    // log for a header a client sends
    printw("[Header]version: %d, type: %d, object: %d, body-size: %d\n", header.version_type.version, header.version_type.type, header.object, header.body_size);
    refresh();
    //

    if (result < 0) {
        perror("[SERVER]Error: read_header()");
        return -1;
    }

    if (header.version_type.version != CURRENT_VERSION) {
        perror("[SERVER]Error: wrong version");
        return ERROR_WRONG_VERSION;
    }

    assert(header.version_type.version == CURRENT_VERSION);
    // do server base on request, read body if it needs to
    char token[TOKEN_NAME_LENGTH] = { '\0', };
    switch (header.object) {
        case OBJECT_USER:
            if (header.version_type.type == TYPE_CREATE) {
                result = read_and_create_user(fd, token, header.body_size);
                send_create_user_response(fd, header, result, token, clnt_addr);
            } else if (header.version_type.type == TYPE_READ) {

            } else if (header.version_type.type == TYPE_UPDATE) {

            } else if (header.version_type.type == TYPE_DESTROY) {
                // TODO WORK ON THIS NOW
            } else {
                perror("[SERVER]Error: wrong type");
                assert(!"should not be here");
            }
            break;
        case OBJECT_CHANNEL:
            if (header.version_type.type == TYPE_CREATE) {
                result = read_and_create_channel(fd, token, header.body_size, clnt_addr);
                send_create_channel_response(fd, header, result, token, clnt_addr);
            } else if (header.version_type.type == TYPE_READ) {

            } else if (header.version_type.type == TYPE_UPDATE) {

            } else if (header.version_type.type == TYPE_DESTROY) {

            } else {
                perror("[SERVER]Error: wrong type");
                assert(!"should not be here");
            }
            break;
        case OBJECT_MESSAGE:
            if (header.version_type.type == TYPE_CREATE) {
                result = read_and_create_message(fd, token, cache);
                send_create_message_response(fd, header, result, token, clnt_addr);
//                channel_info_t* current_channel = get_channel_info_malloc_or_null(token);
//                for (int i = 0; i <= get_num_connected_users(cache); i++) {
                    // function that takes cache[i]->display_name as param that searches and
                    // returns true if current display_name in current_channel->user_list matches
                        // if true
                            // int user_in_channel_fd = cache[i]->fd
                            // char* this_ip_addr = cache[i]->in_channel_ip_addr
                            // send_create_message_response(user_in_channel_fd, header, SUCCESS_FORWARD_REQ, this_ip_addr)
                        // else continue;
//                }
            } else if (header.version_type.type == TYPE_READ) {

            } else if (header.version_type.type == TYPE_UPDATE) {

            } else if (header.version_type.type == TYPE_DESTROY) {

            } else {
                perror("[SERVER]Error: wrong type");
                assert(!"should not be here");
            }
            break;
        case OBJECT_AUTH:
            // it relates to login(CREATE), logout(DESTROY)
            if (header.version_type.type == TYPE_CREATE) {
                // TODO add terminate_and_restablish_connection check here
                result = read_and_login_user(fd, token, clnt_addr, cache);
//                if (result == TERMINATE_AND_RESTABLISH_CONNECTION) {
//                    int terminate = find_duplicate_user(cache, get_num_connected_users(cache));
//                    send_logout_user_response(terminate, header, result, token, clnt_addr);
//                    send_login_user_response(fd, header, 0, token, clnt_addr);
//                } else {
                    send_login_user_response(fd, header, result, token, clnt_addr);
//                }
            } else if (header.version_type.type == TYPE_UPDATE) {

            } else if (header.version_type.type == TYPE_DESTROY) {
                result = read_and_logout_user(fd, token, clnt_addr, cache);
                send_logout_user_response(fd, header, result, token, clnt_addr);
            } else {
                perror("[SERVER]Error: wrong type");
                assert(!"should not be here");
            }
            break;
        default:
//            perror("[SERVER]Error: wrong object number");
//            assert(!"This should not be here");
            // Check object type being sent
            printw("object type: %hhu\n", header.object);
            refresh();
    }

    return 0;
}

int read_header(int fd, chat_header_t *header_out)
{
    ssize_t nread = read(fd, &header_out->version_type, 1);
    if (nread < 0) {
        perror("[SERVER]Error: read() in read_header(), reading version_type");
        return -1;
    }

    nread = read(fd, &header_out->object, 1);
    if (nread < 0) {
        perror("[SERVER]Error: read() in read_header(), object");
        return -1;
    }

    nread = read(fd, &header_out->body_size, 2);
    if (nread < 0) {
        perror("[SERVER]Error: read() in read_header(), body_size");
        return -1;
    }

    header_out->body_size = ntohs(header_out->body_size);
    return 0;
}

int read_and_create_user(int fd, char token_out[TOKEN_NAME_LENGTH], uint16_t body_size)
{
    char buffer[DEFAULT_BUFFER];
    memset(buffer, '\0', DEFAULT_BUFFER);
    ssize_t nread = read(fd, buffer, body_size);
    if (nread <= 0 || nread != body_size) {
        perror("[SERVER]Error: read body");
        return -1;
    }

    // log
    printw("[Body]nread: %d, body: %s\n", nread, buffer);
    refresh();

    char login_token[TOKEN_NAME_LENGTH] = { '\0', };
    char display_name[TOKEN_NAME_LENGTH] = { '\0', };
    char password[PSWD_MAX_LENGTH] = { '\0', };

    char* token = strtok(buffer, "\3"); // login_token
    strncpy(login_token, token, strlen(token));
    token = strtok(NULL, "\3"); // display-name
    strncpy(display_name, token, strlen(token));

    bool is_token_duplicate = false;
    user_login_t* login_info = get_login_info_malloc_or_null(login_token);
    bool is_display_name_duplicates = check_duplicate_display_name(display_name);
    if (login_info != NULL) {
        // the user already exist
        is_token_duplicate = true;
    }

    if (is_token_duplicate == true && is_display_name_duplicates == true) {
        goto error_exit_duplicate_token_and_name;
    }
    if (is_token_duplicate == true) {
        goto error_exit_duplicate_login_token;
    }
    if (is_display_name_duplicates == true) {
        goto error_exit_duplicate_display_name;
    }
    assert(is_token_duplicate == false && is_display_name_duplicates == false);
    assert(login_info == NULL);

    token = strtok(NULL, "\3"); // password
    strncpy(password, token, strlen(token));

    //  store the user in the db (login_info db, user_account db, and also display_name db)
    char* user_uuid_malloc = generate_random_uuid_malloc();
    if (user_uuid_malloc != NULL) {
        insert_display_name(display_name, user_uuid_malloc);
    }
    user_login_t* user_login = generate_user_login_malloc_or_null(login_token, password, user_uuid_malloc);
    if (user_login != NULL) {
        insert_user_login(user_login);
    }
    user_account_t* user_account = generate_user_account_malloc_or_null(user_uuid_malloc, display_name);
    if (user_account != NULL) {
        insert_user_account(user_account);
    }
    strncpy(token_out, login_token, TOKEN_NAME_LENGTH);

    free(user_account);
    free(user_login);
    free(user_uuid_malloc);
    free(login_info);
    return 0;
    error_exit_duplicate_token_and_name:
    free(login_info);
    return ERROR_CREATE_USER_DUPLICATE_ALL;
    error_exit_duplicate_login_token:
    free(login_info);
    return ERROR_CREATE_USER_DUPLICATE_TOKEN;
    error_exit_duplicate_display_name:
    free(login_info);
    return ERROR_CREATE_USER_DUPLICATE_DISPLAY_NAME;
}

int read_and_destroy_user(int fd, char token_out[TOKEN_NAME_LENGTH], uint16_t body_size)
{
    char buffer[DEFAULT_BUFFER];
    memset(buffer, '\0', DEFAULT_BUFFER);
    ssize_t nread = read(fd, buffer, body_size);
    if (nread <= 0 || nread != body_size) {
        perror("[SERVER]Error: read body");
        return -1;
    }

    // log
    printw("[Body]nread: %d, body: %s\n", nread, buffer);
    refresh();

    char login_token[TOKEN_NAME_LENGTH] = { '\0', };
    char display_name[TOKEN_NAME_LENGTH] = { '\0', };
    char password[PSWD_MAX_LENGTH] = { '\0', };

    char* token = strtok(buffer, "\3"); // login_token
    strncpy(login_token, token, strlen(token));
    token = strtok(NULL, "\3"); // display-name
    strncpy(display_name, token, strlen(token));

    bool is_token_duplicate = false;
    user_login_t* login_info = get_login_info_malloc_or_null(login_token);
    bool is_display_name_duplicates = check_duplicate_display_name(display_name);
    if (login_info != NULL) {
        // the user already exist
        is_token_duplicate = true;
    }

    if (is_token_duplicate == true && is_display_name_duplicates == true) {
        goto error_exit_duplicate_token_and_name;
    }
    if (is_token_duplicate == true) {
        goto error_exit_duplicate_login_token;
    }
    if (is_display_name_duplicates == true) {
        goto error_exit_duplicate_display_name;
    }
    assert(is_token_duplicate == false && is_display_name_duplicates == false);
    assert(login_info == NULL);

    token = strtok(NULL, "\3"); // password
    strncpy(password, token, strlen(token));

    //  store the user in the db (login_info db, user_account db, and also display_name db)
    char* user_uuid_malloc = generate_random_uuid_malloc();
    if (user_uuid_malloc != NULL) {
        insert_display_name(display_name, user_uuid_malloc);
    }
    user_login_t* user_login = generate_user_login_malloc_or_null(login_token, password, user_uuid_malloc);
    if (user_login != NULL) {
        insert_user_login(user_login);
    }
    user_account_t* user_account = generate_user_account_malloc_or_null(user_uuid_malloc, display_name);
    if (user_account != NULL) {
        insert_user_account(user_account);
    }
    strncpy(token_out, login_token, TOKEN_NAME_LENGTH);

    free(user_account);
    free(user_login);
    free(user_uuid_malloc);
    free(login_info);
    return 0;
    error_exit_duplicate_token_and_name:
    free(login_info);
    return ERROR_CREATE_USER_DUPLICATE_ALL;
    error_exit_duplicate_login_token:
    free(login_info);
    return ERROR_CREATE_USER_DUPLICATE_TOKEN;
    error_exit_duplicate_display_name:
    free(login_info);
    return ERROR_CREATE_USER_DUPLICATE_DISPLAY_NAME;
}

int read_and_login_user(int fd, char token_out[TOKEN_NAME_LENGTH], const char* clnt_addr, connected_user* cache)
{
    char buffer[DEFAULT_BUFFER];
    memset(buffer, '\0', DEFAULT_BUFFER);
    ssize_t nread = read(fd, buffer, DEFAULT_BUFFER);
    if (nread <= 0) {
        perror("[SERVER]Error: read body");
        return -1;
    }

    // log
    printw("[Body]nread: %d, body: %s\n", nread, buffer);
    refresh();

    char login_token[TOKEN_NAME_LENGTH] = { '\0', };
    char password[PSWD_MAX_LENGTH] = { '\0', };
    char* token = strtok(buffer, "\3"); // login_token
    strncpy(login_token, token, strlen(token));
    token = strtok(NULL, "\3"); // display-name
    strncpy(password, token, strlen(token));

    bool is_token_duplicate = false;
    user_login_t* login_info = get_login_info_malloc_or_null(login_token);
    if (login_info != NULL) {
        // the user already exist
        is_token_duplicate = true;
    }
    if (is_token_duplicate == false) {
        // the user does not exist
        goto error_exit_no_such_token;
    }
    if (compare_strings(login_info->password, password) == false
        || compare_strings(login_info->login_token, login_token) == false) {
        // the credentials entered do not match account credentials in the db
        goto error_exit_invalid_credentials;
    }

    assert(is_token_duplicate == true &&
           (compare_strings(login_info->password, password) == true
            || compare_strings(login_info->login_token, login_token) == true));
    assert(login_info != NULL);

    // store uuid and remove extra char at end of uuid string
    char* clnt_uuid = malloc(UUID_LEN);
    strncpy(clnt_uuid, login_info->uuid, UUID_LEN);
    clnt_uuid[strlen(clnt_uuid)] = '\0';

    user_account_t* user_account = get_user_account_malloc_or_null(clnt_uuid);

    // check to see if a user is logging in from a different ip addr
    int active_users_count = get_num_connected_users(cache);
    for (int i = 0; i < active_users_count; i++) {
        if (strcmp(cache[i].dsply_name, user_account->display_name) == 0) {
            if (strcmp(cache[i].ip_address, clnt_addr) != 0) {
                if (user_account->online_status == 1) {
                    goto terminate_and_restablish_connection;
                }
            }
        }
    }

    if (user_account != NULL) {
        login_user_account_malloc_or_null(user_account, clnt_addr);
        insert_user_account(user_account);
    }
    strncpy(token_out, user_account->display_name, TOKEN_NAME_LENGTH);

    // store user in active user cache upon successful login
    if (find_connected_user_with_same_cred(user_account, cache,
                                           get_num_connected_users(cache), fd) == true &&
            get_num_connected_users(cache) != 0) {
        printw("===Restarting your session.===");
        refresh();
    } else {
        insert_user_in_cache(fd, cache, user_account, login_info, get_num_connected_users(cache));
    }

    free(user_account);
    free(login_info);
    free(clnt_uuid);
    return 0;
    error_exit_no_such_token:
    free(login_info);
    return ERROR_LOGIN_DOES_NOT_EXIST;
    error_exit_invalid_credentials:
    free(login_info);
    return ERROR_LOGIN_INVALID_CREDENTIALS;
    terminate_and_restablish_connection:
//    if (user_account != NULL) {
//        logout_user_account_malloc_or_null(user_account);
//        login_user_account_malloc_or_null(user_account, clnt_addr);
//        insert_user_account(user_account);
//    }
    strncpy(token_out, login_token, TOKEN_NAME_LENGTH);
    printw("\n%s\n", "reached terminate_and_restablish_connection");
    refresh();
    // store user in active user cache upon successful login
    insert_user_in_cache(fd, cache, user_account, login_info, get_num_connected_users(cache));
    return TERMINATE_AND_RESTABLISH_CONNECTION;
}

int read_and_logout_user(int fd, char token_out[TOKEN_NAME_LENGTH], const char* clnt_addr, connected_user* cache)
{
    char buffer[DEFAULT_BUFFER];
    memset(buffer, '\0', DEFAULT_BUFFER);
    ssize_t nread = read(fd, buffer, DEFAULT_BUFFER);
    if (nread <= 0) {
        perror("[SERVER]Error: read body");
        return -1;
    }

    // log
    printw("[Body]nread: %d, body: %s\n", nread, buffer);
    refresh();

    char display_name[DSPLY_NAME_LENGTH] = { '\0', };
    char* token = strtok(buffer, "\3"); // login_token
    strncpy(display_name, token, strlen(token));

    bool is_token_duplicate = false;
    user_login_t* login_info = get_login_info_malloc_or_null(display_name);

    char* clnt_uuid = malloc(UUID_LEN);
    strncpy(clnt_uuid, login_info->uuid, UUID_LEN);
    clnt_uuid[strlen(clnt_uuid)] = '\0';

    user_account_t* user_account = get_user_account_malloc_or_null(clnt_uuid);

    // TODO PROBLEM HERE
    switch (user_account->privilege_level) {
        case 0:
            if (login_info != NULL) {
                // user exists in db
                is_token_duplicate = true;
            } else {
                // set variable to true if user exists in db
                goto error_exist_invalid_fields;
            }

            if (strcmp(user_account->ip_addr, clnt_addr) != 0) {
                // the connection ip addr does not match the one stored in db
                goto error_exit_mismatch_address;
            }
        case 1:
            if (login_info != NULL) {
                // user exists in db
                is_token_duplicate = true;
            } else {
                // set variable to true if user exists in db
                goto error_exit_admin_user_not_exist;
            }

            if (user_account->online_status != 1) {
                // the user is not online
                goto error_exit_admin_user_not_online;
            }
    }

    assert(is_token_duplicate == true && user_account->online_status == 1);
    assert(login_info != NULL);

    if (user_account != NULL) {
        logout_user_account_malloc_or_null(user_account);
        insert_user_account(user_account);
    }
    strncpy(token_out, user_account->display_name, DSPLY_NAME_LENGTH);

    // remove user in active user cache upon successful login
    remove_user_in_cache(cache, user_account, get_num_connected_users(cache));

    free(user_account);
    free(login_info);
    free(clnt_uuid);
    return 0;
    error_exist_invalid_fields:
    free(login_info);
    return ERROR_LOGOUT_INVALID_FIELDS;
    error_exit_mismatch_address:
    free(login_info);
    return ERROR_LOGOUT_USER_MISMATCH_ADDRESS;
    error_exit_admin_user_not_exist:
    free(login_info);
    return ERROR_LOGOUT_ADMIN_USER_NOT_EXIST;
    error_exit_admin_user_not_online:
    free(login_info);
    return ERROR_LOGOUT_ADMIN_USER_NOT_ONLINE;
}

int read_and_create_channel(int fd, char token_out[TOKEN_NAME_LENGTH], uint16_t body_size, const char clnt_addr[CLNT_IP_ADDR_LENGTH])
{
    int result;
    char buffer[DEFAULT_BUFFER];
    memset(buffer, '\0', DEFAULT_BUFFER);
    ssize_t nread = read(fd, buffer, body_size);
    if (nread <= 0 || nread != body_size) {
        perror("[SERVER]Error: read body");
        return -1;
    }

    // log
    printw("[Body]nread: %d, body: %s\n", nread, buffer);
    refresh();

    char channel_name[TOKEN_NAME_LENGTH] = { '\0', };
    char display_name[TOKEN_NAME_LENGTH] = { '\0', };
    bool publicity = false;

    char* token = strtok(buffer, "\3"); // channel_name
    strncpy(channel_name, token, strlen(token));

    token = strtok(NULL, "\3"); // display-name
    if (token == NULL) {
        return ERROR_CREATE_CHANNEL_400;
    }
    strncpy(display_name, token, strlen(token));

    token = strtok(NULL, "\3"); // publicity
    if (strcmp(token, "1") == 0) {
        publicity = true;
    } else if (token == NULL || strlen(token) != 1) {
        return ERROR_CREATE_CHANNEL_400;
    }

    char* dis_name_in_cache = get_display_name_in_cache_malloc_or_null(clnt_addr);
    if (dis_name_in_cache == NULL) {
        return ERROR_CREATE_CHANNEL_500;
    }
    bool display_name_exist = check_duplicate_display_name(display_name);

    if (strcmp(dis_name_in_cache, "admin") == 0) {
        if (display_name_exist != true) {
            return ERROR_CREATE_CHANNEL_404;
        }
    } else {
        if (strcmp(display_name, dis_name_in_cache) != 0) {
            return ERROR_CREATE_CHANNEL_403;
        }
    }

    bool is_channel_duplicate = check_duplicate_channel_name(channel_name);
    if (is_channel_duplicate == true) {
        return ERROR_CREATE_CHANNEL_409;
    }

    channel_info_t* channel = create_channel_or_null_malloc(channel_name, display_name, publicity);
    if (channel == NULL) {
        return ERROR_CREATE_CHANNEL_500;
    }
    result = insert_channel_info(channel);
    if (result != 0) {
        return ERROR_CREATE_CHANNEL_500;
    }

    memset(token_out, '\0', TOKEN_NAME_LENGTH);
    strncpy(token_out, channel_name, TOKEN_NAME_LENGTH);

    free(dis_name_in_cache);
    free(channel);
    return 0;
}

int read_and_create_message(int fd, char token_out[TOKEN_NAME_LENGTH], connected_user* cache)
{
    char buffer[DEFAULT_BUFFER];
    memset(buffer, '\0', DEFAULT_BUFFER);
    ssize_t nread = read(fd, buffer, DEFAULT_BUFFER);
    if (nread <= 0) {
        perror("[SERVER]Error: read body");
        return -1;
    }

    // log
    printw("[Body]nread: %d, body: %s\n", nread, buffer);
    refresh();

    char display_name[DSPLY_NAME_LENGTH] = { '\0', };
    char channel_name[TOKEN_NAME_LENGTH] = { '\0', };
    char message_content[MAX_MESSAGE_SIZE] = { '\0', };
    char timestamp[TIMESTAMP_SIZE] = { '\0', };
    char* token = strtok(buffer, "\3");
    strncpy(display_name, token, strlen(token)); // display-name
    token = strtok(NULL, "\3");
    strncpy(channel_name, token, strlen(token)); // channel-name
    token = strtok(NULL, "\3");
    strncpy(message_content, token, strlen(token)); // message-body
    token = strtok(NULL, "\3");
    strncpy(timestamp, token, strlen(token)); // time stamp

    strncpy(token_out, channel_name, TOKEN_NAME_LENGTH);

    // check if all fields in dispatch are present
    bool is_valid_fields;

    if (display_name[0] != '\0' &&
        channel_name[0] != '\0' &&
        message_content[0] != '\0' &&
        timestamp[0] != '\0' &&
        token != NULL) {
        is_valid_fields = true;
    } else {
        goto error_invalid_request_fields;
    }

    // check if the channel in dispatch exists
    bool is_channel_exist;

    channel_info_t* channel = get_channel_info_malloc_or_null(channel_name);

     if (channel == NULL) {
        goto error_channel_not_exist;
     } else {
        is_channel_exist = true;
     }

    assert(is_valid_fields == true && is_channel_exist == true);

    connected_user* user_in_cache = get_connected_user_by_display_name(cache, display_name);

    // store uuid and remove extra char at end of uuid string
    char* clnt_uuid = malloc(UUID_LEN);
    strncpy(clnt_uuid, user_in_cache->uuid, UUID_LEN);
    clnt_uuid[strlen(clnt_uuid)] = '\0';

    user_account_t* user_account = get_user_account_malloc_or_null(clnt_uuid);

    bool is_token_duplicate;

    // TODO CHECK REQ SENDER'S PRIVILEGE FLAG
    switch (user_account->privilege_level) {
        case 0:
            if (user_account != NULL) {
                // user exists in db
                is_token_duplicate = true;
            } else {
                // set variable to true if user exists in db
                goto error_user_does_not_exist_user;
            }
        case 1:
            if (user_account != NULL) {
                // user exists in db
                is_token_duplicate = true;
            } else {
                // set variable to true if user exists in db
                goto error_user_does_not_exist_admin;
            }
    }

    assert(is_token_duplicate == true && user_account != NULL);

    if (user_account != NULL) {
        message_info_t* message = generate_message_malloc_or_null(user_in_cache->login_token, channel, message_content, (uint8_t*)timestamp);
        insert_message(message);
    }

    free(channel);
    free(user_account);
    return 0;
    error_invalid_request_fields:
    return ERROR_INVALID_REQUEST;
    error_channel_not_exist:
    free(channel);
    return ERROR_CHANNEL_DOES_NOT_EXIST;
    error_user_does_not_exist_admin:
    free(channel);
    free(user_account);
    return ERROR_ADMIN_DOES_NOT_EXIST;
    error_user_does_not_exist_user:
    free(channel);
    free(user_account);
    return ERROR_USER_DOES_NOT_EXIST;
}

user_login_t* generate_user_login_malloc_or_null(const char* login_token, const char* password, const char* user_id)
{
    user_login_t* user_login = (user_login_t*) malloc(sizeof(user_login_t));
    if (user_login == NULL) {
        perror("generate user_login_t");
        return NULL;
    }
    memset(user_login, 0, sizeof(user_login_t));
    strncpy(user_login->login_token, login_token, TOKEN_NAME_LENGTH);
    strncpy(user_login->password, password, PSWD_MAX_LENGTH);
    strncpy(user_login->uuid, user_id, UUID_LEN);
    return user_login;
}

user_account_t* generate_user_account_malloc_or_null(const char* uuid, const char* display_name)
{
    user_account_t* user_account = (user_account_t*) malloc(sizeof(user_account_t));
    if (user_account == NULL) {
        perror("generate user_account_t");
        return NULL;
    }
    memset(user_account, 0, sizeof(user_account_t));
    strncpy(user_account->user_id, uuid, UUID_LEN);
    strncpy(user_account->display_name, display_name, TOKEN_NAME_LENGTH);
    strncpy(user_account->ip_addr, "0", CLNT_IP_ADDR_LENGTH);
    user_account->online_status = false;
    user_account->privilege_level = 0;

    if (strcmp(user_account->display_name, "admin") == 0) {
        user_account->privilege_level = 1;
    }

    return user_account;
}

channel_info_t* create_channel_or_null_malloc(const char* channel_name, const char* display_name, bool publicity)
{
    channel_info_t* ret_channel = (channel_info_t*) malloc(sizeof(channel_info_t));
    if (ret_channel == NULL) {
        return NULL;
    }
    memset(ret_channel, 0, sizeof(channel_info_t));

    char* channel_uuid = generate_random_uuid_malloc();
    if (channel_uuid == NULL) {
        return NULL;
    }

    strncpy(ret_channel->channel_id, channel_uuid, UUID_LEN);
    strncpy(ret_channel->channel_name, channel_name, TOKEN_NAME_LENGTH);
    strncpy(ret_channel->creator, display_name, TOKEN_NAME_LENGTH);
    ret_channel->publicity = publicity;
    strncpy(ret_channel->admin_list[0], display_name, TOKEN_NAME_LENGTH);
    strncpy(ret_channel->user_list[0], display_name, TOKEN_NAME_LENGTH);

    free(channel_uuid);
    return ret_channel;
}

message_info_t* generate_message_malloc_or_null(char* display_name, channel_info_t* channel,
                                                char* message_body, const uint8_t* timestamp)
{
    message_info_t* message = (message_info_t*) malloc(sizeof(message_info_t));
    if (message == NULL) {
        perror("generate message_info_t");
        return NULL;
    }

    char* message_uuid = generate_random_uuid_malloc();

    size_t message_body_length = strlen(message_body);

    user_login_t* userLogin = get_login_info_malloc_or_null(display_name);

    // TODO TIMESTAMP ISSUE
    memset(message, 0, sizeof(user_account_t));
    strncpy(message->message_id, message_uuid, UUID_LEN);
    strncpy(message->user_id, userLogin->uuid, TOKEN_NAME_LENGTH);
    strncpy(message->channel_id, channel->channel_id, UUID_LEN);
    strncpy(message->message_content, message_body, sizeof(message_body_length));
    for (size_t i = 0; i < TIMESTAMP_SIZE; ++i) {
        (message->time_stamp)[i] = timestamp[i];
    }

    return message;
}

user_account_t* login_user_account_malloc_or_null(user_account_t* user_acc, const char* clnt_addr)
{
    if (user_acc == NULL) {
        perror("fetch user_account_t");
        return NULL;
    }
    strncpy(user_acc->ip_addr, clnt_addr, CLNT_IP_ADDR_LENGTH);
    user_acc->online_status = true;
    if (strcmp(user_acc->display_name, "admin") == 0) {
        // intialize privilege to 1 for admin account
        user_acc->privilege_level = 1;
    }
    return user_acc;
}

user_account_t* logout_user_account_malloc_or_null(user_account_t* user_acc)
{
    if (user_acc == NULL) {
        perror("fetch user_account_t");
        return NULL;
    }
    strncpy(user_acc->ip_addr, "0", CLNT_IP_ADDR_LENGTH);
    user_acc->online_status = false;
    return user_acc;
}

int send_create_user_response(int fd, chat_header_t header, int result, const char* token, const char* clnt_addr)
{
    char body[DEFAULT_BUFFER] = {'\0', };
    if (result == 0) {
        strcpy(body, "201\3\0");
        strcat(body, token);
    } else {
        sprintf(body, "409\3%d\3", result);
        if (result == ERROR_CREATE_USER_DUPLICATE_TOKEN) {
            strcat(body, "Login Token was not unique");
        } else if (result == ERROR_CREATE_USER_DUPLICATE_DISPLAY_NAME) {
            strcat(body, "Display Name was not unique");
        } else if (result == ERROR_CREATE_USER_DUPLICATE_ALL) {
            strcat(body, "Login Token and Display name both were not unique");
        } else {
            assert(!"should not be here");
        }
    }

    uint16_t body_size = strlen(body);
    header.body_size = body_size;
    uint32_t header_int = create_response_header(&header);
    if (write(fd, &header_int, sizeof(uint32_t)) < 0) {
        perror("send header (send_create_user_response)");
        return -1;
    }
    if (write(fd, body, body_size) < 0) {
        perror("send body (send_create_user_response)");
        return -1;
    }
    printw("Success to send the res to %s/res-body:%s\n", clnt_addr, body);
    refresh();
    return 0;
}

int send_destroy_user_response(int fd, chat_header_t header, int result, const char* token, const char* clnt_addr)
{
    char body[DEFAULT_BUFFER] = {'\0', };
    if (result == 0) {
        strcpy(body, "204\3\0");
        strcat(body, token);
    } else {
        if (result == ERROR_DESTROY_USER_INVALID_FIELDS) {
            strcpy(body, "400\3\0");
            strcat(body, "Invalid request fields");
            strcat(body, token);
        } else if (result == ERROR_DESTROY_USER_USER_NOT_EXIST) {
            strcpy(body, "404\3\0");
            strcat(body, "User does not exist");
            strcat(body, token);
        } else if (result == ERROR_DESTROY_USER_INVALID_CREDENTIALS) {
            strcpy(body, "403\3\0");
            strcat(body, "Invalid credentials");
            strcat(body, token);
        } else {
            assert(!"should not be here");
        }
    }

    uint16_t body_size = strlen(body);
    header.body_size = body_size;
    uint32_t header_int = create_response_header(&header);
    if (write(fd, &header_int, sizeof(uint32_t)) < 0) {
        perror("send header (send_create_user_response)");
        return -1;
    }
    if (write(fd, body, body_size) < 0) {
        perror("send body (send_destroy_user_response)");
        return -1;
    }
    printw("Success to send the res to %s/res-body:%s\n", clnt_addr, body);
    refresh();
    return 0;
}

int send_login_user_response(int fd, chat_header_t header, int result, const char* token, const char* clnt_addr)
{
    char body[DEFAULT_BUFFER] = {'\0', };
    if (result == 0) {
        strcpy(body, "200\3\0");
        strcat(body, token);
    } else {
        if (result == ERROR_LOGIN_INVALID_CREDENTIALS) {
            strcpy(body, "403\3\0");
            strcat(body, "Login Token and password does not match");
            strcat(body, token);
        } else if (result == ERROR_LOGIN_DOES_NOT_EXIST) {
            strcpy(body, "400\3\0");
            strcat(body, "Invalid Login Token or password");
            strcat(body, token);
        } else {
            assert(!"should not be here");
        }
    }

    uint16_t body_size = strlen(body);
    header.body_size = body_size;
    uint32_t header_int = create_response_header(&header);
    if (write(fd, &header_int, sizeof(uint32_t)) < 0) {
        perror("send header (send_create_user_response)");
        return -1;
    }
    if (write(fd, body, body_size) < 0) {
        perror("send body (send_create_user_response)");
        return -1;
    }
    printw("Success to send the res to %s/res-body:%s\n", clnt_addr, body);
    refresh();
    return 0;
}

int send_logout_user_response(int fd, chat_header_t header, int result, const char* token, const char* clnt_addr)
{
    char body[DEFAULT_BUFFER] = {'\0', };
    if (result == 0) {
        strcpy(body, "200\3\0");
        strcat(body, token);
    } else {
        if (result == ERROR_LOGOUT_INVALID_FIELDS) {
            strcpy(body, "400\3\0");
            strcat(body, "Invalid fields provided");
            strcat(body, token);
        } else if (result == ERROR_LOGOUT_USER_MISMATCH_ADDRESS) {
            strcpy(body, "403\3\0");
            strcat(body, "Connection ip address does not match address in database");
            strcat(body, token);
        } else if (result == ERROR_LOGOUT_ADMIN_USER_NOT_EXIST) {
            strcpy(body, "404\3\0");
            strcat(body, "User does not exist");
            strcat(body, token);
        } else if (result == ERROR_LOGOUT_ADMIN_USER_NOT_ONLINE) {
            strcpy(body, "412\3\0");
            strcat(body, "User is not currently online");
            strcat(body, token);
        } else if (result == TERMINATE_AND_RESTABLISH_CONNECTION) {
            strcpy(body, "333\3\0");
            strcat(body, "Account has been accessed on another device");
            strcat(body, token);
        } else {
            assert(!"should not be here");
        }
    }

    uint16_t body_size = strlen(body);
    header.body_size = body_size;
    uint32_t header_int = create_response_header(&header);
    if (write(fd, &header_int, sizeof(uint32_t)) < 0) {
        perror("send header (send_create_user_response)");
        return -1;
    }
    if (write(fd, body, body_size) < 0) {
        perror("send body (send_create_user_response)");
        return -1;
    }

    printw("Success to send the res to %s/res-body:%s\n", clnt_addr, body);
    refresh();
    close(fd);
    return 0;
}

int send_create_channel_response(int fd, chat_header_t header, int result, const char* token, const char* clnt_addr)
{
    char body[DEFAULT_BUFFER] = {'\0', };
    if (result == 0) {
        strcpy(body, "201\3\0");
        strcat(body, token);
    } else {
        sprintf(body, "%d\3", result);
        if (result == ERROR_CREATE_CHANNEL_400) {
            strcat(body, "Invalid request\3");
        } else if (result == ERROR_CREATE_CHANNEL_404) {
            strcat(body, "Display name NOT found\3");
        } else if (result == ERROR_CREATE_CHANNEL_409) {
            strcat(body, "Channel name is not unique\3");
        } else if (result == ERROR_CREATE_CHANNEL_403) {
            strcat(body, "Display name does not match\3");
        } else if (result == ERROR_CREATE_CHANNEL_500) {
            strcat(body, "Internal server error\3");
        } else {
            printw("the result of read_and_create_channel is wrong\n");
            refresh();
            return -1;
        }
    }

    uint16_t body_size = strlen(body);
    header.body_size = body_size;
    uint32_t header_int = create_response_header(&header);

    if (write(fd, &header_int, sizeof(uint32_t)) < 0) {
        perror("send header (send_create_user_response)");
        return -1;
    }
    if (write(fd, body, body_size) < 0) {
        perror("send body (send_create_user_response)");
        return -1;
    }
    printw("Success to send the res to %s/res-body:%s\n", clnt_addr, body);
    refresh();
    return 0;
}

int send_create_message_response(int fd, chat_header_t header, int result, const char* token, const char* clnt_addr)
{
    char body[DEFAULT_BUFFER] = {'\0', };
    if (result == 0) {
        strcpy(body, "201\3\0");
        strcat(body, token);
    } else {
        if (result == ERROR_INVALID_REQUEST) {
            strcpy(body, "400\3\0");
            strcat(body, "Invalid request");
            strcat(body, token);
        } else if (result == ERROR_CHANNEL_DOES_NOT_EXIST) {
            strcpy(body, "404\3\0");
            strcat(body, "Channel does not exist");
            strcat(body, token);
        } else if (result == ERROR_ADMIN_DOES_NOT_EXIST) {
            strcpy(body, "404\3\0");
            strcat(body, "Referenced display name does not exist");
            strcat(body, token);
        } else if (result == ERROR_USER_DOES_NOT_EXIST) {
            strcpy(body, "403\3\0");
            strcat(body, "Referenced display name does not exist");
            strcat(body, token);
        } else if (result == SUCCESS_FORWARD_REQ) {
            strcpy(body, "200\3\0");
            strcat(body, token);
        }else {
            assert(!"should not be here");
        }
    }

    uint16_t body_size = strlen(body);
    header.body_size = body_size;
    uint32_t header_int = create_response_header(&header);
    if (write(fd, &header_int, sizeof(uint32_t)) < 0) {
        perror("send header (send_create_message_response)");
        return -1;
    }
    if (write(fd, body, body_size) < 0) {
        perror("send body (send_create_message_response)");
        return -1;
    }
    printw("Success to send the res to %s/res-body:%s\n", clnt_addr, body);
    refresh();
    return 0;
}

int get_num_connected_users(connected_user* cache)
{
    int n  = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (n <= MAX_CLIENTS && cache[i].dsply_name[0] == '\0') {
            return n;
        } else if (cache[i].dsply_name[0] != '\0') {
            n++;
        }
    }
    return n;
}

void insert_user_in_cache(int fd, connected_user* cache, user_account_t* connecting_user, user_login_t* login_info, int num_active_users)
{
    connected_user insert_user;
    memset(&insert_user, 0, sizeof(connected_user));

    strcpy(insert_user.dsply_name, connecting_user->display_name);
    strcpy(insert_user.ip_address, connecting_user->ip_addr);
    strcpy(insert_user.uuid, connecting_user->user_id);
    strcpy(insert_user.login_token, login_info->login_token);
    insert_user.fd = fd;
    insert_user.access_time = time(NULL);

    cache[num_active_users] = insert_user;
    num_active_users++;
    // TODO remove testing print statement
    printw("\nActive user count: %d\n", num_active_users);
    refresh();
    printw("====CACHE====\n");
    refresh();
    for (int i = 0; i < num_active_users; i++) {
        printw("dsply_name: %s  ip_addr: %s  fd: %d, access_time: %ld\n",
               cache[i].dsply_name, cache[i].ip_address, cache[i].fd, cache[i].access_time);
        refresh();
    }
}

void remove_user_in_cache(connected_user* cache, user_account_t* connecting_user, int num_active_users)
{
    for (int i = 0; i < num_active_users; i++) {
        if (strcmp(cache[i].dsply_name, connecting_user->display_name) == 0) {
            for (int j = i; j < num_active_users - 1; j++) {
                cache[j] = cache[j+1];
            }
            num_active_users--;
            break;
        }
    }

    // TODO remove testing print statement
    printw("\nActive user count: %d\n", num_active_users);
    refresh();
    printw("====CACHE====\n");
    refresh();
    for (int i = 0; i < num_active_users; i++) {
        printw("dsply_name: %s  ip_addr: %s  fd: %d, access_time: %ld\n",
               cache[i].dsply_name, cache[i].ip_address, cache[i].fd, cache[i].access_time);
        refresh();
    }
}

int find_duplicate_user(connected_user* cache, int active_users_count)
{
    int min_fd = 4;
    int found_duplicate = 0;

    for (int i = 0; i < active_users_count - 1; i++) {
        for (int j = i + 1; j < active_users_count; j++) {
            if (strcmp(cache[i].dsply_name, cache[j].dsply_name) == 0 &&
                strcmp(cache[i].ip_address, cache[j].ip_address) != 0) {
                found_duplicate = 1;
                int lower_fd = cache[i].fd < cache[j].fd ? cache[i].fd : cache[j].fd;
                if (lower_fd < min_fd) {
                    min_fd = lower_fd;
                }
            }
        }
    }
    return found_duplicate ? min_fd : -1;
}

connected_user* get_connected_user_by_display_name(connected_user* cache, const char* display_name)
{
    int num_connected_users = get_num_connected_users(cache);

    for (int i = 0; i < num_connected_users; i++) {
        if (strcmp(cache[i].dsply_name, display_name) == 0) {
            return &cache[i];
        }
    }

    return NULL;
}

bool find_connected_user_with_same_cred(user_account_t* user_account, connected_user* conn_users, int num_users, int fd)
{
    if (num_users == 0) {
        return false;
    }

    // iterate through the connected_users array and compare ip addr
    for (int i = 0; i < num_users; i++) {
        if (strcmp(user_account->ip_addr, conn_users[i].ip_address) == 0 &&
        strcmp(user_account->display_name, conn_users[i].dsply_name) == 0 && fd == conn_users[i].fd) {
            // Found a connected user with the same ip addr and dsply name
            return true;
        }
    }

    // no connected user with the same ip addr and dsply name combo found
    return false;
}

uint32_t create_response_header(const chat_header_t* header)
{
    uint32_t serialized_header;
    uint32_t version;
    uint32_t type;
    uint32_t object;
    uint32_t body_size;

    version = header->version_type.version;
    type = header->version_type.type;
    object = header->object;
    body_size = header->body_size;

    version <<= 28;
    type <<= 24;
    object <<= 16;

    serialized_header = (version | type | object | body_size);

    serialized_header = htonl(serialized_header);
    return serialized_header;
}

void view_active_users(connected_user* cache)
{
    int i = 0;
    int num_active_users = get_num_connected_users(cache);
    //Print # of active users
    if (cache[i].dsply_name[0] == '\0') {
        printw("There are no active users.\n");
        refresh();
    } else
    {
        printw("\nActive user count: %d\n", num_active_users);
        refresh();
        for (; i < num_active_users; i++)
        {

            printw("display_name: %s, fd: %d, uuid: %s\n",
                   cache[i].dsply_name, cache[i].fd, cache[i].uuid);
            refresh();
        }
    }
}

char* get_display_name_in_cache_malloc_or_null(const char ip_addr[CLNT_IP_ADDR_LENGTH])
{
    int active_user_count = get_num_connected_users(active_users);
    char* dis_name = NULL;
    for (int i = 0; i < active_user_count; i++) {
        if (strcmp(active_users[i].ip_address, ip_addr) == 0) {
            dis_name = (char*) malloc(TOKEN_NAME_LENGTH);
            memset(dis_name, '\0', TOKEN_NAME_LENGTH);
            strncpy(dis_name, active_users[i].dsply_name, TOKEN_NAME_LENGTH);
            return dis_name;
        }
    }
    return dis_name;
}
