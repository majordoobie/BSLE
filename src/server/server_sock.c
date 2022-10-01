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

static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port);
static int server_listen(uint32_t port, socklen_t * record_len);
static void serve_client(void * sock_void);
static void signal_handler(int signal);
static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port);
static void destroy_worker_pld(worker_payload_t ** pp_w_pld);
static wire_payload_t * read_client_req(worker_payload_t * pld);
static ret_codes_t read_stream(int fd, void * payload, size_t bytes_to_read);
uint64_t swap_byte_order(uint64_t val);

static void write_response(worker_payload_t * p_ld, act_resp_t * p_resp);
void start_server(db_t * p_db, uint32_t port_num, uint32_t timeout)
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
 * @brief Start listening on the port provided on quad 0s. The file descriptor
 * for the socket is returned
 * @param port Port number to listen on
 * @param record_len Populated with the size of the sockaddr. This value is
 * dependant on the structure used. IPv6 structures are larger.
 * @return Either -1 for failure or 0 for success
 */
DEBUG_STATIC int server_listen(uint32_t port, socklen_t * record_len)
{
    // Convert the port number into a string. The port number is already
    // verified, so we do not need to double-check it here
    char port_string[10];
    snprintf(port_string, 10, "%d", port);

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
    char service[NI_MAXSERV];
    if (0 == get_ip_port(network_record->ai_addr, network_record->ai_addrlen, host, service))
    {
        debug_print("[SERVER] Listening on %s:%s\n", host, service);
    }
    else
    {
        debug_print("%s\n", "[SERVER] Unknown ip and port listening on");
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

static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port)
{
    return getnameinfo(addr, addr_size, host, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICSERV);

}

static void signal_handler(int signal)
{
    if (SIGPIPE == signal)
    {
        return;
    }

    debug_print("%s\n", "[SERVER] Gracefully shutting down...");
    atomic_flag_clear(&server_run);
}

static void destroy_worker_pld(worker_payload_t ** pp_w_pld)
{
    if ((NULL == pp_w_pld) || (NULL == *pp_w_pld))
    {
        return;
    }


    worker_payload_t * p_w_pld = *pp_w_pld;
    close(p_w_pld->fd);

    *p_w_pld = (worker_payload_t){
        .fd         = 0,
        .p_db       = NULL,
        .timeout    = 0
    };
    free(p_w_pld);
    *pp_w_pld = NULL;
    return;
}

/*!
 * @brief This function is a thread callback. As soon as the server
 * receives a connection, the server will queue the connection into the
 * threadpool job queue. Once the job is dequeued, the thread will execute
 * this callback function. This is where the individual files are parsed
 * and returned to the client.
 *
 * @param sock_void Void pointer containing the connection file descriptor
 */
static void serve_client(void * sock_void)
{
    worker_payload_t * p_pld = (worker_payload_t *)sock_void;
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = p_pld->timeout;

    int str_err = setsockopt(
        p_pld->fd,
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
    wire_payload_t * p_wire = read_client_req(p_pld);
    if (NULL == p_wire)
    {
        goto ret_null;
    }

    ctrl_destroy(&p_wire, NULL);



ret_null:
    destroy_worker_pld(&p_pld);
    return;
}

static wire_payload_t * read_client_req(worker_payload_t * pld)
{
    wire_payload_t * p_wire = (wire_payload_t *)calloc(1, sizeof(wire_payload_t));
    if (UV_INVALID_ALLOC == verify_alloc(p_wire))
    {
        goto ret_null;
    }

    ret_codes_t result = read_stream(pld->fd, &p_wire->opt_code, H_OPCODE);
    if (OP_SUCCESS != result)
    {
        act_resp_t * resp = ctrl_populate_resp(result);
        if (NULL == resp)
        {
            goto cleanup;
        }
        debug_print("%s\n", "[WORKER - READ_CLIENT] Failed to read from client,"
                            " sending a response packet");
        write_response(pld, resp);
        goto cleanup;
    }

    printf("Got an op code of %d\n", p_wire->opt_code);

cleanup:
    ctrl_destroy(&p_wire, NULL);
ret_null:
    return NULL;
}

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

    uint8_t reserved = 0;
    memcpy((p_stream + offset), &reserved, H_RESERVED);
    offset += H_RESERVED;

    uint32_t session_id = htonl(p_ld->session_id);
    memcpy((p_stream + offset), &session_id, H_SESSION_ID);
    offset += H_SESSION_ID;

    uint64_t pld_size = swap_byte_order(payload_len);
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

/*!
 * @brief Perform a byte order swap of a 64 bit value
 * @param val Value to swap
 * @return Swapped value
 */
uint64_t swap_byte_order(uint64_t val)
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    val = (val << 32) | (val >> 32);
    return val;
}
