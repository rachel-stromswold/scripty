#include "values.h"

int main() {
    sc_error tmp_err;
    value test_bool_true = v_make_bool(1, &tmp_err);
    value test_bool_false = v_make_bool(0, &tmp_err);

    value test_int = v_make_int(1, &tmp_err);
    value test_float = v_make_float(1.0, &tmp_err);
    value test_string = v_make_string("test", &tmp_err);

    sc_error err;
    char result[100];
    printf("true: %d, false: %d\n", v_fetch_bool(test_bool_true, &err), v_fetch_bool(test_bool_false, &err));
    printf("test_int: int: %d, float: %f\n", v_fetch_int(test_int, &err), v_fetch_float(test_int, &err));
    printf("test_float: int: %d, float: %f\n", v_fetch_int(test_float, &err), v_fetch_float(test_float, &err));
    v_fetch_string(test_bool_true, result, 100, &err);
    printf("%s\n", result);
    v_fetch_string(test_bool_false, result, 100, &err);
    printf("%s\n", result);
    v_fetch_string(test_int, result, 100, &err);
    printf("%s\n", result);
    v_fetch_string(test_float, result, 100, &err);
    printf("%s\n", result);
    v_fetch_string(test_string, result, 100, &err);
    printf("%s\n", test_string.val.str->buf);
    return 0;
}
