/*
 * Copyright (c) 2015 Luke Dashjr <luke-jr+jansson@utopios.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <ubjansson.h>

static int passed, failed;

static json_t *test1(void *bin, size_t sz, const char *binraw, const char *testraw)
{
    json_error_t err;
    json_t *json;

    json = ubjson_loadb(bin, sz, JSON_DECODE_ANY, &err);
    if (!json)
    {
        fprintf(stderr, "FAILED parse UBJSON %s test %s: %s\n", binraw, testraw, err.text);
        ++failed;
    }
    return json;
}

static void test2(json_t *json, int result, const char *binraw, const char *testraw)
{
    if (!result)
    {
        fprintf(stderr, "FAILED test  UBJSON %s test %s\n", binraw, testraw);
        ++failed;
    }
    ++passed;
}

#define test(bin, x)  do {  \
    json = test1(bin, sizeof(bin)-1, #bin, #x);  \
    if(json)  \
        test2(json, x, #bin, #x);  \
} while(0)

int main(int argc, char *argv[])
{
    json_t *json;

    test("Z", json_is_null(json));
    test("NZ", json_is_null(json));
    test("T", json_is_true(json));
    test("NT", json_is_true(json));
    test("F", json_is_false(json));

    test("i\0", json_is_integer(json) && json_integer_value(json) == 0);
    test("i\xff", json_is_integer(json) && json_integer_value(json) == -1);
    test("i\x7f", json_is_integer(json) && json_integer_value(json) == 127);
    test("i\x80", json_is_integer(json) && json_integer_value(json) == -128);

    test("U\0", json_is_integer(json) && json_integer_value(json) == 0);
    test("U\xff", json_is_integer(json) && json_integer_value(json) == 255);
    test("U\x7f", json_is_integer(json) && json_integer_value(json) == 127);
    test("U\x80", json_is_integer(json) && json_integer_value(json) == 128);

    test("I\0\0", json_is_integer(json) && json_integer_value(json) == 0);
    test("I\xff\xff", json_is_integer(json) && json_integer_value(json) == -1);
    test("I\0\x7f", json_is_integer(json) && json_integer_value(json) == 127);
    test("I\x80\0", json_is_integer(json) && json_integer_value(json) == -32768);
    test("I\x7f\xff", json_is_integer(json) && json_integer_value(json) == 32767);
    test("I\x12\x34", json_is_integer(json) && json_integer_value(json) == 0x1234);

    test("l\0\0\0\0", json_is_integer(json) && json_integer_value(json) == 0);
    test("l\xff\xff\xff\xff", json_is_integer(json) && json_integer_value(json) == -1);
    test("l\0\0\0\x7f", json_is_integer(json) && json_integer_value(json) == 127);
    test("l\x80\0\0\0", json_is_integer(json) && json_integer_value(json) == (-2147483647L - 1));
    test("l\x7f\xff\xff\xff", json_is_integer(json) && json_integer_value(json) == 2147483647L);
    test("l\x12\x34\x56\x78", json_is_integer(json) && json_integer_value(json) == 0x12345678);

    test("L\0\0\0\0\0\0\0\0", json_is_integer(json) && json_integer_value(json) == 0);
    test("L\xff\xff\xff\xff\xff\xff\xff\xff", json_is_integer(json) && json_integer_value(json) == -1);
    test("L\0\0\0\0\0\0\0\x7f", json_is_integer(json) && json_integer_value(json) == 127);
    test("L\x80\0\0\0\0\0\0\0", json_is_integer(json) && json_integer_value(json) == (-9223372036854775807LL - 1));
    test("L\x7f\xff\xff\xff\xff\xff\xff\xff", json_is_integer(json) && json_integer_value(json) == 9223372036854775807LL);
    test("L\x12\x34\x56\x78\x9a\xbc\xde\xf0", json_is_integer(json) && json_integer_value(json) == 0x123456789abcdef0LL);

    test("d\x3f\x80\0\0", json_is_number(json) && json_number_value(json) == 1.0);
    test("d\x37\x80\0\0", json_is_number(json) && json_number_value(json) == 1.0/0x10000);
    test("d\x3d\xfc\xd6\xea", json_is_number(json) && round(json_number_value(json) * 0x1000000) == 2071261);
    test("d\x49\x96\xb4\x38", json_is_number(json) && json_number_value(json) == 1234567.0);
    test("d\x7f\0\0\0", json_is_number(json) && (int)round(log2(json_number_value(json))) == 127);

    test("d\x3f\x80\0\0", json_is_number(json) && json_number_value(json) == 1.0);
    test("d\x37\x80\0\0", json_is_number(json) && json_number_value(json) == 1.0/0x10000);
    test("d\x3d\xfc\xd6\xea", json_is_number(json) && round(json_number_value(json) * 0x1000000) == 2071261);
    test("d\x49\x96\xb4\x38", json_is_number(json) && json_number_value(json) == 1234567.0);
    test("d\x7f\0\0\0", json_is_number(json) && (int)log2(json_number_value(json)) == 127);

    test("D\x3F\xF0\0\0\0\0\0\0", json_is_number(json) && json_number_value(json) == 1.0);
    test("D\x3e\xef\xff\xff\xfd\xcd\x0c\xd0", json_is_number(json) && round(json_number_value(json) * 0x100000000LL) == 0x10000);
    test("D\x3f\xbf\x9A\xDD\x37\x39\x63\x5F", json_is_number(json) && round(json_number_value(json) * 0x1000000) == 2071261);
    test("D\x41\x32\xD6\x87\0\0\0\0", json_is_number(json) && json_number_value(json) == 1234567.0);
    test("D\x47\xe0\0\0\0\0\0\0", json_is_number(json) && (int)log2(json_number_value(json)) == 127);
    test("D\x7f\xef\xff\xff\xff\xff\xff\xff", json_is_number(json) && (int)round(log2(json_number_value(json))) == 1024);

    test("Hi\x0a""2147483647", json_is_integer(json) && json_integer_value(json) == 2147483647L);
    test("HU\x0a""2147483647", json_is_integer(json) && json_integer_value(json) == 2147483647L);
    test("HI\0\x0a""2147483647", json_is_integer(json) && json_integer_value(json) == 2147483647L);
    test("Hl\0\0\0\x0a""2147483647", json_is_integer(json) && json_integer_value(json) == 2147483647L);
    test("HL\0\0\0\0\0\0\0\x0a""2147483647", json_is_integer(json) && json_integer_value(json) == 2147483647L);
    test("HHi\x02""10""2147483647", json_is_integer(json) && json_integer_value(json) == 2147483647L);
    test("HHHi\x01""2""10""2147483647", json_is_integer(json) && json_integer_value(json) == 2147483647L);

    test("C\x41", json_is_string(json) && !strcmp(json_string_value(json), "A"));

    test("Si\x0a""2147483647", json_is_string(json) && !strcmp(json_string_value(json), "2147483647"));
    test("SU\x0a""2147483647", json_is_string(json) && !strcmp(json_string_value(json), "2147483647"));
    test("SI\0\x0a""2147483647", json_is_string(json) && !strcmp(json_string_value(json), "2147483647"));
    test("Sl\0\0\0\x0a""2147483647", json_is_string(json) && !strcmp(json_string_value(json), "2147483647"));
    test("SL\0\0\0\0\0\0\0\x0a""2147483647", json_is_string(json) && !strcmp(json_string_value(json), "2147483647"));
    test("SHi\x02""10""2147483647", json_is_string(json) && !strcmp(json_string_value(json), "2147483647"));
    test("SHHi\x01""2""10""2147483647", json_is_string(json) && !strcmp(json_string_value(json), "2147483647"));

    test("[]", json_is_array(json) && json_array_size(json) == 0);
    test("[#i\0", json_is_array(json) && json_array_size(json) == 0);
    test("[$N#i\0", json_is_array(json) && json_array_size(json) == 0);
    test("[$T#U\0", json_is_array(json) && json_array_size(json) == 0);
    test("[$S#i\0", json_is_array(json) && json_array_size(json) == 0);
    test("[$T#i\x02", json_is_array(json) && json_array_size(json) == 2 && json_is_true(json_array_get(json, 1)));
    test("[$U#i\x02\x05\xff", json_is_array(json) && json_array_size(json) == 2 && json_is_integer(json_array_get(json, 0)) && json_is_integer(json_array_get(json, 1)) && json_integer_value(json_array_get(json, 0)) == 5 && json_integer_value(json_array_get(json, 1)) == 0xff);
    test("[i\x05i""\x06]", json_is_array(json) && json_array_size(json) == 2 && json_integer_value(json_array_get(json, 0)) == 5 && json_integer_value(json_array_get(json, 1)) == 6);
    test("[i\x05""NF]", json_is_array(json) && json_array_size(json) == 2 && json_integer_value(json_array_get(json, 0)) == 5 && json_is_false(json_array_get(json, 1)));
    test("[#i\x02""i\x05i""\x06", json_is_array(json) && json_array_size(json) == 2 && json_integer_value(json_array_get(json, 0)) == 5 && json_integer_value(json_array_get(json, 1)) == 6);
    test("[#i\x03""i\x05""NF", json_is_array(json) && json_array_size(json) == 2 && json_integer_value(json_array_get(json, 0)) == 5 && json_is_false(json_array_get(json, 1)));
    test("[i\x05""NF]", json_is_array(json) && json_array_size(json) == 2 && json_integer_value(json_array_get(json, 0)) == 5 && json_is_false(json_array_get(json, 1)));

    test("{}", json_is_object(json) && json_object_size(json) == 0);
    test("{#i\0", json_is_object(json) && json_object_size(json) == 0);
    test("{$N#i\0", json_is_object(json) && json_object_size(json) == 0);
    test("{$T#U\0", json_is_object(json) && json_object_size(json) == 0);
    test("{$S#i\0", json_is_object(json) && json_object_size(json) == 0);
    test("{$T#i\x02""i\0""i\x04""abcd", json_is_object(json) && json_object_size(json) == 2 && json_is_true(json_object_get(json, "")) && json_is_true(json_object_get(json, "abcd")));
    test("{$U#i\x02""i\x02""ab""\x05""i\x01""a\xff", json_is_object(json) && json_object_size(json) == 2 && json_is_integer(json_object_get(json, "a")) && json_is_integer(json_object_get(json, "ab")) && json_integer_value(json_object_get(json, "ab")) == 5 && json_integer_value(json_object_get(json, "a")) == 0xff);
    test("{#i\x02""i\x02""ab""i\x05""i\x01""aU\xff", json_is_object(json) && json_object_size(json) == 2 && json_is_integer(json_object_get(json, "a")) && json_is_integer(json_object_get(json, "ab")) && json_integer_value(json_object_get(json, "ab")) == 5 && json_integer_value(json_object_get(json, "a")) == 0xff);
    test("{i\x02""ab""U\x05""i\x01""aU\xff}", json_is_object(json) && json_object_size(json) == 2 && json_is_integer(json_object_get(json, "a")) && json_is_integer(json_object_get(json, "ab")) && json_integer_value(json_object_get(json, "ab")) == 5 && json_integer_value(json_object_get(json, "a")) == 0xff);

    printf("%d passed, %d failed\n", passed, failed);
    return failed;
}
