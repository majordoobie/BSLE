#include <server_sock.h>
#include <stdatomic.h>

static volatile atomic_flag server_run;

typedef struct
{
    db_t * p_db;
    uint32_t timeout;
    uint32_t session_id;
    int fd;
} worker_payload_t;

static int server_listen(uint32_t serv_port, socklen_t * record_len);
static void serve_client(void * sock_void);
static void signal_handler(int signal);
static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port);
static void destroy_worker_pld(worker_payload_t ** pp_ld);
static wire_payload_t * read_client_req(worker_payload_t * p_ld);
static ret_codes_t read_stream(int fd, void * payload, size_t bytes_to_read);
static void write_response(worker_payload_t * p_ld, act_resp_t * p_resp);

// Readability functions
static bool std_payload_has_file(uint64_t payload_len, uint16_t path_len);
static bool user_payload_has_password(uint64_t payload_len, uint16_t username_len);
static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port);
static uint64_t get_file_stream_size(uint64_t payload_len, uint16_t path_len);

static ret_codes_t make_byte_array(worker_payload_t * p_ld,
                                   uint8_t ** pp_byte_array,
                                   uint64_t array_len,
                                   bool make_string);
static ret_codes_t read_client_user_payload(worker_payload_t * p_ld,
                                            wire_payload_t * p_wire);
static ret_codes_t read_client_std_payload(worker_payload_t * p_ld,
                                           wire_payload_t * p_wire);
static const char * action_to_string(act_t code);
void start_server(db_t * p_db, uint32_t port_num, uint8_t timeout)
{

    // Make the server start listening
    int server_socket = server_listen(port_num, 0);
    if (-1 == server_socket)
    {
        goto ret_null;
    }

    // Initialize the thread pool for the connections of clients
    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    thpool_t * thpool = thpool_init((uint8_t)number_of_processors);
    if (NULL == thpool)
    {
        goto cleanup_sock;
    }

    // Set up SIGINT signal handling
    struct sigaction signal_action;
    memset(&signal_action, 0, sizeof(signal_action));
    signal_action.sa_handler = signal_handler;

    // Make the system call to change the action taken by the process on receipt
    // of the signal
    if (-1 == (sigaction(SIGINT, &signal_action, NULL)))
    {
        debug_print_err("%s\n", "Unable to set up signal handler");
        goto cleanup_thpool;
    }
    if (-1 == (sigaction(SIGPIPE, &signal_action, NULL)))
    {
        fprintf(stderr , "Unable to set up signal handler\n");
        goto cleanup_thpool;
    }

    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_storage);

    atomic_flag_test_and_set(&server_run);
    while (atomic_flag_test_and_set(&server_run))
    {
        // Clear the client_addr before the next connection
        memset(&client_addr, 0, addr_size);
        int client_fd = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (-1 == client_fd)
        {
            debug_print_err("Failed to accept: %s\n", strerror(errno));
        }
        else
        {
            char host[NI_MAXHOST];
            char service[NI_MAXSERV];
            if (0 == get_ip_port((struct sockaddr *)&client_addr, addr_size, host, service))
            {
                debug_print("[SERVER] Received connection from %s:%s\n", host, service);
            }
            else
            {
                printf("[SERVER] Received connection from unknown peer\n");
            }
            worker_payload_t * w_pld = (worker_payload_t *)malloc(sizeof(worker_payload_t));
            if (UV_INVALID_ALLOC == verify_alloc(w_pld))
            {
                debug_print_err("[SERVER] Unable to allocate memory for fd "
                                "for connection %s:%s\n", host, service);
                close(client_fd);
            }
            else
            {
                *w_pld = (worker_payload_t){
                    .timeout    = timeout,
                    .fd         = client_fd,
                    .p_db       = p_db,
                };
                thpool_enqueue_job(thpool, serve_client, w_pld);
            }
        }
    }

    // Wait for all the jobs to finish
    thpool_wait(thpool);

    // Close the server
    close(server_socket);
    thpool_destroy(&thpool);

cleanup_thpool:
    thpool_destroy(&thpool);
cleanup_sock:
    close(server_socket);
ret_null:
    return;
}

/*!
 * @brief Handle the registered signals
 */
