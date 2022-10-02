#include <server_ctrl.h>

static const char * OP_1 = "Server action was successful";
static const char * OP_2 = "Provided Session ID was invalid or expired";
static const char * OP_3 = "User associated with provided Session ID has insufficient permissions to perform the action";
static const char * OP_4 = "User could not be created because it already exists";
static const char * OP_5 = "File could not be created because it already exists";
static const char * OP_6 = "Username must be between 3 and 20 characters and password must be between 6 and 32 characters";
static const char * OP_7 = "Either username or password is incorrect";
static const char * OP_8 = "Directory is not empty, cannot remove";
static const char * OP_9 = "Path could not be resolved. This could be because it does not exist, or the path does not resolve within the home directory of the server";
static const char * OP_10 = "Path provided is not of type directory.";
static const char * OP_11 = "Path provided is not of type regular file.";
static const char * OP_12 = "Directory could not be created because it already exists";
static const char * OP_13 = "Network socket is closed, cannot read or send anymore data";
static const char * OP_14 = "User could not be removed because they do not exist";
static const char * OP_254 = "I/O error occurred during the action. This could be due to permissions, file not existing, or error while writing and reading.";
static const char * OP_255 = "Server action failed";


static const char * get_err_msg(ret_codes_t res);
static void set_resp(act_resp_t ** pp_resp, ret_codes_t code);
static ret_codes_t user_action(db_t * p_db, wire_payload_t * p_ld);
static ret_codes_t do_del_file(db_t * p_db, wire_payload_t * p_ld);
static ret_codes_t do_make_dir(db_t * p_db, wire_payload_t * p_ld);
static ret_codes_t do_put_file(db_t * p_db, wire_payload_t * p_ld);
static void do_get_file(db_t * p_db, wire_payload_t * p_ld, act_resp_t ** pp_resp);
static void do_list_dir(db_t * p_db,
                        wire_payload_t * p_ld,
                        act_resp_t ** pp_resp);


static act_resp_t * get_resp(void);


act_resp_t * ctrl_populate_resp(ret_codes_t code)
{
    act_resp_t * p_resp = get_resp();
    if (NULL == p_resp)
    {
        return NULL;
    }

    *p_resp = (act_resp_t){
        .msg    = get_err_msg(code),
        .result = code
    };

    return p_resp;
}

/*!
 * @brief Function handles authenticating the user and calling the correct
 * API to perform the action requested.
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @return Response object containing the response code, response message,
 * and a f_content object if available.
 */
act_resp_t * ctrl_parse_action(db_t * p_user_db, wire_payload_t * p_ld)
{
    if ((NULL == p_user_db) || (NULL == p_ld))
    {
        goto ret_null;
    }

    act_resp_t * p_resp = get_resp();
    if (NULL == p_resp)
    {
        goto ret_null;
    }

    // Authenticate the user
    user_account_t * p_user = NULL;
    ret_codes_t res = db_authenticate_user(p_user_db,
                                           &p_user,
                                           p_ld->p_username,
                                           p_ld->p_passwd);

    // If the user authentication failed, return the error code for it
    if (OP_SUCCESS != res)
    {
        *p_resp = (act_resp_t){
            .msg    = get_err_msg(res),
            .result = res
        };
        goto ret_resp;
    }

    // Check operations from most to least exclusive
    switch (p_ld->opt_code)
    {
        case ACT_LOCAL_OPERATION:
        {
            set_resp(&p_resp, OP_SUCCESS);
            goto ret_resp;
        }
        case ACT_USER_OPERATION:
            switch (p_ld->p_user_payload->user_flag)
            {
                case USR_ACT_CREATE_USER:
                {
                    // Ensure that the permission requested to be used
                    // for the new user is equal or less than the permission
                    // of the user performing the action
                    if (p_user->permission < p_ld->p_user_payload->user_perm)
                    {
                        set_resp(&p_resp, OP_PERMISSION_ERROR);
                        goto ret_resp;
                    }
                    set_resp(&p_resp, user_action(p_user_db, p_ld));
                    goto ret_resp;
                }
                case USR_ACT_DELETE_USER:
                {
                    // Removal of users can only be performed by admins
                    if (ADMIN != p_user->permission)
                    {
                        set_resp(&p_resp, OP_PERMISSION_ERROR);
                        goto ret_resp;
                    }
                    set_resp(&p_resp, user_action(p_user_db, p_ld));
                    goto ret_resp;
                }
                default:
                {
                    set_resp(&p_resp, OP_FAILURE);
                    goto ret_resp;
                }
            }


        case ACT_DELETE_REMOTE_FILE:
        {
            if (p_user->permission < READ_WRITE)
            {
                set_resp(&p_resp, OP_PERMISSION_ERROR);
                goto ret_resp;
            }
            set_resp(&p_resp, do_del_file(p_user_db, p_ld));
            goto ret_resp;
        }
        case ACT_MAKE_REMOTE_DIRECTORY:
        {
            if (p_user->permission < READ_WRITE)
            {
                set_resp(&p_resp, OP_PERMISSION_ERROR);
                goto ret_resp;
            }
            set_resp(&p_resp, do_make_dir(p_user_db, p_ld));
            goto ret_resp;
        }

        case ACT_PUT_REMOTE_FILE:
        {
            if (p_user->permission < READ_WRITE)
            {
                set_resp(&p_resp, OP_PERMISSION_ERROR);
                goto ret_resp;
            }
            set_resp(&p_resp, do_put_file(p_user_db, p_ld));
            goto ret_resp;
        }

        case ACT_LIST_REMOTE_DIRECTORY:
        {
            do_list_dir(p_user_db, p_ld, &p_resp);
            goto ret_resp;
        }
        case ACT_GET_REMOTE_FILE:
            do_get_file(p_user_db, p_ld, &p_resp);
            goto ret_resp;
        default:
        {
            set_resp(&p_resp, OP_FAILURE);
            goto ret_resp;
        }
    }

ret_resp:
    return p_resp;
ret_null:
    return NULL;
}

