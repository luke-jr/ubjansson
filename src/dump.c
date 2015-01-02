/*
 * Copyright (c) 2015 Luke Dashjr <luke-jr+jansson@utopios.org>
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <string.h>

#include <jansson.h>

#include "ubjansson.h"

static int dump_ubjson_int(json_int_t num, json_dump_callback_t dump, void *data);

static int dump_ubjson_buf(const void *buf, size_t bufsz, json_dump_callback_t dump, void *data)
{
    if(dump_ubjson_int(bufsz, dump, data))
        return -1;
    if(dump(buf, bufsz, data))
        return -1;
    return 0;
}

static int dump_ubjson_hpn(json_t *json, json_dump_callback_t dump, void *data)
{
    if(dump("H", 1, data))
        return -1;
    char *st = json_dumps(json, JSON_ENCODE_ANY);
    int ret = dump_ubjson_buf(st, strlen(st), dump, data);
    free(st);
    return ret;
}

static int dump_ubjson_int(json_int_t num, json_dump_callback_t dump, void *data)
{
    if(num < 0) {
        json_t *json = json_integer(num);
        if(dump_ubjson_hpn(json, dump, data))
            return -1;
        json_decref(json);
        return 0;
    }

    unsigned char s[9];
    int i;

    s[0] = 'L';
    for(i = 8; i > 0; --i) {
        s[i] = num & 0xff;
        num >>= 8;
    }

    if (dump((void *)s, 9, data))
        return -1;

    return 0;
}

static int dump_ubjson_value(json_t *json, size_t flags, int depth,
                   json_dump_callback_t dump, void *data)
{
    switch(json_typeof(json)) {
        case JSON_OBJECT: {
            const char *key;
            json_t *value;
            void *iter;

            dump("{#", 2, data);
            dump_ubjson_int(json_object_size(json), dump, data);

            for (iter = json_object_iter(json); iter; iter = json_object_iter_next(json, iter))
            {
                key = json_object_iter_key(iter);
                value = json_object_iter_value(iter);

                if(dump_ubjson_buf(key, strlen(key), dump, data))
                    return -1;
                if(dump_ubjson_value(value, flags, depth + 1, dump, data))
                    return -1;
            }
            return 0;
        }
        case JSON_ARRAY: {
            json_t *elem;
            size_t i;
            size_t count = json_array_size(json);

            dump("[#", 2, data);
            dump_ubjson_int(count, dump, data);

            for (i = 0; i < count; ++i)
            {
                elem = json_array_get(json, i);
                if(dump_ubjson_value(elem, flags, depth + 1, dump, data))
                    return -1;
            }
            return 0;
        }
        case JSON_STRING: {
            const char *st = json_string_value(json);
            if(dump("S", 1, data))
                return -1;
            if(dump_ubjson_buf(st, strlen(st), dump, data))
                return -1;
            return 0;
        }
        case JSON_INTEGER:
            return dump_ubjson_int(json_integer_value(json), dump, data);
        case JSON_REAL:
            return dump_ubjson_hpn(json, dump, data);
        case JSON_TRUE:
            return dump("T", 1, data);
        case JSON_FALSE:
            return dump("F", 1, data);
        case JSON_NULL:
            return dump("Z", 1, data);
    }
    return -1;
}

int ubjson_dump_callback(json_t *json, json_dump_callback_t callback, void *data, size_t flags)
{
    if(!(flags & JSON_ENCODE_ANY)) {
        if(!json_is_array(json) && !json_is_object(json))
           return -1;
    }

    return dump_ubjson_value(json, flags, 0, callback, data);
}

struct dumpb_data {
    unsigned char *p;
    size_t rem;
    size_t sz;
};

static int dumpb_callback(const char *buffer, size_t size, void *datap)
{
    struct dumpb_data *data = datap;
    if(data->rem) {
        size_t copysz = (data->rem < size) ? data->rem : size;
        memcpy(data->p, buffer, copysz);
        data->p += copysz;
        data->rem -= copysz;
    }
    data->sz += size;
    return 0;
}

ssize_t ubjson_dumpb(json_t *json, void *buffer, size_t buflen, size_t flags)
{
    struct dumpb_data data;

    data.p = buffer;
    data.rem = buflen;
    data.sz = 0;

    if (ubjson_dump_callback(json, dumpb_callback, &data, flags))
        return -1;

    return data.sz;
}
