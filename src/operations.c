#include "operations.h"

#ifdef __cplusplus 
extern "C" {
#endif

// ============================ MATHEMATICAL OPERATIONS ============================

/**
 *  Creates a new value object which holds the result of the addition operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  string a, (any type b): Converts b into a string representation and appends the result to the end of a.
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a+b.
 *  int a, int b: Returns a+b
 *  array a, (any type b): This is an invalid operation unless the type of b matches the type stored in a, in which case a deep copy of b is made and appended to the end of a.
 *  bool a, bool b: invalid
 */
value op_add(value a, value b, sc_error* err) {sc_reset_error(err);
    value ret = {0};

    //check for invalid types
    if (a.type == VT_ERROR || b.type == VT_ERROR) {
	sc_set_error(err, E_BADTYPE, "can't add errors");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_ARRAY || (b.type == VT_ARRAY && a.type != VT_STRING)) {
	sc_set_error(err, E_BADTYPE, "can't perform addition on array values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_BOOL || (b.type == VT_BOOL && a.type != VT_STRING)) {
	sc_set_error(err, E_BADTYPE, "can't perform addition on boolean values");
	ret.type = VT_ERROR;
	return ret;
    }

    //handling strings is rather complicated...
    if (a.type == VT_STRING) {
	ret.type = VT_STRING;
	ret.val.str = (String*)sc_malloc(sizeof(String), err);
	if (err->type != E_SUCCESS) {
	    ret.type = VT_ERROR;
	    return ret;
	}
	String* str = ret.val.str;

	if (b.type == VT_STRING) {
	    //figure out the length of the output array and allocate memory
	    str->buf_size = a.val.str->size + b.val.str->size + 1;
	    str->size = str->buf_size - 1;
	    str->buf = (char*)sc_malloc(sizeof(char)*(str->buf_size), err);
	    //copy memory from the old a string
	    strncpy(str->buf, a.val.str->buf, a.val.str->size);
	    //copy memory from the old b string to the end of the last string
	    strncpy(str->buf + a.val.str->size, b.val.str->buf, b.val.str->size);
	    str->buf[str->size] = 0;
	} else if (b.type == VT_ARRAY) {
	    Array* b_arr = (Array*)(b.val.ptr);
	    //let n be the size of the array and m be the number of bytes for the contents of each value we allocate 3 bytes for the '[', ']', and NULL characters. According to the fencepost rule there are n-1 separators (each given two characters) so the total allocated should be m*n + 2*(n-1) + 3 = n*(m+2) + 1.
	    //This is only a heuristic, we must call _grow_s during execution
	    ret.val.str->buf_size = (b_arr->size)*(DEF_ELEMENT_CHARS) + 1;
	    ret.val.str->size = 0;
	    //allocate memory
	    ret.val.str->buf = (char*)sc_malloc(sizeof(char)*(ret.val.str->buf_size), err);
	    //copy memory from the old a string
	    strncpy(ret.val.str->buf, a.val.str->buf, a.val.str->size);
	    
	    size_t off = a.val.str->size;
	    ret.val.str->buf[off] = '[';
	    ++off;
	    for (size_t i = 0; i < b_arr->size; ++i) {
		size_t cur_ele_size = get_format_string_size(((value*)b_arr->buf)[i], DEF_FLOAT_PRECISION);
		// +2 for the separators between strings
		_grow_s(ret.val.str, cur_ele_size + 2, err);
		off += v_fetch_string(((value*)b_arr->buf)[i], ret.val.str->buf + off, cur_ele_size, err);
		if (err->type != E_SUCCESS) {
		    ret.type = VT_ERROR;
		    return ret;
		}
		//only write a ',' if we aren't at the end of the list
		if (i < b_arr->size - 1) {
		    ret.val.str->buf[off] = ',';ret.val.str->buf[off+1] = ' ';off += 2;
		}
	    }
	    //write the end of the array and update the size of the string
	    ret.val.str->buf[off] = ']';ret.val.str->buf[off+1] = 0;
	    ret.val.str->size = off+1;//include the ']'
	    str = ret.val.str;
	} else if (b.type == VT_INT || b.type == VT_FLOAT) {
	    //figure out the length of the output array and allocate memory
	    int n_num_bytes = get_format_string_size(b, DEF_FLOAT_PRECISION) + 1;
	    str->buf_size = len(a) + n_num_bytes;
	    str->size = str->buf_size - 1;
	    str->buf = (char*)sc_malloc(sizeof(char)*(str->buf_size), err);
	    //copy memory from the old a string
	    strncpy(str->buf, a.val.str->buf, a.val.str->size);
	    str->buf[str->size] = 0;

	    //copy the integer value into the end of the string
	    v_fetch_string(b, str->buf + a.val.str->size, n_num_bytes, err);
	    //check for errors and free memory if necessary
	    if (err->type != E_SUCCESS) {
		sc_free(str->buf);
		ret.type = VT_ERROR;
		return ret;
	    }
	} else if (b.type == VT_BOOL) {
	    //alocate memory for the string buffer
	    str->buf_size = len(a) + BOOL_STRING_GROW;
	    str->size = str->buf_size;
	    str->buf = (char*)sc_malloc(sizeof(char)*(str->buf_size), err);
	    //copy memory from the old a string
	    strncpy(str->buf, a.val.str->buf, a.val.str->size);
	    //depending on the value store strings "true" or "false" and set size accordingly
	    if (b.val.i == 0) {
		strncpy(str->buf + a.val.str->size, "false", BOOL_STRING_GROW);
	    } else {
		strncpy(str->buf + a.val.str->size, "true", BOOL_STRING_GROW-1);
		str->size -= 1;//"false" is one character longer than "true"
	    }
	    str->buf[str->buf_size - 1] = 0;
	}
	return ret;
    }
    if (a.type == VT_FLOAT || b.type == VT_FLOAT) {
	//try casting both values to integers or throw an error
	double a_val = v_fetch_float(a, err);
	if (err->type != E_SUCCESS) { return ret; }
	double b_val = v_fetch_float(b, err);
	if (err->type != E_SUCCESS) { return ret; }
	ret.val.f = a_val + b_val;
	ret.type = VT_FLOAT;
	return ret;
    }
    //chars and ints are actually the same type at a lower level (both are stored as ints).
    int a_val = v_fetch_int(a, err);
    if (err->type != E_SUCCESS) { return ret; }
    int b_val = v_fetch_int(b, err);
    if (err->type != E_SUCCESS) { return ret; }

    //set the type and value
    ret.val.i = a_val + b_val;
    //default to char if either value is a char
    if (a.type == VT_CHAR || b.type == VT_CHAR) {
	ret.type = VT_CHAR;
    } else {
	ret.type = VT_INT;
    }
    return ret;
}

/**
 *  Creates a new value object which holds the result of the subtraction operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a-b.
 *  int a, int b: Returns a-b
 *  array a, (any type b): invalid
 *  string a, (any type b): invalid
 *  bool a, bool b: invalid
 */
value op_sub(value a, value b, sc_error* err) {sc_reset_error(err);
    value ret = {0};

    //check for invalid types
    if (a.type == VT_ERROR || b.type == VT_ERROR) {
	sc_set_error(err, E_BADTYPE, "can't add errors");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_ARRAY || b.type == VT_ARRAY) {
	sc_set_error(err, E_BADTYPE, "can't perform addition on array values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_BOOL || b.type == VT_BOOL) {
	sc_set_error(err, E_BADTYPE, "can't perform addition on boolean values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_STRING || b.type == VT_STRING) {
	sc_set_error(err, E_BADTYPE, "can't perform subtraction on string values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_FLOAT || b.type == VT_FLOAT) {
	//try casting both values to integers or throw an error
	double a_val = v_fetch_float(a, err);
	if (err->type != E_SUCCESS) { return ret; }
	double b_val = v_fetch_float(b, err);
	if (err->type != E_SUCCESS) { return ret; }
	ret.val.f = a_val - b_val;
	ret.type = VT_FLOAT;
	return ret;
    }
    //chars and ints are actually the same type at a lower level (both are stored as ints).
    int a_val = v_fetch_int(a, err);
    if (err->type != E_SUCCESS) { return ret; }
    int b_val = v_fetch_int(b, err);
    if (err->type != E_SUCCESS) { return ret; }

    //set the type and value
    ret.val.i = a_val - b_val;
    //default to char if either value is a char
    if (a.type == VT_CHAR || b.type == VT_CHAR) {
	ret.type = VT_CHAR;
    } else {
	ret.type = VT_INT;
    }
    return ret;
}

/**
 *  Creates a new value object which holds the result of the multiply operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a*b.
 *  int a, int b: Returns a*b
 *  array a, (float or int b): Performs a scalar multiply to each element of a so that for each element e of a the new array has the same value as a call to mult(e, a) would. Note that a deep copy of the data is performed, so changes to a may be made safely and the result must be freed.
 *  string a, (any type b): invalid
 *  bool a, bool b: invalid
 */
value op_mult(value a, value b, sc_error* err) {sc_reset_error(err);
    value ret = {0};

    //check for invalid types
    if (a.type == VT_ERROR || b.type == VT_ERROR) {
	sc_set_error(err, E_BADTYPE, "can't add errors");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_ARRAY || b.type == VT_ARRAY) {
	sc_set_error(err, E_BADTYPE, "can't perform multiplication on array values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_BOOL || b.type == VT_BOOL) {
	sc_set_error(err, E_BADTYPE, "can't perform multiplication on boolean values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_STRING || b.type == VT_STRING) {
	sc_set_error(err, E_BADTYPE, "can't perform multiplication on string values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_CHAR || b.type == VT_CHAR) {
	sc_set_error(err, E_BADTYPE, "can't perform multiplication on character values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_FLOAT || b.type == VT_FLOAT) {
	//try casting both values to floats or throw an error
	double a_val = v_fetch_float(a, err);
	if (err->type != E_SUCCESS) { return ret; }
	double b_val = v_fetch_float(b, err);
	if (err->type != E_SUCCESS) { return ret; }
	
	//set the value and the type
	ret.type = VT_FLOAT;
	ret.val.f = a_val * b_val;
	return ret;
    }
    //chars and ints are actually the same type at a lower level (both are stored as ints).
    int a_val = v_fetch_int(a, err);
    if (err->type != E_SUCCESS) { return ret; }
    int b_val = v_fetch_int(b, err);
    if (err->type != E_SUCCESS) { return ret; }
    ret.type = VT_INT;
    ret.val.i = a_val * b_val;

    return ret;
}

/**
 *  Creates a new value object which holds the result of the divide operation applied to a and b. The behaviour depends upon the types of a and b.
 *  Behaviours:
 *  float a, float b or int a, float b or float a, int b: If either a or b is a floating point value, the result is guaranteed to be a floating point type with value a/b.
 *  int a, int b: Returns the greatest integer less than a/b (i.e. floor(a/b)).
 *  array a, (float or int b): Performs a scalar multiply to each element of a so that for each element e of a the new array has the same value as a call to mult(e, a) would.
 *  string a, (any type b): invalid
 *  bool a, bool b: invalid
 */
value op_div(value a, value b, sc_error* err) {sc_reset_error(err);
    value ret = {0};

    //check for invalid types
    if (a.type == VT_ERROR || b.type == VT_ERROR) {
	sc_set_error(err, E_BADTYPE, "can't add errors");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_ARRAY || b.type == VT_ARRAY) {
	sc_set_error(err, E_BADTYPE, "can't perform division on array values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_BOOL || b.type == VT_BOOL) {
	sc_set_error(err, E_BADTYPE, "can't perform division on boolean values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_STRING || b.type == VT_STRING) {
	sc_set_error(err, E_BADTYPE, "can't perform division on string values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_CHAR || b.type == VT_CHAR) {
	sc_set_error(err, E_BADTYPE, "can't perform division on character values");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.type == VT_FLOAT || b.type == VT_FLOAT) {
	//try casting both values to integers or throw an error
	double a_val = v_fetch_float(a, err);
	if (err->type != E_SUCCESS) { return ret; }
	double b_val = v_fetch_float(b, err);
	if (err->type != E_SUCCESS) { return ret; }
	//check for division by zero
	if (b_val == 0) {
	    sc_set_error(err, E_BADTYPE, "divide by zero");
	    ret.type = VT_ERROR;
	    return ret;  
	}

	//set the type and value
	ret.val.f = a_val / b_val;
	ret.type = VT_FLOAT;
	return ret;
    }
    //chars and ints are actually the same type at a lower level (both are stored as ints).
    int a_val = v_fetch_int(a, err);
    if (err->type != E_SUCCESS) { return ret; }
    int b_val = v_fetch_int(b, err);
    if (err->type != E_SUCCESS) { return ret; }
    //check for division by zero
    if (b_val == 0) {
	sc_set_error(err, E_BADTYPE, "divide by zero");
	ret.type = VT_ERROR;
	return ret;  
    }
    //set the type and value
    ret.type = VT_INT;
    ret.val.i = a_val / b_val;

    return ret;
}

/**
 *  Creates a new boolean value object which is set to true if a and b are equal.
 *  Behaviours:
 *  numeric a, numeric b: returns if the values are equal, note that integers are cast to floats for comparison
 *  arrays a, b: returns true if op_eq evaluates true for each pair of elements from a and b
 *  strings a, b: returns true if each character in strings
 *  bool a, bool b: equivalent to (a and b) or (~a and ~b)
 */
value op_eq(value a, value b, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_BOOL;
    switch (a.type) {
    case VT_BOOL:
    if (b.type == VT_BOOL || b.type == VT_INT) {
	ret.val.i = 1;
	if (a.val.i == 0 && b.val.i == 0) { return ret; }
	if (a.val.i != 0 && b.val.i != 0) { return ret; }
	ret.val.i = 0;
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;

    case VT_CHAR:
    case VT_INT:
    if (b.type != VT_INT && b.type != VT_CHAR) {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.val.i == b.val.i) { ret.val.i = 1; } else {ret.val.i = 0; }
    return ret;

    case VT_FLOAT:
    if (b.type == VT_FLOAT || b.type == VT_INT) {
	if (a.val.f == v_fetch_float(b, err)) { ret.val.i = 1; } else {ret.val.i = 0; }
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;

    case VT_STRING:
    if (b.type == VT_STRING) {
	ret.val.i = 0;
	//if they are of unequal length then we know they aren't equal
	if (a.val.str->size != b.val.str->size) { return ret; }
	for (size_t i = 0; i < a.val.str->size; ++i) {
	    if (a.val.str->buf[i] != b.val.str->buf[i]) { return ret; }
	}
	ret.val.i = 1;
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;

    case VT_ARRAY:
    if (b.type == VT_ARRAY) {
	ret.val.i = 0;
	Array* arr_a = (Array*)a.val.ptr;
	Array* arr_b = (Array*)b.val.ptr;
	//if they are of unequal length then we know they aren't equal
	if (arr_a->size != arr_b->size) { return ret; }
	for (size_t i = 0; i < arr_a->size; ++i) {
	    value tmp_ret = op_eq(arr_a->buf[i], arr_b->buf[i], err);
	    //an invalid comparison should result in a false evaluation rather than throwing an error
	    if (err->type != E_SUCCESS) {
		sc_reset_error(err);
		return ret;
	    } else if (tmp_ret.val.i == 0) {
		return ret;
	    }
	}
	ret.val.i = 1;
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;
    }
}

/**
 *  Creates a new boolean value object which is set to true if a is greater than b.
 *  Behaviours:
 *  numeric a, numeric b: returns if the values are equal, note that integers are cast to floats for comparison
 *  arrays invalid
 *  strings a, b: returns true if a comes before b alphabetically
 *  bool a, bool b: invalid
 */
value op_grt(value a, value b, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_BOOL;
    if (a.type == VT_ARRAY || b.type == VT_ARRAY) {
	ret.type = VT_ERROR;
	sc_set_error(err, E_BADTYPE, "Can't use '>' to compare arrays");
	return ret;
    }
    if (a.type == VT_BOOL || b.type == VT_BOOL) {
	ret.type = VT_ERROR;
	sc_set_error(err, E_BADTYPE, "Can't use '>' to compare boolean values");
	return ret;
    }
    switch (a.type) {
    case VT_CHAR:
    case VT_INT:
    if (b.type != VT_INT && b.type != VT_CHAR) {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.val.i > b.val.i) { ret.val.i = 1; } else {ret.val.i = 0; }
    return ret;

    case VT_FLOAT:
    if (b.type == VT_FLOAT || b.type == VT_INT) {
	if (a.val.f > v_fetch_float(b, err)) { ret.val.i = 1; } else {ret.val.i = 0; }
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;

    case VT_STRING:
    if (b.type == VT_STRING) {
	ret.val.i = 0;
	//if they are of unequal length then we know they aren't equal
	if (a.val.str->size < b.val.str->size) { return ret; }
	int tmp = strcmp(a.val.str->buf, b.val.str->buf);
	if (tmp > 0) { ret.val.i = 1; }
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;
    }
    return ret;
}

/**
 *  Creates a new boolean value object which is set to true if a is greater than or equal to b.
 *  Behaviours:
 *  numeric a, numeric b: returns if the values are equal, note that integers are cast to floats for comparison
 *  arrays invalid
 *  strings a, b: returns true if a comes before b alphabetically
 *  bool a, bool b: invalid
 *  NOTE: there is no low level implementation of < or <=, these are evaluated by reversing parameters.
 */
value op_geq(value a, value b, sc_error* err) {sc_reset_error(err);
    value ret = {0};
    ret.type = VT_BOOL;
    if (a.type == VT_ARRAY || b.type == VT_ARRAY) {
	ret.type = VT_ERROR;
	sc_set_error(err, E_BADTYPE, "Can't use '>=' to compare arrays");
	return ret;
    }
    if (a.type == VT_BOOL || b.type == VT_BOOL) {
	ret.type = VT_ERROR;
	sc_set_error(err, E_BADTYPE, "Can't use '>=' to compare boolean values");
	return ret;
    }
    switch (a.type) {
    case VT_CHAR:
    case VT_INT:
    if (b.type != VT_INT && b.type != VT_CHAR) {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
	return ret;
    }
    if (a.val.i >= b.val.i) { ret.val.i = 1; } else {ret.val.i = 0; }
    return ret;

    case VT_FLOAT:
    if (b.type == VT_FLOAT || b.type == VT_INT) {
	if (a.val.f >= v_fetch_float(b, err)) { ret.val.i = 1; } else {ret.val.i = 0; }
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;

    case VT_STRING:
    if (b.type == VT_STRING) {
	ret.val.i = 0;
	//if they are of unequal length then we know they aren't equal
	if (a.val.str->size <= b.val.str->size) { return ret; }
	int tmp = strcmp(a.val.str->buf, b.val.str->buf);
	if (tmp >= 0) { ret.val.i = 1; }
    } else {
	sc_set_error(err, E_BADTYPE, "can't compare types");
	ret.type = VT_ERROR;
    }
    return ret;
    }
    return ret;
}

// ================================== MATH EXPRESSION PARSING ==================================

/**
  * Recursively evaluates the operation tree with the root specified by o. All values are treated as floats during calculation. For integer arithmetic use evali().
  * Returns: the value of the operation tree. Note that boolean operations consider 0.0 false and all other values true.
  */
value eval(struct Operation* o, Stack* st, sc_error* err) {
    value ret = {0};
    ret.type = VT_ERROR;

    sc_set_error(err, E_SUCCESS, "");

    //if this is a leaf then we return the value
    if (o->child_l == NULL || o->child_r == NULL) {
	//if the type is a reference then we should dereference it
	if (o->val.type == VT_OPREF) {
	    //Trying to bitwise and with the low nibble caused a bug in the gcc compiler
	    size_t ind = o->val.val.i;
	    return st->top[ind];
	} else {
	    //otherwise just return the value
	    return o->val;
	}
    }
    value lf = eval(o->child_l, st, err);
    if (err->type != E_SUCCESS) { return ret; }
    value rf = eval(o->child_r, st, err);
    if (err->type != E_SUCCESS) { return ret; }

    //perform the appropriate operation specified by the tree
    switch (o->op) {
    case NOP: return o->val;
    case OP_ADD: return op_add(lf, rf, err);
    case OP_SUB: return op_sub(lf, rf, err);
    case OP_MULT: return op_mult(lf, rf, err);
    case OP_DIV: return op_div(lf,rf, err);
    case OP_EQ: return op_eq(lf, rf, err);
    case OP_LST: return op_grt(rf, lf, err);
    case OP_GRT: return op_grt(lf, rf, err);
    case OP_LEQ: return op_geq(rf, lf, err);
    case OP_GEQ: return op_geq(lf, rf, err);
    case OP_AND:
    ret.type = VT_BOOL;
    //throw an error if the type isn't boolean or convertable to boolean
    if ( (lf.type != VT_BOOL && lf.type != VT_INT)
      || (rf.type != VT_BOOL && rf.type != VT_INT) ) {
	sc_set_error(err, E_BADTYPE, "Can't apply not to non boolean type");
	return ret;
    }
    //set to 1 if the value was false, 0 otherwise
    if (rf.val.i == 0 || lf.val.i == 0) { ret.val.i = 0; } else { ret.val.i = 1; }
    return ret;

    case OP_OR:
    ret.type = VT_BOOL;
    //throw an error if the type isn't boolean or convertable to boolean
    if ((lf.type != VT_BOOL && lf.type != VT_INT) || (rf.type != VT_BOOL && rf.type != VT_INT)) {
	sc_set_error(err, E_BADTYPE, "Can't apply not to non boolean type");
	return ret;
    }
    //set to 1 if the value was false, 0 otherwise
    if (rf.val.i == 0 && lf.val.i == 0) { ret.val.i = 0; } else { ret.val.i = 1; }
    return ret;

    case OP_NOT:
    ret.type = VT_BOOL;
    //throw an error if the type isn't boolean or convertable to boolean
    if (rf.type != VT_BOOL && rf.type != VT_INT) {
	sc_set_error(err, E_BADTYPE, "Can't apply not to non boolean type");
	return ret;
    }
    //set to 1 if the value was false, 0 otherwise
    if (rf.val.i == 0) { ret.val.i = 1; } else { ret.val.i = 0; }
    return ret;

    default: sc_set_error(err, E_SYNTAX, "unrecognized operator");return o->val;
    }
}

/**
  * Helper function which parses a string expression into a tree of operations. The optree can then be evaluated using a call to eval() or evali()
  */
struct Operation* gen_optree(char* str, NamedStack* st, sc_error* err) {
    struct Operation* ret = sc_malloc(sizeof(struct Operation), err);
    ret->op = NOP;
    ret->val.val.f = 0.0;
    ret->child_l = NULL;
    ret->child_r = NULL;

    //store locations of the first instance of different operators. We do this so we can quickly look up new operators if we didn't find any other operators of a lower precedence (such operators are placed in the tree first).
    int first_com_loc = -1;
    int first_add_loc = -1;
    int first_mul_loc = -1;
    int first_bit_loc = -1;

    int first_open_ind = -1;
    int last_close_ind = -1;

    //keep track of the nesting level within parenthetical statements
    int nest_level = 0;
    int found_valid_op = 0;//boolean

    //boolean (if true, then '+' and '-' operators are interpreted as signs instead of operations. We set to true to ensure this behaviour is performed for the first character
    int ignore_op = 1;

    //first try to find base scope addition and subtraction operations
    for (_uint i = 0; str[i] != 0; i++) {
	//keep track of open and close parenthesis, these will come in handy later
	if (str[i] == '(') {
	    ++nest_level;
	    //only set the open index if this is the first match
	    if (first_open_ind == -1) { first_open_ind = i; }
	}
	if (str[i] == ')') {
	    --nest_level;
	    last_close_ind = i;
	    if (nest_level < 0) {
		sc_set_error(err, E_SYNTAX, "encountered close brace without matching open");
		return ret;
	    }
	}

	if (nest_level == 0) {
	    //we need to store whether this was a valid operator separately in order to properly handle parenthetical groups. This valid_op can take on three values FLAG_NORMAL FLAG_OPER and FLAG_AND. The last flag is used to indicate that a logical operation of && or || was obtained. These operations should always be evaluated last (which means they are placed closer to the root of the tree).
	    int this_valid_op = FLAG_NORMAL;
	    //keep track of the number of characters used by the operator
	    int code_n_chars = 1;
	    //check if we found a greater than symbol
	    if (str[i] == '>') {
		this_valid_op = FLAG_COMP;
		first_com_loc = i;
		//if this is a ">=" then we use greater than OR equal otherwise, just greater than
		if (str[i+1] == '=') {
		    ret->op = OP_GEQ;
		    code_n_chars = 2;
		} else {
		    ret->op = OP_GRT;
		}
		ignore_op = 1;
	    }
	    //do the same for less than symbols
	    if (str[i] == '<') {
		this_valid_op = FLAG_COMP;
		first_com_loc = i;
		//if this is a "<=" then we use greater than OR equal otherwise, just greater than
		if (str[i+1] == '=') {
		    ret->op = OP_LEQ;
		    code_n_chars = 2;
		} else {
		    ret->op = OP_LST;
		}
		ignore_op = 1;
	    }
	    //last test for "=" and "==" expressions
	    if (!ignore_op && str[i] == '=') {
		//we can elegantly allow for a single '=' and an '==' to be treated on an equal footing by incrementing
		first_com_loc = i;
		ret->op = OP_ASSN;
		if (str[i+1] == '=') {
		    code_n_chars = 2;
		    ret->op = OP_EQ;
		} else {
		    //if this is an assignment operation then proceed
		    ret->op = OP_ASSN;
		    str[i] = 0;
		    ret->child_l = gen_optree(str, st, err);
		    if (err->type != E_SUCCESS) { free(ret);return NULL; }
		    ret->child_r = gen_optree( str+(i+code_n_chars), st, err );
		    if (err->type != E_SUCCESS) { free(ret);return NULL; }
		    return ret;
		}
		this_valid_op = FLAG_COMP;
		ignore_op = 1;
	    }
	    //store the location of other operators for later
	    if (str[i] == '&' || str[i] == '|' || str[i] == '!' || str[i] == '~') {
		if (str[i] == '!' || str[i] == '~') {
		    //first_and_loc = i;
		    this_valid_op = FLAG_AND;
		    code_n_chars = 1;
		}else if (str[i+1] == str[i]) {
		    //first_and_loc = i;
		    this_valid_op = FLAG_AND;
		    code_n_chars = 2;
		} else {
		    first_bit_loc = i;
		}
		ignore_op = 1;
	    }
	    if (!ignore_op && (str[i] == '+' || str[i] == '-')) {
		first_add_loc = i;
		ignore_op = 1;
		this_valid_op = FLAG_OPER;
	    }
	    if (str[i] == '*' || str[i] == '/') {
		first_mul_loc = i;
		ignore_op = 1;
		this_valid_op = FLAG_OPER;
	    }

	    if (this_valid_op == FLAG_AND) {
		found_valid_op = 1;
		//decide the operator based on the character
		switch (str[i]) {
		case '&': ret->op = OP_AND;break;
		case '|': ret->op = OP_OR;break;
		case '!': //no break is intentional
		case '~': ret->op = OP_NOT;break;
		}
		//recursively examine other expressions
		str[i] = 0;
		struct Operation* tmp_l = gen_optree(str, st, err);
		if (err->type != E_SUCCESS) { free(ret);return NULL; }
		struct Operation* tmp_r = gen_optree(str+(i+code_n_chars), st, err);
		if (err->type != E_SUCCESS) { free(ret);return NULL; }
		ret->child_l = tmp_l;
		ret->child_r = tmp_r;
		return ret;
	    }
	    //we want whitespace to leave the state of operator interpretation unaltered
	    if (this_valid_op == FLAG_NORMAL && !(str[i] == ' ' || str[i] == '\t')) {
		ignore_op = 0;
	    }
	}
    }

    if (!found_valid_op && first_com_loc >= 0) {
	int i = first_com_loc;
	//figure out how many characters this operation uses
	size_t n_skip = 1;
	if (str[i+1] == '=') { n_skip += 1; }
	found_valid_op = 1;

	str[i] = 0;//null terminate the string
	ret->child_l = gen_optree(str, st, err);
	if (err->type != E_SUCCESS) { return NULL; }
	ret->child_r = gen_optree(str + i + 2, st, err);
	if (err->type != E_SUCCESS) { return NULL; }
	return ret;
    }
    if (!found_valid_op && first_add_loc >= 0) {
	int i = first_add_loc;
	if (str[i] == '+') {
	    ret->op = OP_ADD;
	    found_valid_op = 1;
	} else if (str[i] == '-') {
	    ret->op = OP_SUB;
	    found_valid_op = 1;
	}
	if (found_valid_op) {
	    str[i] = 0;//null terminate the string
	    ret->child_l = gen_optree(str, st, err);
	    if (err->type != E_SUCCESS) { return NULL; }
	    ret->child_r = gen_optree(str + i + 1, st, err);
	    if (err->type != E_SUCCESS) { return NULL; }
	}
    }
    if (!found_valid_op && first_mul_loc >= 0) {
	int i = first_mul_loc;
	if (str[i] == '*') {
	    ret->op = OP_MULT;
	    found_valid_op = 1;
	} else if (str[i] == '/') {
	    ret->op = OP_DIV;
	    found_valid_op = 1;
	}
	if (found_valid_op) {
	    str[i] = 0;//null terminate the string
	    ret->child_l = gen_optree(str, st, err);
	    if (err->type != E_SUCCESS) { return NULL; }
	    ret->child_r = gen_optree(str + i + 1, st, err);
	    if (err->type != E_SUCCESS) { return NULL; }
	}
    }
    //last try removing parenthesis
    if (!found_valid_op) {
	//if there isn't a valid parenthetical expression, then we should interpret this as a value string
	if (first_open_ind < 0 || last_close_ind < 0) {
	    value tmp_val = read_value_string(str, 0, err); 
	    if (err->type != E_SUCCESS) {
		//if there was an error then we still have to check to see if we can substitute a variable from the NamedStack if one was provided.
		if (st) {
		    //iterate the stack looking for a match
		    int i = 0;
		    //trim whitespace
		    str = _trim_whitespace(str);
		    //iterate through every item in the stack looking for a match
		    for (HashedItem* h = st->top; h != st->bottom; ++h) {
			if (strcmp(h->key, str) == 0) {
			    //we have to reset the error if we actually found a match
			    sc_reset_error(err);
			    //if we found a match then we set the type to indicate that the value should be read from the stack
			    ret->val.type = VT_OPREF;
			    ret->val.val.i = i;
			    return ret;
			}
			++i;
		    }
		}
		free_value(&tmp_val);
		return NULL;
	    }
	    ret->val = tmp_val;
	    return ret;
	}
	//if there is a valid parenthetical expression free the memory we allocated for ret and create a new Operation
	sc_free(ret);
	str[last_close_ind] = 0;
	struct Operation* tmp = gen_optree(str + first_open_ind + 1, st, err);
	if (err->type != E_SUCCESS) { sc_free(tmp);return NULL; }
	return tmp;
    }
    return ret;
}

/**
 * Frees the operation pointed to by op and all of its children
 */
void free_Operation(struct Operation* op) {
    if (op != NULL) {
	free_Operation(op->child_l);
	free_Operation(op->child_r);
	sc_free(op);
    }
}

// ============================ Instruction Buffer ============================

/**
 * make a new empty instruction_buffer struct
 */
instruction_buffer make_instruction_buffer(sc_error* err) {
    instruction_buffer ret = {0};
    ret.cap = DEF_NUM_INS;
    ret.n_insts = 0;
    ret.buf = (union Instruction*)sc_malloc(sizeof(union Instruction)*ret.cap, err);
    return ret;
}

/**
 * Append the instruction inst to the end of buf and store potential errors in err
 */
void append_Instruction(instruction_buffer* buf, union Instruction inst, sc_error* err) {
    if (buf) {
	//ensure there is enough space to store the instructions
	if (buf->n_insts >= buf->cap) {
	    buf->cap = 2*(buf->cap);
	    if (buf->n_insts >= buf->cap) { buf->cap += buf->n_insts; }
	    buf->buf = (union Instruction*)sc_realloc(buf->buf, sizeof(union Instruction)*(buf->cap), err);
	}
	//if there is an E_NOMEM, buf->buf may be NULL, check
	if (buf->buf) {
	    buf->buf[buf->n_insts] = inst;
	    buf->n_insts += 1;
	} else {
	    free(buf->buf);
	    buf->n_insts = -1;
	}
    }
}

/**
 * Append n_in instructions from the array inst to the end of buf and store potential errors in err
 */
void append_Instructions(instruction_buffer* buf, size_t n_in, union Instruction* inst, sc_error* err) {
    if (buf) {
	//ensure there is enough space to store the instructions
	if (buf->n_insts + n_in >= buf->cap) {
	    buf->cap = 2*(buf->cap) + n_in;
	    buf->buf = (union Instruction*)sc_realloc(buf->buf, sizeof(union Instruction)*(buf->cap), err);
	}
	//if there is an E_NOMEM, buf->buf may be NULL, check
	if (buf->buf) {
	    for (size_t i = 0; i < n_in; ++i) {
		buf->buf[buf->n_insts + i] = inst[i];
	    }
	    buf->n_insts += n_in;
	} else {
	    free(buf->buf);
	    buf->n_insts = -1;
	}
    }
}

/**
 * Deallocate memory used by the instruction_buffer buf
 */
void free_instruction_buffer(instruction_buffer* buf) {
    if (buf) {
	if(buf->buf) {
	    size_t i = 0;
	    while (i < buf->n_insts) {
		size_t tmp = buf->buf[i].i;
		size_t skip = 1;
		//if this instruction is a constant type we should free the next value
		if (tmp & HI_NIB == INS_HH_C && i < buf->n_insts - 1) {
		    free_value(buf->buf[i+1].ptr);
		    sc_free(buf->buf[i+1].ptr);
		    skip += 1;
		}
		//if this is a move instruction, then potentially there is a move from a constant allocated value that we will need to free.
		if ((tmp & LO_NIB == INS_MOV) && (tmp & 0x30 == INS_HL_C) && (i < buf->n_insts - 2)) {
		    free_value(buf->buf[i+2].ptr);
		    sc_free(buf->buf[i+2].ptr);
		    skip += 1;
		}
		i += skip;
	    }
	}
	free(buf->buf);
    }
}

// ============================ FUNCTIONS ============================

/**
 * Helper function to lookup the stack index associated with a given stack variable. If the given name is not found -1 is returned.
 */
int get_stack_ind(size_t stack_size, char** name_stack, char* name) {
    for (size_t i = 0; i < stack_size; ++i) {
	if (strcmp(name_stack[i], name) == 0) { return i; }
    }
    return -1;
}

/**
 * This is a helper function which reads the specified token and pushes it onto the named stack st.
 */
HashedItem _push_valtup(NamedStack* st, char* token, sc_error* err) {
    //read the value and check for errors
    HashedItem ret = _read_valtup(token, err);
    if (err->type == E_SUCCESS) {
	//push said value onto the stack and check for errors
	push_n(st, ret.key, ret.val, err);
	if (err->type != E_SUCCESS) { free_NamedStack(st); }
    }
    return ret;
}

/**
 * Helper function which parses an rval string into a sequence of instructions. This is done by recursively looking up values from the provided context and replacing with optrees or functions to evaluate where appropriate. The resulting set of instructions is appended to i_list and i_size is modified appropriately. The string str is modified "in place".
 * param c: the context of the calling function
 * param str: the string to parse into a list of instructions
 * param force_1ret: if this is 1, then all functions which are parsed must return one value, or an error will be thrown. This is applied to expressions in optrees and arguments in functions.
 * param i_list: this is the current list of instructions. This list will be modified by appending results to the end. As such this function is not parallelizable without making some careful considerations.
 * param i_size: the current size of the list of instructions. This is modified during the execution of the function.
 * param err: check for errors
 * returns: on success, the number of values pushed onto the stack during execution is returned. on failure, a negative value is returned
 */
int _parse_rval(context* c, char* str, int force_1ret, instruction_buffer* buf, sc_error* err) {sc_reset_error(err);
    //we create a temporary dyamic array to hold instructions
    //TODO: figure out a better way to do this
    /*size_t n_insts = *i_size;
    size_t tmp_cap = 2*n_insts;
    union Instruction* tmp_insts = (union Instruction*)sc_malloc(sizeof(union Instruction)*tmp_cap, err);
    if (err->type != E_SUCCESS) { return -1; }*/

    int is_var = 1;
    int st_off = -1;

    HashedItem* tmp_hash = NULL;

    //iterate through str to see if this is a function
    for (size_t i = 0; str[i] != 0; ++i) {
	if (str[i] == '('/*)*/) {
	    char* arg_ilist = _get_enclosed(str + i, "(", ")");
	    str[i] = 0;
	    char* func_name = _trim_whitespace(str);
	    int f_ind = search_val(c, func_name, &tmp_hash);

	    //throw an error if we couldn't find the function
	    if (f_ind < -1) {
		sc_set_error(err, E_BADVAL, "");
		snprintf(err->msg, DTG_MAX_MSG_SIZE, "Couldn't find function \"%s\"", str);
		return -1;
	    }
	    //throw an error if the value isn't a function
	    if (tmp_hash->val.type != VT_FUNC) {
		sc_set_error(err, E_BADVAL, "");
		snprintf(err->msg, DTG_MAX_MSG_SIZE, "\"%s\" is of non function type %d", str, tmp_hash->val.type);
		return -1;
	    }

	    //cast the pointer to a function struct
	    function* f = (function*)(tmp_hash->val.val.ptr);

	    //throw an error if the function doesn't have the correct number of return values
	    if (force_1ret && f->n_rets != 1) {
		sc_set_error(err, E_BADVAL, "functions in operations or function arguments may only return one value");
		return -1;
	    }

	    //break up the inside of the parenthesis by commas to find arguments
	    size_t n_args_i = 0;
	    char** args_i = csv_to_list(arg_ilist, ',', &n_args_i, err);
	    if (err->type != E_SUCCESS) { return -1; }

	    //iterate over each argument and parse it into a list of instructions
	    for (size_t j = 0; j < n_args_i; ++j) {
		char* arg = _trim_whitespace(args_i[j]);
		int tmp_err = _parse_rval(c, arg, 1, buf, err);
		if (tmp_err < 0) {
		    free(args_i);
		    return tmp_err;
		}
	    }
	    is_var = 0;

	    //append the function call instruction to the buffer
	    union Instruction tmp[2];
	    tmp[0].i = INS_FN_EVAL;
	    if (f_ind < 0) {
		tmp[0].i = tmp[0].i | INS_HH_G;
		tmp[1].ptr = tmp_hash->key;
	    } else {
		tmp[0].i = tmp[0].i | INS_HH_S;
		tmp[1].i = f_ind;
	    }
	   
	    append_Instructions(buf, 2, tmp, err);
	    if (err->type != E_SUCCESS) { return -2; }
	    return f->n_rets;
	} else if (str[i] == '['/*]*/) {
	    char* arg = _get_enclosed(str + i, "[", "]");
	    str[i] = 0;
	    char* arr_name = _trim_whitespace(str);
	    int f_ind = search_val(c, arr_name, &tmp_hash);

	    //throw an error if we couldn't find the array
	    if (f_ind < -1) {
		sc_set_error(err, E_BADVAL, "");
		snprintf(err->msg, DTG_MAX_MSG_SIZE, "Couldn't find Array \"%s\"", str);
		return -1;
	    }
	    //throw an error if the value isn't an array
	    if (tmp_hash->val.type != VT_ARRAY) {
		sc_set_error(err, E_BADVAL, "");
		snprintf(err->msg, DTG_MAX_MSG_SIZE, "\"%s\" is of non Array type %d", str, tmp_hash->val.type);
		return -1;
	    }

	    //cast the pointer to a function struct
	    Array* a = (Array*)(tmp_hash->val.val.ptr);

	    //parse slices
	    size_t n_args_i;
	    char** args_i = csv_to_list(arg, ':', &n_args_i, err);
	    if (err->type != E_SUCCESS) { return -1; }
	    //depending on the number of colons we interpret the index differently
	    if (n_args_i == 1) {
		int ind = sc_atoi(args_i[0], err);
		if (err->type == E_SUCCESS) {
		    //make sure that the index is in bounds
		    if (a->size <= ind || ind < 0) {
			sc_set_error(err, E_BADVAL, "");
			snprintf(err->msg, DTG_MAX_MSG_SIZE, "index %d out of bounds for array %s of size %d", ind, str, a->size);
			return -1;
		    }
		    union Instruction tmp[5];
		    //read the appropriate array index into the register A
		    if (f_ind < 0) {
			tmp[0].i = INS_IND_READ | INS_HH_G;
			tmp[1].ptr = tmp_hash->key;

		    } else {
			tmp[0].i = INS_IND_READ | INS_HH_S;
			tmp[1].i = f_ind;
		    }
		    //push register A onto the stack
		    tmp[2].i = ind;
		    tmp[3].i = INS_PUSH | INS_HH_R;
		    tmp[4].i = 0;
		    append_Instructions(buf, 5, tmp, err);
		    return 1;
		} else {
		    return -1;
		}
	    //if there are at least two entries we interpret the first two as the start and end respectively
	    } else if (n_args_i >= 2) {
		//figure out the start and end values
		/*TODO: uncomment
		int start = sc_atoi(args_i[0], err);
		if (err->type != E_SUCCESS) { return -1; }
		int end = sc_atoi(args_i[1], err);
		if (err->type != E_SUCCESS) { return -1; }
		
		//interpret the last number as a span so we only include every n entries
		int span = 1;
		if (n_args_i > 2) {
		    span = sc_atoi(args_i[2], err);
		    if (err->type != E_SUCCESS) { return -1; }
		}

		//parse negative values as offsets from the end
		if (start < 0) { start = a->size + start; }
		if (end < 0) { end = a->size + end; }
		//make sure both indices are in bounds
		if (start < 0 || end < 0 || start > end ||
		      start >= a->size || end >= a->size) {
		    sc_set_error(err, E_BADVAL, "");
		    snprintf(err->msg, DTG_MAX_MSG_SIZE, "indices %d:%d out of bounds for array %s of size %d", start, end, str, a->size);
		    return -1;
		}

		//throw an error if the slice doesn't have the correct number of return values
		if (force_1ret && end-start > 1) {
		    sc_set_error(err, E_BADVAL, "functions in operations or function arguments may only return one value");
		    return -1;
		}

		//place a pointer to the array in register A.
		union Instruction tmp[3];
		if (f_ind < 0) {
		    tmp[0].i = INS_MOV & INS_HH_R & INS_HL_G;
		    tmp[1].i = 0;
		    tmp[2].ptr = tmp_hash->key;
		} else {
		    tmp[0].i = INS_MOV & INS_HH_R & INS_HL_S;
		    tmp[1].i = 0;
		    tmp[2].i = f_ind;
		}

		//set instructions to push values onto stack

		for (size_t j = start; j < end; j += span) {
		    if (f_ind < 0) {
			tmp[0].i = INS_PUSH_A;
			tmp[1].ptr = tmp_hash->key;
			tmp[2].i = j;
		    } else {
			tmp[0].i = INS_PUSH_A_ST;
			tmp[1].i = f_ind;
			tmp[2].i = j;
		    }
		    append_Instructions(buf, 2, tmp, err);
		}
		return end - start;*/
	    }
	}
    }
    //if we reach the end of the loop we treat the name as a variable
    char* t_str = _trim_whitespace(str);
    int f_ind = search_val(c, t_str, &tmp_hash);
    //create an array for the instruction
    union Instruction tmp[2];
    if (f_ind < -1) {
	//in the event that we didn't find a variable with a matching name, try parsing the value as a constant. For example 'int i = 1234' should create a new value with the name i and an integer type value, val, with val.i = 1234.
	//TODO: allow the caller to supply hints for value type
	//allocate memory for a constant value
	value* tmp_val = (value*)sc_malloc(sizeof(value), err);
	*tmp_val = read_value_string(t_str, VT_UNDEF, err);
	//if there was an error, try parsing as an operation
	/*if (err->type != E_SUCCESS) {
	    sc_reset_error(err);
	    tmp[0].i = INS_OP_EVAL;
	    tmp[1].ptr = gen_optree(str, &(c->callstack), err);
	    if (err->type != E_SUCCESS) {
		return -1;
	    }
	}*/
	tmp[0].i = INS_PUSH | INS_HH_C;
	tmp[1].ptr = tmp_val;
    } else if (f_ind == -1) {
	//otherwise push from global or stack memory accordingly
	tmp[0].i = INS_PUSH | INS_HH_G;
	tmp[1].ptr = tmp_hash->key;
    } else {
	tmp[0].i = INS_PUSH | INS_HH_S;
	tmp[1].i = f_ind;
    }

    append_Instructions(buf, 2, tmp, err);
    return 1;
}

/**
 *  A helper function to parse the string str (of the form <type> <name>) into a type and value string.
 *  param str: string to parse, including the type
 *  param type: the type to cast the resulting value to
 *  param off: the offset specifying the end of the type specifier string in str
 *  param n_st: named stack to push the new value to
 *  param i_buf: instruction buffer to write to
 *  param err: track errors
 *  returns: the number of characters from the name which were read, see the specifications for read_dtg_word
 */
size_t __read_declaration(char* str, Valtype_e type, size_t off, context* c, instruction_buffer* i_buf, sc_error* err) {
    char next_word[N_WORD_BYTES];

    //we can safely write to the location that has the variable type since a code for the type will be stored in the instruction buffer
    size_t ret = read_dtg_word(str, off, str, N_WORD_BYTES);

    //set the value to a sane default
    value tmp;
    tmp.type = type;
    tmp.val.i = 0;

    //push onto the list of named items
    char* dec_namestr = strdup(str);
    push_n(&(c->callstack), dec_namestr, tmp, err);
    if (err->type != E_SUCCESS) { return ret; }

    union Instruction tmp_ins[2];
    //read the next word to check if there is an assignment
    size_t new_off = read_dtg_word(str, ret, next_word, N_WORD_BYTES);
    if (next_word[0] == '=' && next_word[1] == 0) {
	//read the term after the '=' sign
	new_off += read_dtg_word(str, new_off, next_word, N_WORD_BYTES);

	//first try interpreting the rval as a literal assignment
	/*tmp = read_value_string(next_word, type, err);
	//if that didn't work, then search the current context
	if (err->type != E_SUCCESS) {
	    //look for the matching rval and perform a deep copy into tmp
	    HashedItem* hash;
	    int ind = search_val(c, next_word, &hash);
	    if (ind >= 0) {
		tmp_ins[0].i = INS_PUSH | INS_HH_S;
		tmp_ins[1].i = (size_t)ind;
	    } else if (ind == -1) {
		tmp_ins[0].i = INS_PUSH | INS_HH_G;
		tmp_ins[1].ptr = hash->key;
	    } else {
		sc_set_error(err, E_BADVAL, "");
		snprintf(err->msg, DTG_MAX_MSG_SIZE, "bad rval %s", next_word);
		return ret;
	    }
	} else {
	    tmp_ins[0].i = INS_PUSH | INS_HH_C;
	    value* tmp_val = sc_malloc(sizeof(value), err);
	    tmp_val->type = tmp.type;
	    tmp_val->val = tmp.val;
	    tmp_ins[0].ptr = tmp_val;
	}
	//update the return value
	ret = new_off;*/
	_parse_rval(c, next_word, 1, i_buf, err);
    }

    /*append_Instructions(i_buf, 2, tmp_ins, err);
    if (err->type != E_SUCCESS) { return ret; }*/

    return ret;
}

/**
 * Interprets the string str into an executable function. The number of arguments and return values are interpreted.
 */
function make_function(context* con, char* str, sc_error* err) {
    function ret = {0};
    //we need to store the current stack index so that we can erase everything we added after completion
    size_t stack_start = get_size_n(con->callstack);

    //find the arguments and return lists and the main program list
    char* endptr;
    char* arglist = _get_enclosed_r(str, &endptr, "(", ")");
    //check for errors or try to perform an interpretation without the parenthesis
    if (endptr == NULL) {
	//this is only an error if there was no matching () expression. Otherwise use => as the separator
	if (arglist != str) {
	    sc_set_error(err, E_SYNTAX, /*(*/"Expected termintating ')' for argument list.");
	    return ret;
	} else {
	    endptr = strchr(str, '=');
	    //make sure we found an argument
	    if (endptr == NULL) {
		sc_set_error(err, E_SYNTAX, "Couldn't identify argument list in function string.");
		return ret;
	    }
	    //see if this is a 2 character expression of the form =>
	    if (endptr[1] = '>') {
		endptr[0] = 0;
		*endptr = *endptr + 2;
	    } else {
		//if it isn't then we try once more under the assumption that this is a function assignment of the form "f = arg => out {}"
		endptr = strchr(str, '=');
		if (endptr == NULL) {
		    sc_set_error(err, E_SYNTAX, "Couldn't identify argument list in function string.");
		    return ret;
		}
		if (endptr[1] = '>') {
		    endptr[0] = 0;
		    *endptr = *endptr + 2;
		}
	    }
	}
    }

    //find the return list enclosed in a parenthetical pair
    char* start = endptr;
    char* retlist = _get_enclosed_r(start, &endptr, "(", ")");
    if (endptr == NULL && retlist != start) {
	sc_set_error(err, E_SYNTAX, /*(*/"Expected termintating ')' for return list.");
	return ret;
    }

    //find the main block enclosed in a pair of curly braces
    char* main_block = _get_enclosed_r(endptr, &endptr, "{", "}");
    if (endptr == NULL) {
	sc_set_error(err, E_SYNTAX, /*{*/"Expected termintating '}' for return list.");
	return ret;
    }

    /*REMOVE?
     * Array* args = make_Array(sizeof(value), 1, err);

    //break the arglist by commas
    char* saveptr;
    char* token = strtok_r(arglist, ",", &saveptr);
    if (err->type != E_SUCCESS) { free_Array(args);return ret; }
    size_t i = 0;
    //iterate over every comma
    while (token != NULL) {
	//expand the array to accomodate the new element and append it
	_grow_a(args, 1, err);
	if (err->type != E_SUCCESS) { free_Array(args);return ret; }
	args->buf[i].val.str->buf = _trim_whitespace(token);
	++i;
	token = strtok_r(NULL, ",", &saveptr); 
    }
    ret.n_args = i;*/

    //create a stack to hold values by name. Since we hold off execution until later the val of each stack element is actually a string which holds the name of the variable which is then used for substitutions
    //NamedStack name_stack = make_NamedStack(err);
    if (err->type != E_SUCCESS) { return ret; }

    char* saveptr;
    char* type_name;
    char* token = strtok_r(arglist, ",", &saveptr);
    ret.n_args = 0;
    //iterate over every comma
    while (token != NULL) {
	//read the value and check for errors
	_push_valtup(&(con->callstack), token, err);
	if (err->type != E_SUCCESS) { return ret; }

	ret.n_args += 1;
	token = strtok_r(NULL, ",", &saveptr); 
    }

    /*REMOVE?
     * //create two lists, one to hold the types and one to hold names. Names are only used for this function. The final returned value will only contain references to stack indices.
    ret.argument_types = sc_malloc(sizeof(Valtype_e)*(ret.n_args), err);
    if (err->type != E_SUCCESS) { free_Array(args);return ret; }

    //create a stack for names
    size_t n_names = 0;
    size_t names_cap = ret.n_args;
    char** name_stack = (char**)sc_malloc(sizeof(char*)*names_cap, err);
    if (err->type != E_SUCCESS) { free_Array(args);sc_free(ret.argument_types);return ret; }

    //assign argument types by finding the last separating space in the string
    for (size_t i = 0; i < names_cap; ++i) {
	token = strtok_r(args->buf[i].val.str->buf, " \t\n", &saveptr);
	ret.argument_types[i] = read_valtype(token);
	token = strtok_r(NULL, " \t\n", &saveptr);
	//assign the name only if it is valid
	if (token) {
	    n_names += 1;
	    name_stack[i] = token;
	} else {
	    //set a helpful error message
	    sc_set_error(err, E_SYNTAX, "");
	    snprintf(err->msg, DTG_MAX_MSG_SIZE, "argument %d has no name", i);
	    //free allocated memory
	    sc_free(ret.argument_types);
	    sc_free(name_stack);
	    free_Array(args);
	    return ret;
	}
    }*/

    //break the retlist by commas
    Array* rets = _make_Array(sizeof(value), 1, err);
    if (err->type != E_SUCCESS) { free_Array(rets);return ret; }

    //break the arglist by commas
    token = strtok_r(retlist, ",", &saveptr);
    size_t i = 0;
    while (token != NULL) {
	//expand the array to accomodate the new element and append it
	_grow_a(rets, 1, err);
	if (err->type != E_SUCCESS) { free_Array(rets);return ret; }
	rets->buf[i] = v_make_string(_trim_whitespace(token), err);
	if (err->type != E_SUCCESS) {
	    free_Array(rets);
	    //free_NamedStack(&name_stack);
	}
	++i;
	token = strtok_r(NULL, ",", &saveptr); 
    }
    ret.n_rets = i;

    //assign return types by finding the last separating space in the string
    ret.return_types = sc_malloc(sizeof(Valtype_e)*(ret.n_rets), err);
    for (size_t i = 0; i < ret.n_rets; ++i) {
	token = strtok_r(rets->buf[i].val.str->buf, " \t\n", &saveptr);
	ret.return_types[i] = read_valtype(token, err);
	if (err->type != E_SUCCESS) {
	    //free_NamedStack(&name_stack);
	    sc_free(ret.return_types);
	    ret.return_types = NULL;
	    return ret;
	}
    }
    //free_Array(args);
    free_Array(rets);

    //actually start parsing the main body
    //char** f_lines = csv_to_list(main_block, ';', &(ret.n_instructions), err);

    ret.buf = make_instruction_buffer(err);
    if (err->type != E_SUCCESS) {
	//free_NamedStack(&name_stack);
	sc_free(ret.return_types);
	ret.return_types = NULL;
	return ret;
    }

    int commented = 0;
    //store instruction "tokens". These include statements like "if", "while", "for", type indicators (e.g. bool, int), value names and literals
    char* ins_toks[N_INS_TOKS];
    int n_toks = 0;
    int paren_nest = 0;
    int block_nest = 0;
    //we have to keep track of the location of block size indices so that we can properly set our GOTOs
    Stack block_inds = make_Stack(err);
    if (err->type != E_SUCCESS) {
	//free_NamedStack(&name_stack);
	sc_free(ret.return_types);
	ret.return_types = NULL;
	free_instruction_buffer(&(ret.buf));
	return ret;
    }

    //iterate over the main block string
    i = 0;
    char next_word[N_WORD_BYTES];
    char scnd_word[N_WORD_BYTES];
    size_t n_read = 0;
    while (main_block[i] != 0) {
	if (!commented) {
	    if (main_block[i] == '\n' && paren_nest == 0) {
		//check if we have a complete statement
		//grow the number of instructions and check for errors if necessary
		/*if (ret.n_instructions >= ins_cap) {
		    ins_cap *= 2;
		    ret.i_list = (union Instruction*)sc_realloc(ret.i_list, sizeof(union Instruction)*ins_cap, err);
		    if (err->type != E_SUCCESS) {
			free_NamedStack(&name_stack);
			sc_free(ret.return_types);
			sc_free(ret.i_list);
			ret.i_list = NULL;
			return ret;
		    }
		}*/

		//read the next word
		n_read = read_dtg_word(main_block, i, next_word, N_WORD_BYTES);

		//check if the next word is "while" or "if" (the start of a block)
		char blk_type = INS_NOP;
		if (strcmp(next_word, "while") == 0) {
		    blk_type = BLOCK_WHILE;
		} else if (strcmp(next_word, "if") == 0) {
		    blk_type = BLOCK_BRANCH;
		} else if (strcmp(next_word, "else") == 0) {
		    n_read += read_dtg_word(main_block, i+n_read, next_word, N_WORD_BYTES);
		    //check if this is an "else if or just a regular if
		    if (strcmp(next_word, "if") == 0) {
			blk_type = BLOCK_SUB_BRANCH;
		    } else {
			blk_type = BLOCK_ELSE;
		    }
		} else if (strcmp(next_word, "return") == 0) {
		    blk_type = INS_RETURN;
		}

		//check for declarations
		if (strcmp(next_word, "bool") == 0) {
		    n_read += __read_declaration(main_block+i, VT_BOOL, n_read, con, &(ret.buf), err);
		} else if (strcmp(next_word, "char") == 0) {
		    n_read += __read_declaration(main_block+i, VT_CHAR, n_read, con, &(ret.buf), err);
		} else if (strcmp(next_word, "int") == 0) {
		    n_read += __read_declaration(main_block+i, VT_INT, n_read, con, &(ret.buf), err);
		} else if (strcmp(next_word, "float") == 0) {
		    n_read += __read_declaration(main_block+i, VT_FLOAT, n_read, con, &(ret.buf), err);
		} else if (strcmp(next_word, "string") == 0) {
		    n_read += __read_declaration(main_block+i, VT_STRING, n_read, con, &(ret.buf), err);
		} else if (strcmp(next_word, "array") == 0) {
		    n_read += __read_declaration(main_block+i, VT_FLOAT, n_read, con, &(ret.buf), err);
		} else if (strcmp(next_word, "func") == 0) {
		    //TODO: this will require some special implementation
		    n_read += __read_declaration(main_block+i, VT_FUNC, n_read, con, &(ret.buf), err);
		}

		//handle all other types of instruction (mostly assignments and function executions)
		if (blk_type == INS_NOP) {
		    n_read += read_dtg_word(main_block, i+n_read, scnd_word, N_WORD_BYTES);
		    if (scnd_word[0] == '=') {
			//read the lval
			size_t n_l = 0;
			char** args_i = csv_to_list(next_word, ',', &n_l, err);
			if (err->type != E_SUCCESS) {
			    //free_NamedStack(&name_stack);
			    sc_free(ret.return_types);
			    ret.return_types = NULL;
			    free_instruction_buffer(&(ret.buf));
			    return ret;
			}

			//read the string after the equal sign
			n_read += read_dtg_word(main_block, i+n_read, scnd_word, N_WORD_BYTES);
			int n_rvals = _parse_rval(con, scnd_word, 0, &(ret.buf), err);
			if (err->type != E_SUCCESS) {
			    //free_NamedStack(&name_stack);
			    sc_free(ret.return_types);
			    ret.return_types = NULL;
			    free_instruction_buffer(&(ret.buf));
			    return ret;
			}
			//TODO: allow more flexibility
			if (n_l == n_rvals) {
			    //lookup each term in the lval
			    for (size_t j = 0; j < n_l; ++j) {
				//try finding the value by name or create it
				HashedItem* tmp_hash = NULL;
				int f_ind = search_val(con, args_i[j], &tmp_hash);
				
				//append the assignment operation to the instruction buffer
				union Instruction tmp[2];
				if (f_ind < -1) {
				    //If we didn't find the value in the stack or global memory then we need to create a new one
				    //TODO: implement global keyword
				    _push_valtup(&(con->callstack), args_i[j], err);
				    //NOTE: we don't need to do any further stack manipulation, as _parse_rval has already pushed the appropriate value onto the stack
				} else if (f_ind == -1) {
				    tmp[0].i = INS_POP & INS_HH_G;
				    tmp[1].ptr = tmp_hash->key;
				    append_Instructions(&(ret.buf), 2, tmp, err);
				} else {
				    tmp[0].i = INS_POP & INS_HH_S;
				    tmp[1].i = f_ind;
				    append_Instructions(&(ret.buf), 2, tmp, err);
				}

				if (err->type != E_SUCCESS) {
				    //free_NamedStack(&name_stack);
				    sc_free(ret.return_types);
				    ret.return_types = NULL;
				    free_instruction_buffer(&(ret.buf));
				    return ret;
				}
			    }
			}
		    } else if (scnd_word[0] == ';') {
			size_t n_l = 0;
			char** args_i = csv_to_list(next_word, ',', &n_l, err);
			for (size_t j = 0; j < n_l; ++j) {
			    //try finding the value by name
			    HashedItem* tmp_hash = NULL;
			    int f_ind = search_val(con, args_i[j], &tmp_hash);
			    if (f_ind < -1) {
				//If we didn't find the value in the stack or global memory then we need to create a new one
				//TODO: implement global keyword
				_push_valtup(&(con->callstack), args_i[j], err);
				if (err->type != E_SUCCESS) {
				    //free_NamedStack(&name_stack);
				    sc_free(ret.return_types);
				    ret.return_types = NULL;
				    free_instruction_buffer(&(ret.buf));
				    return ret;
				}
				//append the assignment operation to the instruction buffer
				union Instruction tmp[2];
				tmp[0].i = INS_MAKE_VAL;
				tmp[1].i = (con->callstack).top->val.type;
				append_Instructions(&(ret.buf), 2, tmp, err);
			    } else {
				//if we already created this value then we need to throw an error
				sc_set_error(err, E_BADVAL, "");
				snprintf(err->msg, DTG_MAX_MSG_SIZE, "Redeclaration of value name %s", args_i[j]);
				//free_NamedStack(&name_stack);
				sc_free(ret.return_types);
				ret.return_types = NULL;
				free_instruction_buffer(&(ret.buf));
				return ret;
			    }
			}
		    }
		}

		//read the instruction
		/*if (blk_loc > 0) {
		    //make sure we have enough parameters to form an instruction
		    if (n_toks < 3) {
			sc_set_error(err, E_SYNTAX, "while loop requires condition and execution block");
		    }
		    //add the instruction and push the block index onto the stack. This way, we can pop the last block off the stack when we encounter a close curly brace.
		    ret.i_list[ret.n_instructions].i = INS_JNCOND;
		    ret.i_list[ret.n_instructions+1].ptr = gen_optree(ins_toks[1], err);
		    value tmp_val = v_make_int(ret.n_instructions+2, err);
		    //store the type of instruction (while statements require an additional unconditional jump at the end of the block
		    tmp_val.type = blk_type;

		    push(&block_inds, tmp_val, err);
		    //check for errors
		    if (err->type != E_SUCCESS) {
			sc_free(name_stack);
			sc_free(ret.argument_types);
			sc_free(ret.return_types);
			sc_free(ret.i_list);
			ret.argument_types = NULL;
			ret.return_types = NULL;
			ret.i_list = NULL;
			return ret;
		    }
		} else if (n_toks == 1) {
		    //check if we hit the end of a block
		}
		ret.i_list[ret.n_instructions] = */
	    }
	}
	i += n_read;
    }

    //cleanup the stack
    size_t n_added = get_size_n(con->callstack) - stack_start;
    for (size_t i = 0; i < n_added; ++i) {
	HashedItem tmp = pop_n(&(con->callstack), err);
	free_HashedItem(&tmp);
    }
    return ret;
}

/**
 * Free memory used by the function pointed to by f.
 */
void free_function(function* f) {
    if (f) {
	//sc_free(f->argument_types);
	if (f->return_types) { sc_free(f->return_types); }
	//for (size_t i = 0; i < f->n_consts; ++i) { free_value(f->consts[i]); }
	//sc_free(f->consts);
	free_instruction_buffer(&(f->buf));
    }
}

/**
 * Executes the function f using the context c to infer the stack and global variables. The context is altered by this proceedure as f pops arguments off of the stack (including the caller) and pushes returned values onto the stack.
 */
void execute_function(context* c, function* f, sc_error* err) {

}

/**
 * Lookup the function named func_name and an array of arguments args and n_args
 * param c: current context with stack and global variables
 * param func_name: name of the function in global scope to call
 * param n_args: number of arguments accessible in the args array
 * param args: a list of arguments to be supplied to the function for calling
 * param res: a pointer to which the list of return values will be stored
 * param err: save error messages
 * returns the number of return values on success or -1 on error
 */
int call_func_by_name(context* c, const char* func_name, size_t n_args, char** args, value** results, sc_error* err) {
    //lookup the name in the hashtable
    HashedItem* v_f = lookup(&(c->global), func_name);

    //only proceed if we found the function with the proper name
    if (!v_f) {
	sc_set_error(err, E_BADVAL, "");
	snprintf(err->msg, DTG_MAX_MSG_SIZE, "Couldn't find function %s", func_name);
	return -1;
    }

    //base case, simply return the associated value
    if (v_f->val.type != VT_FUNC) {
	push_n(&(c->callstack), "", v_f->val, err);
	/*sc_set_error(err, E_BADVAL, "");
	snprintf(err->msg, DTG_MAX_MSG_SIZE, "%s is not a function", func_name);
	return -1;*/
    }

    //make sure the function has the proper number of arguments or throw an error
    function* f = (function*)(v_f->val.val.ptr);
    if (f->n_args != n_args) {
	sc_set_error(err, E_SYNTAX, "");
	snprintf(err->msg, DTG_MAX_MSG_SIZE, "Invalid number of arguments %d=/=%d", n_args, f->n_args);
	return -1;
    }

    sc_reset_error(err);
    for (size_t i = 0; i < n_args; ++i) {
	int found_func = 1;
	for (size_t j = 0; args[i][j] != 0; ++j) {
	    if (args[i][j] == '('/*)*/) {
		char* arg_ilist = _get_enclosed(args[i]+j, "(", ")");
		args[i][j] = 0;

		//break up the inside of the parenthesis by commas
		size_t n_args_i = 0;
		char** args_i = csv_to_list(arg_ilist, ',', &n_args_i, err);
		if (err->type != E_SUCCESS) { return -1; }

		//call the function and check for errors
		value* rets;
		int n_rets = call_func_by_name(c, args[i], n_args_i, args_i, &rets, err);
		if (err->type != E_SUCCESS) {
		    free(args_i);
		    return n_rets;
		}
		found_func = 1;
		
		//push the results onto the callstack
		for (size_t j = 0; j < n_rets; ++j) {
		    push_n(&(c->callstack), "", rets[j], err);
		    if (err->type != E_SUCCESS) { return -2; }
		}
		return n_rets;

		break;
	    }
	}
    }
}

#ifdef __cplusplus 
}
#endif
