/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */


/*******************************************************
 *   Enrico Fucile
 ******************************************************/

#include "grib_api_internal.h"

/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = accessor
   SUPER      = grib_accessor_class_gen
   IMPLEMENTS = unpack_double;pack_double
   IMPLEMENTS = unpack_string;pack_string;string_length
   IMPLEMENTS = unpack_long;pack_long;destroy
   IMPLEMENTS = init;dump;value_count;get_native_type
   IMPLEMENTS = compare
   MEMBERS = char* key
   MEMBERS = grib_hash_array_value* ha
    END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static int get_native_type(grib_accessor*);
static int pack_double(grib_accessor*, const double* val, size_t* len);
static int pack_long(grib_accessor*, const long* val, size_t* len);
static int pack_string(grib_accessor*, const char*, size_t* len);
static int unpack_double(grib_accessor*, double* val, size_t* len);
static int unpack_long(grib_accessor*, long* val, size_t* len);
static int unpack_string(grib_accessor*, char*, size_t* len);
static size_t string_length(grib_accessor*);
static int value_count(grib_accessor*, long*);
static void destroy(grib_context*, grib_accessor*);
static void dump(grib_accessor*, grib_dumper*);
static void init(grib_accessor*, const long, grib_arguments*);
static int compare(grib_accessor*, grib_accessor*);

typedef struct grib_accessor_hash_array
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in hash_array */
    char* key;
    grib_hash_array_value* ha;
} grib_accessor_hash_array;

extern grib_accessor_class* grib_accessor_class_gen;

static grib_accessor_class _grib_accessor_class_hash_array = {
    &grib_accessor_class_gen,                      /* super */
    "hash_array",                      /* name */
    sizeof(grib_accessor_hash_array),  /* size */
    0,                           /* inited */
    0,                           /* init_class */
    &init,                       /* init */
    0,                  /* post_init */
    &destroy,                    /* destroy */
    &dump,                       /* dump */
    0,                /* next_offset */
    &string_length,              /* get length of string */
    &value_count,                /* get number of values */
    0,                 /* get number of bytes */
    0,                /* get offset to bytes */
    &get_native_type,            /* get native type */
    0,                /* get sub_section */
    0,               /* pack_missing */
    0,                 /* is_missing */
    &pack_long,                  /* pack_long */
    &unpack_long,                /* unpack_long */
    &pack_double,                /* pack_double */
    0,                 /* pack_float */
    &unpack_double,              /* unpack_double */
    0,               /* unpack_float */
    &pack_string,                /* pack_string */
    &unpack_string,              /* unpack_string */
    0,          /* pack_string_array */
    0,        /* unpack_string_array */
    0,                 /* pack_bytes */
    0,               /* unpack_bytes */
    0,            /* pack_expression */
    0,              /* notify_change */
    0,                /* update_size */
    0,             /* preferred_size */
    0,                     /* resize */
    0,      /* nearest_smaller_value */
    0,                       /* next accessor */
    &compare,                    /* compare vs. another accessor */
    0,      /* unpack only ith value (double) */
    0,       /* unpack only ith value (float) */
    0,  /* unpack a given set of elements (double) */
    0,   /* unpack a given set of elements (float) */
    0,     /* unpack a subarray */
    0,                      /* clear */
    0,                 /* clone accessor */
};


grib_accessor_class* grib_accessor_class_hash_array = &_grib_accessor_class_hash_array;

/* END_CLASS_IMP */

#define MAX_HASH_ARRAY_STRING_LENGTH 255

static void init(grib_accessor* a, const long len, grib_arguments* args)
{
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    a->length                      = 0;
    self->key                      = 0;
    self->ha                       = NULL;
}

static void dump(grib_accessor* a, grib_dumper* dumper)
{
    grib_dump_string(dumper, a, NULL);
}

static int pack_double(grib_accessor* a, const double* val, size_t* len)
{
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    char s[200]                    = {0,};
    snprintf(s, sizeof(s), "%g", *val);
    self->key = grib_context_strdup(a->context, s);
    self->ha  = 0;
    return GRIB_SUCCESS;
}

