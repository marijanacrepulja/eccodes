/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_api_internal.h"
/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = accessor
   SUPER      = grib_accessor_class_ascii
   IMPLEMENTS = unpack_string;value_count
   IMPLEMENTS = init;string_length
   MEMBERS =  int gts_offset
   MEMBERS =  int gts_length
   END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static int unpack_string(grib_accessor*, char*, size_t* len);
static size_t string_length(grib_accessor*);
static int value_count(grib_accessor*, long*);
static void init(grib_accessor*, const long, grib_arguments*);

typedef struct grib_accessor_gts_header
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in ascii */
    /* Members defined in gts_header */
    int gts_offset;
    int gts_length;
} grib_accessor_gts_header;

extern grib_accessor_class* grib_accessor_class_ascii;

static grib_accessor_class _grib_accessor_class_gts_header = {
    &grib_accessor_class_ascii,                      /* super */
    "gts_header",                      /* name */
    sizeof(grib_accessor_gts_header),  /* size */
    0,                           /* inited */
    0,                           /* init_class */
    &init,                       /* init */
    0,                  /* post_init */
    0,                    /* destroy */
    0,                       /* dump */
    0,                /* next_offset */
    &string_length,              /* get length of string */
    &value_count,                /* get number of values */
    0,                 /* get number of bytes */
    0,                /* get offset to bytes */
    0,            /* get native type */
    0,                /* get sub_section */
    0,               /* pack_missing */
    0,                 /* is_missing */
    0,                  /* pack_long */
    0,                /* unpack_long */
    0,                /* pack_double */
    0,                 /* pack_float */
    0,              /* unpack_double */
    0,               /* unpack_float */
    0,                /* pack_string */
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
    0,                    /* compare vs. another accessor */
    0,      /* unpack only ith value (double) */
    0,       /* unpack only ith value (float) */
    0,  /* unpack a given set of elements (double) */
    0,   /* unpack a given set of elements (float) */
    0,     /* unpack a subarray */
    0,                      /* clear */
    0,                 /* clone accessor */
};


grib_accessor_class* grib_accessor_class_gts_header = &_grib_accessor_class_gts_header;

/* END_CLASS_IMP */

static void init(grib_accessor* a, const long l, grib_arguments* c)
{
    grib_accessor_gts_header* self = (grib_accessor_gts_header*)a;
    self->gts_offset = -1;
    self->gts_length = -1;
    self->gts_offset = grib_arguments_get_long(grib_handle_of_accessor(a), c, 0);
    self->gts_length = grib_arguments_get_long(grib_handle_of_accessor(a), c, 1);
    a->flags |= GRIB_ACCESSOR_FLAG_READ_ONLY;
}

static int unpack_string(grib_accessor* a, char* val, size_t* len)
{
    grib_accessor_gts_header* self = (grib_accessor_gts_header*)a;
    grib_handle* h                 = grib_handle_of_accessor(a);
    int offset                     = 0;
    size_t length                  = 0;

    if (h->gts_header == NULL || h->gts_header_len < 8) {
        if (*len < 8)
            return GRIB_ARRAY_TOO_SMALL;
        snprintf(val, 1024, "missing");
        return GRIB_SUCCESS;
    }
    if (*len < h->gts_header_len)
        return GRIB_ARRAY_TOO_SMALL;

    offset = self->gts_offset > 0 ? self->gts_offset : 0;
    length = self->gts_length > 0 ? self->gts_length : h->gts_header_len;

    memcpy(val, h->gts_header + offset, length);

    *len = length;

    return GRIB_SUCCESS;
}

static int value_count(grib_accessor* a, long* count)
{
    *count = 1;
    return 0;
}

static size_t string_length(grib_accessor* a)
{
    grib_handle* h = grib_handle_of_accessor(a);
    return h->gts_header_len;
}
