/*
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef JANSSON_PRIVATE_H
#define JANSSON_PRIVATE_H

#include <stddef.h>
#include "jansson.h"

void jsonp_error_init(json_error_t *error, const char *source);
void jsonp_error_set_source(json_error_t *error, const char *source);
void jsonp_error_set(json_error_t *error, int line, int column,
                     size_t position, const char *msg, ...);
void jsonp_error_vset(json_error_t *error, int line, int column,
                      size_t position, const char *msg, va_list ap);

/* Windows compatibility */
#ifdef _WIN32
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#endif