static int pack_long(grib_accessor* a, const long* val, size_t* len)
{
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    char s[200]                    = {0,};
    snprintf(s, sizeof(s), "%ld", *val);
    if (self->key)
        grib_context_free(a->context, self->key);
    self->key = grib_context_strdup(a->context, s);
    self->ha  = 0;
    return GRIB_SUCCESS;
}

static int pack_string(grib_accessor* a, const char* v, size_t* len)
{
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    self->key                      = grib_context_strdup(a->context, v);
    self->ha                       = 0;
    return GRIB_SUCCESS;
}

static int unpack_double(grib_accessor* a, double* val, size_t* len)
{
    return GRIB_NOT_IMPLEMENTED;
}

static grib_hash_array_value* find_hash_value(grib_accessor* a, int* err)
{
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    grib_hash_array_value* ha_ret  = 0;
    grib_hash_array_value* ha      = NULL;

    ha = get_hash_array(grib_handle_of_accessor(a), a->creator);
    if (!ha) {
        grib_context_log(a->context, GRIB_LOG_ERROR,
                         "unable to get hash value for %s", a->creator->name);
        *err = GRIB_HASH_ARRAY_NO_MATCH;
        return NULL;
    }

    *err = GRIB_SUCCESS;

    Assert(ha != NULL);
    if (!self->key) {
        grib_context_log(a->context, GRIB_LOG_ERROR,
                         "unable to get hash value for %s, set before getting", a->creator->name);
        *err = GRIB_HASH_ARRAY_NO_MATCH;
        return NULL;
    }

    ha_ret = (grib_hash_array_value*)grib_trie_get(ha->index, self->key);
    if (!ha_ret)
        ha_ret = (grib_hash_array_value*)grib_trie_get(ha->index, "default");

    if (!ha_ret) {
        *err = GRIB_HASH_ARRAY_NO_MATCH;
        grib_context_log(a->context, GRIB_LOG_ERROR,
                         "hash_array: no match for %s=%s",
                         a->creator->name, self->key);
        return NULL;
    }
    return ha_ret;
}

static int unpack_long(grib_accessor* a, long* val, size_t* len)
{
    grib_hash_array_value* ha = 0;
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    int err = 0;
    size_t i = 0;

    if (!self->ha) {
        ha = find_hash_value(a, &err);
        if (err)
            return err;
        self->ha = ha;
    }

    switch (self->ha->type) {
        case GRIB_HASH_ARRAY_TYPE_INTEGER:
            if (*len < self->ha->iarray->n) {
                return GRIB_ARRAY_TOO_SMALL;
            }
            *len = self->ha->iarray->n;
            for (i = 0; i < *len; i++)
                val[i] = self->ha->iarray->v[i];
            break;

        default:
            return GRIB_NOT_IMPLEMENTED;
    }

    return GRIB_SUCCESS;
}

static int get_native_type(grib_accessor* a)
{
    int type = GRIB_TYPE_STRING;
    if (a->flags & GRIB_ACCESSOR_FLAG_LONG_TYPE)
        type = GRIB_TYPE_LONG;

    return type;
}

static void destroy(grib_context* c, grib_accessor* a)
{
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    if (self->key)
        grib_context_free(c, self->key);
}

static int unpack_string(grib_accessor* a, char* val, size_t* len)
{
    return GRIB_NOT_IMPLEMENTED;
}

static size_t string_length(grib_accessor* a)
{
    return MAX_HASH_ARRAY_STRING_LENGTH;
}

static int value_count(grib_accessor* a, long* count)
{
    int err                        = 0;
    grib_accessor_hash_array* self = (grib_accessor_hash_array*)a;
    grib_hash_array_value* ha      = 0;

    if (!self->ha) {
        ha = find_hash_value(a, &err);
        if (err)
            return err;
        self->ha = ha;
    }

    *count = self->ha->iarray->n;
    return err;
}

static int compare(grib_accessor* a, grib_accessor* b)
{
    return GRIB_NOT_IMPLEMENTED;
}
