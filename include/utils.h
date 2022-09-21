#ifndef BSLE_GALINDEZ_SRC_SERVER_UTILS_H_
#define BSLE_GALINDEZ_SRC_SERVER_UTILS_H_
#ifdef __cplusplus
extern "C" {
#endif //END __cplusplus

/*
 * Macro creates a variable used to enable or disable print statements
 * depending on if the application was compiled in either debug or released mode
 */
#ifdef NDEBUG
#define DEBUG_PRINT 0
#else
#define DEBUG_PRINT 1
#endif // End of DEBUG_PRINT


/*
 * Enable private function to be used publicly when in debug release mode
 * This makes it possible to unit test functions that are normally meant to
 * be private
 */
#ifdef NDEBUG
#define DEBUG_STATIC static
#else
#define DEBUG_STATIC
#endif // END of static replacement

#include <stdio.h>


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
util_verify_t verify_alloc(void * ptr);

#ifdef __cplusplus
}
#endif //END __cplusplus
#endif //BSLE_GALINDEZ_SRC_SERVER_UTILS_H_