/*!
 * @brief Function reads the file on disk if found and saves the contents
 * to the response object
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @param pp_resp Double pointer to the response object. The status code,
 * status message and the data requested will be saved to this object
 */
static void do_get_file(db_t * p_db, wire_payload_t * p_ld, act_resp_t ** pp_resp)
{
    std_payload_t * p_std = p_ld->p_std_payload;
    verified_path_t * p_path = f_ver_path_resolve(p_db->p_home_dir, p_std->p_path);
    if (NULL == p_path)
    {
        set_resp(pp_resp, OP_RESOLVE_ERROR);
        return;
    }

    ret_codes_t code = OP_SUCCESS;
    file_content_t * p_content = f_read_file(p_path, &code);
    f_destroy_path(&p_path);
    if (NULL == p_content)
    {
        set_resp(pp_resp, code);
        return;
    }

    set_resp(pp_resp, OP_SUCCESS);
    (*pp_resp)->p_content = p_content;
    return;
}

/*!
 * @brief Function reads the directory contents of the path provided by the
 * payload and writes the information into the f_content field of the resp
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @param pp_resp Double pointer to the response object. The status code,
 * status message and the data requested will be saved to this object
 */
static void do_list_dir(db_t * p_db,
                        wire_payload_t * p_ld,
                        act_resp_t ** pp_resp)
{
    std_payload_t * p_std = p_ld->p_std_payload;
    verified_path_t * p_path = f_ver_path_resolve(p_db->p_home_dir, p_std->p_path);
    if (NULL == p_path)
    {
        set_resp(pp_resp, OP_RESOLVE_ERROR);
        return;
    }

    ret_codes_t code = OP_SUCCESS;
    file_content_t * p_content = f_list_dir(p_path, &code);
    f_destroy_path(&p_path);
    if (NULL == p_content)
    {
        set_resp(pp_resp, code);
        return;
    }

    set_resp(pp_resp, OP_SUCCESS);
    (*pp_resp)->p_content = p_content;
    return;
}

/*!
 * @brief Put the file on the server IF the file does not already exist. If
 * the file exist, return an error indicating so.
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @return Returns the action response
 */
static ret_codes_t do_put_file(db_t * p_db, wire_payload_t * p_ld)
{
    std_payload_t * p_std = p_ld->p_std_payload;
    verified_path_t * p_path = f_ver_path_resolve(p_db->p_home_dir, p_std->p_path);
    if (NULL != p_path)
    {
        f_destroy_path(&p_path);
        return OP_FILE_EXISTS;
    }
    p_path = f_ver_valid_resolve(p_db->p_home_dir, p_std->p_path);
    if (NULL == p_path) // This should not happen but just in case
    {
        return OP_RESOLVE_ERROR;
    }
    ret_codes_t ret = f_write_file(p_path,
                                   p_std->p_byte_stream,
                                   p_std->byte_stream_len);
    f_destroy_path(&p_path);
    return ret;

}

/*!
 * @brief Create the directory specified by the payload
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @return Returns the action response
 */
static ret_codes_t do_make_dir(db_t * p_db, wire_payload_t * p_ld)
{
    std_payload_t * p_std = p_ld->p_std_payload;
    verified_path_t * p_path = f_ver_valid_resolve(p_db->p_home_dir, p_std->p_path);
    if (NULL == p_path)
    {
        return OP_RESOLVE_ERROR;
    }
    ret_codes_t ret = f_create_dir(p_path);
    f_destroy_path(&p_path);
    return ret;
}

/*!
 * @brief Delete the file specified by the payload
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @return Returns the action response
 */
static ret_codes_t do_del_file(db_t * p_db, wire_payload_t * p_ld)
{
    std_payload_t * p_std = p_ld->p_std_payload;
    verified_path_t * p_path = f_ver_path_resolve(p_db->p_home_dir, p_std->p_path);
    if (NULL == p_path)
    {
        return OP_RESOLVE_ERROR;
    }
    ret_codes_t ret = f_del_file(p_path);
    f_destroy_path(&p_path);
    return ret;
}

