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
   SUPER      = grib_accessor_class_long
   IMPLEMENTS = unpack_long;pack_long
   IMPLEMENTS = init;dump
   IMPLEMENTS = next_offset
   IMPLEMENTS = value_count
   IMPLEMENTS = byte_offset
   IMPLEMENTS = update_size
   MEMBERS    = grib_accessor* unexpandedDescriptorsEncoded
   MEMBERS    = const char* createNewData

   END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static int pack_long(grib_accessor*, const long* val, size_t* len);
static int unpack_long(grib_accessor*, long* val, size_t* len);
static long byte_offset(grib_accessor*);
static long next_offset(grib_accessor*);
static int value_count(grib_accessor*, long*);
static void dump(grib_accessor*, grib_dumper*);
static void init(grib_accessor*, const long, grib_arguments*);
static void update_size(grib_accessor*, size_t);

typedef struct grib_accessor_unexpanded_descriptors
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in long */
    /* Members defined in unexpanded_descriptors */
    grib_accessor* unexpandedDescriptorsEncoded;
    const char* createNewData;
} grib_accessor_unexpanded_descriptors;

extern grib_accessor_class* grib_accessor_class_long;

static grib_accessor_class _grib_accessor_class_unexpanded_descriptors = {
    &grib_accessor_class_long,                      /* super */
    "unexpanded_descriptors",                      /* name */
    sizeof(grib_accessor_unexpanded_descriptors),  /* size */
    0,                           /* inited */
    0,                           /* init_class */
    &init,                       /* init */
    0,                  /* post_init */
    0,                    /* destroy */
    &dump,                       /* dump */
    &next_offset,                /* next_offset */
    0,              /* get length of string */
    &value_count,                /* get number of values */
    0,                 /* get number of bytes */
    &byte_offset,                /* get offset to bytes */
    0,            /* get native type */
    0,                /* get sub_section */
    0,               /* pack_missing */
    0,                 /* is_missing */
    &pack_long,                  /* pack_long */
    &unpack_long,                /* unpack_long */
    0,                /* pack_double */
    0,                 /* pack_float */
    0,              /* unpack_double */
    0,               /* unpack_float */
    0,                /* pack_string */
    0,              /* unpack_string */
    0,          /* pack_string_array */
    0,        /* unpack_string_array */
    0,                 /* pack_bytes */
    0,               /* unpack_bytes */
    0,            /* pack_expression */
    0,              /* notify_change */
    &update_size,                /* update_size */
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


grib_accessor_class* grib_accessor_class_unexpanded_descriptors = &_grib_accessor_class_unexpanded_descriptors;

/* END_CLASS_IMP */

static void init(grib_accessor* a, const long len, grib_arguments* args)
{
    grib_accessor_unexpanded_descriptors* self = (grib_accessor_unexpanded_descriptors*)a;
    int n                                      = 0;
    grib_handle* hand                          = grib_handle_of_accessor(a);
    self->unexpandedDescriptorsEncoded         = grib_find_accessor(hand, grib_arguments_get_name(hand, args, n++));
    self->createNewData                        = grib_arguments_get_name(hand, args, n++);
    a->length                                  = 0;
}

static void dump(grib_accessor* a, grib_dumper* dumper)
{
    grib_dump_long(dumper, a, NULL);
}

static int unpack_long(grib_accessor* a, long* val, size_t* len)
{
    grib_accessor_unexpanded_descriptors* self = (grib_accessor_unexpanded_descriptors*)a;
    int ret                                    = 0;
    long pos                                   = 0;
    long rlen                                  = 0;
    long f, x, y;
    long* v = val;
    long i;
    grib_handle* hand = grib_handle_of_accessor(a);

    pos = accessor_raw_get_offset(self->unexpandedDescriptorsEncoded) * 8;

    ret = value_count(a, &rlen);
    if (ret)
        return ret;

    if (rlen == 0) {
        grib_context_log(a->context, GRIB_LOG_ERROR,
                         "No descriptors in section 3. Malformed message.");
        return GRIB_MESSAGE_MALFORMED;
    }

    if (*len < rlen) {
        /* grib_context_log(a->context, GRIB_LOG_ERROR, */
        /* " wrong size (%ld) for %s it contains %d values ",*len, a->name , rlen); */
        *len = 0;
        return GRIB_ARRAY_TOO_SMALL;
    }

    for (i = 0; i < rlen; i++) {
        f    = grib_decode_unsigned_long(hand->buffer->data, &pos, 2);
        x    = grib_decode_unsigned_long(hand->buffer->data, &pos, 6);
        y    = grib_decode_unsigned_long(hand->buffer->data, &pos, 8);
        *v++ = f * 100000 + x * 1000 + y;
    }
    *len = rlen;
    return GRIB_SUCCESS;
}

static int pack_long(grib_accessor* a, const long* val, size_t* len)
{
    grib_accessor_unexpanded_descriptors* self = (grib_accessor_unexpanded_descriptors*)a;
    int ret                                    = 0;
    long pos                                   = 0;
    unsigned long f, x, y;
    unsigned char* buf      = NULL;
    grib_accessor* expanded = NULL;
    size_t buflen           = *len * 2;
    size_t i = 0, length = *len;
    long createNewData = 1;
    grib_handle* hand  = grib_handle_of_accessor(a);

    grib_get_long(hand, self->createNewData, &createNewData);

    buf = (unsigned char*)grib_context_malloc_clear(a->context, buflen);

    for (i = 0; i < length; i++) {
        const long tmp = val[i] % 100000;
        f              = val[i] / 100000;
        x              = tmp / 1000;
        y              = tmp % 1000;
        grib_encode_unsigned_longb(buf, f, &pos, 2);
        grib_encode_unsigned_longb(buf, x, &pos, 6);
        grib_encode_unsigned_longb(buf, y, &pos, 8);
    }

    grib_pack_bytes(self->unexpandedDescriptorsEncoded, buf, &buflen);
    grib_context_free(hand->context, buf);

    if (createNewData == 0)
        return ret;

    expanded = grib_find_accessor(hand, "expandedCodes");
    Assert(expanded != NULL);
    ret = grib_accessor_class_expanded_descriptors_set_do_expand(expanded, 1);
    if (ret != GRIB_SUCCESS)
        return ret;

    ret = grib_set_long(hand, "unpack", 3); /* BUFR new data */
    if (ret != GRIB_SUCCESS)
        return ret;

    ret = grib_set_long(hand, "unpack", 1); /* Unpack structure */

    return ret;
}

static int value_count(grib_accessor* a, long* numberOfUnexpandedDescriptors)
{
    grib_accessor_unexpanded_descriptors* self = (grib_accessor_unexpanded_descriptors*)a;
    long n                                     = 0;

    grib_value_count(self->unexpandedDescriptorsEncoded, &n);
    *numberOfUnexpandedDescriptors = n / 2;

    return 0;
}

static long byte_offset(grib_accessor* a)
{
    return a->offset;
}

static void update_size(grib_accessor* a, size_t s)
{
    a->length = s;
}

static long next_offset(grib_accessor* a)
{
    return byte_offset(a) + a->length;
}
