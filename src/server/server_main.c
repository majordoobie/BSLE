#include <server_main.h>

int main(int argc, char ** argv)
{
    args_t * p_args = parse_args(argc, argv);
    if (NULL == p_args)
    {
        return 0;
    }

    printf("got the p_args\n");
    free_args(&p_args);
}