/*!
 * @brief Function handles the user operations. The authentication and
 * permissions have already been checked before this point.
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @return Returns the action response
 */
static ret_codes_t user_action(db_t * p_db, wire_payload_t * p_ld)
{
    user_payload_t * p_usr_ld = p_ld->p_user_payload;
    switch (p_usr_ld->user_flag)
    {
        case USR_ACT_CREATE_USER:
        {
            return db_create_user(p_db,
                                  p_usr_ld->p_username,
                                  p_usr_ld->p_passwd,
                                  p_usr_ld->user_perm);
        }
        case USR_ACT_DELETE_USER:
        {
            return db_remove_user(p_db, p_usr_ld->p_username);
        }
        default:
        {
            return OP_FAILURE;
        }
    }
}

/*!
 * @brief Destroy the payload and response objects
 *
 * @param pp_payload Double pointer to the payload object
 * @param pp_res Double pointer to the response object
 */
void ctrl_destroy(wire_payload_t ** pp_payload, act_resp_t ** pp_res)
{
    if (NULL != pp_res)
    {
        destroy_resp(pp_res);
    }

    if ((NULL == pp_payload) || (NULL == *pp_payload))
    {
        return;
    }

    // Destroy the wire_payload_t
    wire_payload_t * p_payload = *pp_payload;

    // Destroy the union struct
    if (STD_PAYLOAD == p_payload->type)
    {
        std_payload_t * p_ld = p_payload->p_std_payload;
        if (NULL != p_ld->p_path)
        {
            free(p_ld->p_path);
        }
        if (NULL != p_ld->p_byte_stream)
        {
            free(p_ld->p_byte_stream);
        }
        *p_ld = (std_payload_t){
            .byte_stream_len = 0,
            .p_byte_stream   = NULL,
            .p_path          = NULL,
            .path_len        = 0,
        };
        free(p_ld);
        p_payload->p_std_payload = NULL;
    }
    else if (USER_PAYLOAD == p_payload->type)
    {
        user_payload_t * p_ld = p_payload->p_user_payload;
        if (NULL != p_ld->p_username)
        {
            free(p_ld->p_username);
        }
        if (NULL != p_ld->p_passwd)
        {
            free(p_ld->p_passwd);
        }
        *p_ld = (user_payload_t){
            .user_flag      = 0,
            .user_perm      = 0,
            .username_len   = 0,
            .p_username     = NULL,
            .passwd_len     = 0,
            .p_passwd       = NULL
        };
        free(p_ld);
        p_payload->p_user_payload = NULL;
    }

    free(p_payload->p_username);
    free(p_payload->p_passwd);
    *p_payload = (wire_payload_t){
        .opt_code       = 0,
        .username_len   = 0,
        .passwd_len     = 0,
        .session_id     = 0,
        .p_username     = NULL,
        .p_passwd       = NULL,
        .payload_len    = 0,
        .type           = 0,
    };

    free(p_payload);
    p_payload   = NULL;
    *pp_payload = NULL;
}

static void set_resp(act_resp_t ** pp_resp, ret_codes_t code)
{
    *(*pp_resp) = (act_resp_t){
        .msg    = get_err_msg(code),
        .result = code
    };
}
/*!
 * @brief Return the status code message for the status provided
 *
 * @param res ret_codes_t code provided by the response of the action
 * @return Message for the status code
 */
static const char * get_err_msg(ret_codes_t res)
{
    switch (res)
    {
        case OP_SUCCESS:
            return OP_1;
        case OP_SESSION_ERROR:
            return OP_2;
        case OP_PERMISSION_ERROR:
            return OP_3;
        case OP_USER_EXISTS:
            return OP_4;
        case OP_FILE_EXISTS:
            return OP_5;
        case OP_CRED_RULE_ERROR:
            return OP_6;
        case OP_USER_AUTH:
            return OP_7;
        case OP_DIR_NOT_EMPTY:
            return OP_8;
        case OP_RESOLVE_ERROR:
            return OP_9;
        case OP_PATH_NOT_DIR:
            return OP_10;
        case OP_PATH_NOT_FILE:
            return OP_11;
        case OP_DIR_EXISTS:
            return OP_12;
        case OP_SOCK_CLOSED:
            return OP_13;
        case OP_USER_NO_EXIST:
            return OP_14;
        case OP_IO_ERROR:
            return OP_254;
        default:
            return OP_255;
    }
}

static act_resp_t * get_resp(void)
{
    act_resp_t * p_resp = (act_resp_t *)malloc(sizeof(act_resp_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_resp))
    {
        return NULL;
    }
    *p_resp = (act_resp_t){
        .msg        = NULL,
        .result     = OP_SUCCESS,
        .p_content  = NULL
    };
    return p_resp;
}
