#ifndef DTG_ERROR_H
#define DTG_ERROR_H

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus 
extern "C" {
#endif

#define DTG_MAX_MSG_SIZE	127

#define DEF_FLOAT_PRECISION	16
#define EXP_N_CHARS		5
//all floats above this constant in absolute value are represented in scientific notion (E+n)
#define HI_SCIENTIFIC_THRESHOLD	1000000000.0
//all floats below this constant in absolute value are represented in scientific notion (E+n)
#define LO_SCIENTIFIC_THRESHOLD	0.000001

typedef unsigned int _uint;
typedef unsigned int _uint32;

typedef enum {
  E_SUCCESS = 0,
  E_NOMEM,
  E_RANGE,
  E_UNDEF,
  E_SYNTAX,
  E_BADVAL,
  E_BADTYPE,
  E_STACK_OVERFLOW,
  E_STACK_UNDERFLOW,
  E_UNEXPECT_CHAR,
  N_ERRORS} err_type_e;

typedef struct ssc_error {
    err_type_e type;
    char msg[DTG_MAX_MSG_SIZE];
} sc_error;

/**
  * Creates a new error of the type p_err with an error message p_msg and stores the result in p_errloc.
  * NOTE: At most DTG_MAX_MSG_SIZE bytes will be copied from p_msg. The string is guaranteed to be null terminated.
  * NOTE: p_msg is copied upon creation.
  */
void sc_set_error(sc_error* p_errloc, err_type_e p_type, const char* p_msg);

/**
  * Resets the error to an empty message and type=E_SUCCESS
  */
void sc_reset_error(sc_error* p_errloc);

/**
 * Tries copying at most n bytes of the string src into the destination dest. A null termination character is always written if dest has at least one character and a RANGE error may be thrown if n was not large enough to store src
 * Returns: the number of bytes written, excluding the null terminator
 */
size_t sc_strncpy(char* dest, const char* src, size_t n, sc_error* err);

void sc_free(void* loc);

/**
 * Helper function which tries to allocate a block of memory of size buf_size or sets err in the event of a failure
 */
void* sc_malloc(size_t buf_size, sc_error* err);

/**
 * This helper function is similar to DTG_malloc() but accepts an additional parameter, ptr which is a pointer to the currently allocated block. This function attempts to expand the currently allocated block in place and only copies memory to a new location if necessary.
 */
void* sc_realloc(void* ptr, size_t buf_size, sc_error* err);

/**
 * Tries reading the string str as an integer or sets err on failure
 */
int sc_atoi(const char* str, sc_error* err);

/**
 * Tries reading the string str as an integer or sets err on failure
 */
double sc_atof(const char* str, sc_error* err);

/**
 * Returns the number of digits needed to represent the integer a in base b
 * NOTE: this is not guaranteed to be an exact figure, but it is guaranteed to be greater than or equal to the number of bytes written by DTG_itof.
 */
size_t get_int_digits(int a, int b);

/**
 * Returns the number of digits needed to represent the floating point number a using n digits of precision. (Only base 10 is supported at present).
 * NOTE: this is not guaranteed to be an exact figure, but it is guaranteed to be greater than or equal to the number of bytes written by DTG_itof.
 */
size_t get_float_digits(double a, int n);

/**
 * Tries writing the representation of the integer val to the string str filling at most n bytes.
 * param str: string to write to
 * param n: maximum number of bytes to write
 * int b: the base of the formatted string. If b <= 0 then the default value, 10 is used.
 * returns: number of characters actually written
 * WARNING: this function does not null terminate!
 */
size_t sc_itoa(int a, char* str, size_t n, int b, sc_error* err);

/**
 * Tries writing the representation of the floating point number a to the string str filling at most n bytes.
 * param str: string to write to
 * param n: maximum number of bytes to write
 * int b: the base of the formatted string. If b <= 0 then the default value, 10 is used.
 * returns: number of characters actually written
 * WARNING: this function does not null terminate!
 */
size_t sc_ftoa(double a, char* str, size_t n, int precision, sc_error* err);

#ifdef __cplusplus 
}
#endif

#endif //DTG_ERROR_H
