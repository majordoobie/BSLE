#include <server_main.h>

int main(int argc, char ** argv)
{
    args_t * p_args = args_parse(argc, argv);
    if (NULL == p_args)
    {
        goto ret_null;
    }

    db_t * p_db = db_init(p_args->p_home_directory);
    if (NULL == p_db)
    {
        goto cleanup_args;
    }
    p_args->p_home_directory = NULL; // p_db consumes the pointer

    start_server(p_db, p_args->port, p_args->timeout);

    db_shutdown(&p_db);
    args_destroy(&p_args);
    return 0;

cleanup_args:
    args_destroy(&p_args);
ret_null:
    return -1;
}