static void signal_handler(int signal)
{
    if (SIGPIPE == signal)
    {
        return;
    }

    debug_print("%s\n", "[SERVER] Gracefully shutting down...");
    atomic_flag_clear(&server_run);
}

/*!
 * @brief Start listening on the serv_port provided on quad 0s. The file descriptor
 * for the socket is returned
 *
 * @param serv_port Port number to listen on
 * @param record_len Populated with the size of the sockaddr. This value is
 * dependant on the structure used. IPv6 structures are larger.
 * @return Either -1 for failure or 0 for success
 */
static int server_listen(uint32_t serv_port, socklen_t * record_len)
{
    // Convert the serv_port number into a string. The serv_port number is already
    // verified, so we do not need to double-check it here
    char port_string[10];
    snprintf(port_string, 10, "%d", serv_port);

    // Set up the hints structure to specify the parameters we want
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints = (struct addrinfo) {
        .ai_canonname = NULL,
        .ai_addr      = NULL,
        .ai_next      = NULL,
        .ai_socktype  = SOCK_STREAM,
        .ai_family    = AF_UNSPEC,        // Allows IPv4 or IPv6
        .ai_flags     = AI_PASSIVE,       // Use wildcard IP address (quad 0)
    };

    struct addrinfo * network_record_root;
    struct addrinfo * network_record;
    int sock_fd, enable_setsockopt;


    // Request socket structures that meet our criteria
    if (0 != getaddrinfo(NULL, port_string, &hints, &network_record_root))
    {

        debug_print_err("[SERVER] Unable to fetch socket structures: %s\n", strerror(errno));
        return -1;
    }

    /* Walk through returned list until we find an address structure
       that can be used to successfully create and bind a socket */

    enable_setsockopt = 1;
    for (network_record = network_record_root; network_record != NULL; network_record = network_record->ai_next) {

        // Using the current network record, attempt to create a socket fd
        // out of it. If it fails, grab the next one
        sock_fd = socket(network_record->ai_family,
                         network_record->ai_socktype,
                         network_record->ai_protocol);
        if (-1 == sock_fd)
        {
            continue;
        }

        // Attempt to modify the socket to be used for listening
        if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable_setsockopt, sizeof(enable_setsockopt)))
        {
            close(sock_fd);
            freeaddrinfo(network_record_root);
            return -1;
        }

        if (0 == bind(sock_fd, network_record->ai_addr, network_record->ai_addrlen))
        {
            break; // If successful bind, break we are done
        }

        debug_print("[SERVER] bind error: %s. Trying next record\n", strerror(errno));

        // If we get here, then we failed to make the current network_record
        // work. Close the socker fd and fetch the next one
        close(sock_fd);
    }

    // If network_record is still NULL, then we failed to make all the
    // network records work
    if (NULL == network_record)
    {
        debug_print_err("%s\n", "[SERVER] Unable to bind to any socket");
        freeaddrinfo(network_record_root);
        return -1;
    }

    if (-1 == listen(sock_fd, BACK_LOG))
    {
        debug_print_err("[SERVER] LISTEN: %s\n", strerror(errno));
        freeaddrinfo(network_record_root);
        return -1;
    }

    char host[NI_MAXHOST];
    char port[NI_MAXSERV];
    if (0 == get_ip_port(network_record->ai_addr, network_record->ai_addrlen, host, port))
    {
        debug_print("[SERVER] Listening on %s:%s\n", host, port);
    }
    else
    {
        debug_print("%s\n", "[SERVER] Unknown ip and serv_port listening on");
    }


    // Set the size of the structure used. This could change based on the
    // ip version used
    if ((NULL != network_record) && (NULL != record_len))
    {
        *record_len = network_record->ai_addrlen;
    }
    freeaddrinfo(network_record_root);

    return (network_record == NULL) ? -1 : sock_fd;
}


/*!
 * @brief This function is a thread callback. As soon as the server
 * receives a connection, the server will queue the connection into the
 * threadpool job queue. Once the job is dequeued, the thread will execute
 * this callback function. This is where the individual files are parsed
 * and returned to the client.
 *
 * The function creates a critical object called the worker_payload_t or p_ld
 * that contains the necessary information to properly perform
 * server operations (sending/receiving) such as the socket fd itself.
 *
 * @param sock_void Void pointer containing the connection file descriptor
 */
