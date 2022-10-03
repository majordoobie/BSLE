#include <utils.h>

static uint64_t swap_byte_order(uint64_t val);

/*!
 * @brief Verify if the pointer passed in is NULL. If the pointer is NULL then
 * return UV_INVALID_ALLOC and set errno value to `ENOMEM`.
 *
 * @param ptr Pointer to newly allocated memory
 * @return UV status indicating if successful or failure allocation
 */
util_verify_t verify_alloc(void * ptr)
{
    if (NULL == ptr)
    {
        debug_print("%s", "[!] Unable to allocate memory\n");
        errno = ENOMEM;
        return UV_INVALID_ALLOC;
    }
    return UV_VALID_ALLOC;
}

/*!
 * @brief Function takes a 64bit int and converts it to network order. This is
 * done by checking the endianness of the machine
 *
 * @param val Value to swap to network order
 * @return Value in big endian
 */
uint64_t htonll(uint64_t val)
{
    return swap_byte_order(val);
}

/*!
 * @brief Function takes a 64bit int and converts it to host order. This is
 * done by checking the endianness of the machine
 *
 * @param val Value to swap to host order
 * @return Value in big endian
 */
uint64_t ntohll(uint64_t val)
{
    return swap_byte_order(val);
}

static uint64_t swap_byte_order(uint64_t val)
{
    // Source: https://tinyurl.com/bdhe4sy3
    // little endian if true
    int n = 1;
    if(1 == *(char *)&n)
    {
        val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
        val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
        val = (val << 32) | (val >> 32);
        return val;
    }
    return val;
}
