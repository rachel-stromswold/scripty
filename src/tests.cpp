#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
//#include "../extern/catch.hpp"
#include <doctest.h>
#include <stdlib.h>

extern "C" {
#include "utils.h"
#include "values.h"
#include "operations.h"
}

#define TEST_ARR_SIZE 3
#define N_ARITH_TESTS	100
#define TEST_STR_SIZE	64

#define EPSILON 0.001
#define APPROX(a,b) ((a-b < EPSILON && a-b >= 0) || (b-a < EPSILON && b-a >= 0))

#define RAND_RANGE	1000
#define RAND_SMALL_RANGE	6
#define TEST_INT_VAL	12
#define TEST_FLOAT_VAL	3.0
#define TEST_STRING_VAL	"foo"

#define TEST_ARR_0_VAL	1
#define TEST_ARR_1_VAL	1.0
#define TEST_ARR_2_VAL	"test"

/*class valueOperationsTestFixture {
private:
    value test_bool;
    value test_char;
    value test_int;
    value test_float;
    value test_string;
    value test_array;

public:
  valueOperationsTestFixture() {}
};*/

TEST_CASE( "Test Hash Table [values]") {
    SUBCASE("Test assigning and fetching values") {
	//setup
	sc_error tmp_err;
	HashTable hasher = make_HashTable(&tmp_err);
	value test_arr[TEST_ARR_SIZE];
	test_arr[0] = v_make_int(TEST_ARR_0_VAL, &tmp_err);
	test_arr[1] = v_make_float(TEST_ARR_1_VAL, &tmp_err);
	test_arr[2] = v_make_string(TEST_ARR_2_VAL, &tmp_err);

	//add some values
	value test_int = v_make_int(TEST_INT_VAL, &tmp_err);
	insert(&hasher, "test_int", test_int, &tmp_err);
	value test_flt = v_make_float(TEST_FLOAT_VAL, &tmp_err);
	insert_deep(&hasher, "test_flt", test_flt, &tmp_err);
	value test_string = v_make_string(TEST_STRING_VAL, &tmp_err);
	insert_deep(&hasher, "test_string", test_string, &tmp_err);
	value test_array = v_make_array(test_arr, TEST_ARR_SIZE, &tmp_err);
	insert_deep(&hasher, "test_array", test_array, &tmp_err);
	value test_int2 = v_make_int(-TEST_INT_VAL, &tmp_err);
	insert_deep(&hasher, "test_int2", test_int2, &tmp_err);

	//make sure that we can find the values we added and that they have the correct contents
	HashedItem* tmp = lookup(&hasher, "test_int");
	CHECK(tmp != NULL);
	value res_eq = op_eq(test_int, tmp->val, &tmp_err);
	CHECK(res_eq.val.i != 0);

	tmp = lookup(&hasher, "test_flt");
	CHECK(tmp != NULL);
	res_eq = op_eq(test_flt, tmp->val, &tmp_err);
	CHECK(res_eq.val.i != 0);

	tmp = lookup(&hasher, "test_string");
	CHECK(tmp != NULL);
	res_eq = op_eq(test_string, tmp->val, &tmp_err);
	CHECK(res_eq.val.i != 0);

	tmp = lookup(&hasher, "test_array");
	CHECK(tmp != NULL);
	res_eq = op_eq(test_array, tmp->val, &tmp_err);
	CHECK(res_eq.val.i != 0);

	tmp = lookup(&hasher, "test_int2");
	CHECK(tmp != NULL);
	res_eq = op_eq(test_int2, tmp->val, &tmp_err);
	CHECK(res_eq.val.i != 0);

	//cleanup
	free_HashTable(&hasher);
	free_value(&test_int);
	free_value(&test_flt);
	free_value(&test_string);
	free_value(&test_array);
	free_value(&test_int2);
	for (size_t i = 0; i < TEST_ARR_SIZE; ++i) {
	    free_value(test_arr + i);
	}
    }
}