static void serve_client(void * sock_void)
{
    worker_payload_t * p_ld = (worker_payload_t *)sock_void;
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = p_ld->timeout;

    int str_err = setsockopt(
        p_ld->fd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        (const char*)&tv,
        sizeof(tv)
    );
    if (-1 == str_err)
    {
        fprintf(stderr, "[!] Unable to set client sockets timeout\n");
        goto ret_null;
    }

    // If we get a null then we know that some kind of error occurred and
    // has been handled
    wire_payload_t * p_wire = read_client_req(p_ld);
    if (NULL == p_wire)
    {
        goto ret_null;
    }

    act_resp_t * resp = ctrl_parse_action(p_ld->p_db, p_wire);
    write_response(p_ld, resp);
    ctrl_destroy(&p_wire, &resp);

ret_null:
    destroy_worker_pld(&p_ld);
    return;
}

/*!
 * @brief Function destroys the worker payload
 *
 * @param pp_ld Double pointer to the worker payload
 */
static void destroy_worker_pld(worker_payload_t ** pp_ld)
{
    if ((NULL == pp_ld) || (NULL == *pp_ld))
    {
        return;
    }
    worker_payload_t * p_ld = *pp_ld;
    close(p_ld->fd);

    *p_ld = (worker_payload_t){
        .fd         = 0,
        .p_db       = NULL,
        .timeout    = 0
    };
    free(p_ld);
    *pp_ld = NULL;
    return;
}


/*!
 * @brief Function performs the "reading" portion of the client communications.
 * It parses out the headers to return a wire_payload_t. The wire_payload_t
 * contains a union that holds the type of payload sent which is either
 * the std_payload_t (file stuffs) or user_payload_t (user creations)
 *
 * @param p_ld Pointer to the worker_payload object
 * @return wire_payload_t object
 */
