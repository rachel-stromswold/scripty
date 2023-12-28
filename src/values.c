#include "values.h"

#ifdef __cplusplus 
extern "C" {
#endif

/**
 * Returns the length of the value p_val. Arrays and strings return the number of elements while primitive types all return 1
 */
size_t len(value p_val) {
    switch (p_val.type) {
    case VT_STRING: return p_val.val.str->size;
    case VT_ARRAY: return ((Array*)p_val.val.ptr)->size;
    default: return 1;
    }
}

/**
 * Helper function that returns the string where contents for the string type value p_val are stored. This function is only valid for strings, for arrays use _get_buf(). In the event that this function is called for a non string type NULL is returned.
 * WARNING: calls to grow() or append() may invalidate the buffer
 */
/*char* _get_char_buf(value p_val); {
    if (p_val.type != VT_STRING) { return NULL; }
    return ((String*)p_val.val)->buf;
}*/

/**
 * Frees the value pointed to by p_val. If p_val is an array like object, then free_val will properly free the stored array as well. In most cases, end users shouldn't need to call lower level functions like free_Array().
 */
void free_value(value* p_val) {
    if (p_val) {

    if (p_val->type == VT_STRING) {
	String* tmp_str = p_val->val.str;
	if (tmp_str) {
	    if (tmp_str->buf) { free(tmp_str->buf); }
	    sc_free(tmp_str);
	}
    } else if (p_val->type == VT_ARRAY) {
	Array* tmp_arr = (Array*)(p_val->val.ptr);
	free_Array(tmp_arr);
	sc_free(p_val->val.ptr);
	/*if (tmp_arr) {
	    if (tmp_arr->buf) { free(tmp_arr->buf); }
	    free_Array(tmp_arr);
	}*/
    }

    }
}

/**
 * Perform a deep copy of the value p_val. This function is guaranteed not to error.
 */
value v_deep_copy(value p_val, sc_error* err) {
    value ret = {0};
    ret.type = p_val.type;
    switch (p_val.type) {
    case VT_ERROR: 
    ret.val.ptr = sc_malloc(sizeof(sc_error), err);
    ((sc_error*)ret.val.ptr)->type = ((sc_error*)p_val.val.ptr)->type;
    strncpy(((sc_error*)ret.val.ptr)->msg, ((sc_error*)p_val.val.ptr)->msg, DTG_MAX_MSG_SIZE);
    break;
    case VT_CHAR: //internally chars bools and ints all use an int type for storage
    case VT_BOOL:
    case VT_INT: ret.val.i = p_val.val.i; break;
    case VT_FLOAT: ret.val.f = p_val.val.f; break;
    case VT_STRING:
    ret.val.str = (String*)sc_malloc(sizeof(String), err);
    ret.val.str->buf_size = p_val.val.str->size;
    ret.val.str->size = p_val.val.str->size;
    ret.val.str->buf = (char*)sc_malloc(sizeof(char)*(ret.val.str->buf_size), err);
    strncpy(ret.val.str->buf, p_val.val.str->buf, ret.val.str->size);
    break;
    case VT_ARRAY:
    Array* p_arr = (Array*)p_val.val.ptr;
    Array* ret_arr = (Array*)sc_malloc(sizeof(Array), err);
    *ret_arr = _copy_a(*p_arr, err);
    ret.val.ptr = ret_arr;
    }

    return ret;
}

/**
 * Returns the length in bytes of the stringified version of a value. (Because of UTF-8 a byte is not necessarily equivalent to a character).
 */
size_t get_format_string_size(value p_val, size_t precision) {
    switch (p_val.type) {
    case VT_ERROR:
    return strlen( ((sc_error*)p_val.val.ptr)->msg );

    case VT_CHAR:
    _uint32 tmp_ui = (_uint)p_val.val.i;
    //check if we need unicode formatting
    if (tmp_ui < 128) { return 1; }
    if (tmp_ui < 0x0FFF) { return 2; }
    if (tmp_ui < 0x010000) { return 3; }
    if (tmp_ui < 0x110000) { return 4; }
    return 8;

    case VT_BOOL:
    if (p_val.val.i == 0) { return 5; }
    return 4;

    case VT_INT:
    int tmp_i = p_val.val.i;
    if (tmp_i >= 0) {
	return (size_t)floor(log10( (double)tmp_i )) + 1;
    }
    return (size_t)floor(log10( (double)(-tmp_i) )) + 2;

    case VT_FLOAT:
    double tmp_f = p_val.val.f;
    if (tmp_f >= 0) {
	//add precision and the decimal
	return (size_t)floor(log10( (double)tmp_f )) + precision + 2;
    }
    return (size_t)floor(log10( (double)tmp_f )) + precision + 3;

    case VT_STRING: return p_val.val.str->size + 1;

    case VT_ARRAY:
    size_t ret = 3;//characters for [] and null termination
    Array* arr = (Array*)p_val.val.ptr;
    for (size_t i = 0; i < arr->size; ++i) {
	ret += get_format_string_size(arr->buf[i], precision);
	if (i < arr->size - 1) { ret += 2; }// add characters for ", " separators
    }
    return ret;

    default: return DEFAULT_STRING_SIZE;
    }
}

/**
 * Reads an input stream str into a value of type hint. If hint is set to 0 (VT_ERROR), the type is inferred based on inputs.
 * NOTE: boolean values are by default interpreted as true. To set a boolean to false str must either be '0' or 'false'.
 * WARNING: str is modified in place and an address returned. You should duplicate the string before calling this function if you need to use the original string.
 */
value read_value_string(char* str, Valtype_e hint, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    if (hint) {
	switch (hint) {
	case VT_INT:
	ret.type = VT_INT;
	ret.val.i = sc_atoi(str, err);
	return ret;

	case VT_CHAR:
	ret.type = VT_CHAR;
	ret.val.i = sc_atoi(str, err);
	return ret;

	case VT_BOOL:
	ret.type = VT_BOOL;
	ret.val.i = 1;
	//try reading a boolean value
	if (str[0] == '1') { ret.val.i = 1;return ret; }
	if (str[0] == '0') { ret.val.i = 0;return ret; }
	if (strcmp(str, "true") == 0) { ret.val.i = 1;return ret; }
	if (strcmp(str, "false") == 0) { ret.val.i = 0;return ret; }
	//if we couldn't a valid boolean keyword, then set an error
	ret.type = VT_ERROR;
	sc_set_error(err, E_SYNTAX, "");
	snprintf(err->msg, DTG_MAX_MSG_SIZE, "unrecognized keyword %s", str);
	return ret;

	case VT_STRING:
	ret = v_make_string(str, err);
	if (err->type != E_SUCCESS) { ret.type = VT_ERROR;return ret; }
	return ret;

	case VT_ARRAY:
	ret.type = VT_ARRAY;
	//create a new array and check for errors
	Array* arr = _make_Array(sizeof(value), DEF_ARR_N, err);
	if (err->type != E_SUCCESS) { free_Array(arr);return ret; }

	//copy the sting so that we can do strtok and iterate over comma separated terms
	char* saveptr;
	char* token = strtok_r(str, ",", &saveptr);
	size_t i = 0;
	while (token != NULL) {
	    _grow_a(arr, 1, err);
	    arr->buf[i] = read_value_string(token, 0, err);
	    if (err->type != E_SUCCESS) { free_Array(arr);return ret; }
	    ++i;
	    token = strtok_r(NULL, ",", &saveptr); 
	}
	//figure out the size of the newly created array
	arr->size = i;
	//tie the array to our returned value
	ret.val.ptr = arr;
	return ret;

	default: sc_set_error(err, E_BADTYPE, "Unrecognized hint type");
	}
    } else {
	//initialize the index count and find the first non whitespace character
	size_t i = 0;
	while (str[i] == ' ' || str[i] == '\t' || str[i] == '+') { ++i; }

	//Arrays and strings are signaled by special characters, try interpreting those
	if (str[i] == '\"') {
	    char* tmp_str = _get_enclosed(str+i, "\"", "\"");
	    return read_value_string(tmp_str, VT_STRING, err);
	}
	if (str[i] == '[') {
	    char* tmp_str = _get_enclosed(str+i, "[", "]");
	    return read_value_string(tmp_str, VT_ARRAY, err);
	}

	//figure out the sign and skip this character
	int sign = 1;
	if (str[i] == '-') { sign = -1;++i; }

	//first try interpreting our value as an integer
	ret.type = VT_INT;
	_uint base = 10;
	//look for preceeding 0, 0b, or 0x and change base accordingly
	if (str[i] == '0') {
	    if (str[i+1] == 'x' || str[i+1] == 'X') {
		base = 16;
		i += 2;
	    } else if (str[i+1] == 'b' || str[i+1] == 'B') {
		base = 2;
		i += 2;
	    } else {
		base = 8;
		i += 1;
	    }
	}
	int tmp = 0;
	ret.val.i = 0;
	size_t dec_place = 0;
	int power = 0;
	int use_power = 0;
	for (; str[i] != 0 && str[i] != ' '; ++i) {
	    //read the sign and skip the rest of this iteration
	    /*if (str[i] == '-') { sign *= -1;continue; }
	    if (str[i] == '+') { sign = 1;continue; }*/

	    //if we encounter a '.' then switch to floating point interpretation
	    if (str[i] == '.') {
		//Strings of the form '0.123' have a first character 0 so they are interpreted as octal. We want them to be interpreted in decimal.
		if (tmp == 0) { base = 10; }
		ret.type = VT_FLOAT;
		ret.val.f = (double)(sign*tmp);
		tmp = 0;
		dec_place = i + 1;
	    } else if (str[i] == 'e' && base < 15) {
		//if we encounter an 'E' then interpret the rest of the number as a power
		ret.type = VT_FLOAT;
		//add the decimal precision
		ret.val.f += (double)(sign*tmp) / pow(base, i - dec_place);
		tmp = 0;
		//set the sign accordingly if necessary
		if (str[i+1] == '-') { sign = -1;++i; }
		if (str[i+1] == '+') { sign = 1;++i; }
		use_power = 1;
	    } else {
		tmp *= base;
		int digit = _read_digit(str[i], base);
		//catch errors by interpreting as a bool
		//TODO: consider the best behaviour for this case
		if (digit < 0) {	
		    //ignore whitespace
		    if (str[i] == ' ' || str[i] == '\t') { break; }
		    //return read_value_string(str, VT_BOOL, err);
		    //set an error and return undefined
		    sc_set_error(err, E_BADVAL, "invalid rvalue");
		    ret.type = VT_UNDEF;
		    ret.val.i = 0;
		    return ret;
		}
		tmp += digit;
	    }
	}
	if (ret.type == VT_FLOAT) {
	    if (use_power) {
		//if a power was supplied then we just need to multiply the final result
		ret.val.f *= pow(base, sign*tmp);
	    } else {
		//otherwise add the decimal part of the float
		ret.val.f += (double)(sign*tmp) / pow(base, i - dec_place);
	    }
	} else {
	    ret.val.i = sign*tmp;
	}
    }

    return ret;
}

/**
 * Helper function that reads a string of the form (int, bool, etc...) into a valtype
 */
Valtype_e read_valtype(char* str, sc_error* err) {sc_reset_error(err);
    //TODO: find a better way to do this
    if (strcmp(str, "error") == 0) {
	return VT_ERROR;
    } else if (strcmp(str, "char") == 0) {
	return VT_CHAR;
    } else if (strcmp(str, "bool") == 0) {
	return VT_BOOL;
    } else if (strcmp(str, "int") == 0) {
	return VT_INT;
    } else if (strcmp(str, "float") == 0) {
	return VT_FLOAT;
    } else if (strcmp(str, "string") == 0) {
	return VT_STRING;
    } else if (strcmp(str, "array") == 0) {
	return VT_ARRAY;
    } else if (strcmp(str, "func") == 0) {
	return VT_FUNC;
    } else {
	sc_set_error(err, E_BADVAL, "");
	snprintf(err->msg, DTG_MAX_MSG_SIZE, "unrecognized type %s", str);
	return VT_ERROR;
    }
}

/**
 * Helper function which reads a string of the form "<type> <name>", "<type> <name> = <val>" or "<name> = <val>" into a hashed item which is used as a name value pair.
 */
HashedItem _read_valtup(char* str, sc_error* err) {
    sc_reset_error(err);
    HashedItem ret;
    char* type_name;
    char* val_name;

    //read the <type> section
    size_t n_read = read_word_i(str, 0, &type_name);
    if (n_read == 0) {
	sc_set_error(err, E_SYNTAX, "empty argument");
	return ret;
    }

    //read the type name and push the appropriate value type onto the stack
    ret.val.type = read_valtype(type_name, err);
    if (err->type != E_SUCCESS) {
	//if we couldn't read the value type, then we try to interpret the value type on the basis of the assignment
	size_t tmp = read_word_i(str, n_read, &val_name);
	if (tmp > 0 && val_name[0] == '=' && val_name[1] != '=') {
	    tmp += read_word_i(str, n_read+tmp, &val_name);
	    if (tmp == 0) {
		sc_set_error(err, E_SYNTAX, "expected rval for assignment");
		return ret;
	    }
	    
	    //read the value to interpret the type
	    ret.key = DTG_strdup(type_name, err);
	    if (err->type != E_SUCCESS) { return ret; }
	    ret.val = read_value_string(type_name, 0, err);
	    if (err->type != E_SUCCESS) { return ret; }
	}
	return ret;
    }

    //if we reach this point of execution we have a declaration of the form <type> <name>
    n_read += read_word_i(str, n_read, &val_name);
    //check to ensure that there is actually a variable name
    if (val_name == NULL) {
	ret.key = NULL;
	ret.val.val.i = 0;
	sc_set_error(err, E_SYNTAX, "missing variable name");
	return ret;
    }
    ret.key = DTG_strdup(val_name, err);
    if (err->type != E_SUCCESS) { return ret; }
    //check if there was an equal sign
    n_read += read_word_i(str, n_read, &val_name);
    if (val_name && val_name[0] == '=' && val_name[1] != '=') {
	n_read += read_word_i(str, n_read, &val_name);
	if (val_name == NULL) {
	    sc_set_error(err, E_SYNTAX, "expected rval for assignment");
	    return ret;
	}
	
	//read the value to interpret the type
	ret.val = read_value_string(val_name, ret.val.type, err);
	//check if there were any errors and try to find the value from the context
	if (err->type != E_SUCCESS) {
	    //set a special code to indicate that we successfully found an equal sign and a 
	    ret.val.type |= BIT_UNRES_NAME;
	    ret.val.val.ptr = val_name;
	}

	return ret;
    }
    return ret;
}

/**
 * Frees the memory allocated for usage by h. This assumes a call to _read_valtup has been used or the key and value have both been allocated using sc_malloc().
 */
void free_HashedItem(HashedItem* h) {
    if (h) {
	sc_free(h->key);
	free_value( &(h->val) );
    }
}

// ================================== ARRAYS ==================================

/**
 * Initializes an array of n elements with size el_size or stores a result to the error err
 */
Array* _make_Array(size_t el_size, size_t n, sc_error* err) {sc_reset_error(err);
    //create the array and check for errors
    Array* ret = sc_malloc(sizeof(Array), err);
    if (err->type != E_SUCCESS || ret == NULL) { return NULL; }

    ret->el_size = el_size;
    ret->buf_size = n;
    ret->size = 0;
    ret->buf = (value*)sc_malloc(el_size*n, err);
    //in the event of an out of memory error, free the allocated result and return null
    if (err->type != E_SUCCESS || ret->buf == NULL) {
	free(ret);
	return NULL;
    }
    return ret;
}

/**
 * Frees the array pointed to by arr
 */
void free_Array(Array* arr) {
    if (arr) {	
	if (arr->buf) {
	    //free the elements of the array
	    for (size_t i = 0; i < arr->size; ++i) { free_value(arr->buf + i); }
	    sc_free(arr->buf);
	}
	arr->buf = NULL;
	arr->buf_size = 0;
	arr->size = 0;
    }
}

/**
 * Grows the array arr to accomodate n additional entries of size el_size
 */
void _grow_a(Array* arr, size_t n, sc_error* err) {sc_reset_error(err);
    if (arr) {

    if (arr->buf_size <= arr->size + n) {
	arr->buf_size = 2*(arr->buf_size) + n;
	value* tmp = (value*)sc_malloc(sizeof(value)*(arr->buf_size), err);
	//if there weren't any problems allocating the new block then copy the data to the new location
	if (tmp != NULL) {
	    for (size_t i = 0; i < arr->size; ++i) {
		tmp[i] = arr->buf[i];
	    }
	    free(arr->buf);
	    arr->buf = tmp;
	} else {
	    //make doubly sure that the error flag is set
	    if (err) { err->type = E_NOMEM; }
	}
    }

    }
}

/**
 * Grows the array arr to accomodate n additional entries of size el_size
 */
void _grow_pa(PrimArray* arr, size_t n, sc_error* err) {sc_reset_error(err);
    if (arr) {

    if (arr->buf_size <= arr->size + n) {
	arr->buf_size = 2*(arr->buf_size) + n;
	void* tmp = sc_malloc((arr->el_size)*(arr->buf_size), err);
	//if there weren't any problems allocating the new block then copy the data to the new location
	if (tmp != NULL) {
	    for (size_t i = 0; i < (arr->el_size)*(arr->size); ++i) {
		((char*)tmp)[i] = ((char*)(arr->buf))[i];
	    }
	    free(arr->buf);
	    arr->buf = tmp;
	} else {
	    //make doubly sure that the error flag is set
	    if (err) { err->type = E_NOMEM; }
	}
    }

    }
}

/**
 * Resizes the array arr to hold exactly n entries of size el_size. If n is less than the current size of the array, then elements at the end are discarded.
 */
void _resize_a(Array* arr, size_t n, sc_error* err) {sc_reset_error(err);
    if (arr) {

    arr->buf_size += n;
    void* tmp = sc_malloc((arr->el_size)*(arr->buf_size), err);
    //if there weren't any problems allocating the new block then copy the data to the new location
    if (tmp != NULL) {
	//set n_write to be the minimum of the number of bytes indicated by arr->size and n
	size_t n_write = (arr->el_size)*(arr->size);
	if (n < arr->size) {
	    n_write = (arr->el_size)*n;
	}
	for (size_t i = 0; i < n_write; ++i) {
	    ((char*)tmp)[i] = ((char*)(arr->buf))[i];
	}
	free(arr->buf);
	arr->buf = tmp;
    } else {
	//make doubly sure that the error flag is set
	if (err) { err->type = E_NOMEM; }
    }

    }
}

/**
 * Returns a deep copy of the array pointed to by arr
 */
Array _copy_a(Array arr, sc_error* err) {
    //initialize the array and the buffer and check for errors
    Array ret = {0};
    //set the size appropriately
    ret.buf_size = arr.size;
    ret.el_size = arr.el_size;
    ret.size = ret.buf_size;
    //in the case of an empty array we just set the buffer to be NULL
    if (ret.size == 0) { ret.buf = NULL;return ret; }

    //at last! we allocate the actual array buffer and copy data
    ret.buf = (value*)sc_malloc(sizeof(value)*(ret.buf_size), err);
    if (err->type != E_SUCCESS) { ret.buf = NULL;return ret; }

    for (size_t i = 0; i < arr.size; ++i) {
	ret.buf[i] = v_deep_copy(arr.buf[i], err);
	//if there is an error free the memory we allocated and return
	if (err->type != E_SUCCESS) {
	    for (size_t j = 0; j <= i; ++j) { free_value(ret.buf + j); }
	    free(ret.buf);
	    return ret;
	}
    }
    //no errors, yay!
    sc_reset_error(err);
    return ret;
}

/**
 * Returns a new array with elements ranging from start_ind to end_ind of the original array. A shallow copy is made, and the contents of the returned array only have a lifespan matching arr. Call _copy_a() before this function if you need a deep copy.
 * Note negative values "wrap around", so _slice(arr, -2, -1, NULL) would return the second to last element in the array. Indices greater than the size of the array are fixed to be equal to the size of the array.
 */
Array _slice_a(Array arr, long int start_ind, long int end_ind, sc_error* err) {sc_reset_error(err);
    //set to zero to make sure errors don't return garbage values
    Array ret = {0};
    ret.buf_size = 0;
    ret.size = 0;
    ret.buf = NULL;
    
    //size_t span = arr->el_size;//save on typing
    size_t span = sizeof(value);

    //translate the indices from negative or out of bounds values to valid ones
    if (end_ind > arr.size) { end_ind = arr.size; }
    if (start_ind > arr.size) { end_ind = arr.size; }
    if (end_ind < -1*(long int)(arr.size) || start_ind < -1*(long int)(arr.size)) {
	sc_set_error(err, E_RANGE, "negative slice less than size");
	return ret;
    }
    if (end_ind < 0) { end_ind += arr.size; }
    if (start_ind < 0) { start_ind += arr.size; }
    if (end_ind < start_ind) {
	sc_set_error(err, E_RANGE, "end must be >= to start");
	return ret;
    }

    if (err->type != E_SUCCESS) { return ret; }
    //set the size appropriately
    ret.buf_size = (end_ind - start_ind);
    ret.el_size = arr.el_size;
    ret.size = ret.buf_size;
    //in the case of an empty array we just set the buffer to be NULL
    if (ret.size == 0) { ret.buf = NULL;return ret; }

    //at last! we allocate the actual array buffer and copy data
    ret.buf = sc_malloc(sizeof(value)*(end_ind - start_ind), err);
    if (err->type != E_SUCCESS) { sc_free(ret.buf);ret.buf = NULL;return ret; }

    for (size_t i = start_ind; i < end_ind; ++i) {
	ret.buf[i - span*start_ind] = arr.buf[i];
    }
    //no errors, yay!
    sc_reset_error(err);
    return ret;
}

/**
 * Returns a new array with elements ranging from start_ind to end_ind of the original array. This is a half open interval including start_ind but not end_ind. A deep copy of each element is performed, thus the caller should ensure that free_Array is called.
 * Note negative values "wrap around", so _slice(arr, -2, -1, NULL) would return the second to last element in the array. Indices greater than the size of the array are fixed to be equal to the size of the array.
 */
/*PrimArray* _slice_pa(PrimArray* arr, long int start_ind, long int end_ind, sc_error* err) {sc_reset_error(err);
    if (arr) {

    size_t span = arr->el_size;//save on typing

    //translate the indices from negative or out of bounds values to valid ones
    if (end_ind > arr->size) { end_ind = arr->size; }
    if (start_ind > arr->size) { end_ind = arr->size; }
    if (end_ind < -1*(long int)(arr->size) || start_ind < -1*(long int)(arr->size)) {
	sc_set_error(err, E_RANGE, "negative slice less than size");
	return NULL;
    }
    if (end_ind < 0) { end_ind += arr->size; }
    if (start_ind < 0) { start_ind += arr->size; }
    if (end_ind < start_ind) {
	sc_set_error(err, E_RANGE, "end must be >= to start");
	return NULL;
    }

    //initialize the array and the buffer and check for errors
    PrimArray* ret = sc_malloc(sizeof(PrimArray), err);
    if (err->type != E_SUCCESS) { free(ret);return NULL; }
    //set the size appropriately
    ret->buf_size = (end_ind - start_ind);
    ret->el_size = arr->el_size;
    ret->size = ret->buf_size;
    //in the case of an empty array we just set the buffer to be NULL
    if (ret->size == 0) { ret->buf = NULL;return ret; }

    //at last! we allocate the actual array buffer and copy data
    ret->buf = sc_malloc(span*(end_ind - start_ind), err);
    if (err->type != E_SUCCESS) { free(ret);return NULL; }

    for (size_t i = span*start_ind; i < span*end_ind; ++i) {
	( (char*)(ret->buf) )[i - span*start_ind] = ( (char*)(arr->buf) )[i];
    }
    //no errors, yay!
    sc_reset_error(err);
    return ret;

    }
}*/

/**
 * Appends the array of values of length n specified by new vals to the end of the Array pointed to by arr. A deep copy of elements is performed.
 */
void _extend_a(Array* arr, value* new_vals, size_t n, sc_error* err) {
    _grow_a(arr, n, err);
    for (size_t i = 0; i < n; ++i) {
	arr->buf[arr->size + i] = v_deep_copy(new_vals[i], err);
    }
    arr->size += n;
}

// ================================== ARRAY VALUES ==================================

/**
 * Creates a new array value with contents identical to those stored in p_val (a deep copy is performed.
 * NOTE: A deep copy of p_val is made. Thus, no guarantees as to the lifespan of the memory pointed to by p_val are required.
 */
value v_make_array(const value* p_val, size_t n_vals, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_ARRAY;
    ret.val.ptr = sc_malloc(sizeof(Array), err);
    Array* arr = (Array*)ret.val.ptr;
    arr->buf_size = n_vals;
    arr->size = n_vals;
    arr->buf = sc_malloc(sizeof(value)*n_vals, err);
    for (size_t i = 0; i < n_vals; ++i) {
	arr->buf[i] = v_deep_copy(p_val[i], err);
    }
    return ret;
}

/**
 * Creates a new empty array value with memory allocated to hold n members of type p_eltyp.
 */
value v_make_array_n(size_t p_n, value tmplt, sc_error* err) {
    value ret = {0};
    ret.type = VT_ARRAY;
    ret.val.ptr = sc_malloc(sizeof(Array), err);
    Array* arr = (Array*)ret.val.ptr;
    arr->buf_size = p_n;
    arr->size = p_n;
    arr->buf = sc_malloc(sizeof(value)*p_n, err);
    for (size_t i = 0; i < p_n; ++i) {
	arr->buf[i] = v_deep_copy(tmplt, err);
    }
    return ret;
}

/**
 * Returns: A deep copy of the value array p_val.
 */
value v_copy_array(value p_val, sc_error* err);

// ================================== STRINGS ==================================

/**
 * Creates a new String with contents indicated by p_str. A deep copy of the string is performed and memory should be deallocated by a matching call to free_String().
 */
String make_String(const char* p_str, sc_error* err) {sc_reset_error(err);
    String str = {0};
    //figure out the size of the array, allocate memory and check for errors
    str.buf_size = DEFAULT_STRING_SIZE;
    str.buf = (char*)sc_malloc(sizeof(char)*DEFAULT_STRING_SIZE, err);
    if (err && err->type != E_SUCCESS) {
	str.buf_size = 0;
	str.buf = NULL;
	return str;
    }

    //copy the string to the new buffer
    for (str.size = 0; (err == NULL) || (err->type == E_SUCCESS); ++str.size) {
	str.buf[str.size] = p_str[str.size];
	if (p_str[str.size] == 0) { break; }//wait for the null terminator character
	_grow_s(&str, STRING_GROW_SIZE, err);
    }

#ifdef SMALL_STRINGS
    //fit the Array to exactly hold the size
    _resize(&str, i+1);
#endif
    return str;
}

/**
 * Creates an uninitialized empty string with the capacity to hold n bytes without any additional calls to grow
 */
String make_String_n(size_t n, sc_error* err) {sc_reset_error(err);
    String str = {0};
    //set the size of the string and check for errors
    str.buf_size = n;
    str.size = 0;
    str.buf = sc_malloc(sizeof(char)*n, err);
    if (err->type != E_SUCCESS) {
	str.buf_size = 0;
	str.buf = NULL;
    }
    return str;
}

/**
 * Frees the memory used by str. after a call to free_String the String str still has not been allocated, but it is safe to call sc_free(str) if the string itself was malloced.
 */
void free_String(String* str) {
    if (str) {
	if (str->buf) {
	    sc_free(str->buf);
	}
	str->buf = NULL;
	str->buf_size = 0;
	str->size = 0;
    }
}

/**
 * Grows the string value val to accomodate n additional bytes.
 * NOTE: the size and contents of this operation are unchanged. Call this before writing any data to ensure that there is sufficient allocated space.
 */
void _grow_s(String* str, size_t n, sc_error* err) {sc_reset_error(err);
    if (str) {

    if (str->buf_size <= str->size + n) {
	str->buf_size = 2*(str->buf_size) + n;
	char* tmp = (char*)sc_malloc(str->buf_size, err);
	//only proceed if there wasn't an allocation error
	if (err->type == E_SUCCESS) {
	    strncpy(tmp, str->buf, str->size+1);
	}
	//tmp[str->size] = 0;
	sc_free(str->buf);
	str->buf = tmp;
	if (err) { sc_reset_error(err); }
    }

    }
}

/**
 * Reallocates the buffer used by val so that it holds exactly n bytes.
 * TODO: remove?
 */
void _resize_s(String* str, size_t n, sc_error* err) {sc_reset_error(err);
    if (str) {

    if (str->buf_size != n) {
	str->buf_size = n+1;
	char* tmp = (char*)sc_malloc(str->buf_size, err);
	//only proceed if there wasn't an allocation error
	if (err->type == E_SUCCESS) {
	    strncpy(tmp, str->buf, n);
	    tmp[n] = 0;
	    sc_free(str->buf);
	    str->buf = tmp;
	}
    }

    }
}

/**
 * Appends the string p_str to the string value val, resizing the buffer to accomodate results if necessary.
 */
void _append_string(String* str, const char* p_str, sc_error* err) {sc_reset_error(err);
    if (str) {

    //try growing the array to initialize the error properly
    _grow_s(str, STRING_GROW_SIZE, err);

    size_t off = str->size;
    for (; err->type == E_SUCCESS; str->size += 1) {
	str->buf[str->size] = p_str[str->size - off];
	if (str->buf[str->size] == 0) { break; }
	_grow_s(str, STRING_GROW_SIZE, err);
    }

    sc_reset_error(err);

    } else {
	sc_set_error(err, E_BADVAL, "tried to append to null");
    }
}

// ================================== STRING VALUES ==================================

/**
 * Helper function that returns the character buffer used by string values
 * Note: ideally this function should only be used to fetch read only data. Call resize or grow functions to adjust the allocated size of the array
 */
char* get_string(value* p_val, sc_error* err) {
    if (p_val) {

    //make sure that we are appending to a string
    if (p_val->type != VT_STRING) {
	sc_set_error(err, E_BADTYPE, "tried to fetch array from non string");
    } else {
	return p_val->val.str->buf;
    }

    }
    return NULL;
}

/**
 * Creates a new string value with val identical to p_val.
 * NOTE: a deep copy of p_val so no guarantees as to the lifespan of the memory pointed to by p_val are required
 */
value v_make_string(const char* p_val, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_STRING;
    //initialize the array
    ret.val.str = (String*)sc_malloc(sizeof(String), err);
    if (err && err->type != E_SUCCESS) {
	ret.val.str = NULL;
	ret.type = VT_ERROR;
	return ret;
    }
    *(ret.val.str) = make_String(p_val, err);
    if (err && err->type != E_SUCCESS) {
	free(ret.val.str);
	ret.val.str = NULL;
	ret.type = VT_ERROR;
    }

    return ret;
}

/**
 * Creates a new empty string value with memory allocated to hold n bytes.
 * NOTE: a deep copy of p_val so no guarantees as to the lifespan of the memory pointed to by p_val are required
 */
value v_make_string_n(size_t n, sc_error* err) {sc_reset_error(err);
    //create the value and the string, check for errors
    value ret = {0};
    ret.type = VT_STRING;
    ret.val.str = sc_malloc(sizeof(String), err);
    if (err->type != E_SUCCESS) {
	ret.val.str = NULL;
	ret.type = VT_ERROR;
	return ret;
    }
    *(ret.val.str) = make_String_n(n, err);
    if (err && err->type != E_SUCCESS) {
	free(ret.val.str);
	ret.val.str = NULL;
	ret.type = VT_ERROR;
    }

    return ret;
}

/**
 * Appends the string p_str to the string value val, resizing the buffer to accomodate results if necessary.
 * TODO: remove?
 */
void _append_string_s(String* p_val, String o_val, sc_error* err) {sc_reset_error(err);
    if (p_val) {
	_grow_s(p_val, o_val.size + 1, err);
	if (err == NULL || err->type == E_SUCCESS) {
	    //expand the string
	    size_t off = p_val->size;
	    p_val->size += o_val.size;
	    for (size_t i = off; i < p_val->size; ++i) {
		p_val->buf[i] = o_val.buf[i - off];
		//check for stray null termination characters
		if (o_val.buf[i-off] == 0) {
		    p_val->size = i;
		    break;
		}
	    }
	    p_val->buf[p_val->size] = 0;//null terminate the string
	}
    } else {
	sc_set_error(err, E_BADVAL, "attempted to append to null string");
    }
}

/**
 * Fetches the string stored in p_val and stores the result in str and performs casts if necessary.
 * param p_val: value to read into the string
 * param p_str: the string to write the value to
 * param n: the maximum size in bytes to write to the string p_str
 * param err: if an error occurs, store the result here
 * Returns: the number of characters written to the buffer p_str including the null termination bit if it was included.
 */
int v_fetch_string(value p_val, char* p_str, size_t n, sc_error* err) {sc_reset_error(err);
    int ret = 0;
    switch (p_val.type) {
	case VT_STRING:
	    size_t i = 0;
	    for (; i < p_val.val.str->size && i < n; ++i) {
		p_str[i] = p_val.val.str->buf[i];
		if (p_val.val.str->buf[i] == 0) { break; }
	    }
	    return i;
	    //strncpy(p_str, p_val.val.str->buf, n);
	    break;

	case VT_BOOL:
	    sc_error tmp;
	    if (p_val.val.i == 0) {
		//just write 0 or 1 if there isn't enough space
		if (n < 6) {
		    sc_strncpy(p_str, "0", n, err);
		    return 1;
		} else {
		    sc_strncpy(p_str, "false", n, err);
		    return 5;
		}
	    } else {
		//just write 0 or 1 if there isn't enough space
		if (n < 5) {
		    sc_strncpy(p_str, "1", n, err);
		    return 1;
		} else {
		    sc_strncpy(p_str, "true", n, err);
		    return 4;
		}
	    }
	    break;

	case VT_INT:
	    ret = sc_itoa(p_val.val.i, p_str, n, 10, err);
	    if (err->type != E_SUCCESS) { return 0; }
	    /*ret = snprintf(p_str, n, "%d", p_val.val.i);
	    if (ret < 0) { sc_set_error(err, E_BADVAL, ""); }*/
	    return ret;

	case VT_FLOAT:
	    ret = sc_ftoa(p_val.val.f, p_str, n, DEF_FLOAT_PRECISION, err);
	    /*ret = snprintf(p_str, n, "%f", p_val.val.f);
	    if (ret < 0) { sc_set_error(err, E_BADVAL, ""); }*/
	    return ret;

	default: sc_set_error(err, E_UNDEF, ""); break;
    }
    return 0;
}

/**
 * a() is a special accessor for string type values that returns the character stored at location i within the string. If the caller attempts to access an invalid character, then the sc_error err is set.
 */
/*char a(value p_val, int i, sc_error* err) {
    if (p_val.type != VT_STRING) {
	sc_set_error(err, E_BADTYPE, "tried to call a() for non string value");
	return 0;
    }
    if (i >= len(p_val) || i < 0) {
	sc_set_error(err, E_RANGE, "")
	snprintf(err->msg, DTG_MAX_MSG_SIZE, "a(str, %d) out of bounds", i);
	return 0;
    }

    return (char*)(((Array*)(p_val.val))->buf)[i] ;
}*/

// ================================== BOOLS ==================================

/**
 * Creates a new integer value with val p_val.
 */
value v_make_bool(int p_val, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_BOOL;
    if (err) { sc_reset_error(err); }
    if (p_val) { ret.val.i = 1; } else { ret.val.i = 0; }
    return ret;
}

/**
 * Fetches the integer stored in p_val and performs casts if necessary.
 */
int v_fetch_bool(value p_val, sc_error* err) {sc_reset_error(err);
    switch (p_val.type) {
	case VT_STRING:
	    if (strcmp(p_val.val.str->buf, "false") == 0 ||
		strcmp(p_val.val.str->buf, "0") == 0) {
		return 0;
	    } else {
		return 0;
	    }
	case VT_BOOL: return ( p_val.val.i != 0 );
	case VT_INT: return ( p_val.val.i != 0 );
	case VT_FLOAT: return ( p_val.val.f != 0.0 );
	default: sc_set_error(err, E_UNDEF, ""); return 0;
    }
}

// ================================== INTS ==================================

/**
 * Creates a new integer value with val p_val.
 */
value v_make_int(int p_val, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_INT;
    if (err) { sc_reset_error(err); }
    ret.val.i = p_val;
    return ret;
}

/**
 * Fetches the integer stored in p_val and performs casts if necessary.
 */
int v_fetch_int(value p_val, sc_error* err) {sc_reset_error(err);
    switch (p_val.type) {
	case VT_STRING:
	    if (p_val.val.str == NULL || p_val.val.str->buf == NULL) {
		sc_set_error(err, E_BADVAL, "tried to fetch value of null string");
	    }
	    return sc_atoi(p_val.val.str->buf, err);
	case VT_BOOL: return p_val.val.i;
	case VT_INT: return p_val.val.i;
	case VT_FLOAT: return (int)(p_val.val.f);
	default: sc_set_error(err, E_UNDEF, ""); return 0;
    }
}

// ================================== FLOATS ==================================

/**
 * Creates a new integer value with val p_val.
 */
value v_make_float(double p_val, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_FLOAT;
    if (err) { sc_reset_error(err); }
    ret.val.f = p_val;
    return ret;
}

/**
 * Fetches the integer stored in p_val and performs casts if necessary.
 */
double v_fetch_float(value p_val, sc_error* err) {sc_reset_error(err);
    switch (p_val.type) {
	case VT_STRING:
	  if (p_val.val.str == NULL || p_val.val.str->buf == NULL) {
	      sc_set_error(err, E_BADVAL, "tried to fetch value of null string");
	  }
	  return sc_atof(p_val.val.str->buf, err);
	case VT_INT: return (double)(p_val.val.i);
	case VT_FLOAT: return p_val.val.f;
	default: sc_set_error(err, E_UNDEF, ""); return 0;
    }
}

// ================================== STRING UTILITIES ==================================

/**
  * Returns the index of the nearest match for a character contained in terminators that is not less than offset. The returned index of this character is relative to the start of the string (as if offset = 0). If no matching character was found then the end of the string is returned. If an invalid offset is supplied (offset < 0 or offset >= len(str)) then an error is thrown and -2 is returned.
  */
int find_nearest(value p_val, size_t offset, const char* terminators, sc_error* err) {sc_reset_error(err);
    if (p_val.type == VT_STRING) {

    if (offset >= len(p_val) || offset < 0) {
	sc_set_error(err, E_RANGE, "offset out of range");
	return -2;
    }
    //we need to iterate over the entire loop since we don't know how long str is
    
    for (size_t i = offset; i < len(p_val); ++i) {
	//error checking probably isn't necessary
	char c = p_val.val.str->buf[i];
	//iterate over the list of terminators
	for (size_t j = 0; terminators[j] != 0; ++j) {
	    if (c == terminators[j]) { return i; }
	}
    }
    return -1; 

    } else {
	sc_set_error(err, E_BADTYPE, "tried to call find_nearest on non string");
	return -2;
    }
}

// ================================== REFERENCES ==================================

/**
 * Frees the Reference struct pointed to by r. You should call release() instead if you want refcounts to change properly
 */
void _free_Reference(Reference* r) {
    if (r) {
	free_value(r->ptr);
    }
}

/**
 * Releases the Reference pointed to by r. This function should be called by end users when they no longer need an object. When the refcount reaches zero the garbage collector will call _free_Reference().
 */
void release(Reference* r) {
    if (r && r->refcount > 0) {
	r->refcount -= 1;
    }
}

// ================================== MEMORY MANAGEMENT AND HASHING ==================================

/**
 * Uses FNV-1a with the bias and prime defined previously to compute the hash of the specified key. This function is used internally by lookup(), insert_deep() and insert(). This hash is not cryptographically secure.
 */
_uint32 hash(const char* key) {
    _uint32 ret = FNV_OFFSET_BIAS;
    for (size_t i = 0; key[i] != 0 && i < MAX_KEY_SIZE; ++i) {
	ret = ret ^ key[i];
	ret *= FNV_PRIME;
    }
    return ret;
}

/**
 * This function is identical to hash(), but it also stores the length of the key string into keylen if keylen is not NULL.
 */
_uint32 hash_l(const char* key, size_t* keylen) {
    _uint32 ret = FNV_OFFSET_BIAS;
    size_t i = 0;
    for (; key[i] != 0 && i < MAX_KEY_SIZE; ++i) {
	ret = ret ^ key[i];
	ret *= FNV_PRIME;
    }
    if (keylen) { *keylen = i; }
    return ret;
}

/**
 * Creates a new HashTable and sets appropriate values. You can remove it when you are done using free_HashTable.
 */
HashTable make_HashTable(sc_error* err) {sc_reset_error(err);
    //make the hash table
    HashTable ret = {0};
    ret.table_size = DEF_TABLE_SIZE;
    ret.n_els = 0;
    //allocate memory for the table
    ret.table = (HashedItem*)sc_malloc(sizeof(HashedItem)*ret.table_size, err);
    if (err->type != E_SUCCESS) { ret.table = NULL; }

    //clear the contents of the table to indicate that entries are uninitialized
    for (size_t i = 0; i < ret.table_size; ++i) {
	ret.table[i].key = NULL;
	ret.table[i].val.type = VT_UNDEF;
    }

    return ret;
}

/**
 * Deallocates the HashTable pointed to by h.
 */
void free_HashTable(HashTable* h) {
    if (h) {
	if (h->table) {
	    //free memory allocated for keys then free the table
	    for (size_t i = 0; i < h->table_size; ++i) {
		if (h->table[i].key) {
		    free_value( &(h->table[i].val) );
		    sc_free(h->table[i].key);
		}
	    }
	    free(h->table);
	}

	//make the contents of the table invalid
	h->table = NULL;
	h->table_size = 0;
	h->n_els = 0;
    }
}

/**
 * Look through the hash table for an entry that matches the supplied key
 * param h: the hash table to look through
 * param key: the key of the desired entry
 * returns: a pointer to the associated value or NULL if the specified value wasn't found. The user should not attempt to free this value.
 */
HashedItem* lookup(HashTable* h, const char* key) {
    _uint32 ind = hash(key) % (h->table_size);
    //search through the table until we find a match
    /*while (strcmp(key, h->table[ind].key) != 0) {
	if (h->table[ind].key == NULL) { return NULL; }
	++ind;
	if (ind == h->table_size) { ind = 0; }
    }*/
    while (h->table[ind].key != NULL) {
	if (strcmp(key, h->table[ind].key) == 0) {
	    return h->table + ind;
	}
	//increment and wrap around
	++ind;
	if (ind == h->table_size) {ind = 0; }
    }
    return NULL;
}

/**
 * Expands the HashTable pointed to by h to accomodate the insertion of an additional element
 */
void h_grow(HashTable* h, sc_error* err) {
    size_t new_size = h->table_size*2;
    HashedItem* tmp = (HashedItem*)sc_malloc(sizeof(HashedItem)*new_size, err);
    //to safely check for unallocated elements we need to make sure the new table contains empty keys
    for (size_t i = 0; i < new_size; ++i) { tmp[i].key = NULL; }

    //shallow copy data from the old table to the new
    for (size_t i = 0; i < h->table_size; ++i) {
	//we only need to copy entries with contents
	if (h->table[i].key != NULL) {
	    //since we use modulo table_size we need to recalculate the index of each element
	    _uint32 new_ind = hash(h->table[i].key) % new_size;
	    while (tmp[new_ind].key != NULL) {
		++new_ind;
		if (new_ind == new_size) { new_ind = 0; }
	    }
	    //move the contents from the old table to the new
	    tmp[new_ind] = h->table[i];
	}
    }
    //deallocate the old and replace it with the new
    sc_free(h->table);
    h->table_size = new_size;
    h->table = tmp;
}

/**
 * Insert a new item into the hash table with the specified key and value. Note that a shallow copy of the contents of val are performed. In order to produce a deep copy use insert_deep instead.
 * param h: the hash table to look through
 * param key: the key of the entry to create
 * param val: the value to be inserted
 */
void insert(HashTable* h, const char* key, value p_val, sc_error* err) {
    //make sure we have enough room to insert the new element
    if (h->n_els >= GROW_THRESH*(h->table_size)) {
	h_grow(h, err);
    }

    //figure out the length of the key and the hash
    size_t key_len;
    _uint32 ind = hash_l(key, &key_len) % (h->table_size);

    //search through the table until we find a match
    while (h->table[ind].key != NULL) {
	++ind;
	if (ind == h->table_size) { ind = 0; }
    }

    //allocate memory for the key and copy contents
    h->table[ind].key = (char*)sc_malloc(sizeof(char)*(key_len+1), err);
    strncpy(h->table[ind].key, key, key_len+1);
    h->table[ind].key[key_len] = 0;
    //copy the value
    h->table[ind].val = p_val;
    //h->table[ind].ind = h->n_els;
    h->n_els += 1;
}

/**
 * Insert a new item into the hash table with the specified key and value. Note that a deep copy of the contents of val are performed if the value is of a non primitive type. This may be slower than using insert() so using a call to that function may be prudent.
 * param h: the hash table to look through
 * param key: the key of the entry to create
 * param val: the value to be inserted
 */
void insert_deep(HashTable* h, const char* key, value p_val, sc_error* err) {
    //make sure we have enough room to insert the new element
    if (h->n_els >= GROW_THRESH*(h->table_size)) {
	h_grow(h, err);
    }

    //figure out the length of the key and the hash
    size_t key_len;
    _uint32 ind = hash_l(key, &key_len) % (h->table_size);

    //search through the table until we find a match
    while (h->table[ind].key != NULL) {
	++ind;
	if (ind == h->table_size) { ind = 0; }
    }

    //allocate memory for the key and copy contents
    h->table[ind].key = (char*)sc_malloc(sizeof(char)*(key_len+1), err);
    strncpy(h->table[ind].key, key, key_len+1);
    h->table[ind].key[key_len] = 0;
    //copy the value
    h->table[ind].val = v_deep_copy(p_val, err);
    //h->table[ind].ind = h->n_els;
    h->n_els += 1;
}

// ================================== STACK ==================================

Stack make_Stack(sc_error* err) {
    Stack ret = {0};
    ret.block = (value*)sc_malloc(sizeof(value)*DEF_STACK_SIZE, err);
    if (err->type != E_SUCCESS) {
	ret.bottom = NULL;
	ret.top = NULL;
	ret.block = NULL;
	return ret;
    }
    ret.cap = DEF_STACK_SIZE;
    ret.bottom = ret.block + ret.cap;
    ret.top = ret.bottom;
    return ret;
}

void free_Stack(Stack* st) {
    if (st) {
	for (value* v = st->top; v != st->bottom; v += 1) {
	    free_value(v);
	}
	free(st->block);
	st->cap = 0;
	st->bottom = NULL;
	st->top = NULL;
	st->block = NULL;
    }
}

/**
 * Pushes the value p_val onto the stack pointed to by st. Note that this function performs a deep copy of p_val unless p_val is of the type reference. In which case the address of the value pointed to by the top stack entry will be the same as p_val before assignment.
 */
void push(Stack* st, value p_val, sc_error* err) {
    //reallocate memory if necessary
    if (st->top <= st->block) {
	//allocate new block with twice the size
	size_t tmp_size = (st->cap)*2;
	value* new_buf = (value*)sc_malloc(sizeof(value)*tmp_size, err);
	value* old_block = st->block;

	if (err->type == E_SUCCESS) {
	    //perform a shallow copy of entries from the old to the new stacks
	    for (size_t i = 0; i < st->cap; ++i) {
		new_buf[i + st->cap] = st->block[i];
	    }
	    //assign the new pointers
	    st->block = new_buf;
	    st->top = st->block + st->cap;
	    st->bottom = st->block + tmp_size;
	    st->cap = tmp_size;
	} else {
	    sc_set_error(err, E_STACK_OVERFLOW, "");
	}
	free(old_block);
    }
    //only proceed if there were no errors
    if (err->type == E_SUCCESS) {
	st->top -= 1;
	*(st->top) = p_val;
    }
}

/**
 * Pushes an uninitialized value of type t onto the stack pointed to by st. Note that this function performs a deep copy of p_val unless p_val is of the type reference. In which case the address of the value pointed to by the top stack entry will be the same as p_val before assignment.
 */
void push_uninit(Stack* st, Valtype_e t, sc_error* err) {
    //reallocate memory if necessary
    if (st->top <= st->block) {
	size_t tmp_size = (st->cap)*2;
	value* new_buf = (value*)sc_malloc(sizeof(value)*tmp_size, err);
	if (err->type == E_SUCCESS) {
	    for (size_t i = 0; i < st->cap; ++i) {
		new_buf[i + st->cap] = st->block[i];
	    }
	    st->block = new_buf;
	    st->top = st->block + st->cap;
	    st->bottom = st->block + tmp_size;
	    st->cap = tmp_size;
	} else {
	    sc_set_error(err, E_STACK_OVERFLOW, "");
	}
	free(st->block);
    }
    //only proceed if there were no errors
    if (err->type == E_SUCCESS) {
	st->top -= 1;
	st->top->type = t;
	st->top->val.i = 0;
    }
}

/**
 * Pops the last value off of the stack and returns the result. After a call to pop a shallow copy is made and the caller is responsible for freeing any memory which may have been allocated.
 */
value pop(Stack* st, sc_error* err) {
    value ret = {0};
    if (st) {
	if (st->top > st->bottom) {
	    sc_set_error(err, E_STACK_UNDERFLOW, "tried to pop from empty stack");
	    return ret;
	}
	sc_reset_error(err);
	ret = *(st->top);
	st->top += 1;
	return ret;
    }
    sc_set_error(err, E_BADVAL, "tried to pop from NULL stack");
    return ret;
}

/**
 * Returns 1 if the stack pointed to by st is empty or 0 otherwise. Upon an error or invalid value, -1 is returned.
 */
int is_empty(Stack* st) {
    if (st == NULL) { return -1; }
    return st->bottom <= st->top;
}

/**
 * Get the size of the NamedStack pointed to by st
 */
size_t get_size(Stack st) {
    return (size_t)(st.bottom - st.top);
}

// ================================== NAMED STACK ==================================

/**
 * Creates a new (empty) stack. The returned Stack has a pointer to the invalid index -1 and a call to push() must be made before any calls to pop()
 */
NamedStack make_NamedStack(sc_error* err) {
    NamedStack ret = {0};
    ret.block = (HashedItem*)sc_malloc(sizeof(HashedItem)*DEF_STACK_SIZE, err);
    if (err->type != E_SUCCESS) {
	ret.bottom = NULL;
	ret.top = NULL;
	ret.block = NULL;
	return ret;
    }
    ret.cap = DEF_STACK_SIZE;
    ret.bottom = ret.block + ret.cap;
    ret.top = ret.bottom;
    return ret;
}

/**
 * Frees the stack pointed to by st and any memory allocated for its contained members.
 */
void free_NamedStack(NamedStack* st) {
    if (st) {
	for (HashedItem* v = st->top; v != st->bottom; v += 1) {
	    free_value( &(v->val) );
	}
	free(st->block);
	st->cap = 0;
	st->bottom = NULL;
	st->top = NULL;
	st->block = NULL;
    }
}

/**
 * Pushes the value p_val onto the stack pointed to by st. Note that this function performs a deep copy of p_val unless p_val is of the type reference. In which case the address of the value pointed to by the top stack entry will be the same as p_val before assignment.
 */
void push_n(NamedStack* st, char* name, value v, sc_error* err) {
    //reallocate memory if necessary
    if (st->top <= st->block) {
	//allocate new block with twice the size
	size_t tmp_size = (st->cap)*2;
	HashedItem* new_buf = (HashedItem*)sc_malloc(sizeof(HashedItem)*tmp_size, err);
	HashedItem* old_block = st->block;

	if (err->type == E_SUCCESS) {
	    //perform a shallow copy of entries from the old to the new stacks
	    for (size_t i = 0; i < st->cap; ++i) {
		new_buf[i + st->cap] = st->block[i];
	    }
	    //assign the new pointers
	    st->block = new_buf;
	    st->top = st->block + st->cap;
	    st->bottom = st->block + tmp_size;
	    st->cap = tmp_size;
	}
	free(old_block);
    }
    //only proceed if there were no errors
    if (err->type == E_SUCCESS) {
	st->top -= 1;
	st->top->key = name;
	st->top->val = v;
    }
}

/**
 * Pops the last value off of the stack and returns the result. After a call to pop a shallow copy is made and the caller is responsible for freeing any memory which may have been allocated.
 */
HashedItem pop_n(NamedStack* st, sc_error* err) {
    HashedItem ret = {0};
    if (st) {
	if (st->top > st->bottom) {
	    sc_set_error(err, E_STACK_UNDERFLOW, "tried to pop from empty stack");
	    return ret;
	}
	sc_reset_error(err);
	ret = *(st->top);
	st->top += 1;
	return ret;
    }
    sc_set_error(err, E_BADVAL, "tried to pop from NULL stack");
    return ret;
}

/**
 * Returns 1 if the stack pointed to by st is empty or 0 otherwise. Upon an error or invalid value, -1 is returned.
 */
int is_empty_n(NamedStack* st) {
    if (st == NULL) { return -1; }
    return st->bottom <= st->top;
}

/**
 * Access the key of the stack element at index ind. If ind is out of bounds, NULL is returned.
 */
char* read_key(NamedStack* st, size_t ind) {
    if (st) {
	if (st->top + ind <= st->bottom) {
	    return st->top[ind].key;
	} else {
	    return NULL;
	}
    }
}

/**
 * Access the value of the stack element at index ind. If ind is out of bounds, an invalid value is returned.
 */
value read_value(NamedStack* st, size_t ind) {
    if (st) {
	if (st->top + ind <= st->bottom) {
	    return st->top[ind].val;
	} else {
	    value ret = {0};
	    return ret;
	}
    }
}

/**
 * Get the size of the NamedStack pointed to by st
 */
size_t get_size_n(NamedStack st) {
    return (size_t)(st.bottom - st.top);
}

// =============================== CONTEXTS ===============================

/**
 * Search for the value with the name name
 */
context make_context(sc_error* err) {
    context ret = {0};
    ret.callstack = make_NamedStack(err);
    if (err->type != E_SUCCESS) {
	//ret.callstack = {0};
	return ret;
    }
    ret.global = make_HashTable(err);
    if (err->type != E_SUCCESS) {
	free_NamedStack(&(ret.callstack));
	//ret.global = {0};
	return ret;
    }
    return ret;
}

/**
 * Free the memory allocated for the context pointed to by c.
 */
void free_context(context* c) {
    if (c) {
	free_NamedStack( &(c->callstack) );
	free_HashTable( &(c->global) );
	/*c->callstack = {0};
	c->global = {0};*/
    }
}

/**
 * Search for the value with the name name. The hashtable is searched first and if the value is found there, then -1 is returned. In the event that the entry is not found in the table, the stack is searched. If the entry with the matching name is found, then the index (relative to th bottom of the stack) is stored into off. If the entry is not found, -2 is returned.
 */
int search_val(context* c, const char* name, HashedItem** val) {
    //lookup the name in the hashtable
    HashedItem* tmp = lookup(&(c->global), name);

    if (!tmp) {
	//try looking up the value from the callstack
	HashedItem* curr = c->callstack.top;
	size_t i = 0;
	for (; curr < c->callstack.bottom; ++curr) {
	    if (strcmp(curr->key, name) == 0) {
		//store the resulting hashed value
		if (val) { *val = curr; }
		return i;
	    }
	    ++i;
	}
	if (val) { *val = NULL; }
	return -2;
    }
    if (val) {
	*val = tmp;
    }
    return -1;
}

/**
 * Add a value to the context c with the name name
 */
value* add_val(context* c, const char* str, sc_error* err) {
    //_push_valtup(
    /*value val = {0};

    char* saveptr;
    char* type_name;
    char* token = strtok_r(name, ",", &saveptr);
    ret.n_args = 0;
    //iterate over every comma
    while (token != NULL) {
	//read the value and check for errors
	_push_valtup(&name_stack, token, err)
	if (err->type != E_SUCCESS) { return ret; }

	ret.n_args += 1;
	token = strtok_r(NULL, ",", &saveptr); 
    }

    push_n(c->callstack, name, val, err)*/
}

#ifdef __cplusplus 
}
#endif
