#include <utils.h>
#include <stdio.h>


util_verify_t verify_alloc(void * p_ptr)
{
    if (NULL == p_ptr)
    {
        debug_print("%s", "[!] Unable to allocate memory");
        return UV_INVALID_ALLOC;
    }
    return UV_VALID_ALLOC;
}
