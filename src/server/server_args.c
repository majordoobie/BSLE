#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include <server.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>

DEBUG_STATIC uint32_t get_port(char * port);
DEBUG_STATIC uint32_t get_timeout(char * timeout);
static uint8_t str_to_long(char * str_num, long int * int_val);
char * get_home_dir(char * home_dir);
static void print_usage(void);
/*!
 * @brief Free the args object
 * @param args_p Double pointer to the args object
 */
void free_args(args_t ** args_p)
{
    if (NULL == args_p)
    {
        return;
    }
    args_t * args = *args_p;
    if (NULL == args)
    {
        return;
    }

    if (NULL != args->home_directory)
    {
        free(args->home_directory);
    }

    // NULL out values
    *args = (args_t){
        .home_directory = NULL,
        .timeout        = 0,
        .port           = 0
    };

    free(args);
    *args_p = NULL;
}

/*!
 * @brief Parse command line arguments to modify how th server is ran
 * @param argc Number of arguments  in the argv
 * @param argv Array of char arrays
 * @return Pointer to arg_t object if the args were valid otherwise NULL
 */
args_t * parse_args(int argc, char ** argv)
{
    // If not arguments are provided return NULL
    if (1 == argc)
    {
        fprintf(stderr, "[!] Must at least provide the home "
                        "directory to serve\n");
        return NULL;
    }

    args_t * args = (args_t *)malloc(sizeof(args_t));
    if (UV_INVALID_ALLOC == verify_alloc(args))
    {
        return NULL;
    }
    *args = (args_t){
        .port           = DEFAULT_PORT,
        .timeout        = DEFAULT_TIMEOUT,
        .home_directory = NULL
    };


    opterr = 0;
    int c = 0;

    // Flags indicating if the argument was provided
    bool port = false;
    bool timeout = false;
    bool home_dir = false;

    while ((c = getopt(argc, argv, "p:t:d:h")) != -1)
        switch (c)
        {
            case 'p':
                if (port)
                {
                    goto duplicate_args;
                }
                args->port = get_port(optarg);
                if (0 == args->port)
                {
                    goto cleanup;
                }
                port = true;
                break;
            case 't':
                if (timeout)
                {
                    goto duplicate_args;
                }
                args->timeout = get_timeout(optarg);
                if (0 == args->timeout)
                {
                    goto cleanup;
                }
                timeout = true;
                break;
            case 'd':
                if (home_dir)
                {
                    goto duplicate_args;
                }
                args->home_directory = get_home_dir(optarg);
                if (NULL == args->home_directory)
                {
                    goto cleanup;
                }
                home_dir = true;
                break;
            case 'h':
                print_usage();
                goto cleanup;
            case '?':
                if ((optopt == 'p') || (optopt == 'n'))
                {
                    fprintf(stderr,
                            "Option -%c requires an argument.\n",
                            optopt);
                }

                else if (isprint(optopt))
                {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                else
                {
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                }
                goto cleanup;
        }

    // If there is anything else to process, error out
    if (optind != argc)
    {
        goto cleanup;
    }

    // If the home dir was not provided, error out
    if (NULL == args->home_directory)
    {
        fprintf(stderr, "[!] -d argument is mandatory\n");
        goto cleanup;
    }
    return args;

duplicate_args:
    fprintf(stderr, "[!] Duplicate arguments provided\n");
cleanup:
    free_args(&args);
    return NULL;
}
static void print_usage(void)
{
    printf("Start up a file transfer server and server up the files "
           "located in the home directory which is specified by the -d argument."
           "\n"
           "All operations must first be authenticated. Once authenticated "
           "a session ID is assigned to the connection until the connection "
           "terminates or the session times out. After which the user must "
           "re-authenticate."
           "\n\n"
           "options:\n"
           "\t-t\tSession timeout in seconds (default: 10s)\n"
           "\t-p\tPort number to listen on (default: 31337)\n"
           "\t-d\tHome directory of the server. Path must have read and write "
           "permissions.\n");
}

/*!
 * @brief Verify that the path provided is a directory with read and write
 * permissions.
 *
 * @param home_dir Path to the home directory for the server
 * @return Pointer to the allocated string or NULL if failure
 */
char * get_home_dir(char * home_dir)
{
    struct stat stat_buff;
    if (-1 == stat(home_dir, &stat_buff))
    {
        perror("home directory");
        return NULL;
    }

    // Check if the file path is a directory
    if (!S_ISDIR(stat_buff.st_mode))
    {
        fprintf(stderr, "[!] Provided home directory path "
                        "is not a directory\n");
        return NULL;
    }

    // Check that we have at least read and write access to the directory
    else if (!(stat_buff.st_mode & R_OK) || !(stat_buff.st_mode & W_OK))
    {
        fprintf(stderr, "[!] Home directory must have READ "
                        "and WRITE permissions\n");
        return NULL;
    }

    else if (stat_buff.st_mode & X_OK)
    {
        fprintf(stderr, "[!] Directory must not have execution "
                        "rights\n");
        return NULL;
    }

    // Allocate the memory for the path and return the pointer to the path
    char * path = strdup(home_dir);
    if (UV_INVALID_ALLOC == verify_alloc(path))
    {
        return NULL;
    }
    return path;
}

/*!
 * @brief Convert the string argument into a integer
 * @param timeout Timeout parameter to convert
 * @return 0 if failure or a uint32_t integer
 */
DEBUG_STATIC uint32_t get_timeout(char * timeout)
{
    long int converted_timeout = 0;
    int result = str_to_long(timeout, &converted_timeout);

    // 0 indicates a conversion error
    if (0 == result)
    {
        return 0;
    }

    // Check that the value is not larger than an uint32max. We cannot
    // guarantee that the value is 32 bits
    else if (converted_timeout > UINT32_MAX)
    {
        fprintf(stderr, "[!] Provided timeout exceeds limit of "
                        "%u seconds\n", UINT32_MAX);
        return 0;
    }
    else if (converted_timeout < 0)
    {
        fprintf(stderr, "[!] Timeout value cannot be a negative "
                        "integer\n");
        return 0;
    }

    return (uint32_t)converted_timeout;
}

/*!
 * @brief Convert the provided port argument into a valid port number this
 * includes ensuring that the port value is not less than the 1024 range
 *
 * @param port Port value to convert
 * @return 0 if failure or a valid uint32_t integer
 */
DEBUG_STATIC uint32_t get_port(char * port)
{
    long int converted_port = 0;
    int result = str_to_long(port, &converted_port);

    // If 0 is returned, return 0 indicating an error
    if (0 == result)
    {
        return 0;
    }

    // Return error if the value converted is larger than a port value
    if ((converted_port > MAX_PORT) || (converted_port < MIN_PORT))
    {
        return 0;
    }

    return (uint32_t)converted_port;
}

/*!
 * @brief Function is mostly a replica of the strtol help menu to convert a
 * string into a long int
 *
 * @param str_num String to convert
 * @param int_val The result value
 * @return 0 if failure or 1 if successful
 */
static uint8_t str_to_long(char * str_num, long int * int_val)
{
    //To distinguish success/failure after call
    errno = 0;
    char * endptr = {0};
    *int_val = strtol(str_num, &endptr, 10);

    // Check for errors
    if (0 != errno)
    {
        return 0;
    }

    // No digits were found in the string to convert
    if (endptr == str_num)
    {
        return 0;
    }

    // If there are any extra characters, return 0
    if (* endptr != '\0')
    {
        return 0;
    }

    return 1;
}