static wire_payload_t * read_client_req(worker_payload_t * p_ld)
{
    /*
     *
     * 0               1               2               3
     * 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |     OPCODE    |   USER_FLAG   |           RESERVED            |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |        USERNAME_LEN           |        PASSWORD_LEN           |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                          SESSION_ID                           |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                    **USERNAME + PASSWORD**                    |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                          PAYLOAD_LEN ->                       |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                       <- PAYLOAD_LEN                          |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                ~user_payload || std_payload~                  |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    wire_payload_t * p_wire = (wire_payload_t *)calloc(1, sizeof(wire_payload_t));
    act_resp_t * resp = NULL;
    if (UV_INVALID_ALLOC == verify_alloc(p_wire))
    {
        goto ret_null;
    }

    ret_codes_t result = read_stream(p_ld->fd, &p_wire->opt_code, H_OPCODE);
    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }

    result = read_stream(p_ld->fd, &p_wire->user_flag, H_USER_FLAG);
    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }

    result = read_stream(p_ld->fd, &p_wire->_reserved, H_RESERVED);
    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }

    result = read_stream(p_ld->fd, &p_wire->username_len, H_USERNAME_LEN);
    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }
    p_wire->username_len = ntohs(p_wire->username_len);

    result = read_stream(p_ld->fd, &p_wire->passwd_len, H_PASSWORD_LEN);
    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }
    p_wire->passwd_len = ntohs(p_wire->passwd_len);

    result = read_stream(p_ld->fd, &p_wire->session_id, H_SESSION_ID);
    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }
    p_wire->session_id = ntohl(p_wire->session_id);

    result = make_byte_array(p_ld,
                             (uint8_t **)& p_wire->p_username,
                             p_wire->username_len,
                             true);

    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }
    result = make_byte_array(p_ld,
                             (uint8_t **)&p_wire->p_passwd,
                             p_wire->passwd_len,
                             true);

    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }

    result = read_stream(p_ld->fd, &p_wire->payload_len, H_PAYLOAD_LEN);
    if (OP_SUCCESS != result)
    {
        goto failure_response;
    }
    p_wire->payload_len = ntohll(p_wire->payload_len);

    if (ACT_USER_OPERATION == p_wire->opt_code)
    {
        debug_print("%s\n", "[WORKER - READ_CLIENT] Parsing user_payload "
                            "in client request");
        result = read_client_user_payload(p_ld, p_wire);
        if (OP_SUCCESS != result)
        {
            goto failure_response;
        }
    }
    else if (ACT_LOCAL_OPERATION != p_wire->opt_code)
    {
        debug_print("%s\n", "[WORKER - READ_CLIENT] Parsing std_payload "
                            "in client request");
        result = read_client_std_payload(p_ld, p_wire);
        if (OP_SUCCESS != result)
        {
            goto failure_response;
        }
    }
    debug_print("[WORKER - READ_CLIENT] Inner payload type: %d\n", p_wire->opt_code);
    return p_wire;

failure_response:
    resp = ctrl_populate_resp(result);
    if (NULL == resp)
    {
        goto cleanup;
    }
    debug_print("%s\n", "[WORKER - READ_CLIENT] Failed to read from client,"
                        " sending a response packet");
    write_response(p_ld, resp);
cleanup:
    ctrl_destroy(&p_wire, NULL);
ret_null:
    return NULL;
}

static ret_codes_t read_client_std_payload(worker_payload_t * p_ld,
                                           wire_payload_t * p_wire)
{
    ret_codes_t result = OP_FAILURE;
    p_wire->p_std_payload = (std_payload_t *)calloc(1, sizeof(std_payload_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_wire->p_std_payload))
    {
        goto ret_null;
    }
    std_payload_t * p_load =  p_wire->p_std_payload;

    /*
     * 0               1               2               3
     * 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |          PATH_LEN             |         **PATH_NAME**         |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                     **FILE_DATA_STREAM**                      |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */

    result = read_stream(p_ld->fd, &p_load->path_len, H_PATH_LEN);
    if (OP_SUCCESS != result)
    {
        goto ret_null;
    }
    p_load->path_len = htons(p_load->path_len);

    result = make_byte_array(p_ld,
                             (uint8_t **)&p_load->p_path,
                             p_load->path_len,
                             true);

    if (OP_SUCCESS != result)
    {
        goto ret_null;
    }

    if (std_payload_has_file(p_wire->payload_len, p_load->path_len))
    {
        result = make_byte_array(p_ld,
                                 &p_load->p_hash_stream,
                                 H_HASH_LEN,
                                 false);
        if (OP_SUCCESS != result)
        {
            goto ret_null;
        }

        p_load->byte_stream_len = get_file_stream_size(p_wire->payload_len,
                                                       p_load->path_len);

        result = make_byte_array(p_ld,
                                 &p_load->p_byte_stream,
                                 p_load->byte_stream_len,
                                 false);
        if (OP_SUCCESS != result)
        {
            goto ret_null;
        }
    }

    debug_print("[~] Parsed std payload:\n"
                "[~]    Command:   %s\n"
                "[~]    PATH_LEN:  %d\n"
                "[~]    PATH_NAME: %s\n"
                "[~]    FileLen:   %ld\n",
                action_to_string(p_wire->opt_code),
                p_load->path_len,
                p_load->p_path,
                p_load->byte_stream_len
                );

    return OP_SUCCESS;

ret_null:
    return result;
}

