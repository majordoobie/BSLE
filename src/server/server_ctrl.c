#include <server_ctrl.h>

static const char * OP_1 = "Server action was successful";
static const char * OP_2 = "Provided Session ID was invalid or expired";
static const char * OP_3 = "User associated with provided Session ID has insufficient permissions to perform the action";
static const char * OP_4 = "User could not be created because it already exists";
static const char * OP_5 = "File could not be created because it already exists";
static const char * OP_6 = "Username must be between 3 and 20 characters and password must be between 6 and 32 characters";
static const char * OP_7 = "Either p_username or password is incorrect";
static const char * OP_8 = "Directory is not empty, cannot remove";
static const char * OP_255 = "Server action failed";


static const char * get_err_msg(ret_codes_t res);
static void set_resp(act_resp_t ** pp_resp, ret_codes_t code);

static ret_codes_t user_action(db_t * p_db, wire_payload_t * p_ld);


/*!
 * @brief Function handles authenticating the user and calling the correct
 * API to perform the action requested.
 *
 * @param p_user_db Pointer to the user_db object
 * @param p_ld Pointer to the wire_payload object
 * @return Response object containing the response code and the response message
 */
act_resp_t * ctrl_parse_action(db_t * p_user_db, wire_payload_t * p_ld)
{
    act_resp_t * p_resp = (act_resp_t *)malloc(sizeof(act_resp_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_resp))
    {
        goto ret_null;
    }
    *p_resp = (act_resp_t){
        .msg    = NULL,
        .result = OP_SUCCESS
    };

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
        case ACT_USER_OPERATION:
            switch (p_ld->p_user_payload->user_flag)
            {
                case USR_ACT_CREATE_USER:
                {
                    if (p_user->permission < p_ld->p_user_payload->user_perm)
                    {
                        set_resp(&p_resp, OP_PERMISSION_ERROR);
                        goto ret_resp;
                    }
                    res = user_action(p_user_db, p_ld);
                }
                case USR_ACT_DELETE_USER:
                {
                    if (ADMIN != p_ld->p_user_payload->user_perm)
                    {
                        set_resp(&p_resp, OP_PERMISSION_ERROR);
                        goto ret_resp;
                    }
                    //TODO Call a deletion function call
                }
                default:
                {
                    set_resp(&p_resp, OP_FAILURE);
                    goto ret_resp;
                }
            }
        case ACT_LIST_REMOTE_DIRECTORY:break;
        case ACT_GET_REMOTE_FILE:break;

        case ACT_DELETE_REMOTE_FILE:
        {
            if (p_user->permission < READ_WRITE)
            {
                set_resp(&p_resp, OP_PERMISSION_ERROR);
                goto ret_resp;
            }
            // TODO Call function
        }
        case ACT_MAKE_REMOTE_DIRECTORY:
        {
            if (p_user->permission < READ_WRITE)
            {
                set_resp(&p_resp, OP_PERMISSION_ERROR);
                goto ret_resp;
            }
        }
        case ACT_PUT_REMOTE_FILE:
        {
            if (p_user->permission < READ_WRITE)
            {
                set_resp(&p_resp, OP_PERMISSION_ERROR);
                goto ret_resp;
            }
        }
        default:
        {
            set_resp(&p_resp, OP_FAILURE);
            goto ret_resp;
        }
    }


    // Set success and return
    set_resp(&p_resp, OP_SUCCESS);
    return p_resp;

ret_resp:
    return p_resp;
ret_null:
    return NULL;
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
            ret_codes_t resp = db_create_user(p_db,
                                              p_usr_ld->p_username,
                                              p_usr_ld->p_passwd,
                                              p_usr_ld->user_perm);
            return resp;
        }
        case USR_ACT_DELETE_USER:
        {
            ret_codes_t resp = db_remove_user(p_db, p_usr_ld->p_username);
            return resp;
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
    if ((NULL == pp_res) || (NULL == *pp_res))
    {
        return;
    }

    // Destroy the act_resp_t
    **pp_res = (act_resp_t){
        .msg    = NULL,
        .result = 0
    };
    free(*pp_res);
    *pp_res = NULL;

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
        free(p_ld->p_path);
        free(p_ld->p_byte_stream);
        *p_ld = (std_payload_t){
            .byte_stream_len = 0,
            .p_byte_stream   = NULL,
            .p_path          = NULL,
            .path_len        = 0,
        };
        free(p_ld);
        p_payload->p_std_payload = NULL;
    }
    else
    {
        user_payload_t * p_ld = p_payload->p_user_payload;
        free(p_ld->p_username);
        free(p_ld->p_passwd);
        *p_ld = (user_payload_t){
            .user_flag      = 0,
            .user_perm      = 0,
            .username_len   = 0,
            .p_username       = NULL,
            .passwd_len     = 0,
            .p_passwd         = NULL
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
        .p_username       = NULL,
        .p_passwd         = NULL,
        .payload_len    = 0,
        .type           = 0,
    };

    free(p_payload);
    p_payload = NULL;
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
        default:
            return OP_255;
    }
}
