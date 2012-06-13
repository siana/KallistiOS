/* KallistiOS ##version##

   dc/fmath.h
   (C)2001 Andrew Kieschnick

*/

#ifndef __DC_FMATH_H
#define __DC_FMATH_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/**
    \file dc/fmath.h
    \brief  Inline functions for the DC's special math instructions
    \author Andrew Kieschnick
*/

/** PI constant (if you don't want full math.h) */
#define F_PI 3.1415926f

/** \cond */
#define __fsin(x) \
    ({ float __value, __arg = (x), __scale = 10430.37835; \
        asm(    "fmul	%2,%1\n\t" \
                "ftrc	%1,fpul\n\t" \
                "fsca	fpul,dr0\n\t" \
                "fmov	fr0,%0" \
                : "=f" (__value), "+&f" (__scale) \
                : "f" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })

#define __fcos(x) \
    ({ float __value, __arg = (x), __scale = 10430.37835; \
        asm(    "fmul	%2,%1\n\t" \
                "ftrc	%1,fpul\n\t" \
                "fsca	fpul,dr0\n\t" \
                "fmov	fr1,%0" \
                : "=f" (__value), "+&f" (__scale) \
                : "f" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })

#define __ftan(x) \
    ({ float __value, __arg = (x), __scale = 10430.37835; \
        asm(    "fmul	%2,%1\n\t" \
                "ftrc	%1,fpul\n\t" \
                "fsca	fpul,dr0\n\t" \
                "fdiv   fr1, fr0\n\t" \
                "fmov	fr0,%0" \
                : "=f" (__value), "+&f" (__scale) \
                : "f" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })


#define __fisin(x) \
    ({ float __value, __arg = (x); \
        asm(    "lds	%1,fpul\n\t" \
                "fsca	fpul,dr0\n\t" \
                "fmov	fr0,%0" \
                : "=f" (__value) \
                : "r" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })

#define __ficos(x) \
    ({ float __value, __arg = (x); \
        asm(    "lds	%1,fpul\n\t" \
                "fsca	fpul,dr0\n\t" \
                "fmov	fr1,%0" \
                : "=f" (__value) \
                : "r" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })

#define __fitan(x) \
    ({ float __value, __arg = (x); \
        asm(    "lds	%1,fpul\n\t" \
                "fsca	fpul,dr0\n\t" \
                "fdiv   fr1, fr0\n\t" \
                "fmov	fr0,%0" \
                : "=f" (__value) \
                : "r" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })


#define __fsqrt(x) \
    ({ float __arg = (x); \
        asm(    "fsqrt %0\n\t" \
                : "=f" (__arg) : "0" (__arg)); \
        __arg; })

#define __frsqrt(x) \
    ({ float __arg = (x); \
        asm(    "fsrra %0\n\t" \
                : "=f" (__arg) : "0" (__arg)); \
        __arg; })

/* Floating point inner product (dot product) */
#define __fipr(x, y, z, w, a, b, c, d) ({ \
        register float __x __asm__("fr0") = (x); \
        register float __y __asm__("fr1") = (y); \
        register float __z __asm__("fr2") = (z); \
        register float __w __asm__("fr3") = (w); \
        register float __a __asm__("fr4") = (a); \
        register float __b __asm__("fr5") = (b); \
        register float __c __asm__("fr6") = (c); \
        register float __d __asm__("fr7") = (d); \
        __asm__ __volatile__( \
                              "fipr	fv4,fv0" \
                              : "+f" (__w) \
                              : "f" (__x), "f" (__y), "f" (__z), "f" (__w), \
                              "f" (__a), "f" (__b), "f" (__c), "f" (__d) \
                            ); \
        __w; })

/* Floating point inner product w/self (square of vector magnitude) */
#define __fipr_magnitude_sqr(x, y, z, w) ({ \
        register float __x __asm__("fr4") = (x); \
        register float __y __asm__("fr5") = (y); \
        register float __z __asm__("fr6") = (z); \
        register float __w __asm__("fr7") = (w); \
        __asm__ __volatile__( \
                              "fipr	fv4,fv4" \
                              : "+f" (__w) \
                              : "f" (__x), "f" (__y), "f" (__z), "f" (__w) \
                            ); \
        __w; })

/** \endcond */

/** \return v1 dot v2 (inner product) */
extern inline float fipr(float x, float y, float z, float w,
                         float a, float b, float c, float d) {
    return __fipr(x, y, z, w, a, b, c, d);
}

/**
    \brief Floating point inner product w/self (square of vector magnitude)
    \return v1 dot v1 (square of magnitude)
*/
extern inline float fipr_magnitude_sqr(float x, float y, float z, float w) {
    return __fipr_magnitude_sqr(x, y, z, w);
}

/**
    \brief Floating point sine
    \param r a floating point number between 0 and 2*PI
    \return sin(r), where r is [0..2*PI]
*/
extern inline float fsin(float r) {
    return __fsin(r);
}

/**
    \brief Floating point cosine
    \param r a floating point number between 0 and 2*PI
    \return cos(r), where r is [0..2*PI]
*/
extern inline float fcos(float r) {
    return __fcos(r);
}

/**
    \brief Floating point tangent
    \param r a floating point number between 0 and 2*PI
    \return tan(r), where r is [0..2*PI]
*/
extern inline float ftan(float r) {
    return __ftan(r);
}

/**
    \brief Integer sine
    \param d an integer between 0 and 65535
    \return sin(d), where d is [0..65535]
*/
extern inline float fisin(int d) {
    return __fisin(d);
}

/**
    \brief Integer cosine
    \param d an integer between 0 and 65535
    \return cos(d), where d is [0..65535]
*/
extern inline float ficos(int d) {
    return __ficos(d);
}

/**
    \brief Integer tangent
    \param d an integer between 0 and 65535
    \return tan(d), where d is [0..65535]
*/
extern inline float fitan(int d) {
    return __fitan(d);
}

/**
    \brief Floating point square root
    \return sqrt(f)
*/
extern inline float fsqrt(float f) {
    return __fsqrt(f);
}

/**
    \return 1.0f / sqrt(f)
*/
extern inline float frsqrt(float f) {
    return __frsqrt(f);
}

__END_DECLS

#endif  /* __DC_FMATH_H */

