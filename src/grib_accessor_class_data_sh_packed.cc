/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_scaling.h"
#include "grib_api_internal.h"
#include <cmath>
/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = accessor
   SUPER      = grib_accessor_class_data_simple_packing
   IMPLEMENTS = unpack_double
   IMPLEMENTS = value_count
   IMPLEMENTS = init
   MEMBERS= const char*  GRIBEX_sh_bug_present
   MEMBERS= const char*  ieee_floats
   MEMBERS= const char*  laplacianOperatorIsSet
   MEMBERS= const char*  laplacianOperator
   MEMBERS= const char*  sub_j
   MEMBERS= const char*  sub_k
   MEMBERS= const char*  sub_m
   MEMBERS= const char*  pen_j
   MEMBERS= const char*  pen_k
   MEMBERS= const char*  pen_m
   END_CLASS_DEF

 */


/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static int unpack_double(grib_accessor*, double* val, size_t* len);
static int value_count(grib_accessor*, long*);
static void init(grib_accessor*, const long, grib_arguments*);

typedef struct grib_accessor_data_sh_packed
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in values */
    int  carg;
    const char* seclen;
    const char* offsetdata;
    const char* offsetsection;
    int dirty;
    /* Members defined in data_simple_packing */
    int edition;
    const char*  units_factor;
    const char*  units_bias;
    const char*  changing_precision;
    const char*  number_of_values;
    const char*  bits_per_value;
    const char*  reference_value;
    const char*  binary_scale_factor;
    const char*  decimal_scale_factor;
    const char*  optimize_scaling_factor;
    /* Members defined in data_sh_packed */
    const char*  GRIBEX_sh_bug_present;
    const char*  ieee_floats;
    const char*  laplacianOperatorIsSet;
    const char*  laplacianOperator;
    const char*  sub_j;
    const char*  sub_k;
    const char*  sub_m;
    const char*  pen_j;
    const char*  pen_k;
    const char*  pen_m;
} grib_accessor_data_sh_packed;

extern grib_accessor_class* grib_accessor_class_data_simple_packing;

