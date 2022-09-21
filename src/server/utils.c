#include <utils.h>
#include <stdio.h>


util_verify_t verify_alloc(void * ptr)
{
    if (NULL == ptr)
    {
        debug_print("%s", "[!] Unable to allocate memory");
        return UV_INVALID_ALLOC;
    }
    return UV_VALID_ALLOC;
}