static ret_codes_t read_client_user_payload(worker_payload_t * p_ld,
                                            wire_payload_t * p_wire)
{
    ret_codes_t result = OP_FAILURE;
    p_wire->type = USER_PAYLOAD;

    // Create the payload portion that goes inside the wire_payload_t
    p_wire->p_user_payload = (user_payload_t *)calloc(1, sizeof(user_payload_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_wire->p_user_payload))
    {
        goto ret_null;
    }
    user_payload_t * p_load =  p_wire->p_user_payload;

    /*
     * 0               1               2               3
     * 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |  USR_ACT_FLAG |   PERMISSION  |          USERNAME_LEN         |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * | **USERNAME**  |         PASSWORD_LEN          | **PASSWORD**  |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    result = read_stream(p_ld->fd, &p_load->user_flag, H_USR_ACT_FLAG);
    if (OP_SUCCESS != result)
    {
        goto ret_null;
    }

    result = read_stream(p_ld->fd, &p_load->user_perm, H_USR_PERMISSION);
    if (OP_SUCCESS != result)
    {
        goto ret_null;
    }

    result = read_stream(p_ld->fd, &p_load->username_len, H_USERNAME_LEN);
    if (OP_SUCCESS != result)
    {
        goto ret_null;
    }
    p_load->username_len = ntohs(p_load->username_len);

    result = make_byte_array(p_ld,
                             (uint8_t **)&p_load->p_username,
                             p_load->username_len,
                             true);
    if (OP_SUCCESS != result)
    {
        goto ret_null;
    }

    // Only "Create user" commands have the password field filled
    if (user_payload_has_password(p_wire->payload_len, p_load->username_len))
    {
        result = read_stream(p_ld->fd, &p_load->passwd_len, H_PASSWORD_LEN);
        if (OP_SUCCESS != result)
        {
            goto ret_null;
        }
        p_load->passwd_len = htons(p_load->passwd_len);

        result = make_byte_array(p_ld,
                                 (uint8_t **)&p_load->p_passwd,
                                 p_load->passwd_len,
                                 true);
    }
    debug_print("[~] Parsed user payload:\n"
           "[~]    USER_OP: %s\n"
           "[~]    O_PERM:  %d\n"
           "[~]    O_User:  %s\n"
           "[~]    O_Pass:  %s\n",
           (USR_ACT_CREATE_USER == p_load->user_flag) ? "CREATE" : "DELETE",
           p_load->user_perm,
           p_load->p_username,
           (NULL != p_load->p_passwd) ? p_load->p_passwd : "None");

    return result;

ret_null:
    return result;
}

/*!
 * @brief Function handles writing the response message to the client after
 * their request has been parsed. All responses have the same header to
 * include the "**MSG**" which is a string representing the status of the
 * operation.
 *
 * The file data stream is an optional section that is only used with
 * LS command and GET command holding that data stream to return in the
 * format for (BYTE_STREAM_HASH)(BYTE_STREAM).
 *
 * @param p_ld Pointer to the worker_payload_t object
 * @param p_resp act_resp_t contains the data that has been created
 * by the user after it processed the user request. It will contain all
 * the information that was requested even if it's just an error message.
 */
static void write_response(worker_payload_t * p_ld, act_resp_t * p_resp)
{
    /*
     *   0               1               2               3
     *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  | RETURN_CODE   |    RESERVED   |          SESSION_ID->         |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |       <- SESSION_ID           |         PAYLOAD_LEN ->        |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |                      <- PAYLOAD_LEN ->                        |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |       <- PAYLOAD_LEN          |    MSG_LEN     |   **MSG**    |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |                    **FILE DATA STREAM**                       |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */

    // Complete size of the whole response packet limited to 2048 bytes
    size_t pkt_msg_size = H_RETURN_CODE + H_RESERVED +
                       H_SESSION_ID + H_PAYLOAD_LEN + H_MSG_LEN;

    // msg is guaranteed to be null terminated
    size_t msg_len = strlen(p_resp->msg);
    pkt_msg_size += msg_len;

    // The size of the payload portion of the packet see the README.md
    size_t payload_len = msg_len + H_MSG_LEN;

    // Size of the data stream which is limited to 1016 bytes per packet
    size_t data_stream_size = 0;
    if (NULL != p_resp->p_content)
    {
        pkt_msg_size    += p_resp->p_content->stream_size;
        pkt_msg_size    += p_resp->p_content->p_hash->size;

        payload_len      += p_resp->p_content->stream_size;
        payload_len      += p_resp->p_content->p_hash->size;

        data_stream_size += p_resp->p_content->stream_size;
        data_stream_size += p_resp->p_content->p_hash->size;
    }

    uint8_t * p_stream = (uint8_t *)calloc(pkt_msg_size, sizeof(uint8_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_stream))
    {
        goto ret_null;
    }

    size_t offset = 0;
    memcpy(p_stream, &p_resp->result, H_RETURN_CODE);
    offset += H_RETURN_CODE;

    uint16_t reserved = 0;
    memcpy((p_stream + offset), &reserved, H_RESERVED);
    offset += H_RESERVED;

    uint32_t session_id = htonl(p_ld->session_id);
    memcpy((p_stream + offset), &session_id, H_SESSION_ID);
    offset += H_SESSION_ID;

    uint64_t pld_size = htonll(payload_len);
    memcpy((p_stream + offset), &pld_size, H_PAYLOAD_LEN);
    offset += H_PAYLOAD_LEN;

    memcpy((p_stream + offset), &msg_len, H_MSG_LEN);
    offset += H_MSG_LEN;

    memcpy((p_stream + offset), p_resp->msg, msg_len);
    offset += msg_len;

    if (NULL != p_resp->p_content)
    {
        memcpy((p_stream + offset), p_resp->p_content->p_hash->array, p_resp->p_content->p_hash->size);
        offset += p_resp->p_content->p_hash->size;

        memcpy((p_stream + offset), p_resp->p_content->p_stream, p_resp->p_content->stream_size);
    }

    // Data is going to be sent two segments to facilitate the packet size
    // limit for the message response packet of 2048 and the file data stream
    // for 1016 bytes per packet. So the first everything but the file data
    // stream
    offset                  = 0;
    ssize_t sent_bytes      = 0; // Number of bytes to send per packet
    ssize_t total_sent      = 0; // Total number of bytes sent so far
    size_t total_send_size  = pkt_msg_size - data_stream_size; // total to send

    // If total bytes to send is less than the MAX size to send, then
    // set to send value to the total_send_size. Otherwise, set it to
    // the MAX_MSG_SIZE of 2048
    size_t send_size = (total_send_size < MAX_MSG_SIZE) ? total_send_size
                                                        : MAX_MSG_SIZE;
    for (;;)
    {
        sent_bytes = write(p_ld->fd, (p_stream + offset), send_size);
        if (-1 == sent_bytes)
        {
            debug_print_err("%s\n", strerror(errno));
            goto cleanup;
        }

        total_sent += sent_bytes;
        if (total_sent == total_send_size)
        {
            debug_print("[WORKER - RESP] Responded with %ld bytes\n", total_sent);
            break;
        }

        offset += (size_t)sent_bytes;
        send_size = total_send_size - (size_t)total_sent;
        send_size = (send_size < MAX_MSG_SIZE) ? send_size : MAX_MSG_SIZE;
    }

    // If there is a file stream send it while taking into account
    // the file size limit of 1016 bytes
    if (data_stream_size > 0)
    {
        total_send_size = data_stream_size;
        send_size = (total_send_size < MAX_FILE_SIZE) ? total_send_size
                                                      : MAX_FILE_SIZE;
        for (;;)
        {
            sent_bytes = write(p_ld->fd, (p_stream + offset), send_size);
            if (-1 == sent_bytes)
            {
                debug_print_err("%s\n", strerror(errno));
                goto cleanup;
            }

            total_sent += sent_bytes;
            if (total_sent == total_send_size)
            {
                debug_print("[WORKER - RESP] Responded with %ld bytes\n", total_sent);
                break;
            }

            offset += (size_t)sent_bytes;
            send_size = total_send_size - (size_t)total_sent;
            send_size = (send_size < MAX_FILE_SIZE) ? send_size : MAX_FILE_SIZE;
        }
    }
    free(p_stream);
    return;

cleanup:
    free(p_stream);
ret_null:
    return;
}

/*!
 * @brief Function handles reading from the file descriptor and populates the
 * void pointer supplied with using the bytes_to_read parameter
 *
 * @param fd
 * @param payload
 * @param bytes_to_read
 * @return
 */
static ret_codes_t read_stream(int fd, void * payload, size_t bytes_to_read)
{
    ssize_t read_bytes;
    ssize_t total_bytes_read = 0;
    ret_codes_t res = OP_FAILURE;

    if (0 == bytes_to_read)
    {
        res = OP_SUCCESS;
        goto ret_null;
    }

    if (NULL == payload)
    {
        goto ret_null;
    }

    // Buffer will be used to read from the file descriptor
    uint8_t * read_buffer = (uint8_t *)calloc(1, bytes_to_read + 1);
    if (UV_INVALID_ALLOC == verify_alloc(read_buffer))
    {
        debug_print_err("%s\n", "Unable to allocate memory for read_buffer");
        goto ret_null;
    }

    uint8_t * payload_buffer = (uint8_t *)calloc(1, bytes_to_read + 1);
    if (UV_INVALID_ALLOC == verify_alloc(payload_buffer))
    {
        debug_print_err("%s\n", "Unable to allocate memory for read_buffer");
        goto clean_read_buff;
    }

    bool keep_reading = true;
    while (keep_reading)
    {
        read_bytes = read(fd, read_buffer, 1);
        if (-1 == read_bytes)
        {
            // If timed out, display message indicating that it timed out
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                debug_print("%s\n", "[STREAM READ] Read timed out");
                res = OP_SESSION_ERROR;
            }
            else
            {
                debug_print_err("[STREAM READ] Unable to read from fd: %s\n", strerror(errno));
            }
            goto clean_buffers;
        }
        else if (0 == read_bytes)
        {
            debug_print_err("%s\n", "[STREAM READ] Read zero bytes. Client likely closed connection.");
            res = OP_SOCK_CLOSED;
            goto clean_buffers;
        }

        // Use string operations to concatenate the final payload buffer
        memcpy(payload_buffer + total_bytes_read, read_buffer, (unsigned long)read_bytes);
        total_bytes_read = total_bytes_read + read_bytes;

        if (total_bytes_read == bytes_to_read)
        {
            keep_reading = false;
        }
    }

    // Copy the data read into the callers buffer
    memcpy(payload, payload_buffer, bytes_to_read);

    // Free the working buffers
    free(read_buffer);
    free(payload_buffer);
    return OP_SUCCESS;

clean_buffers:
    free(payload_buffer);
clean_read_buff:
    free(read_buffer);
ret_null:
    return res;
}




