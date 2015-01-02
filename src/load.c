/*
 * Copyright (c) 2015 Luke Dashjr <luke-jr+jansson@utopios.org>
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <math.h>

#include <jansson.h>

#include "ubjansson.h"
#include "jansson_private.h"

#define error_set error_set__ubjson
#define buffer_get buffer_get__ubjson

static void error_set(json_error_t *error, const void *dummy,
                      const char *msg, ...)
{
    va_list ap;
    char msg_text[JSON_ERROR_TEXT_LENGTH];

    int line = -1, col = -1;
    size_t pos = 0;
    const char *result = msg_text;

    if(!error)
        return;

    va_start(ap, msg);
    vsnprintf(msg_text, JSON_ERROR_TEXT_LENGTH, msg, ap);
    msg_text[JSON_ERROR_TEXT_LENGTH - 1] = '\0';
    va_end(ap);

    jsonp_error_set(error, line, col, pos, "%s", result);
}

typedef int (*getchar_func_t)(void *);

#define PUIF_UNSIGNED  0
#define PUIF_SIGNED    1
#define PUIF_CHAR      2

static json_t *parse_ubjson_int(getchar_func_t getchar_func, void *getchar_arg, size_t flags, json_error_t *error, int sz, unsigned puif_flags)
{
    unsigned char s[8];
    int c;
    int i;
    int significant_bits = 0;
    int negative = 0;
    double f;
    json_int_t val = 0;

    for(i = 0; i < sz; ++i) {
        c = getchar_func(getchar_arg);
        if(c == EOF) {
            error_set(error, NULL, "premature end of input");
            return NULL;
        }
        if(i == 0 && (puif_flags & PUIF_SIGNED) && (c & 0x80))
            negative = 1;
        if(negative)
            c ^= 0xFF;
        s[i] = c;
        if(significant_bits)
            significant_bits += 8;
        else if(c)
            significant_bits = (c & 0x80) ? 8 : 7;
    }

    if(significant_bits > (sizeof(val) * 8) - 1)
    {
        /* Not possible to represent this value in json_int_t, so we need to use a double */
        f = 0;
        for(i = 0; i < sz; ++i)
            f = (f * 0x100) + s[i];

        if(negative)
            f = (-f) - 1;

        return json_real(f);
    }

    for(i = 0; i < sz; ++i)
        val = (val << 8) | s[i];

    if(negative)
        val = (-val) - 1;

    if (puif_flags & PUIF_CHAR)
    {
        const char st[] = {val, '\0'};
        return json_string(st);
    }

    return json_integer(val);
}

static json_t *parse_ubjson_float(getchar_func_t getchar_func, void *getchar_arg, size_t flags, json_error_t *error, int sz)
{
    unsigned char s[8];
    int c;
    int i;
    int exponent;
    double f = 0;

    for(i = 0; i < sz; ++i) {
        c = getchar_func(getchar_arg);
        if(c == EOF) {
            error_set(error, NULL, "premature end of input");
            return NULL;
        }
        s[i] = c;
    }

    if(sz == 4) {
        exponent = (((s[0] & 0x7F) << 1) | (s[1] >> 7)) - 127 - 23;
        s[1] |= 0x80;
    }
    else  /* sz == 8 */
    {
        exponent = (((int)(s[0] & 0x7F) << 4) | (s[1] >> 4)) - 1023 - 52;
        s[1] &= 0xf;
        s[1] |= 0x10;
    }

    for(i = 1; i < sz; ++i)
        f = (f * 0x100) + s[i];

    f *= pow(2, exponent);

    if(s[0] & 0x80)
        f = -f;

    return json_real(f);
}

static json_t *parse_ubjson_value(getchar_func_t getchar_func, void *getchar_arg, size_t flags, json_error_t *error, int type);

static int parse_ubjson_any_size(getchar_func_t getchar_func, void *getchar_arg, size_t flags, json_error_t *error, int type, json_int_t *out)
{
    json_int_t i;
    json_t *jlen = parse_ubjson_value(getchar_func, getchar_arg, flags, error, type);
    if(!json_is_integer(jlen)) {
        json_decref(jlen);
        error_set(error, NULL, "non-integer size");
        return 0;
    }
    i = json_integer_value(jlen);
    json_decref(jlen);
    if(i < 0) {
        error_set(error, NULL, "negative size");
        return 0;
    }
    *out = i;
    return 1;
}

static char *parse_ubjson_str(getchar_func_t getchar_func, void *getchar_arg, size_t flags, json_error_t *error, int type)
{
    char *buf;
    int c;
    int i;
    json_int_t len;

    if(!parse_ubjson_any_size(getchar_func, getchar_arg, flags, error, type, &len))
        return NULL;
    buf = malloc(len + 1);
    for(i = 0; i < len; ++i)
    {
        c = getchar_func(getchar_arg);
        if(c == EOF) {
            free(buf);
            error_set(error, NULL, "premature end of input");
            return NULL;
        }
        buf[i] = c;
    }
    buf[len] = '\0';
    return buf;
}

