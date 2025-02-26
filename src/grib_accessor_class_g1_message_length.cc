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
   SUPER      = grib_accessor_class_section_length
   IMPLEMENTS = init;unpack_long;pack_long
   MEMBERS    = const char *sec4_length
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
static void init(grib_accessor*, const long, grib_arguments*);

typedef struct grib_accessor_g1_message_length
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in long */
    /* Members defined in unsigned */
    long nbytes;
    grib_arguments* arg;
    /* Members defined in section_length */
    /* Members defined in g1_message_length */
    const char *sec4_length;
} grib_accessor_g1_message_length;

extern grib_accessor_class* grib_accessor_class_section_length;

static grib_accessor_class _grib_accessor_class_g1_message_length = {
    &grib_accessor_class_section_length,                      /* super */
    "g1_message_length",                      /* name */
    sizeof(grib_accessor_g1_message_length),  /* size */
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


grib_accessor_class* grib_accessor_class_g1_message_length = &_grib_accessor_class_g1_message_length;

/* END_CLASS_IMP */

static void init(grib_accessor* a, const long len, grib_arguments* args)
{
    grib_accessor_g1_message_length* self = (grib_accessor_g1_message_length*)a;
    self->sec4_length                     = grib_arguments_get_name(grib_handle_of_accessor(a), args, 0);
}

int grib_get_g1_message_size(grib_handle* h, grib_accessor* tl, grib_accessor* s4,
                             long* total_length, long* sec4_len)
{
    unsigned long tlen, slen;
    long off;

    if (!tl)
        return GRIB_NOT_FOUND;
    if (!s4) {
        *sec4_len     = 0;
        off           = tl->offset * 8;
        *total_length = grib_decode_unsigned_long(h->buffer->data, &off, tl->length * 8);
        return GRIB_SUCCESS;
    }

    off  = tl->offset * 8;
    tlen = grib_decode_unsigned_long(h->buffer->data, &off, tl->length * 8);

    off  = s4->offset * 8;
    slen = grib_decode_unsigned_long(h->buffer->data, &off, s4->length * 8);

    /* printf("\nlarge grib tlen=%ld slen=%ld diff=%ld\n",tlen&0x7fffff,slen,tlen-slen); */

    if (slen < 120 && (tlen & 0x800000)) {
        /* printf("DECODING large grib tlen=%ld slen=%ld\n",tlen,slen); */

        tlen &= 0x7fffff;
        tlen *= 120;
        tlen -= slen;
        tlen += 4;

        slen = tlen - s4->offset - 4; /* 4 is for 7777 */

        /*printf("DECODING large grib total=%ld section4=%ld\n",tlen,slen);*/
    }

    *total_length = tlen;
    *sec4_len     = slen;

    return GRIB_SUCCESS;
}

static int pack_long(grib_accessor* a, const long* val, size_t* len)
{
    grib_accessor_g1_message_length* self = (grib_accessor_g1_message_length*)a;
    /*grib_accessor_class* super = *(a->cclass->super);*/

    /* Here we assume that the totalLength will be coded AFTER the section4 length, and
       the section4 length will be overwritten by the totalLength accessor for large GRIBs */

    grib_accessor* s4 = grib_find_accessor(grib_handle_of_accessor(a), self->sec4_length);
    long tlen, slen;
    long t120;
    int ret;

    tlen = *val;
    if ((tlen < 0x800000 || !a->context->gribex_mode_on) && tlen < 0xFFFFFF) {
        /* printf("ENCODING small grib total = %ld\n",tlen); */
        /*return super->pack_long(a,val,len);*/

        /* Do not directly call pack_long on base class */
        /* because in this special case we want to skip the checks. */
        /* So we call the helper function which has an extra argument */
        return pack_long_unsigned_helper(a, val, len, /*check=*/0);
    }

    if (!s4)
        return GRIB_NOT_FOUND;

    /* special case for large GRIBs */
    tlen -= 4;
    t120 = (tlen + 119) / 120;
    slen = t120 * 120 - tlen;
    tlen = 0x800000 | t120;

    /* printf("ENCODING large grib total = %ld tlen=%ld slen=%ld \n",*val,tlen,slen);  */
    *len = 1;
    if ((ret = grib_pack_long(s4, &slen, len)) != GRIB_SUCCESS)
        return ret;

    *len = 1;
    /* Do not do the length checks in this special case */
    if ((ret = pack_long_unsigned_helper(a, &tlen, len, /*check=*/0)) != GRIB_SUCCESS)
        return ret;

    // if((ret = super->pack_long(a,&tlen,len)) != GRIB_SUCCESS) return ret;

    {
        long total_length = -1, sec4_length = -1;
        grib_get_g1_message_size(grib_handle_of_accessor(a),
                                 a,
                                 grib_find_accessor(grib_handle_of_accessor(a), self->sec4_length),
                                 &total_length,
                                 &sec4_length);

        Assert(total_length == *val);
    }

    return GRIB_SUCCESS;
}

static int unpack_long(grib_accessor* a, long* val, size_t* len)
{
    grib_accessor_g1_message_length* self = (grib_accessor_g1_message_length*)a;
    int ret;

    long total_length, sec4_length;

    if ((ret = grib_get_g1_message_size(grib_handle_of_accessor(a),
                                        a,
                                        grib_find_accessor(grib_handle_of_accessor(a), self->sec4_length),
                                        &total_length,
                                        &sec4_length)) != GRIB_SUCCESS)
        return ret;


    *val = total_length;
    return GRIB_SUCCESS;
}
