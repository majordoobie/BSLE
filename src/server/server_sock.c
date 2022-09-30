#include <server_sock.h>
#include <stdatomic.h>

static volatile atomic_flag server_run;

static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port);
DEBUG_STATIC int server_listen(uint32_t port, socklen_t * record_len);
DEBUG_STATIC void serve_client(void * sock_void);
static void signal_handler(int signal);
static int get_ip_port(struct sockaddr * addr, socklen_t addr_size, char * host, char * port);

void start_server(uint32_t port_num)
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
            int * fd = (int *)malloc(sizeof(int));
            if (UV_INVALID_ALLOC == verify_alloc((fd)))
            {
                debug_print_err("[SERVER] Unable to allocate memory for fd "
                                "for connection %s:%s\n", host, service);
                close(client_fd);
            }
            else
            {
                *fd = client_fd;
                thpool_enqueue_job(thpool, serve_client, fd);
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
 * @brief This function is a thread callback. As soon as the server
 * receives a connection, the server will queue the connection into the
 * threadpool job queue. Once the job is dequeued, the thread will execute
 * this callback function. This is where the individual files are parsed
 * and returned to the client.
 *
 * @param sock_void Void pointer containing the connection file descriptor
 */
DEBUG_STATIC void serve_client(void * sock_void)
{
    int client_sock = *(int *)sock_void;

    printf("Thread got work\n");

    close(client_sock);
    free(sock_void);
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