static grib_accessor_class _grib_accessor_class_data_sh_packed = {
    &grib_accessor_class_data_simple_packing,                      /* super */
    "data_sh_packed",                      /* name */
    sizeof(grib_accessor_data_sh_packed),  /* size */
    0,                           /* inited */
    0,                           /* init_class */
    &init,                       /* init */
    0,                  /* post_init */
    0,                    /* destroy */
    0,                       /* dump */
    0,                /* next_offset */
    0,              /* get length of string */
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
    &unpack_double,              /* unpack_double */
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


grib_accessor_class* grib_accessor_class_data_sh_packed = &_grib_accessor_class_data_sh_packed;

/* END_CLASS_IMP */

typedef unsigned long (*encode_float_proc)(double);
typedef double (*decode_float_proc)(unsigned long);

static void init(grib_accessor* a, const long v, grib_arguments* args)
{
    grib_accessor_data_sh_packed* self = (grib_accessor_data_sh_packed*)a;
    grib_handle* hand                  = grib_handle_of_accessor(a);

    self->GRIBEX_sh_bug_present  = grib_arguments_get_name(hand, args, self->carg++);
    self->ieee_floats            = grib_arguments_get_name(hand, args, self->carg++);
    self->laplacianOperatorIsSet = grib_arguments_get_name(hand, args, self->carg++);
    self->laplacianOperator      = grib_arguments_get_name(hand, args, self->carg++);
    self->sub_j                  = grib_arguments_get_name(hand, args, self->carg++);
    self->sub_k                  = grib_arguments_get_name(hand, args, self->carg++);
    self->sub_m                  = grib_arguments_get_name(hand, args, self->carg++);
    self->pen_j                  = grib_arguments_get_name(hand, args, self->carg++);
    self->pen_k                  = grib_arguments_get_name(hand, args, self->carg++);
    self->pen_m                  = grib_arguments_get_name(hand, args, self->carg++);

    a->flags |= GRIB_ACCESSOR_FLAG_DATA;
    a->length = 0;
}

static int value_count(grib_accessor* a, long* count)
{
    grib_accessor_data_sh_packed* self = (grib_accessor_data_sh_packed*)a;
    grib_handle* hand = grib_handle_of_accessor(a);
    int ret = 0;
    const char* cclass_name = a->cclass->name;

    long sub_j = 0;
    long sub_k = 0;
    long sub_m = 0;
    long pen_j = 0;
    long pen_k = 0;
    long pen_m = 0;

    if ((ret = grib_get_long_internal(hand, self->sub_j, &sub_j)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(hand, self->sub_k, &sub_k)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(hand, self->sub_m, &sub_m)) != GRIB_SUCCESS)
        return ret;

    if ((ret = grib_get_long_internal(hand, self->pen_j, &pen_j)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(hand, self->pen_k, &pen_k)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(hand, self->pen_m, &pen_m)) != GRIB_SUCCESS)
        return ret;

    if (pen_j != pen_k || pen_j != pen_m) {
        grib_context_log(a->context, GRIB_LOG_ERROR, "%s: pen_j=%ld, pen_k=%ld, pen_m=%ld\n",
                         cclass_name, pen_j, pen_k, pen_m);
        return GRIB_DECODING_ERROR;
    }
    *count = (pen_j + 1) * (pen_j + 2) - (sub_j + 1) * (sub_j + 2);
    return ret;
}

static int unpack_double(grib_accessor* a, double* val, size_t* len)
{
    grib_accessor_data_sh_packed* self = (grib_accessor_data_sh_packed*)a;

    size_t i    = 0;
    int ret     = GRIB_SUCCESS;
    long hcount = 0, lcount = 0, hpos = 0, lup = 0, mmax = 0, n_vals = 0;
    double* scals = NULL;
    /* double *pscals=NULL; */

    double s = 0, d = 0, laplacianOperator = 0;
    unsigned char* buf       = NULL;
    unsigned char* hres      = NULL;
    unsigned char* lres      = NULL;
    unsigned long packed_offset;
    long lpos = 0;

    long maxv                  = 0;
    long GRIBEX_sh_bug_present = 0;
    long ieee_floats           = 0;
    long offsetdata           = 0;
    long bits_per_value       = 0;
    double reference_value    = 0;
    long binary_scale_factor  = 0;
    long decimal_scale_factor = 0;

    long sub_j = 0, sub_k = 0, sub_m = 0, pen_j = 0, pen_k = 0, pen_m = 0;

    double operat = 0;
    int bytes = 0;
    int err = 0;

    decode_float_proc decode_float = NULL;

    n_vals = 0;
    err    = grib_value_count(a, &n_vals);
    if (err)
        return err;

    if (*len < n_vals) {
        *len = n_vals;
        return GRIB_ARRAY_TOO_SMALL;
    }

    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->offsetdata, &offsetdata)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->bits_per_value, &bits_per_value)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_double_internal(grib_handle_of_accessor(a), self->reference_value, &reference_value)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->binary_scale_factor, &binary_scale_factor)) != GRIB_SUCCESS)
        return ret;

    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->decimal_scale_factor, &decimal_scale_factor)) != GRIB_SUCCESS)
        return ret;

    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->GRIBEX_sh_bug_present, &GRIBEX_sh_bug_present)) != GRIB_SUCCESS)
        return ret;

    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->ieee_floats, &ieee_floats)) != GRIB_SUCCESS)
        return ret;

    if ((ret = grib_get_double_internal(grib_handle_of_accessor(a), self->laplacianOperator, &laplacianOperator)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->sub_j, &sub_j)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->sub_k, &sub_k)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->sub_m, &sub_m)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->pen_j, &pen_j)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->pen_k, &pen_k)) != GRIB_SUCCESS)
        return ret;
    if ((ret = grib_get_long_internal(grib_handle_of_accessor(a), self->pen_m, &pen_m)) != GRIB_SUCCESS)
        return ret;

    self->dirty = 0;

    switch (ieee_floats) {
        case 0:
            decode_float = grib_long_to_ibm;
            bytes        = 4;
            break;
        case 1:
            decode_float = grib_long_to_ieee;
            bytes        = 4;
            break;
        case 2:
            decode_float = grib_long_to_ieee64;
            bytes        = 8;
            break;
        default:
            return GRIB_NOT_IMPLEMENTED;
    }

    Assert(sub_j == sub_k);
    Assert(sub_j == sub_m);
    Assert(pen_j == pen_k);
    Assert(pen_j == pen_m);

    buf = (unsigned char*)grib_handle_of_accessor(a)->buffer->data;

    maxv = pen_j + 1;

    buf += offsetdata;
    hres = buf;
    lres = buf;

    packed_offset = offsetdata + bytes * (sub_k + 1) * (sub_k + 2);

    lpos = 8 * (packed_offset - offsetdata);

    s = codes_power<double>(binary_scale_factor, 2);
    d = codes_power<double>(-decimal_scale_factor, 10);

    scals = (double*)grib_context_malloc(a->context, maxv * sizeof(double));
    if(!scals) return GRIB_OUT_OF_MEMORY;

    scals[0] = 0;
    for (i = 1; i < maxv; i++) {
        operat = pow(i * (i + 1), laplacianOperator);
        if (operat != 0)
            scals[i] = (1.0 / operat);
        else {
            scals[i] = 0;
        }
    }

    i = 0;

    while (maxv > 0) {
        lup = mmax;
        if (sub_k >= 0) {
            for (hcount = 0; hcount < sub_k + 1; hcount++) {
                decode_float(grib_decode_unsigned_long(hres, &hpos, 8 * bytes));
                decode_float(grib_decode_unsigned_long(hres, &hpos, 8 * bytes));
                lup++;
            }
            sub_k--;
        }

        /* pscals=scals+lup; */
        for (lcount = hcount; lcount < maxv; lcount++) {
            val[i++] = d * (double)((grib_decode_unsigned_long(lres, &lpos,
                                                               bits_per_value) *
                                     s) +
                                    reference_value);
            val[i++] = d * (double)((grib_decode_unsigned_long(lres, &lpos,
                                                               bits_per_value) *
                                     s) +
                                    reference_value);
            if (mmax == 0)
                val[i - 1] = 0;
            lup++;
        }

        maxv--;
        hcount = 0;
        mmax++;
    }

    Assert(*len >= i);
    *len = n_vals;

    grib_context_free(a->context, scals);

    return ret;
}
