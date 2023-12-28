#include "errors.h"

#ifdef __cplusplus 
extern "C" {
#endif

/**
  * Creates a new error of the type p_err with an error message p_msg and stores the result in p_errloc.
  * NOTE: At most DTG_MAX_MSG_SIZE bytes will be copied from p_msg. The string is guaranteed to be null terminated.
  * NOTE: p_msg is copied upon creation.
  */
void sc_set_error(sc_error* p_errloc, err_type_e p_type, const char* p_msg) {
    //only proceed if a valid error location was supplied
    if (p_errloc != NULL) {
	p_errloc->type = p_type;

	//copy the error message and always set the last char to the null terminator
	for (_uint i = 0; i < DTG_MAX_MSG_SIZE-1; ++i) {
	    p_errloc->msg[i] = p_msg[i];
	    if (p_errloc->msg[i] == 0) { break; }
	}
	p_errloc->msg[DTG_MAX_MSG_SIZE-1] = 0;
    }
}

/**
  * Resets the error to an empty message and type=E_SUCCESS
  */
void sc_reset_error(sc_error* p_errloc) {
    if (p_errloc) {
	p_errloc->type = E_SUCCESS;
	p_errloc->msg[0] = 0;
    }
}


/**
 * Tries copying at most n bytes of the string src into the destination dest. A null termination character is always written if dest has at least one character and a E_RANGE error may be thrown if n was not large enough to store src
 * Returns: the number of bytes written, excluding the null terminator
 */
inline size_t sc_strncpy(char* dest, const char* src, size_t n, sc_error* err) {
    size_t i = 0;
    for (; i < n-1; ++i) {
	dest[i] = src[i];
	if (src[i] == 0) { break; }
    }
    if (err) {
	if (i == n-1) {
	    err->type = E_RANGE;
	} else {
	    err->type = E_SUCCESS;
	}
    }
    dest[n-1] = 0;
    return i;
}

/**
 * Frees the memory pointed to by loc. NOTE: it is safe to call sc_free(NULL).
 */
inline void sc_free(void* loc) {
    if (loc) { free(loc); }
}

/**
 * Helper function which tries to allocate a block of memory of size buf_size or sets err in the event of a failure
 */
inline void* sc_malloc(size_t buf_size, sc_error* err) {
    void* ret = malloc(buf_size);

    if (err) {
	if (!ret) {
	    err->type = E_NOMEM;
	    snprintf(err->msg, DTG_MAX_MSG_SIZE, "couldn't allocate %d bytes", buf_size);
	} else {
	    err->type = E_SUCCESS;
	}
    }
    return ret;
}

/**
 * This helper function is similar to DTG_malloc() but accepts an additional parameter, ptr which is a pointer to the currently allocated block. This function attempts to expand the currently allocated block in place and only copies memory to a new location if necessary.
 */
void* sc_realloc(void* ptr, size_t buf_size, sc_error* err) {
    void* ret = realloc(ptr, buf_size);

    if (err) {
	if (!ret) {
	    err->type = E_NOMEM;
	    snprintf(err->msg, DTG_MAX_MSG_SIZE, "couldn't allocate %d bytes", buf_size);
	} else {
	    err->type = E_SUCCESS;
	}
    }
    return ret;
}

/**
 * Tries reading the string str as an integer or sets err on failure
 */
inline int sc_atoi(const char* str, sc_error* err) {
    long tmp = strtol(str , NULL, 0 );
    if (err) {
	if (errno == EINVAL) {
	    err->type = E_SYNTAX;
	    snprintf(err->msg, DTG_MAX_MSG_SIZE, "%s isn't a valid integer", str);
	} else if (errno == ERANGE) {
	    err->type = E_RANGE;
	    err->msg[0] = 0;
	} else {
	    err->type = E_SUCCESS;
	}
    }
    return tmp;
}

/**
 * Tries reading the string str as an integer or sets err on failure
 */
inline double sc_atof(const char* str, sc_error* err) {
    double tmp = strtod(str, NULL);
    if (err) {
	if (errno == EINVAL) {
	    err->type = E_SYNTAX;
	    snprintf(err->msg, DTG_MAX_MSG_SIZE, "%s isn't a valid integer", str);
	} else if (errno == ERANGE) {
	    err->type = E_RANGE;
	    err->msg[0] = 0;
	} else {
	    err->type = E_SUCCESS;
	}
    }
    return tmp;
}

/**
 * Returns the number of digits needed to represent the integer a in base b
 * NOTE: this is not guaranteed to be an exact figure, but it is guaranteed to be greater than or equal to the number of bytes written by DTG_itof.
 */
inline size_t get_int_digits(int a, int b) {
    if (b <= 0) { b = 10; }
    int tmp_i = a;
    if (tmp_i >= 0) {
	return (size_t)floor( log((double)tmp_i)/log(b) ) + 1;
    }
    return (size_t)floor( log((double)tmp_i)/log(b) ) + 2;
}

/**
 * Returns the number of digits needed to represent the floating point number a using n digits of precision. (Only base 10 is supported at present).
 * NOTE: this is not guaranteed to be an exact figure, but it is guaranteed to be greater than or equal to the number of bytes written by DTG_itof.
 */
inline size_t get_float_digits(double a, int n) {
    size_t ret = 0;

    if (a >= 0) {
	ret = (size_t)floor( log10((double)a) ) + n + 1;
    } else {
	ret = (size_t)floor( log10((double)a) ) + n + 2;
    }
    if (a > HI_SCIENTIFIC_THRESHOLD) {
	ret = DEF_FLOAT_PRECISION;
    }
    return ret;
}

/**
 * Tries writing the representation of the integer val to the string str filling at most n bytes.
 * param str: string to write to
 * param n: maximum number of bytes to write
 * int b: the base of the formatted string. If b <= 0 then the default value, 10 is used.
 * returns: number of characters actually written
 * WARNING: this function does not null terminate!
 */
inline size_t sc_itoa(int a, char* str, size_t n, int b, sc_error* err) {
    //figure out the number of digits
    int n_digits = get_int_digits(a, b);
    if (b <= 0) { b = 10; }
    if (b > 36) {
	sc_set_error(err, E_BADVAL, "can't use base larger than 36");
	return 0;
    }
    size_t i = n_digits - 1;
    _uint tmp = a;
    //handle negative numbers
    if (a <= 0) {
	tmp = -a;
	str[0] = '-';
	str += 1;//ignore the minus sign for the rest of this function
	--n_digits;
    }
    //iterate over the string to write
    while (tmp > 0 && i >= 0) {
	int digit = tmp % b;
	if (digit < 10) {
	    str[i] = '0' + digit;
	} else {
	    str[i] = 'A' + digit - 10;
	}

	//check that we don't write before the point we are allowed to
	if (i == 0 && tmp > b) {
	    sc_set_error(err, E_BADVAL, "not enough space to write string");
	    return 0;
	}
	--i;
	tmp /= b;
    }
    return n_digits;
}

/**
 * Tries writing the representation of the floating point number a to the string str filling at most n bytes.
 * param str: string to write to
 * param n: maximum number of bytes to write
 * int b: the base of the formatted string. If b <= 0 then the default value, 10 is used.
 * returns: number of characters actually written
 * WARNING: this function does not null terminate!
 */
inline size_t sc_ftoa(double a, char* str, size_t n, int precision, sc_error* err) {
    if (n < 2) {
	sc_set_error(err, E_BADVAL, "not enough space to write string");
	return 0;
    }
    //figure out the number of digits
    int n_digits = get_float_digits(a, n);
    size_t i = n_digits - 1;
    double tmp = a;
    //handle negative numbers
    if (a <= 0) {
	tmp = -a;
	str[0] = '-';
	str += 1;//ignore the minus sign for the rest of this function
	--n_digits;
    }

    //write exponents to a temporary string +1 for null terminator
    char exp[EXP_N_CHARS+1];
    int write_exp = 0;
    exp[0] = 'E';
    if (tmp >= HI_SCIENTIFIC_THRESHOLD) {
	exp[1] = '+';
	double log_tmp = floor(log10(tmp));
	size_t written = sc_itoa((int)log_tmp + 1, exp + 2, EXP_N_CHARS-2, 10, err);
	exp[written] = 0;
	tmp /= pow(10, log_tmp);//fix tmp to have only one point before the decimal
	write_exp = 1;
    } else if (tmp <= LO_SCIENTIFIC_THRESHOLD) {
	exp[1] = '-';
	double log_tmp = floor(-log10(tmp));
	size_t written = sc_itoa((int)log_tmp + 1, exp + 2, EXP_N_CHARS-2, 10, err);
	exp[written] = 0;
	tmp /= pow(10, log_tmp);
	write_exp = 1;
    }
    double tmp_flr = floor(tmp);
    size_t off = sc_itoa((int)tmp_flr, str, n, 10, err);
    if (off < n && tmp_flr != tmp) {
	str[off] = '.';
	double remain = tmp - tmp_flr;
	//this should be impossible but...	
	if (remain >= 1 || remain < 0) {
	    sc_set_error(err, E_BADVAL, "Invalid floating point number");
	    return off + 1;
	}

	while (off < n && remain != 0) {
	    remain *= 10;
	    str[off] = (char)floor(remain) + '0';
	    remain -= floor(remain);
	    ++off;
	}
    }
    if (write_exp) {
	if (off >= n - EXP_N_CHARS) { off = n - EXP_N_CHARS - 1; }
	for (size_t i = 0; i < EXP_N_CHARS && exp[i] != 0; ++i) {
	    str[off+i] = exp[i];
	    ++off;
	}
    }

    return off;
}

#ifdef __cplusplus 
}
#endif