static json_t *parse_ubjson_value(getchar_func_t getchar_func, void *getchar_arg, size_t flags, json_error_t *error, int type)
{
    while (type == 'N' || !type)
        type = getchar_func(getchar_arg);
    switch (type) {
        case EOF: {
            error_set(error, NULL, "premature end of input");
            return NULL;
        }
        case 'Z':
            return json_null();
        case 'T':
            return json_true();
        case 'F':
            return json_false();
        case 'i':
            return parse_ubjson_int(getchar_func, getchar_arg, flags, error, 1, PUIF_SIGNED);
        case 'U':
            return parse_ubjson_int(getchar_func, getchar_arg, flags, error, 1, PUIF_UNSIGNED);
        case 'I':
            return parse_ubjson_int(getchar_func, getchar_arg, flags, error, 2, PUIF_SIGNED);
        case 'l':
            return parse_ubjson_int(getchar_func, getchar_arg, flags, error, 4, PUIF_SIGNED);
        case 'L':
            return parse_ubjson_int(getchar_func, getchar_arg, flags, error, 8, PUIF_SIGNED);
        case 'C':
            return parse_ubjson_int(getchar_func, getchar_arg, flags, error, 1, PUIF_UNSIGNED | PUIF_CHAR);
        case 'd':
            return parse_ubjson_float(getchar_func, getchar_arg, flags, error, 4);
        case 'D':
            return parse_ubjson_float(getchar_func, getchar_arg, flags, error, 8);
        case 'H': case 'S': {
            char *buf;
            json_t *ret;

            buf = parse_ubjson_str(getchar_func, getchar_arg, flags, error, 0);
            if(!buf)
                return NULL;

            if(type == 'H')
            {
                ret = json_loads(buf, JSON_DECODE_ANY, error);
                free(buf);
                if(!(ret && json_is_number(ret))) {
                    error_set(error, NULL, "failed parsing high-precision number");
                    return NULL;
                }
            }
            else
            {
                ret = json_string(buf);
                free(buf);
            }
            return ret;
        }
        case '[': case '{': {
            int c;
            int contained_type = 0;
            int elem_type;
            json_int_t count = -1;
            json_int_t i;
            json_int_t j;
            json_t *elem;
            json_t *container;
            char *key = NULL;

            c = getchar_func(getchar_arg);
            if(c == '$') {
                /* sole contained type */
                contained_type = getchar_func(getchar_arg);
                c = getchar_func(getchar_arg);
                if(c != '#') {
                    error_set(error, NULL, "container has type without count");
                    return NULL;
                }
            }
            if(c == '#') {
                /* fixed item count */
                if(!parse_ubjson_any_size(getchar_func, getchar_arg, flags, error, 0, &count))
                    return NULL;
                c = 0;
            }
            container = (type == '[') ? json_array() : json_object();
            if(!container)
                return NULL;
            for(i = 0; (count == -1) || (i < count); ++i) {
                if(count == -1) {
                    if(!c)
                        c = getchar_func(getchar_arg);
                    if(c == ((type == '[') ? ']' : '}'))
                        break;
                }
                if(type == '{') {
                    key = parse_ubjson_str(getchar_func, getchar_arg, flags, error, c);
                    c = 0;
                    if(!key) {
                        json_decref(container);
                        return NULL;
                    }
                }
                if(contained_type)
                    elem_type = contained_type;
                else
                {
                    elem_type = c ? c : getchar_func(getchar_arg);
                    c = 0;
                }
                if (elem_type == 'N')
                {
                    free(key);
                    continue;
                }
                elem = parse_ubjson_value(getchar_func, getchar_arg, flags, error, elem_type);
                if (!elem)
                    j = 1;
                else if (type == '[')
                    j = json_array_append_new(container, elem);
                else
                    j = json_object_set_new(container, key, elem);
                free(key);
                if(j) {
                    json_decref(container);
                    return NULL;
                }
            }
            return container;
        }
        default: {
            error_set(error, NULL, "unrecognized type");
            return NULL;
        }
    }
}

static json_t *parse_ubjson(const void *getchar_func_p, void *getchar_arg, size_t flags, json_error_t *error)
{
    getchar_func_t getchar_func = getchar_func_p;
    int type = 0;
    json_t *result;

    if(!(flags & JSON_DECODE_ANY)) {
        type = getchar_func(getchar_arg);
        if(type != '[' && type != '{') {
            error_set(error, NULL, "'[' or '{' expected");
            return NULL;
        }
    }

    result = parse_ubjson_value(getchar_func, getchar_arg, flags, error, type);

    if(!result) {
        if (!error->text[0])
            error_set(error, NULL, "unknown error");
        return NULL;
    }

    if(!(flags & JSON_DISABLE_EOF_CHECK)) {
        if(getchar_func(getchar_arg) != EOF) {
            error_set(error, NULL, "end of file expected");
            json_decref(result);
            return NULL;
        }
    }

    return result;
}

typedef struct
{
    const char *data;
    size_t len;
    size_t pos;
} buffer_data_t;

static int buffer_get(void *data)
{
    char c;
    buffer_data_t *stream = data;
    if(stream->pos >= stream->len)
      return EOF;

    c = stream->data[stream->pos];
    stream->pos++;
    return (unsigned char)c;
}

json_t *ubjson_loadb(void *buffer, size_t buflen, size_t flags, json_error_t *error)
{
    json_t *result;
    buffer_data_t stream_data;

    jsonp_error_init(error, "<buffer>");

    if (buffer == NULL) {
        error_set(error, NULL, "wrong arguments");
        return NULL;
    }

    stream_data.data = buffer;
    stream_data.pos = 0;
    stream_data.len = buflen;

    result = parse_ubjson(buffer_get, &stream_data, flags, error);
    return result;
}

json_t *ubjson_loadf(FILE *input, size_t flags, json_error_t *error)
{
    const char *source;
    json_t *result;

    if(input == stdin)
        source = "<stdin>";
    else
        source = "<stream>";

    jsonp_error_init(error, source);

    if (input == NULL) {
        error_set(error, NULL, "wrong arguments");
        return NULL;
    }

    result = parse_ubjson(fgetc, input, flags, error);
    return result;
}