TEST_CASE( "Test parsing utilities [Parse]" ) {
    SUBCASE ("Test enclosed text")  {
	//setup
	char test_str[TEST_STR_SIZE];
	
	//test a non enclosed string
	strncpy(test_str, "foo", TEST_STR_SIZE);
	char* enclosed = _get_enclosed(test_str, "(", ")");
	INFO("enclosed=", enclosed);
	CHECK(strcmp(enclosed, "foo") == 0);

	//test a simple non nested string
	strncpy(test_str, "(foo)", TEST_STR_SIZE);
	enclosed = _get_enclosed(test_str, "(", ")");
	INFO("enclosed=", enclosed);
	CHECK(strcmp(enclosed, "foo") == 0);

	//test a simple nested string with only one type of paranthesis
	strncpy(test_str, "((foo)bar)", TEST_STR_SIZE);
	enclosed = _get_enclosed(test_str, "(", ")");
	INFO("enclosed=", enclosed);
	CHECK(strcmp(enclosed, "(foo)bar") == 0);

	//test a simple nested string with only different types of paranthesis
	strncpy(test_str, "{(foo)bar}", TEST_STR_SIZE);
	enclosed = _get_enclosed(test_str, "(", ")");
	INFO("enclosed=", enclosed);
	CHECK(strcmp(enclosed, "foo") == 0);
	strncpy(test_str, "{(foo)bar}", TEST_STR_SIZE);
	enclosed = _get_enclosed(test_str, "{", "}");
	INFO("enclosed=", enclosed);
	CHECK(strcmp(enclosed, "(foo)bar") == 0);

	//ensure it still works even in a different order
	strncpy(test_str, "{foo(bar)}", TEST_STR_SIZE);
	enclosed = _get_enclosed(test_str, "(", ")");
	INFO("enclosed=", enclosed);
	CHECK(strcmp(enclosed, "bar") == 0);
    }
    SUBCASE( "Test value interpretation [values]" ) {
	//setup
	char test_str[TEST_STR_SIZE];
	sc_error tmp_err;

	//test integer interpretation
	strncpy(test_str, "12345", TEST_STR_SIZE);
	value v_int = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_int.type == VT_INT);
	CHECK(v_int.val.i == 12345);
	free_value(&v_int);
	//test negative values
	strncpy(test_str, "-12345", TEST_STR_SIZE);
	v_int = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_int.type == VT_INT);
	CHECK(v_int.val.i == -12345);
	free_value(&v_int);
	//test octal interpretation
	strncpy(test_str, "042", TEST_STR_SIZE);
	v_int = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_int.type == VT_INT);
	CHECK(v_int.val.i == 34);
	free_value(&v_int);
	//test negative values
	strncpy(test_str, "-042", TEST_STR_SIZE);
	v_int = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_int.type == VT_INT);
	CHECK(v_int.val.i == -34);
	free_value(&v_int);
	//test hex interpretation
	strncpy(test_str, "0xFF", TEST_STR_SIZE);
	v_int = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_int.type == VT_INT);
	CHECK(v_int.val.i == 255);
	free_value(&v_int);
	//test negative values
	strncpy(test_str, "-0x42", TEST_STR_SIZE);
	v_int = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_int.type == VT_INT);
	CHECK(v_int.val.i == -66);
	free_value(&v_int);

	//test floating point interpretation
	strncpy(test_str, "123.5", TEST_STR_SIZE);
	value v_flt = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_flt.type == VT_FLOAT);
	CHECK(v_flt.val.f == 123.5);
	free_value(&v_int);
	//test negative numbeers
	strncpy(test_str, "-123.5", TEST_STR_SIZE);
	v_flt = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_flt.type == VT_FLOAT);
	CHECK(v_flt.val.f == -123.5);
	free_value(&v_int);
	//test scientific notation
	strncpy(test_str, "1.25e4", TEST_STR_SIZE);
	v_flt = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_flt.type == VT_FLOAT);
	CHECK(v_flt.val.f == 12500);
	free_value(&v_int);
	strncpy(test_str, "1.25e+4", TEST_STR_SIZE);
	v_flt = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_flt.type == VT_FLOAT);
	CHECK(v_flt.val.f == 12500);
	free_value(&v_int);
	strncpy(test_str, "1.25e-4", TEST_STR_SIZE);
	v_flt = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_flt.type == VT_FLOAT);
	CHECK(v_flt.val.f == 0.000125);
	free_value(&v_int);
	strncpy(test_str, "-1.25e+4", TEST_STR_SIZE);
	v_flt = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_flt.type == VT_FLOAT);
	CHECK(v_flt.val.f == -12500);
	free_value(&v_int);
	strncpy(test_str, "-1.25e-4", TEST_STR_SIZE);
	v_flt = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_flt.type == VT_FLOAT);
	CHECK(v_flt.val.f == -0.000125);
	free_value(&v_int);
	
	//test string interpretation
	strncpy(test_str, "\"test strings\"", TEST_STR_SIZE);
	value v_str = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_str.type == VT_STRING);
	INFO("test_str->buf=", v_str.val.str->buf);
	CHECK(strcmp(v_str.val.str->buf, "test strings") == 0);
	free_value(&v_str);

	//make sure whitespace is ignored
	strncpy(test_str, "  \"test strings\"\t", TEST_STR_SIZE);
	v_str = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_str.type == VT_STRING);
	INFO("test_str->buf=", v_str.val.str->buf);
	CHECK(strcmp(v_str.val.str->buf, "test strings") == 0);
	free_value(&v_str);

	//test array interpretation
	strncpy(test_str, "[1, \"test arrays\"]", TEST_STR_SIZE);
	value v_arr = read_value_string(test_str, 0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(v_arr.type == VT_ARRAY);
	Array* tmp = (Array*)v_arr.val.ptr;
	CHECK(tmp->size <= tmp->buf_size);
	CHECK(tmp->size == 2);
	CHECK(tmp->buf[0].type == VT_INT);
	CHECK(tmp->buf[1].type == VT_STRING);
	INFO("test_str->buf=", tmp->buf[1].val.str->buf);
	CHECK(tmp->buf[0].val.i == 1);
	INFO("test_arr->str->buf=", tmp->buf[1].val.str);
	CHECK(strcmp(tmp->buf[1].val.str->buf, "test arrays") == 0);
	free_value(&v_arr);
    }
}