/*
 *
 * Expressive functions used to increase readability
 *
 */

/*!
 * @brief Function is just used for readability sakes
 */
static bool user_payload_has_password(uint64_t payload_len,
                                      uint16_t username_len)
{
    uint32_t r_operand = H_USR_ACT_FLAG + H_USR_PERMISSION + H_USERNAME_LEN + username_len;
    if (payload_len <= r_operand)
    {
        return false;
    }
    return true;
}

/*!
 * @brief Function is just used for readability sakes
 */
static bool std_payload_has_file(uint64_t payload_len, uint16_t path_len)
{
    return ((payload_len - path_len) >= H_HASH_LEN);
}

/*!
 * @brief Safely calculate the amount of bytes left in the payload
 */
static uint64_t get_file_stream_size(uint64_t payload_len, uint16_t path_len)
{
    uint32_t r_operand = path_len + H_HASH_LEN;
    if (payload_len < r_operand)
    {
        return 0;
    }
    return payload_len - r_operand;
}

/*!
 * @brief Function is used to create the strings provided in the payload.
 *
 * @param p_ld Thread worker structure object
 * @param pp_byte_array Double pointer to where the string will be set to
 * @param array_len Length of the string
 * @return Server ret code
 */
static ret_codes_t make_byte_array(worker_payload_t * p_ld,
                                   uint8_t ** pp_byte_array,
                                   uint64_t array_len,
                                   bool make_string)
{
    if (0 == array_len)
    {
        return OP_SUCCESS;
    }

    uint8_t * p_array = NULL;
    ret_codes_t result = OP_FAILURE;
    if (make_string)
    {
        p_array = (uint8_t *)calloc(array_len, sizeof(uint8_t) + 1);
    }
    else
    {
        p_array = (uint8_t *)calloc(array_len, sizeof(uint8_t));
    }

    if (UV_INVALID_ALLOC == verify_alloc(p_array))
    {
        goto ret_null;
    }

    result = read_stream(p_ld->fd, p_array, array_len);
    if (OP_SUCCESS != result)
    {
        goto ret_null;
    }

    if (make_string)
    {
        p_array[array_len] = '\0';
    }

    *pp_byte_array = p_array;
    return OP_SUCCESS;

ret_null:
    return result;
}

/*!
 * @brief Return the port used from the connection
 */
static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port)
{
    return getnameinfo(addr, addr_size, host, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICSERV);

}

static const char * action_to_string(act_t code)
{
    switch (code)
    {
        case ACT_USER_OPERATION:
            return "USER_OPERATION";
        case ACT_DELETE_REMOTE_FILE:
            return "DELETE_REMOTE_FILE";
        case ACT_LIST_REMOTE_DIRECTORY:
            return "LIST_REMOTE_DIR";
        case ACT_GET_REMOTE_FILE:
            return "GET_REMOTE_FILE";
        case ACT_MAKE_REMOTE_DIRECTORY:
            return "MAKE_REMOTE_DIR";
        case ACT_PUT_REMOTE_FILE:
            return "PUT_REMOTE_FILE";
        case ACT_LOCAL_OPERATION:
            return "LOCAL_OP";
        default:
            return "UNKNOWN";
    }
}
