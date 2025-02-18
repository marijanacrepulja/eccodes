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
   SUPER      = grib_accessor_class_gen
   IMPLEMENTS = init; get_native_type
   IMPLEMENTS = pack_long;
   MEMBERS    = const char* doExtractSubsets
   MEMBERS    = const char* numberOfSubsets
   MEMBERS    = const char* extractSubsetList
   MEMBERS    = const char* simpleThinningStart
   MEMBERS    = const char* simpleThinningMissingRadius
   MEMBERS    = const char* simpleThinningSkip
   END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static int get_native_type(grib_accessor*);
static int pack_long(grib_accessor*, const long* val, size_t* len);
static void init(grib_accessor*, const long, grib_arguments*);

typedef struct grib_accessor_bufr_simple_thinning
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in bufr_simple_thinning */
    const char* doExtractSubsets;
    const char* numberOfSubsets;
    const char* extractSubsetList;
    const char* simpleThinningStart;
    const char* simpleThinningMissingRadius;
    const char* simpleThinningSkip;
} grib_accessor_bufr_simple_thinning;

extern grib_accessor_class* grib_accessor_class_gen;

static grib_accessor_class _grib_accessor_class_bufr_simple_thinning = {
    &grib_accessor_class_gen,                      /* super */
    "bufr_simple_thinning",                      /* name */
    sizeof(grib_accessor_bufr_simple_thinning),  /* size */
    0,                           /* inited */
    0,                           /* init_class */
    &init,                       /* init */
    0,                  /* post_init */
    0,                    /* destroy */
    0,                       /* dump */
    0,                /* next_offset */
    0,              /* get length of string */
    0,                /* get number of values */
    0,                 /* get number of bytes */
    0,                /* get offset to bytes */
    &get_native_type,            /* get native type */
    0,                /* get sub_section */
    0,               /* pack_missing */
    0,                 /* is_missing */
    &pack_long,                  /* pack_long */
    0,                /* unpack_long */
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


grib_accessor_class* grib_accessor_class_bufr_simple_thinning = &_grib_accessor_class_bufr_simple_thinning;

/* END_CLASS_IMP */

static void init(grib_accessor* a, const long len, grib_arguments* arg)
{
    int n                                    = 0;
    grib_accessor_bufr_simple_thinning* self = (grib_accessor_bufr_simple_thinning*)a;

    a->length                         = 0;
    self->doExtractSubsets            = grib_arguments_get_name(grib_handle_of_accessor(a), arg, n++);
    self->numberOfSubsets             = grib_arguments_get_name(grib_handle_of_accessor(a), arg, n++);
    self->extractSubsetList           = grib_arguments_get_name(grib_handle_of_accessor(a), arg, n++);
    self->simpleThinningStart         = grib_arguments_get_name(grib_handle_of_accessor(a), arg, n++);
    self->simpleThinningMissingRadius = grib_arguments_get_name(grib_handle_of_accessor(a), arg, n++);
    self->simpleThinningSkip          = grib_arguments_get_name(grib_handle_of_accessor(a), arg, n++);

    a->flags |= GRIB_ACCESSOR_FLAG_FUNCTION;
}

static int get_native_type(grib_accessor* a)
{
    return GRIB_TYPE_LONG;
}

static int apply_thinning(grib_accessor* a)
{
    grib_accessor_bufr_simple_thinning* self = (grib_accessor_bufr_simple_thinning*)a;
    int ret                                  = 0;
    long skip;
    grib_handle* h  = grib_handle_of_accessor(a);
    grib_context* c = h->context;
    long compressed = 0, nsubsets;
    long i;
    grib_iarray* subsets;
    long* subsets_ar = 0;
    long start = 0, radius = 0;

    ret = grib_get_long(h, "compressedData", &compressed);
    if (ret)
        return ret;
    if (compressed) {
        long numberOfSubsets = 0;
        ret                  = grib_get_long(h, self->numberOfSubsets, &numberOfSubsets);
        if (ret)
            return ret;

        ret = grib_get_long(h, self->simpleThinningStart, &start);
        if (ret)
            return ret;

        ret = grib_get_long(h, self->simpleThinningSkip, &skip);
        if (ret)
            return ret;
        if (skip <= 0)
            return GRIB_INVALID_KEY_VALUE;

        ret = grib_get_long(h, self->simpleThinningMissingRadius, &radius);
        if (ret)
            return ret;

        subsets = grib_iarray_new(c, numberOfSubsets / skip + 1, 10);
        for (i = 0; i < numberOfSubsets; i += skip + 1) {
            grib_iarray_push(subsets, i + 1);
        }

        nsubsets = grib_iarray_used_size(subsets);

        if (nsubsets != 0) {
            subsets_ar = grib_iarray_get_array(subsets);
            ret        = grib_set_long_array(h, self->extractSubsetList, subsets_ar, nsubsets);
            grib_context_free(c, subsets_ar);
            if (ret)
                return ret;

            ret = grib_set_long(h, "unpack", 1);
            if (ret)
                return ret;

            ret = grib_set_long(h, self->doExtractSubsets, 1);
            if (ret)
                return ret;
        }
        grib_iarray_delete(subsets);
    }
    else {
        return GRIB_NOT_IMPLEMENTED;
    }

    return ret;
}

static int pack_long(grib_accessor* a, const long* val, size_t* len)
{
    int err                                  = 0;
    grib_accessor_bufr_simple_thinning* self = (grib_accessor_bufr_simple_thinning*)a;

    if (*len == 0)
        return GRIB_SUCCESS;
    err = apply_thinning(a);
    if (err)
        return err;

    return grib_set_long(a->parent->h, self->doExtractSubsets, 1);
}
