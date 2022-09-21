#include <stdio.h>

#include <server.h>

int main(int argc, char ** argv)
{
    args_t * args = parse_args(argc, argv);
    if (NULL == args)
    {
        return 0;
    }

    printf("got the args\n");
    free_args(&args);
}
