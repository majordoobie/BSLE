#ifndef BSLE_GALINDEZ_SRC_SERVER_UTILS_H_
#define BSLE_GALINDEZ_SRC_SERVER_UTILS_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus

/*!
 * DEBUG_PRINT - Used to print debugging statements to stderr only when
 * the release mode is set to debug
 *
 * DEBUG_STATIC - Used to make static functions public when the release
 * mode is in debug. The purpose is to be able to unit test functions
 * that are meant to be private.
 */
#ifdef NDEBUG
#   define DEBUG_PRINT 0
#   define DEBUG_STATIC static
#else
#   define DEBUG_PRINT 1
#   define DEBUG_STATIC
#endif // End of DEBUG_PRINT

#include <stdio.h>
#include <errno.h>
#include <stdint.h>


/*
 * Enable printing debug messages when in debug mode. To print a non text
 * replacement you have to use debug_print("%s\n", "My text") otherwise
 * it is used just like any other printf variant
 * debug_print("My test %s\n", "more text");
 *
 * The debug_print_err adds the file and line number to the output for more
 * information when wanting to debug.
 */
#define debug_print_err(fmt, ...) \
        do { if (DEBUG_PRINT) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define debug_print(fmt, ...) \
            do { if (DEBUG_PRINT) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

typedef enum util_verify_t
{
    UV_VALID_ALLOC,
    UV_INVALID_ALLOC
} util_verify_t;

/*
 * Function is used to verify that a pointer is not NULL. This is used to
 * check allocations of memory. A stderr message is printed and a util_verify_t
 * is returned
 */
util_verify_t verify_alloc(void * p_ptr);

/*!
 * @brief Function takes a 64bit int and converts it to host order. This is
 * done by checking the endianness of the machine
 *
 * @param val Value to swap to host order
 * @return Value in big endian
 */

uint64_t ntohll(uint64_t val);
/*!
 * @brief Function takes a 64bit int and converts it to network order. This is
 * done by checking the endianness of the machine
 *
 * @param val Value to swap to network order
 * @return Value in big endian
 */
uint64_t htonll(uint64_t val);

#ifdef __cplusplus
}
#endif //END __cplusplus
#endif //BSLE_GALINDEZ_SRC_SERVER_UTILS_H_
