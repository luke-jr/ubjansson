/*
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 * Copyright (c) 2015 Luke Dashjr <luke-jr+jansson@utopios.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef UBJANSSON_H
#define UBJANSSON_H

#include <stdio.h>
#include <stdlib.h>  /* for size_t */
#include <stdarg.h>

#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

/* version */

#define UBJANSSON_MAJOR_VERSION  0
#define UBJANSSON_MINOR_VERSION  1
#define UBJANSSON_MICRO_VERSION  0

/* Micro version is omitted if it's 0 */
#define UBJANSSON_VERSION  "0.1"

/* Version as a 3-byte hex number, e.g. 0x010201 == 1.2.1. Use this
   for numeric comparisons, e.g. #if UBJANSSON_VERSION_HEX >= ... */
#define UBJANSSON_VERSION_HEX  ((UBJANSSON_MAJOR_VERSION << 16) |   \
                                (UBJANSSON_MINOR_VERSION << 8)  |   \
                                (UBJANSSON_MICRO_VERSION << 0))


/* decoding */

json_t *ubjson_loadb(void *buffer, size_t buflen, size_t flags, json_error_t *error);
json_t *ubjson_loadf(FILE *input, size_t flags, json_error_t *error);


/* encoding */

ssize_t ubjson_dumpb(json_t *json, void *buffer, size_t buflen, size_t flags);
int ubjson_dump_callback(json_t *json, json_dump_callback_t callback, void *data, size_t flags);


#ifdef __cplusplus
}
#endif

#endif