TEST_CASE( "Test Array objects [Arrays]" ) {
    SUBCASE ("Test Array initialization")  {
	//setup
	sc_error tmp_err;
	value proto_arr[TEST_ARR_SIZE];
	proto_arr[0] = v_make_int(1, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	proto_arr[1] = v_make_float(1.0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	proto_arr[2] = v_make_string("test", &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);

	value proto_val = v_make_string("proto val", &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);

	//test initalization from existing array
	value duplicate_arr = v_make_array(proto_arr, TEST_ARR_SIZE, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	Array* dup_arr = (Array*)duplicate_arr.val.ptr;
	CHECK(dup_arr->size <= dup_arr->buf_size);
	CHECK(dup_arr->size == TEST_ARR_SIZE);
	for (size_t i = 0; i < TEST_ARR_SIZE; ++i) {
	    value comparison = op_eq(dup_arr->buf[i], proto_arr[i], &tmp_err);
	    INFO("i=", i);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("i=", i);
	    CHECK(comparison.type == VT_BOOL);
	    INFO("i=", i);
	    CHECK(comparison.val.i == 1);
	}

	value duplicate_arr_multi = v_make_array_n(TEST_ARR_SIZE, proto_val, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	Array* dup_arr_mult = (Array*)duplicate_arr_multi.val.ptr;
	CHECK(dup_arr_mult->size <= dup_arr->buf_size);
	CHECK(dup_arr_mult->size == TEST_ARR_SIZE);
	for (size_t i = 0; i < TEST_ARR_SIZE; ++i) {
	    value comparison = op_eq(dup_arr_mult->buf[i], proto_val, &tmp_err);
	    INFO("i=", i);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("i=", i);
	    CHECK(comparison.type == VT_BOOL);
	    INFO("i=", i);
	    CHECK(comparison.val.i == 1);
	}

	//cleanup
	free_value(&duplicate_arr);
	free_value(&duplicate_arr_multi);
	free_value(&proto_val);
	for (size_t i = 0; i < TEST_ARR_SIZE; ++i) {
	    free_value(proto_arr + i);
	}
    }
    SUBCASE( "Test appening to arrays [Arrays]" ) {
	//setup
	sc_error tmp_err;
	value proto_arr[TEST_ARR_SIZE];
	proto_arr[0] = v_make_int(1, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	proto_arr[1] = v_make_float(1.0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	proto_arr[2] = v_make_string("test", &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);

	//test initalization from existing array
	value v_arr = v_make_array(proto_arr, TEST_ARR_SIZE, &tmp_err);
	for (size_t i = 0; i < N_ARITH_TESTS; ++i) {
	    size_t off = i % TEST_ARR_SIZE;
	    _extend_a( (Array*)v_arr.val.ptr, &(proto_arr[off]), 1, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	
	}
	for (size_t i = 0; i < ((Array*)v_arr.val.ptr)->size; ++i) {
	    size_t off = i % TEST_ARR_SIZE;
	    value test_eq = op_eq( ((Array*)v_arr.val.ptr)->buf[i], proto_arr[off], &tmp_err );
	    //INFO("i=", i, " v_arr[i]=", ((Array*)v_arr.val.ptr)->buf[i]);
	    CHECK(test_eq.val.i != 0);
	}
	//cleanup
	free_value(&v_arr);
	for (size_t i = 0; i < TEST_ARR_SIZE; ++i) {
	    free_value(proto_arr + i);
	}
    }
}

TEST_CASE( "Test that initialization functions produce correct results [values]") {
    SUBCASE ( "Test simple initializations" ) {
	sc_error tmp_err;
	value test_bool_true = v_make_bool(true, &tmp_err);
	value test_bool_false = v_make_bool(false, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(test_bool_true.val.i == 1);
	CHECK(test_bool_false.val.i == 0);

	//test_char = v_make_char(123);
	value test_int = v_make_int(1, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(test_int.val.i == 1);

	value test_float = v_make_float(1.0, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(test_float.val.f == 1.0);

	value test_string = v_make_string("test", &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(test_string.val.str->size == 4);
	CHECK(len(test_string) == 4);
	CHECK(strcmp(test_string.val.str->buf, "test") == 0);
	CHECK(test_string.val.str->buf[test_string.val.str->size] == 0);

	value test_string_n = v_make_string_n(5, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(test_string_n.val.str->size == 0);
	CHECK(len(test_string_n) == 0);
	CHECK(test_string_n.val.str->buf_size == 5);
	CHECK(strcmp(test_string.val.str->buf, "test") == 0);
	CHECK(test_string.val.str->buf[test_string.val.str->size] == 0);

	//cleanup
	free_value(&test_bool_true);
	free_value(&test_bool_false);
	free_value(&test_int);
	free_value(&test_float);
	free_value(&test_string);
	free_value(&test_string_n);
    }
}

TEST_CASE ( "Test single operations [operations]") {
    SUBCASE ( "String bool addition" ) {
	//setup
	sc_error tmp_err;
	value test_bool_true = v_make_bool(true, &tmp_err);
	value test_bool_false = v_make_bool(false, &tmp_err);
	value test_string = v_make_string("test ", &tmp_err);
	value test_true = op_add(test_string, test_bool_true, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(strcmp(test_true.val.str->buf, "test true") == 0);
	value test_false = op_add(test_string, test_bool_false, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(strcmp(test_false.val.str->buf, "test false") == 0);
	
	//cleanup
	free_value(&test_bool_true);
	free_value(&test_bool_false);
	free_value(&test_string);
	free_value(&test_true);
	free_value(&test_false);
    }
    SUBCASE ( "Integer arithmetic" ) {
	sc_error tmp_err;
	value s1 = v_make_int(1, &tmp_err);
	value s2 = v_make_int(0, &tmp_err);
	//make sure that division by zero throws an error
	value res = op_div(s1, s2, &tmp_err);
	CHECK(tmp_err.type != E_SUCCESS);
	value res_eq = op_eq(s1, s2, &tmp_err);
	CHECK(res_eq.val.i == 0);

	//ensure that identity values work
	res = op_add(s1, s2, &tmp_err);
	CHECK(res.val.i == 1);
	res = op_sub(s1, s2, &tmp_err);
	CHECK(res.val.i == 1);
	res = op_add(s2, s1, &tmp_err);
	CHECK(res.val.i == 1);
	res = op_sub(s2, s1, &tmp_err);
	CHECK(res.val.i == -1);
	s2.val.i = 2;
	res = op_mult(s2, s1, &tmp_err);
	CHECK(res.val.i == 2);
	res = op_div(s2, s1, &tmp_err);
	CHECK(res.val.i == 2);

	for (size_t i = 0; i < N_ARITH_TESTS; ++i) {
	    int tmp_1 = (rand() % RAND_RANGE) - RAND_RANGE/2;
	    int tmp_2 = (rand() % RAND_RANGE) - RAND_RANGE/2;
	    if (tmp_2 == 0) { tmp_2 = 1; }//avoid divisions by zero
	    //figure out the values using c
	    int tmp_sum = tmp_1 + tmp_2;
	    int tmp_dif = tmp_1 - tmp_2;
	    int tmp_mul = tmp_1 * tmp_2;
	    int tmp_div = tmp_1 / tmp_2;
	    value v_sum = v_make_int(tmp_sum, &tmp_err);
	    value v_dif = v_make_int(tmp_dif, &tmp_err);
	    value v_mul = v_make_int(tmp_mul, &tmp_err);
	    value v_div = v_make_int(tmp_div, &tmp_err);

	    s1.val.i = tmp_1;
	    s2.val.i = tmp_2;
	    //test operations
	    res = op_add(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_sum, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.i);
	    CHECK(res_eq.val.i != 0);

	    res = op_sub(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_dif, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.i);
	    CHECK(res_eq.val.i != 0);

	    res = op_mult(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_mul, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.i);
	    CHECK(res_eq.val.i != 0);

	    res = op_div(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_div, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.i);
	    CHECK(res_eq.val.i != 0);
	}
    }
    SUBCASE ( "Floating point arithmetic" ) {
	sc_error tmp_err;
	value s1 = v_make_float(1.0, &tmp_err);
	value s2 = v_make_float(0.0, &tmp_err);
	//make sure that division by zero throws an error
	value res = op_div(s1, s2, &tmp_err);
	CHECK(tmp_err.type != E_SUCCESS);
	value res_eq = op_eq(s1, s2, &tmp_err);
	CHECK(res_eq.val.i == 0);

	//ensure that identity values work
	res = op_add(s1, s2, &tmp_err);
	CHECK(res.val.f == 1.0);
	res = op_sub(s1, s2, &tmp_err);
	CHECK(res.val.f == 1.0);
	res = op_add(s2, s1, &tmp_err);
	CHECK(res.val.f == 1.0);
	res = op_sub(s2, s1, &tmp_err);
	CHECK(res.val.f == -1.0);
	s2.val.f = 2.0;
	res = op_mult(s2, s1, &tmp_err);
	CHECK(res.val.f == 2.0);
	res = op_div(s2, s1, &tmp_err);
	CHECK(res.val.f == 2.0);

	for (size_t i = 0; i < N_ARITH_TESTS; ++i) {
	    double tmp_1p = (double)( (rand() % RAND_RANGE) - RAND_RANGE/2 );
	    double tmp_2p = (double)( (rand() % RAND_RANGE) - RAND_RANGE/2 );
	    double tmp_1q = (double)( (rand() % RAND_RANGE) + 1 );
	    double tmp_2q = (double)( (rand() % RAND_RANGE) + 1 );
	    if (tmp_1q == 0) { tmp_1q = 1; }//avoid divisions by zero
	    if (tmp_2p == 0) { tmp_2p = 1; }//avoid divisions by zero
	    if (tmp_2q == 0) { tmp_2q = 1; }//avoid divisions by zero
	    double tmp_1 = tmp_1p / tmp_1q;
	    double tmp_2 = tmp_2p / tmp_2q;
	    //figure out the values using c
	    double tmp_sum = tmp_1 + tmp_2;
	    double tmp_dif = tmp_1 - tmp_2;
	    double tmp_mul = tmp_1 * tmp_2;
	    double tmp_div = tmp_1 / tmp_2;
	    value v_sum = v_make_float(tmp_sum, &tmp_err);
	    value v_dif = v_make_float(tmp_dif, &tmp_err);
	    value v_mul = v_make_float(tmp_mul, &tmp_err);
	    value v_div = v_make_float(tmp_div, &tmp_err);

	    s1.val.f = tmp_1;
	    s2.val.f = tmp_2;
	    //test operations
	    res = op_add(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_sum, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.f);
	    CHECK(res_eq.val.i != 0);

	    res = op_sub(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_dif, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.f);
	    CHECK(res_eq.val.i != 0);

	    res = op_mult(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_mul, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.f);
	    CHECK(res_eq.val.i != 0);

	    res = op_div(s1, s2, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_div, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.f);
	    CHECK(res_eq.val.i != 0);
	}
    }
    SUBCASE ( "String String addition" ) {
	//setup
	sc_error tmp_err, garbage_err;
	value test_arr[TEST_ARR_SIZE];
	test_arr[0] = v_make_int(1, &tmp_err);
	test_arr[1] = v_make_float(1.0, &tmp_err);
	test_arr[2] = v_make_string("test", &tmp_err);

	//try adding string objects
	value foo_string = v_make_string("foo", &tmp_err);
	value bar_string = v_make_string("bar", &tmp_err);
	value garbage_string = v_make_string("This is a garbage string which only exists to exceed the default number of allocated characters.", &garbage_err);
	//test small addition
	value foobar = op_add(foo_string, bar_string, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(strcmp(foobar.val.str->buf, "foobar") == 0);
	
	//test long strings
	value garbagefoo = op_add(garbage_string, foo_string, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	INFO("garbagefoo = ", garbagefoo.val.str->buf);
	CHECK(strcmp(garbagefoo.val.str->buf, "This is a garbage string which only exists to exceed the default number of allocated characters.foo") == 0);

	//test ints
	value fooint = op_add(foo_string, test_arr[0], &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	INFO("fooint = ", fooint.val.str->buf);
	CHECK(strcmp(fooint.val.str->buf, "foo1") == 0);
	CHECK(fooint.val.str->buf[fooint.val.str->size] == 0);

	//test ints
	value fooflt = op_add(foo_string, test_arr[1], &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	INFO(fooint.val.str->buf);
	CHECK(strcmp(fooint.val.str->buf, "foo1") == 0);
	CHECK(fooflt.val.str->buf[fooflt.val.str->size] == 0);

	//test arrays
	value arr = v_make_array(test_arr, TEST_ARR_SIZE, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	value fooarr = op_add(foo_string, arr, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	INFO(fooarr.val.str->buf);
	CHECK(strcmp(fooarr.val.str->buf, "foo[1, 1, test]") == 0);
	CHECK(fooarr.val.str->buf[fooarr.val.str->size] == 0);

	//cleanup
	free_value(&foo_string);
	free_value(&bar_string);
	free_value(&foobar);
	free_value(&garbage_string);
	free_value(&garbagefoo);
	free_value(&fooint);
	free_value(&fooflt);
	free_value(&arr);
	free_value(&fooarr);
	for (size_t i = 0; i < TEST_ARR_SIZE; ++i) {
	    free_value(test_arr + i);
	}
    }
}

TEST_CASE ( "Test Math Evaluations [operations]") {
    SUBCASE ( "Test parsing optrees" ) {
	//setup
	char test_str[TEST_STR_SIZE];
	sc_error tmp_err;

	struct Operation* op_add = NULL;
	struct Operation* op_sub = NULL;
	struct Operation* op_mul = NULL;
	struct Operation* op_div = NULL;
	value res_eq;
	//test generation of simple arithmetic operations on integers
	for (size_t i = 0; i < N_ARITH_TESTS; ++i) {
	    int tmp_1 = (rand() % RAND_RANGE) - RAND_RANGE/2;
	    int tmp_2 = (rand() % RAND_RANGE) - RAND_RANGE/2;
	    if (tmp_2 == 0) { tmp_2 = 1; }//avoid divisions by zero
	    //figure out the values using c
	    int tmp_sum = tmp_1 + tmp_2;
	    int tmp_dif = tmp_1 - tmp_2;
	    int tmp_mul = tmp_1 * tmp_2;
	    int tmp_div = tmp_1 / tmp_2;
	    value v_sum = v_make_int(tmp_sum, &tmp_err);
	    value v_dif = v_make_int(tmp_dif, &tmp_err);
	    value v_mul = v_make_int(tmp_mul, &tmp_err);
	    value v_div = v_make_int(tmp_div, &tmp_err);

	    snprintf(test_str, TEST_STR_SIZE, "%d+%d", tmp_1, tmp_2);
	    op_add = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    snprintf(test_str, TEST_STR_SIZE, "%d-%d", tmp_1, tmp_2);
	    op_sub = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    snprintf(test_str, TEST_STR_SIZE, "%d*%d", tmp_1, tmp_2);
	    op_mul = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    snprintf(test_str, TEST_STR_SIZE, "%d/%d", tmp_1, tmp_2);
	    op_div = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);

	    //test operations
	    value res = eval(op_add, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_sum, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.i);
	    CHECK(res_eq.val.i != 0);

	    res = eval(op_sub, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_dif, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sub=", res.val.i);
	    CHECK(res_eq.val.i != 0);

	    res = eval(op_mul, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_mul, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val mul=", res.val.i);
	    CHECK(res_eq.val.i != 0);

	    res = eval(op_div, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res_eq = op_eq(res, v_div, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val div=", res.val.i);
	    CHECK(res_eq.val.i != 0);

	    //cleanup
	    free_Operation(op_add);
	    free_Operation(op_sub);
	    free_Operation(op_mul);
	    free_Operation(op_div);
	}
	for (size_t i = 0; i < N_ARITH_TESTS; ++i) {
	    int tmp_1p = (rand() % RAND_RANGE) - RAND_RANGE/2;
	    int tmp_2p = (rand() % RAND_RANGE) - RAND_RANGE/2;
	    int tmp_1q = (rand() % RAND_RANGE) + 1;//guarantees that zero isn't a valid q value
	    int tmp_2q = (rand() % RAND_RANGE) + 1;
	    if (tmp_2p == 0) { tmp_2p = 1; }//avoid divisions by zero
	    double tmp_1 = (double)tmp_1p / (double)tmp_1q;
	    double tmp_2 = (double)tmp_2p / (double)tmp_2q;
	    //figure out the values using c
	    double tmp_sum = tmp_1 + tmp_2;
	    double tmp_dif = tmp_1 - tmp_2;
	    double tmp_mul = tmp_1 * tmp_2;
	    double tmp_div = tmp_1 / tmp_2;

	    snprintf(test_str, TEST_STR_SIZE, "%f+%f", tmp_1, tmp_2);
	    op_add = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    snprintf(test_str, TEST_STR_SIZE, "%f-%f", tmp_1, tmp_2);
	    op_sub = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    snprintf(test_str, TEST_STR_SIZE, "%f*%f", tmp_1, tmp_2);
	    op_mul = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    snprintf(test_str, TEST_STR_SIZE, "%f/%f", tmp_1, tmp_2);
	    op_div = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);

	    //test operations
	    value res = eval(op_add, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val sum=", res.val.f);
	    CHECK(APPROX(res.val.f, tmp_sum));

	    res = eval(op_sub, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val dif=", res.val.f);
	    CHECK(APPROX(res.val.f, tmp_dif));

	    res = eval(op_mul, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val mul=", res.val.f);
	    CHECK(APPROX(res.val.f, tmp_mul));

	    res = eval(op_div, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    INFO("tmp_1=", tmp_1, " tmp_2=", tmp_2, " val div=", res.val.f);
	    CHECK(APPROX(res.val.f, tmp_div));

	    //cleanup
	    free_Operation(op_add);
	    free_Operation(op_sub);
	    free_Operation(op_mul);
	    free_Operation(op_div);
	}
	//test composite expressions for ints
	strncpy(test_str, "(7+2)-3", TEST_STR_SIZE);
	struct Operation* op_comp = gen_optree(test_str, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	value res = eval(op_comp, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(res.type == VT_INT);
	CHECK(res.val.i == 6);
	free_Operation(op_comp);
	//test composite expressions for floats
	strncpy(test_str, "(1.0+2.0) - 0.5", TEST_STR_SIZE);
	op_comp = gen_optree(test_str, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	res = eval(op_comp, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(res.type == VT_FLOAT);
	CHECK(APPROX(res.val.f, 2.5));
	free_Operation(op_comp);
	//test mixed composite expressions
	strncpy(test_str, "17 -\t( (1.0 + 2.0) - 0.5)", TEST_STR_SIZE);
	op_comp = gen_optree(test_str, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	res = eval(op_comp, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(res.type == VT_FLOAT);
	CHECK(APPROX(res.val.f, 14.5));
	free_Operation(op_comp);
    }

    SUBCASE ( "Test logical comparisons" ) {
	//setup
	char test_str[TEST_STR_SIZE];
	sc_error tmp_err;

	struct Operation* op_eq = NULL;
	struct Operation* op_geq = NULL;
	struct Operation* op_leq = NULL;
	struct Operation* op_grt = NULL;
	struct Operation* op_lst = NULL;
	struct Operation* op_not = NULL;

	//test generation of simple arithmetic operations on integers
	for (size_t i = 0; i < N_ARITH_TESTS; ++i) {
	    int tmp_c = (rand() % RAND_SMALL_RANGE) - RAND_SMALL_RANGE/2;
	    int tmp_1 = (rand() % RAND_SMALL_RANGE) - RAND_SMALL_RANGE/2;
	    int tmp_2 = (rand() % RAND_SMALL_RANGE) - RAND_SMALL_RANGE/2;
	    if (tmp_2 == 0) { tmp_2 = 1; }//avoid divisions by zero

	    //test equality
	    snprintf(test_str, TEST_STR_SIZE, "%d == %d+%d", tmp_c, tmp_1, tmp_2);
	    op_eq = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    value res = eval(op_eq, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    CHECK(res.type == VT_BOOL);
	    if (tmp_c == tmp_1+tmp_2) {
		CHECK(res.val.i != 0);
	    } else {
		CHECK(res.val.i == 0);
	    }
	    //test '>' and '>='
	    snprintf(test_str, TEST_STR_SIZE, "%d >= %d+%d", tmp_c, tmp_1, tmp_2);
	    op_geq = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res = eval(op_geq, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    CHECK(res.type == VT_BOOL);
	    if (tmp_c >= tmp_1+tmp_2) {
		CHECK(res.val.i != 0);
	    } else {
		CHECK(res.val.i == 0);
	    }
	    snprintf(test_str, TEST_STR_SIZE, "%d > %d+%d", tmp_c, tmp_1, tmp_2);
	    op_grt = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res = eval(op_grt, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    CHECK(res.type == VT_BOOL);
	    if (tmp_c > tmp_1+tmp_2) {
		CHECK(res.val.i != 0);
	    } else {
		CHECK(res.val.i == 0);
	    }
	    //test '<' and '<='
	    snprintf(test_str, TEST_STR_SIZE, "%d <= %d+%d", tmp_c, tmp_1, tmp_2);
	    op_leq = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res = eval(op_leq, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    CHECK(res.type == VT_BOOL);
	    if (tmp_c <= tmp_1+tmp_2) {
		CHECK(res.val.i != 0);
	    } else {
		CHECK(res.val.i == 0);
	    }
	    snprintf(test_str, TEST_STR_SIZE, "%d < %d+%d", tmp_c, tmp_1, tmp_2);
	    op_lst = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res = eval(op_lst, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    CHECK(res.type == VT_BOOL);
	    if (tmp_c < tmp_1+tmp_2) {
		CHECK(res.val.i != 0);
	    } else {
		CHECK(res.val.i == 0);
	    }
	    snprintf(test_str, TEST_STR_SIZE, "!(%d < %d+%d)", tmp_c, tmp_1, tmp_2);
	    op_not = gen_optree(test_str, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    res = eval(op_not, NULL, &tmp_err);
	    CHECK(tmp_err.type == E_SUCCESS);
	    CHECK(res.type == VT_BOOL);
	    if (tmp_c < tmp_1+tmp_2) {
		CHECK(res.val.i == 0);
	    } else {
		CHECK(res.val.i != 0);
	    }

	    //cleanup
	    free_Operation(op_eq);
	    free_Operation(op_geq);
	    free_Operation(op_leq);
	    free_Operation(op_grt);
	    free_Operation(op_lst);
	    free_Operation(op_not);
	}
	//test composite expressions
	strncpy(test_str, "(7+2 <= 3) || (7-5 <= 3)", TEST_STR_SIZE);
	struct Operation* op_logic_comp = gen_optree(test_str, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	value logic_res = eval(op_logic_comp, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(logic_res.type == VT_BOOL);
	CHECK(logic_res.val.i != 0);
	free_Operation(op_logic_comp);
	strncpy(test_str, "(7+2 <= 3) || (7-2 <= 3)", TEST_STR_SIZE);
	op_logic_comp = gen_optree(test_str, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	logic_res = eval(op_logic_comp, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(logic_res.type == VT_BOOL);
	CHECK(logic_res.val.i == 0);
	free_Operation(op_logic_comp);

	strncpy(test_str, "(7+2 <= 3) && (7-5 <= 3)", TEST_STR_SIZE);
	op_logic_comp = gen_optree(test_str, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	logic_res = eval(op_logic_comp, NULL,&tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(logic_res.type == VT_BOOL);
	CHECK(logic_res.val.i == 0);
	free_Operation(op_logic_comp);
	strncpy(test_str, "(7+2 <= 9) && (7-5 <= 3)", TEST_STR_SIZE);
	op_logic_comp = gen_optree(test_str, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	logic_res = eval(op_logic_comp, NULL, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(logic_res.type == VT_BOOL);
	CHECK(logic_res.val.i != 0);
	free_Operation(op_logic_comp);
    }

	//setup
	sc_error err;

	char tmp_str[TEST_STR_SIZE];
	NamedStack st = make_NamedStack(&err);

	CHECK(err.type == E_SUCCESS);

	//test that the stack is set up correctly (growing backwards)
	CHECK(is_empty_n(&st) == 1);
	CHECK(st.block + st.cap == st.top);
	CHECK(st.bottom == st.top);

    SUBCASE ( "Test variable substitutions" ) {
	//setup
	char test_str[3][TEST_STR_SIZE];
	sc_error tmp_err;

	//make the stacks
	NamedStack n_st = make_NamedStack(&tmp_err);
	Stack st = make_Stack(&tmp_err);
	//initialize values
	value test_1 = v_make_int(TEST_INT_VAL, &tmp_err);
	value test_2 = v_make_int(TEST_INT_VAL*2, &err);
	//push_values onto the stack
	strncpy(test_str[0], "test_a", TEST_STR_SIZE);
	push_n(&n_st, test_str[0], test_1, &tmp_err);
	CHECK(err.type == E_SUCCESS);
	push(&st, test_1, &tmp_err);
	CHECK(err.type == E_SUCCESS);
	strncpy(test_str[1], "test_b", TEST_STR_SIZE);
	push_n(&n_st, test_str[1], test_2, &tmp_err);
	CHECK(err.type == E_SUCCESS);
	push(&st, test_2, &tmp_err);
	CHECK(err.type == E_SUCCESS);

	//test composite expressions
	strncpy(test_str[2], "(test_a- test_b <= test_a)", TEST_STR_SIZE);
	struct Operation* op_logic_comp = gen_optree(test_str[2], &n_st, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	value logic_res = eval(op_logic_comp, &st, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(logic_res.type == VT_BOOL);
	CHECK(logic_res.val.i != 0);

	strncpy(test_str[2], "(test_a + test_b)*test_b", TEST_STR_SIZE);
	struct Operation* op_math = gen_optree(test_str[2], &n_st, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	logic_res = eval(op_math, &st, &tmp_err);
	CHECK(tmp_err.type == E_SUCCESS);
	CHECK(logic_res.type == VT_INT);
	CHECK(logic_res.val.i == 6*TEST_INT_VAL*TEST_INT_VAL);

	//cleanup
	free_Operation(op_logic_comp);
	free_Operation(op_math);
	free_NamedStack(&n_st);
	free_Stack(&st);
    }
}

TEST_CASE( "Test that String structs work as expected [Strings]") {
    SUBCASE ( "Test simple initializations" ) {
	sc_error foo_err, bar_err, garbage_err;
	String foo_str = make_String("foo", &foo_err);
	String bar_str = make_String("bar", &bar_err);
	String garbage_str = make_String("This is a garbage string which only exists to exceed the default number of allocated characters.", &garbage_err);
	//make sure the errors are fine
	CHECK(foo_err.type == E_SUCCESS);
	CHECK(bar_err.type == E_SUCCESS);
	CHECK(garbage_err.type == E_SUCCESS);

	//make sure that the buffers have enough allocated memory to store the contents
	CHECK(foo_str.size <= foo_str.buf_size);
	CHECK(bar_str.size <= bar_str.buf_size);
	CHECK(garbage_str.size <= garbage_str.buf_size);
	//make sure that strings are of the right size
	CHECK(foo_str.size == 3);
	CHECK(bar_str.size == 3);
	CHECK(garbage_str.size == 96);
	//make sure that strings have the right contents
	INFO("str = ", foo_str.buf);
	CHECK(strcmp(foo_str.buf, "foo") == 0);
	INFO("str = ", bar_str.buf);
	CHECK(strcmp(bar_str.buf, "bar") == 0);
	INFO("str = ", garbage_str.buf);
	CHECK(strcmp(garbage_str.buf, "This is a garbage string which only exists to exceed the default number of allocated characters.") == 0);

	//cleanup
	free_String(&foo_str);
	free_String(&bar_str);
	free_String(&garbage_str);
    }

    SUBCASE ( "Test appending values" ) {
	sc_error struct_err, pointer_err;
	String foo_str = make_String("foo", NULL);
	String bar_str = make_String("bar", NULL);
	_append_string_s(&foo_str, bar_str, &struct_err);
	_append_string(&bar_str, "foo", &pointer_err);

	//make sure there weren't any errors
	CHECK(struct_err.type == E_SUCCESS);
	CHECK(pointer_err.type == E_SUCCESS);

	//test the contents of the strings
	CHECK(foo_str.size == 6);
	CHECK(bar_str.size == 6);
	INFO("foo_str = ", foo_str.buf);
	CHECK(strcmp(foo_str.buf, "foobar") == 0);
	INFO("bar_str = ", bar_str.buf);
	CHECK(strcmp(bar_str.buf, "barfoo") == 0);

	//cleanup
	free_String(&foo_str);
	free_String(&bar_str);
    }
}

TEST_CASE( "Test NamedStack and Stack structs [contexts]" ) {
    SUBCASE ( "Test NamedStack" ) {
	//setup
	sc_error err;
	char test_str[2*DEF_STACK_SIZE][TEST_STR_SIZE];
	char tmp_str[TEST_STR_SIZE];
	NamedStack st = make_NamedStack(&err);
	value test_int = v_make_int(TEST_INT_VAL, &err);
	value test_float = v_make_float(TEST_FLOAT_VAL, &err);
	value test_string = v_make_string(TEST_STRING_VAL, &err);
	CHECK(err.type == E_SUCCESS);

	//test that the stack is set up correctly (growing backwards)
	CHECK(is_empty_n(&st) == 1);
	CHECK(st.block + st.cap == st.top);
	CHECK(st.bottom == st.top);

	//test pushing to the stack
	strncpy(test_str[0], "test_int", TEST_STR_SIZE);
	push_n(&st, test_str[0], test_int, &err);
	CHECK(err.type == E_SUCCESS);
	strncpy(test_str[1], "test_float", TEST_STR_SIZE);
	push_n(&st, test_str[1], test_float, &err);
	CHECK(err.type == E_SUCCESS);
	strncpy(test_str[2], "test_string", TEST_STR_SIZE);
	push_n(&st, test_str[2], test_string, &err);
	CHECK(err.type == E_SUCCESS);
	//make sure the stack grew downwards
	CHECK(st.top < st.bottom);
	CHECK(st.bottom == st.block + st.cap);
	CHECK(st.top >= st.block);
	CHECK(is_empty_n(&st) == 0);
	
	//test popping from the stack
	HashedItem hash = pop_n(&st, &err);
	CHECK(err.type == E_SUCCESS);
	value res_eq = op_eq(test_string, hash.val, &err);
	CHECK(strcmp(hash.key, "test_string") == 0);
	CHECK(res_eq.val.i != 0);
	hash = pop_n(&st, &err);
	CHECK(err.type == E_SUCCESS);
	res_eq = op_eq(test_float, hash.val, &err);
	CHECK(strcmp(hash.key, "test_float") == 0);
	CHECK(res_eq.val.i != 0);
	hash = pop_n(&st, &err);
	CHECK(err.type == E_SUCCESS);
	res_eq = op_eq(test_int, hash.val, &err);
	CHECK(strcmp(hash.key, "test_int") == 0);
	CHECK(res_eq.val.i != 0);
	//make sure that the stack is now proper
	CHECK(is_empty_n(&st) == 1);

	//make sure the stack still works after expansion
	HashedItem* old_bottom = st.bottom;
	HashedItem* old_block = st.bottom;
	for (size_t i = 0; i < 2*DEF_STACK_SIZE; ++i) {
	    snprintf(test_str[i], TEST_STR_SIZE, "foo%d", i);
	    test_int.val.i = (int)i;
	    push_n(&st, test_str[i], test_int, &err);
	    CHECK(err.type == E_SUCCESS);
	}
	//make sure the stack grew downwards
	CHECK(st.top < st.bottom);
	INFO("old bottom=", old_bottom, ", old block=", old_block);
	CHECK(st.bottom == st.block + st.cap);
	CHECK(st.top >= st.block);
	CHECK(is_empty_n(&st) == 0);
	for (int i = 2*DEF_STACK_SIZE-1; i >= 0; --i) {
	    hash = pop_n(&st, &err);
	    snprintf(tmp_str, TEST_STR_SIZE, "foo%d", i);
	    CHECK(strcmp(hash.key, tmp_str) == 0);
	    CHECK(hash.val.val.i == i);
	}

	free_NamedStack(&st);
    }

    SUBCASE ( "Test Stack" ) {
	//setup
	sc_error err;
	Stack st = make_Stack(&err);
	value test_int = v_make_int(TEST_INT_VAL, &err);
	value test_float = v_make_float(TEST_FLOAT_VAL, &err);
	value test_string = v_make_string(TEST_STRING_VAL, &err);
	CHECK(err.type == E_SUCCESS);

	//test that the stack is set up correctly (growing backwards)
	CHECK(is_empty(&st) == 1);
	CHECK(st.block + st.cap == st.top);
	CHECK(st.bottom == st.top);

	//test pushing to the stack
	push(&st, test_int, &err);
	CHECK(err.type == E_SUCCESS);
	push(&st, test_float, &err);
	CHECK(err.type == E_SUCCESS);
	push(&st, test_string, &err);
	CHECK(err.type == E_SUCCESS);
	//make sure the stack grew downwards
	CHECK(st.top < st.bottom);
	CHECK(st.bottom == st.block + st.cap);
	CHECK(st.top >= st.block);
	CHECK(is_empty(&st) == 0);
	
	//test popping from the stack
	value tmp = pop(&st, &err);
	CHECK(err.type == E_SUCCESS);
	value res_eq = op_eq(test_string, tmp, &err);
	CHECK(res_eq.val.i != 0);
	tmp = pop(&st, &err);
	CHECK(err.type == E_SUCCESS);
	res_eq = op_eq(test_float, tmp, &err);
	CHECK(res_eq.val.i != 0);
	tmp = pop(&st, &err);
	CHECK(err.type == E_SUCCESS);
	res_eq = op_eq(test_int, tmp, &err);
	CHECK(res_eq.val.i != 0);
	//make sure that the stack is now proper
	CHECK(is_empty(&st) == 1);

	//make sure the stack still works after expansion
	value* old_bottom = st.bottom;
	value* old_block = st.block;
	for (size_t i = 0; i < 2*DEF_STACK_SIZE; ++i) {
	    test_int.val.i = (int)i;
	    push(&st, test_int, &err);
	    CHECK(err.type == E_SUCCESS);
	}
	//make sure the stack grew downwards
	CHECK(st.top < st.bottom);
	INFO("old bottom=", old_bottom, ", old block=", old_block);
	CHECK(st.bottom == st.block + st.cap);
	CHECK(st.top >= st.block);
	CHECK(is_empty(&st) == 0);
	for (int i = 2*DEF_STACK_SIZE-1; i >= 0; --i) {
	    tmp = pop(&st, &err);
	    CHECK(tmp.val.i == i);
	}

	free_Stack(&st);
    }
}

TEST_CASE( "Parsing utilities" ) {
    SUBCASE ( "search_block" ) {
	const char* tst_str = "if(b[1,2] in c) then 3";
	CHECK(_search_block(tst_str, "if", 0) == 0);
	CHECK(_search_block(tst_str, "to", 0) == INONE);
	CHECK(_search_block(tst_str, "in", 0) == INONE);
	CHECK(_search_block(tst_str, "in", 3) == 10);
	CHECK(_search_block(tst_str, "then", 3) == INONE);
	CHECK(_search_block(tst_str, "then", 0) == 16);
    }
}

TEST_CASE( "Test that context fetching works [contexts]" ) {
    SUBCASE ( "Test simple initializations" ) {
	//setup
	sc_error err;
	context con = make_context(&err);
	CHECK(err.type == E_SUCCESS);
	instruction_buffer buf = make_instruction_buffer(&err);
	CHECK(err.type == E_SUCCESS);
	char test_str[TEST_STR_SIZE];

	//add a global variable
	doctest::String glob_test_str = "global_test";
	strncpy(test_str, glob_test_str.c_str(), TEST_STR_SIZE);
	value global_test = v_make_int(14, &err);
	CHECK(err.type == E_SUCCESS);
	insert(&(con.global), test_str, global_test, &err);
	CHECK(err.type == E_SUCCESS);
	//try fetching from global context
	int f_ind = _parse_rval(&con, test_str, 0, &buf, &err);
	CHECK(err.type == E_SUCCESS);
	CHECK(buf.n_insts == 2);
	//make sure the instruction is equivalent to INS_PUSH & INS_HH_G
	CHECK(buf.buf[0].i == 0x85u);
	CHECK(strcmp((char*)(buf.buf[1].ptr), glob_test_str.c_str()) == 0);
	
	//add a local variable
	doctest::String loc_test_str = "local_test";
	strncpy(test_str, loc_test_str.c_str(), TEST_STR_SIZE);
	value local_test = v_make_int(15, &err);
	CHECK(err.type == E_SUCCESS);
	push_n(&(con.callstack), test_str, local_test, &err);
	CHECK(err.type == E_SUCCESS);
	//try fetching from local context
	f_ind = _parse_rval(&con, test_str, 0, &buf, &err);
	CHECK(err.type == E_SUCCESS);
	CHECK(buf.n_insts == 4);
	//make sure the instruction is equivalent to INS_PUSH & INS_HH_S
	CHECK(buf.buf[2].i == 0x45u);
	CHECK(buf.buf[3].i == 0);

	//cleanup
	free_instruction_buffer(&buf);
	free_context(&con);
	free_value(&global_test);
	free_value(&local_test);
    }

    SUBCASE( "Test adding functions" ) {
	//setup
	sc_error err;
	context con = make_context(&err);
	CHECK(err.type == E_SUCCESS);
	instruction_buffer buf = make_instruction_buffer(&err);
	CHECK(err.type == E_SUCCESS);
	char test_str[TEST_STR_SIZE];

	//write the function body
	char func_def[2*TEST_STR_SIZE];
	doctest::String func_def_str = "(int a, int b) => (int) {\nint c = a+b;c= c+1\nreturn c;\n}";
	strncpy(func_def, func_def_str.c_str(), 2*TEST_STR_SIZE);

	//add a function to the global scope
	doctest::String func_test_str = "function_test";
	strncpy(test_str, func_test_str.c_str(), TEST_STR_SIZE);
	value func_test = {0};
	function func_test_val = make_function(&con, func_def, &err);
	func_test.type = VT_FUNC;
	CHECK(err.type == E_SUCCESS);

	//cleanup
	free_instruction_buffer(&buf);
	free_context(&con);
	free_function(&func_test_val);
    }
}

/*TEST_CASE( "Test that parsing rvals works [function parsing]") {


    SUBCASE ( "Test appending values" ) {
	sc_error struct_err, pointer_err;
	String foo_str = make_String("foo", NULL);
	String bar_str = make_String("bar", NULL);
	_append_string_s(&foo_str, bar_str, &struct_err);
	_append_string(&bar_str, "foo", &pointer_err);

	//make sure there weren't any errors
	CHECK(struct_err.type == E_SUCCESS);
	CHECK(pointer_err.type == E_SUCCESS);

	//test the contents of the strings
	CHECK(foo_str.size == 6);
	CHECK(bar_str.size == 6);
	INFO("foo_str = ", foo_str.buf);
	CHECK(strcmp(foo_str.buf, "foobar") == 0);
	INFO("bar_str = ", bar_str.buf);
	CHECK(strcmp(bar_str.buf, "barfoo") == 0);

	//cleanup
	free_String(&foo_str);
	free_String(&bar_str);
    }
}*/
